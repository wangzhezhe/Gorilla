#include "metadata.h"

void MetaDataManager::updateMetaData(size_t step, std::string varName,
                                     RawDataEntry rde) {

  int stepInMap = m_metaDataMap.size();

  if (this->m_metaDataMap.find(step) == this->m_metaDataMap.end()) {
    std::map<std::string, RawDataEntry> innermap;
    m_metaDataMap[step] = innermap;
  }

  this->m_metaDataMapMutex.lock();
  m_metaDataMap[step][varName] = rde;
  this->m_metaDataMapMutex.unlock();

  if (step > m_windowub) {
    this->m_windowub = step;
  }
  //erase the lower bound data when step window is larger than threshold
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