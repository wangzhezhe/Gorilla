

#include "../server/MetadataManager/metadata.h"

void testMetaData() {
  tl::abt scope;
  MetaDataManager metam(10);
  for (int i = 0; i < 15; i++) {
    std::string varname = "testVarName" + std::to_string(i);
    std::string serverAddr = "testserverAddr" + std::to_string(i);
    RawDataEntry rwe(serverAddr, std::to_string(i));
    metam.updateMetaData(i, varname, rwe);
  }

  auto it = metam.m_metaDataMap.begin();
  while (it != metam.m_metaDataMap.end()) {
    std::cout << it->first << std::endl;
    auto innerit = it->second.begin();
    while (innerit != it->second.end()) {
      std::cout << innerit->first << " " << innerit->second.m_rawDataServerAddr
                << " " << innerit->second.m_rawDataID << std::endl;
      innerit++;
    }
    it++;
  }

  if (metam.m_metaDataMap.size() != 10) {
    throw std::runtime_error("failed for metaDataMap size\n");
  }

  if ((metam.m_windowlb != 5) || (metam.m_windowub != 14)) {
    throw std::runtime_error("failed for metaDataMap windlow bound\n");
  }
}

int main() { testMetaData(); }