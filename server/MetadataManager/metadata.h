#include <map>
#include <queue>
#include <string>
#include <thallium.hpp>

namespace tl = thallium;

// the raw data can be indexed direactly by rawdata id at the raw data server
struct RawDataEntry {
  RawDataEntry(){};
  RawDataEntry(std::string rawDataServerAddr, std::string rawDataID)
      : m_rawDataServerAddr(rawDataServerAddr), m_rawDataID(rawDataID){};
  std::string m_rawDataServerAddr;
  std::string m_rawDataID;
  ~RawDataEntry(){};
};

struct MetaDataManager {

  MetaDataManager(){};
  MetaDataManager(int stepNum) : m_stepNum(stepNum){};

  // number of step maintained by metaserver
  size_t m_stepNum = 1024;
  size_t m_windowlb = 0;
  size_t m_windowub = 0;

  // the first index is the step
  // the second index is the variable name
  // lock for map, using thallium lock
  tl::mutex m_metaDataMapMutex;
  std::map<size_t, std::map<std::string, RawDataEntry>> m_metaDataMap;

  void updateMetaData(size_t step, std::string varName, RawDataEntry rde);

  ~MetaDataManager(){};
};