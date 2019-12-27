#ifndef METAMANAGER_H
#define METAMANAGER_H

#include "../../utils/bbxtool.h"
#include <map>
#include <string>
#include <thallium.hpp>
#include <vector>

using namespace BBXTOOL;
namespace tl = thallium;

struct RawDataEndpoint {
  RawDataEndpoint(){};
  RawDataEndpoint(std::string rawDataServerAddr, std::string rawDataID,
                  size_t dims, std::array<int, 3> indexlb,
                  std::array<int, 3> indexub)
      : m_rawDataServerAddr(rawDataServerAddr), m_rawDataID(rawDataID),
        m_dims(dims), m_indexlb(indexlb), m_indexub(indexub){};
  std::string m_rawDataServerAddr;
  std::string m_rawDataID;
  size_t m_dims = 0;
  std::array<int, 3> m_indexlb{{0, 0, 0}};
  std::array<int, 3> m_indexub{{0, 0, 0}};

  void printInfo() {
    std::cout << "server addr " << m_rawDataServerAddr << " dataID "
              << m_rawDataID << "dims " << m_dims << " lb " << m_indexlb[0] << ","
              << m_indexlb[1] << "," << m_indexlb[2] << " ub " << m_indexub[0]
              << "," << m_indexub[1] << "," << m_indexub[2] << std::endl;
  }

  ~RawDataEndpoint(){};
};

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
  std::map<size_t, std::map<std::string, std::vector<RawDataEndpoint>>>
      m_metaDataMap;

  void updateMetaData(size_t step, std::string varName, RawDataEndpoint rde);

  // the bounding box of the RawDataEndpoints is adjusted
  std::vector<RawDataEndpoint> getOverlapEndpoints(size_t, std::string varName,
                                                   BBX *bbx);

  ~MetaDataManager(){};
};

#endif