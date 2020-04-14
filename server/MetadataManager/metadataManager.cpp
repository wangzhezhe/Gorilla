#include "metadataManager.h"
#include <queue>

std::string rawDataVersrion("rawData");

void MetaDataManager::updateMetaData(size_t step, std::string varName,
                                     RawDataEndpoint &rde, std::string dataType)
{

  if (this->m_metaDataMap.find(step) == this->m_metaDataMap.end())
  {

    std::map<std::string, MetaDataBlock *> innermap;
    m_metaDataMap[step] = innermap;
  }

  //if there is no coresponding variable
  if (this->m_metaDataMap[step].find(varName) == this->m_metaDataMap[step].end())
  {
    //init the data block
    MetaDataBlock *mdb = new MetaDataBlock();
    m_metaDataMap[step][varName] = mdb;
  }

  this->m_metaDataMapMutex.lock();
  m_metaDataMap[step][varName]->insertRDEP(dataType, rde);
  this->m_metaDataMapMutex.unlock();

  if (step > m_windowub)
  {
    this->m_windowub = step;
  }

  // erase the lower bound data when step window is larger than threshold
  if (this->m_windowub - this->m_windowlb + 1 > m_stepNum)
  {
    m_metaDataMapMutex.lock();
    if (this->m_metaDataMap.find(this->m_windowlb) !=
        this->m_metaDataMap.end())
    {
      //TODO, need to call the rawdata manager
      //this->m_metaDataMap.erase(this->m_windowlb);
    }
    m_metaDataMapMutex.unlock();
    this->m_windowlb++;
  }
}

std::vector<RawDataEndpoint>
MetaDataManager::getRawEndpoints(size_t step, std::string varName, std::string dataType)
{
  std::vector<RawDataEndpoint> endpointList;
  if (this->m_metaDataMap.find(step) == this->m_metaDataMap.end())
  {
    return endpointList;
  }

  if (this->m_metaDataMap[step].find(varName) ==
      this->m_metaDataMap[step].end())
  {
    return endpointList;
  }

  //check the exist of the dataType

  if (m_metaDataMap[step][varName]->dataTypeExist(dataType) == false)
  {
    throw std::runtime_error("data type does not exist");
    return endpointList;
  }

  return m_metaDataMap[step][varName]->m_metadataBlock[dataType];
}

// need to be locked when the function is called
// this is only for the raw
std::vector<RawDataEndpoint>
MetaDataManager::getOverlapEndpoints(size_t step, std::string varName,
                                     BBX *querybbx, std::string dataType)
{
  // range the vector
  std::vector<RawDataEndpoint> endpointList;
  if (this->m_metaDataMap.find(step) == this->m_metaDataMap.end())
  {
    return endpointList;
  }

  if (this->m_metaDataMap[step].find(varName) ==
      this->m_metaDataMap[step].end())
  {
    return endpointList;
  }

  //check the exist of the dataType

  if (m_metaDataMap[step][varName]->dataTypeExist(dataType) == false)
  {
    throw std::runtime_error("data type does not exist");
    return endpointList;
  }

  int size = m_metaDataMap[step][varName]->m_metadataBlock[dataType].size();

  //TODO, if the registered metadata did not cover all the bounding box, return empty
  //store for key point
  // go through vector of raw data pointer to check the overlapping part

  for (int i = 0; i < size; i++)
  {
    std::array<int, 3> m_indexlb = m_metaDataMap[step][varName]->m_metadataBlock[dataType][i].m_indexlb;
    std::array<int, 3> m_indexub = m_metaDataMap[step][varName]->m_metadataBlock[dataType][i].m_indexub;
    size_t dims = m_metaDataMap[step][varName]->m_metadataBlock[dataType][i].m_dims;
    BBX bbx(dims, m_indexlb, m_indexub);

    BBX *overlapbbx = getOverlapBBX(bbx, *querybbx);

    if (overlapbbx != NULL)
    {

      std::array<int, 3> indexlb = overlapbbx->getIndexlb();
      std::array<int, 3> indexub = overlapbbx->getIndexub();

      RawDataEndpoint rde(m_metaDataMap[step][varName]->m_metadataBlock[dataType][i].m_rawDataServerAddr,
                          m_metaDataMap[step][varName]->m_metadataBlock[dataType][i].m_rawDataID,
                          dims, indexlb,
                          indexub);
      endpointList.push_back(rde);
    }
  }

  return endpointList;
}

// refer to https://stackoverflow.com/questions/4397226/algorithm-required-to-determine-if-a-rectangle-is-completely-covered-by-another
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
