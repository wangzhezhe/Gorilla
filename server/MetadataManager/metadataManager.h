#ifndef METAMANAGER_H
#define METAMANAGER_H

#include "../../utils/bbxtool.h"
#include <unordered_map>
#include <string>
#include <thallium.hpp>
#include <vector>
#include "../../commondata/metadata.h"

using namespace BBXTOOL;
namespace tl = thallium;

//the key is the index for the different version of the data
//it can be the raw or specific filter name such like VTKISO
//the design here maybe not useful currently
//only the metadata of the raw data need to be updated
//the filtered data (intermediate data) are stayed at the same node with the rawdata
struct MetaDataBlock
{
  MetaDataBlock(){};

  void eraseBlocks(std::string dataType){
    m_metadataBlock.erase(dataType);
    return;
  }

  //update status
  //this should be called with lock at the outter layer
  void updateMetaBlockStatus(std::string dataType, std::string rawDataID, MetaStatus newstatus)
  {

    if (m_metadataBlock.count(dataType) == 0)
    {
      throw std::runtime_error("the datatype " + dataType + " for current block not exist");
    }

    int vectorSize = m_metadataBlock[dataType].size();

    for (int i = 0; i < vectorSize; i++)
    {
      if (m_metadataBlock[dataType][i].m_rawDataID == rawDataID)
      {
        m_metadataBlock[dataType][i].m_metaStatus = newstatus;
      }
    }
    return;
  }
  
  //get the meta block status indexed by the datatype and the rawDataID
  int getMetaBlockStatus(std::string dataType, std::string rawDataID)
  {
    if (m_metadataBlock.count(dataType) == 0)
    {
      throw std::runtime_error("the datatype " + dataType + " for current block not exist");
    }

    int vectorSize = m_metadataBlock[dataType].size();

    for (int i = 0; i < vectorSize; i++)
    {
      if (m_metadataBlock[dataType][i].m_rawDataID == rawDataID)
      {
        return m_metadataBlock[dataType][i].m_metaStatus;
      }
    }
    return MetaStatus::ERROR;
  }

  //the mutex is applied by caller
  void insertRDEP(std::string dataType, RawDataEndpoint &rde)
  {

    if (m_metadataBlock.find(dataType) == m_metadataBlock.end())
    {
      std::vector<RawDataEndpoint> rdelist;
      m_metadataBlock[dataType] = rdelist;
    }

    m_metadataBlock[dataType].push_back(rde);
    return;
  }

  bool dataTypeExist(std::string dataType)
  {
    if (m_metadataBlock.find(dataType) != m_metadataBlock.end())
    {
      return true;
    }
    return false;
  }

  int getBlockNumberByVersion(std::string version)
  {
    if (this->m_metadataBlock.count(version) == 0)
    {
      throw std::runtime_error("the version does not exist when getBlockNumberByVersion");
      return 0;
    }
    return m_metadataBlock[version].size();
  }

  //the key is the type/version of the data such as raw mem or vtk or compressed data
  //the value is the endpoints related with the type
  std::unordered_map<std::string, std::vector<RawDataEndpoint>> m_metadataBlock;

  ~MetaDataBlock()
  {
    std::cout << "destroy MetaDataBlock" << std::endl;
  };

  void printInfo(int rank)
  {
    for (auto &kv : m_metadataBlock)
    {
      int value = kv.second.size();
      char str[256];
      sprintf(str, "rank %d metaBlockSize %d", rank, value);
      std::cout << std::string(str) << std::endl;
    }
  }
};

struct MetaDataManager
{

  MetaDataManager(){};
  MetaDataManager(size_t bufferedSteps, size_t windowSize, size_t deletedNum) : m_bufferedSteps(bufferedSteps), m_windowSize(windowSize), m_deletedNum(deletedNum){};

  //this is just a const variable
  //when it needs to check if the data can be put
  //the user can check the number of the map size and compare it with this number
  size_t m_bufferedSteps = 10;
  // number of step maintained by metaserver
  size_t m_windowSize = 5;
  //deleted Num should less than m_windowSize
  //this integer label how many data should be deleted at one time
  size_t m_deletedNum = 1;
  //TODO, the windlow lb and ub should bounded with each varName for future
  //currently, when we delete the step, we delete all related varaibles
  tl::mutex m_boundMutex;
  //assume first step sart with 1
  size_t m_windowlb = 1;
  size_t m_windowub = 1;

  // the first index is the step
  // the second index is the variable name
  // lock for map, using thallium lock
  tl::mutex m_metaDataMapMutex;

  // TODO use more advanced structure such as R tree to index the endpoint here
  // This will decrease the time spent on indexing the overlapping bbx
  // one step contains multiple varaible
  // one variable contains multiple data partitions
  // one data partition is associated with multiple versions
  std::unordered_map<size_t, std::unordered_map<std::string, MetaDataBlock>> m_metaDataMap;

  void updateMetaData(size_t step, std::string varName, RawDataEndpoint &rde, std::string dataType = DRIVERTYPE_RAWMEM);

  // the bounding box of the RawDataEndpoints is adjusted
  std::vector<RawDataEndpoint> getOverlapEndpoints(size_t, std::string varName,
                                                   BBX &querybbx, std::string dataType = DRIVERTYPE_RAWMEM);

  std::vector<RawDataEndpoint> getRawEndpoints(size_t step, std::string varName, std::string dataType = DRIVERTYPE_RAWMEM);

  bool ifCovered(std::vector<RawDataEndpoint> &existlist, BBX queryBBX);

  void updateMetaDataStatus(size_t step, std::string varName, std::string dataType, std::string rawDataID, MetaStatus newstatus);

  int getMetaDataStatus(size_t step, std::string varName, std::string dataType, std::string rawDataID);

  ~MetaDataManager()
  {
    std::cout << "destroy MetaDataManager" << std::endl;
  };

  void printInfo(int rank, int step, std::string varName)
  {
    m_metaDataMapMutex.lock();
    if (m_metaDataMap.count(step) == 0)
    {
      m_metaDataMapMutex.unlock();
      return;
    }
    if (m_metaDataMap[step].count(varName) == 0)
    {
      m_metaDataMapMutex.unlock();
      return;
    }
    m_metaDataMap[step][varName].printInfo(rank);
    m_metaDataMapMutex.unlock();
  }
};

#endif
