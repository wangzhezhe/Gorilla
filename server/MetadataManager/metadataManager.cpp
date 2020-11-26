#include "metadataManager.h"
#include <queue>

std::string rawDataVersrion("rawData");

int MetaDataManager::getMetaDataStatus(size_t step, std::string varName, std::string dataType, std::string rawDataID)
{
  this->m_metaDataMapMutex.lock();
  int stepCount = this->m_metaDataMap.count(step);
  this->m_metaDataMapMutex.unlock();

  if (stepCount == 0)
  {
    throw std::runtime_error("the metadata for step does not exist for getMetaDataStatus");
    return MetaStatus::ERROR;
  }

  this->m_metaDataMapMutex.lock();
  int varCount = this->m_metaDataMap[step].count(varName);
  this->m_metaDataMapMutex.unlock();

  if (varCount == 0)
  {
    throw std::runtime_error("the metadata for varaible does not exist for getMetaDataStatus");
    return MetaStatus::ERROR;
  }

  this->m_metaDataMapMutex.lock();
  int status = m_metaDataMap[step][varName].getMetaBlockStatus(dataType, rawDataID);
  this->m_metaDataMapMutex.unlock();
  return status;
}

void MetaDataManager::updateMetaDataStatus(size_t step, std::string varName, std::string dataType, std::string rawDataID, MetaStatus newstatus)
{
  this->m_metaDataMapMutex.lock();
  int stepCount = this->m_metaDataMap.count(step);
  this->m_metaDataMapMutex.unlock();

  if (stepCount == 0)
  {
    throw std::runtime_error("the metadata for step does not exist for updateMetaDataStatus");
    return;
  }

  this->m_metaDataMapMutex.lock();
  int varCount = this->m_metaDataMap[step].count(varName);
  this->m_metaDataMapMutex.unlock();

  if (varCount == 0)
  {
    throw std::runtime_error("the metadata for varaible does not exist for updateMetaDataStatus");
    return;
  }

  this->m_metaDataMapMutex.lock();
  m_metaDataMap[step][varName].updateMetaBlockStatus(dataType, rawDataID, newstatus);
  this->m_metaDataMapMutex.unlock();
}

void MetaDataManager::updateMetaData(size_t step, std::string varName,
                                     RawDataEndpoint &rde, std::string dataType)
{
  this->m_metaDataMapMutex.lock();
  if (this->m_metaDataMap.find(step) == this->m_metaDataMap.end())
  {

    std::unordered_map<std::string, MetaDataBlock> innermap;
    m_metaDataMap[step] = innermap;
  }

  m_metaDataMap[step][varName].insertRDEP(dataType, rde);
  this->m_metaDataMapMutex.unlock();
}

std::vector<RawDataEndpoint>
MetaDataManager::getRawEndpoints(size_t step, std::string varName, std::string dataType)
{
  std::vector<RawDataEndpoint> endpointList;

  m_metaDataMapMutex.lock();
  int tempSize = this->m_metaDataMap.count(step);
  m_metaDataMapMutex.unlock();

  if (tempSize == 0)
  {
    return endpointList;
  }

  m_metaDataMapMutex.lock();
  tempSize = this->m_metaDataMap[step].count(varName);
  m_metaDataMapMutex.unlock();

  if (tempSize == 0)
  {
    return endpointList;
  }

  //check the exist of the dataType
  m_metaDataMapMutex.lock();
  bool exist = m_metaDataMap[step][varName].dataTypeExist(dataType);
  m_metaDataMapMutex.unlock();

  if (exist == false)
  {
    throw std::runtime_error("data type does not exist");
    return endpointList;
  }
  m_metaDataMapMutex.lock();
  endpointList = m_metaDataMap[step][varName].m_metadataBlock[dataType];
  m_metaDataMapMutex.unlock();

  return endpointList;
}

