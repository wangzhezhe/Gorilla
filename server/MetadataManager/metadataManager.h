#ifndef METAMANAGER_H
#define METAMANAGER_H

#include "../../utils/bbxtool.h"
#include <map>
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

  void insertRDEP(std::string dataType, RawDataEndpoint&rde){

    if(m_metadataBlock.find(dataType)==m_metadataBlock.end()){
      std::vector<RawDataEndpoint> rdelist;
      m_metadataBlock[dataType] = rdelist;
    }

    m_metaDataBlockMutex.lock();
    m_metadataBlock[dataType].push_back(rde);
    m_metaDataBlockMutex.unlock();
    return;
  }

  bool dataTypeExist(std::string dataType){
    if(m_metadataBlock.find(dataType)!=m_metadataBlock.end()){
      return true;
    }
    return false;
  }

  tl::mutex m_metaDataBlockMutex;
  std::map<std::string, std::vector<RawDataEndpoint>> m_metadataBlock;

  ~MetaDataBlock(){
    std::cout << "destroy MetaDataBlock" << std::endl;
  };
};

struct MetaDataManager
{

  MetaDataManager(){};
  MetaDataManager(int stepNum) : m_stepNum(stepNum){};

  // number of step maintained by metaserver
  size_t m_stepNum = 1024;
  int m_windowlb = 0;
  int m_windowub = 0;

  // the first index is the step
  // the second index is the variable name
  // lock for map, using thallium lock
  tl::mutex m_metaDataMapMutex;

  // TODO use more advanced structure such as R tree to index the endpoint here
  // This will decrease the time spent on indexing the overlapping bbx
  // one step contains multiple varaible
  // one variable contains multiple data partitions
  // one data partition is associated with multiple versions
  std::map<size_t, std::map<std::string, MetaDataBlock*>> m_metaDataMap;

  void updateMetaData(size_t step, std::string varName, RawDataEndpoint &rde, std::string dataVersion = DRIVERTYPE_RAWMEM);

  // the bounding box of the RawDataEndpoints is adjusted
  std::vector<RawDataEndpoint> getOverlapEndpoints(size_t, std::string varName,
                                                   BBX *bbx, std::string dataVersion = DRIVERTYPE_RAWMEM);

  ~MetaDataManager(){
     std::cout << "destroy MetaDataManager" << std::endl;
  };
};

#endif
