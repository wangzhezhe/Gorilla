#include "metadataManager.h"

void MetaDataManager::updateMetaData(size_t step, std::string varName,
                                     RawDataEndpoint rde) {

  if (this->m_metaDataMap.find(step) == this->m_metaDataMap.end()) {
    std::map<std::string, std::vector<RawDataEndpoint>> innermap;
    m_metaDataMap[step] = innermap;
  }

  this->m_metaDataMapMutex.lock();
  m_metaDataMap[step][varName].push_back(rde);
  this->m_metaDataMapMutex.unlock();

  if (step > m_windowub) {
    this->m_windowub = step;
  }

  // erase the lower bound data when step window is larger than threshold
  if (this->m_windowub - this->m_windowlb + 1 > m_stepNum) {
    m_metaDataMapMutex.lock();
    if (this->m_metaDataMap.find(this->m_windowlb) !=
        this->m_metaDataMap.end()) {
      this->m_metaDataMap.erase(this->m_windowlb);
    }
    m_metaDataMapMutex.unlock();
    this->m_windowlb++;
  }
}

// need to be locked when the function is called
std::vector<RawDataEndpoint>
MetaDataManager::getOverlapEndpoints(size_t step, std::string varName,
                                     BBX *querybbx) {
  // range the vector
  std::vector<RawDataEndpoint> endpointList;
  if (this->m_metaDataMap.find(step) == this->m_metaDataMap.end()) {
    return endpointList;
  }

  if (this->m_metaDataMap[step].find(varName) ==
      this->m_metaDataMap[step].end()) {
    return endpointList;
  }

  int size = m_metaDataMap[step][varName].size();
  
  // go through vector to check the overlapping part
  for (int i = 0; i < size; i++) {
    std::array<int, 3> m_indexlb = m_metaDataMap[step][varName][i].m_indexlb;
    std::array<int, 3> m_indexub = m_metaDataMap[step][varName][i].m_indexub;
    size_t dims = m_metaDataMap[step][varName][i].m_dims;
    BBX *bbx = new BBX(dims, m_indexlb, m_indexub);

    BBX *overlapbbx = getOverlapBBX(bbx, querybbx);

    if (overlapbbx != NULL) {
      
      std::array<int, 3> indexlb = overlapbbx->getIndexlb();
      std::array<int, 3> indexub = overlapbbx->getIndexub();
      RawDataEndpoint rde(m_metaDataMap[step][varName][i].m_rawDataServerAddr,
                          m_metaDataMap[step][varName][i].m_rawDataID, dims, indexlb,
                          indexub);
      endpointList.push_back(rde);
    }
  }

  return endpointList;
}