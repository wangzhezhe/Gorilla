#ifndef __CLIENTCOMMON_H__
#define __CLIENTCOMMON_H__

#include "../commondata/metadata.h"
#include "../utils/matrixtool.h"

#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <thallium.hpp>
#include <vector>

namespace tl = thallium;

#ifdef DEBUG_BUILD
#define DEBUG(x) std::cout << x << std::endl;
#else
#define DEBUG(x)                                                                                   \
  do                                                                                               \
  {                                                                                                \
  } while (0)
#endif

namespace GORILLA
{

/**
 * @brief this should be the stateless client that wraps all kinds of rpc
 * how to get the assocaited server address is the responsibility of higher layered client service
 * this class only wrap the lowest layer rpc namely the define, lookup and on
 * other realted function such as data cache or associated servers are managed by higher level class
 */
class ClientRPCWrapper
{
public:
  ClientRPCWrapper(tl::engine* clientEnginePtr, std::string addrServer)
    : m_clientEnginePtr(clientEnginePtr)
  {
    this->m_addrServerEndpoint = this->m_clientEnginePtr->lookup(addrServer);
  };

  // the client engine ptr
  tl::engine* m_clientEnginePtr = nullptr;
  // the cache to map the server add into the endpoints
  std::map<std::string, tl::endpoint> m_serverToEndpoints;
  // this server knows all address of the communication group
  tl::endpoint m_addrServerEndpoint;

  virtual ~ClientRPCWrapper(){};

  // function to find and cache the endpoints
  tl::endpoint lookup(std::string& address);
  // this is called if the addr of all the server is already known
  // otherwise, it might be initilized by getAllServerAddr
  void initEndPoints(std::vector<std::string>& serverList);
  // get all associated addresses by the addrServer
  std::vector<MetaAddrWrapper> getAllServerAddr();

  // associated rpcs
  int updateDHT(std::string serverAddr, std::vector<MetaAddrWrapper> metaAddrWrapperList);

  std::vector<BlockDescriptor> getblockDescriptorList(std::string serverAddr, size_t step,
    std::string varName, size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub);

  std::string executeRawFunc(std::string serverAddr, std::string blockID, std::string functionName,
    std::vector<std::string>& funcParameters);

  void eraseRawData(std::string serverAddr, std::string blockID);

  int getSubregionData(std::string serverAddr, std::string blockID, size_t dataSize, size_t dims,
    std::array<int, 3> indexlb, std::array<int, 3> indexub, void* dataContainer);

  void endTimer(std::string serverAddr);

  void startTimer(std::string serverAddr);

  std::vector<std::string> getmetaServerList(
    std::string serverAddr, size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub);

  int putTriggerInfo(std::string serverAddr, std::string triggerName, DynamicTriggerInfo& dti);

  void notifyBack(std::string watcherAddr, BlockSummary& bs);

  void putEventIntoQueue(std::string groupMasterAddr, std::string triggerName, EventWrapper& event);

  EventWrapper getEventFromQueue(std::string groupMasterAddr, std::string triggerName);

  void deleteMetaStep(size_t step);

  int getServerNum(std::string serverAddr);

  void registerWatcher(std::vector<std::string> triggerNameList);

  std::vector<double> getStageStatus(std::string server);

};

}
#endif