// need to be locked when the function is called
// this is only for the raw
// if there is overlap, return the rawdata endpoint
// the bbx in raw data endpoint/descriptor is the overlapped region
std::vector<RawDataEndpoint>
MetaDataManager::getOverlapEndpoints(size_t step, std::string varName,
                                     BBX &querybbx, std::string dataType)
{
  // range the vector
  std::vector<RawDataEndpoint> endpointList;

  m_metaDataMapMutex.lock();
  int tempSize = this->m_metaDataMap.count(step);
  m_metaDataMapMutex.unlock();

  if (tempSize == 0)
  {
    return endpointList;
  }

  m_metaDataMapMutex.lock();
  tempSize = this->m_metaDataMap[step].count(varName);
  m_metaDataMapMutex.unlock();

  if (tempSize == 0)
  {
    return endpointList;
  }

  //check the exist of the dataType
  m_metaDataMapMutex.lock();
  bool exist = m_metaDataMap[step][varName].dataTypeExist(dataType);
  m_metaDataMapMutex.unlock();

  if (exist == false)
  {
    throw std::runtime_error("data type does not exist");
    return endpointList;
  }

  m_metaDataMapMutex.lock();
  int size = m_metaDataMap[step][varName].m_metadataBlock[dataType].size();
  m_metaDataMapMutex.unlock();

  //TODO, if the registered metadata did not cover all the bounding box, return empty
  //store for key point
  // go through vector of raw data pointer to check the overlapping part

  for (int i = 0; i < size; i++)
  {
    m_metaDataMapMutex.lock();
    std::array<int, 3> m_indexlb = m_metaDataMap[step][varName].m_metadataBlock[dataType][i].m_indexlb;
    std::array<int, 3> m_indexub = m_metaDataMap[step][varName].m_metadataBlock[dataType][i].m_indexub;
    size_t dims = m_metaDataMap[step][varName].m_metadataBlock[dataType][i].m_dims;
    m_metaDataMapMutex.unlock();

    BBX bbx(dims, m_indexlb, m_indexub);

    BBX *overlapbbx = getOverlapBBX(bbx, querybbx);

    if (overlapbbx != NULL)
    {

      std::array<int, 3> indexlb = overlapbbx->getIndexlb();
      std::array<int, 3> indexub = overlapbbx->getIndexub();

      m_metaDataMapMutex.lock();
      std::string rawDataServerAddr = m_metaDataMap[step][varName].m_metadataBlock[dataType][i].m_rawDataServerAddr;
      std::string rawDataID = m_metaDataMap[step][varName].m_metadataBlock[dataType][i].m_rawDataID;
      m_metaDataMapMutex.unlock();

      RawDataEndpoint rde(rawDataServerAddr,
                          rawDataID,
                          dims, indexlb,
                          indexub);
      endpointList.push_back(rde);
      delete overlapbbx;
    }
  }

  return endpointList;
}

// refer to https://stackoverflow.com/questions/4397226/algorithm-required-to-determine-if-a-rectangle-is-completely-covered-by-another
// check if the set of small rectangles can fill out the large rectangle
bool MetaDataManager::ifCovered(std::vector<RawDataEndpoint> &existlist, BBX queryBBX)
{

  //query lb should match with the dimes in rdep
  std::queue<BBX> bbxqueue;
  bbxqueue.push(queryBBX);
  int curStatus = 0;
  for (auto rdep : existlist)
  {
    BBX existBBX(rdep.m_dims, rdep.m_indexlb, rdep.m_indexub);
    while (!bbxqueue.empty())
    {
      //the status is at the new layer
      if (bbxqueue.front().m_status != curStatus)
      {
        break;
      }

      BBX curBBX = bbxqueue.front();
      //curBBX.printBBXinfo();
      BBX *overlap = getOverlapBBX(curBBX, existBBX);

      if (overlap == NULL)
      {
        //not overlap
        //std::cout << "debug none overlap" << std::endl;
        curBBX.m_status++;
        bbxqueue.push(curBBX);
        bbxqueue.pop();
      }
      else if (overlap->equal(curBBX))
      {
        //curBBX is covered by existing domain
        //pop current elem direactly
        //std::cout << "debug delete bbx" << std::endl;
        //curBBX.printBBXinfo();
        bbxqueue.pop();
      }
      else
      {
        //there is overlap between two bbx
        //need to split the curent bbx
        //std::cout << "debug split bbx" << std::endl;
        std::vector<BBX> bbxlist = splitReduceBBX(curBBX, *overlap);
        for (auto v : bbxlist)
        {
          //v.printBBXinfo();
          v.m_status = curBBX.m_status + 1;
          bbxqueue.push(v);
        }
        bbxqueue.pop();
        //std::cout << "current queue size " << bbxqueue.size() << std::endl;
      }
    }
    //increase for every outter for every existing bbx
    curStatus++;
  }
  if (bbxqueue.empty())
  {
    return true;
  }
  else
  {
    return false;
  }
}
