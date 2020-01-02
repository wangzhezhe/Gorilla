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

struct MetaDataManager {

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
  std::map<size_t, std::map<std::string, std::vector<RawDataEndpoint>>> m_metaDataMap;

  void updateMetaData(size_t step, std::string varName, RawDataEndpoint& rde);

  // the bounding box of the RawDataEndpoints is adjusted
  std::vector<RawDataEndpoint> getOverlapEndpoints(size_t, std::string varName,
                                                   BBX *bbx);

  ~MetaDataManager(){};
};

#endif