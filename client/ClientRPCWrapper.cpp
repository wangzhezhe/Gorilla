
#include "ClientRPCWrapper.hpp"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#define BILLION 1000000000L
namespace tl = thallium;

namespace GORILLA
{

// find the endpoint if it is not cached locally
tl::endpoint ClientRPCWrapper::lookup(std::string& address)
{
  auto it = m_serverToEndpoints.find(address);
  if (it == m_serverToEndpoints.end())
  {
    // do not lookup here to avoid the potential mercury race condition
    // throw std::runtime_error("failed to find addr, cache the endpoint at the constructor\n");
    auto endpoint = this->m_clientEnginePtr->lookup(address);
    std::string tempAddr = address;
    this->m_serverToEndpoints[tempAddr] = endpoint;
    DEBUG("add new endpoints into the address cache");
    return endpoint;
  }
  return it->second;
}

// this is called by server node, the endpoint is not initilized yet
int ClientRPCWrapper::updateDHT(
  std::string serverAddr, std::vector<MetaAddrWrapper> metaAddrWrapperList)
{
  // std::cout << "debug updateDHT server addr " << serverAddr << std::endl;
  tl::remote_procedure remoteupdateDHT = this->m_clientEnginePtr->define("updateDHT");
  tl::endpoint serverEndpoint = this->lookup(serverAddr);
  // the parameters here should be consistent with the defination at the server end
  int status = remoteupdateDHT.on(serverEndpoint)(metaAddrWrapperList);
  return status;
}

std::vector<BlockDescriptor> ClientRPCWrapper::getblockDescriptorList(std::string serverAddr,
  size_t step, std::string varName, size_t dims, std::array<int, 3> indexlb,
  std::array<int, 3> indexub)
{
  tl::remote_procedure remotegetblockDescriptorList =
    this->m_clientEnginePtr->define("getBlockDescriptorList");
  tl::endpoint serverEndpoint = this->lookup(serverAddr);
  std::vector<BlockDescriptor> blockDescriptorList =
    remotegetblockDescriptorList.on(serverEndpoint)(step, varName, dims, indexlb, indexub);
  return blockDescriptorList;
}

std::string ClientRPCWrapper::executeRawFunc(std::string serverAddr, std::string blockID,
  std::string functionName, std::vector<std::string>& funcParameters)
{
  tl::remote_procedure remoteexecuteRawFunc = this->m_clientEnginePtr->define("executeRawFunc");
  tl::endpoint serverEndpoint = this->lookup(serverAddr);
  std::string result =
    remoteexecuteRawFunc.on(serverEndpoint)(blockID, functionName, funcParameters);
  return result;
}

void ClientRPCWrapper::eraseRawData(std::string serverAddr, std::string blockID)
{
  tl::remote_procedure remoteeraserawdata = this->m_clientEnginePtr->define("eraserawdata");
  tl::endpoint serverEndpoint = this->lookup(serverAddr);
  remoteeraserawdata.on(serverEndpoint)(blockID);
  return;
}

int ClientRPCWrapper::getSubregionData(std::string serverAddr, std::string blockID, size_t dataSize,
  size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub, void* dataContainer)
{

  tl::remote_procedure remotegetSubregionData = this->m_clientEnginePtr->define("getDataSubregion");
  tl::endpoint serverEndpoint = this->lookup(serverAddr);

  std::vector<std::pair<void*, std::size_t> > segments(1);
  segments[0].first = (void*)(dataContainer);
  segments[0].second = dataSize;

  tl::bulk clientBulk = this->m_clientEnginePtr->expose(segments, tl::bulk_mode::write_only);

  int status =
    remotegetSubregionData.on(serverEndpoint)(blockID, dims, indexlb, indexub, clientBulk);
  return status;
}

void ClientRPCWrapper::startTimer(std::string serverAddr)
{
  tl::remote_procedure remotestartTimer =
    this->m_clientEnginePtr->define("startTimer").disable_response();
  tl::endpoint serverEndpoint = this->lookup(serverAddr);
  // the parameters here should be consistent with the defination at the server end
  remotestartTimer.on(serverEndpoint)();
  return;
}

void ClientRPCWrapper::initEndPoints(std::vector<std::string>& serverList)
{
  for (auto it = serverList.begin(); it != serverList.end(); it++)
  {
    auto endpoint = this->m_clientEnginePtr->lookup(*it);
    m_serverToEndpoints[*it] = endpoint;
  }
  return;
}

std::vector<MetaAddrWrapper> ClientRPCWrapper::getAllServerAddr()
{
  tl::remote_procedure remotegetAllServerAddr = this->m_clientEnginePtr->define("getAllServerAddr");
  // the parameters here should be consistent with the defination at the server end
  // the m_addrServerEndpoint is initilized in the constructor
  std::vector<MetaAddrWrapper> adrList = remotegetAllServerAddr.on(m_addrServerEndpoint)();

  if (adrList.size() == 0)
  {
    throw std::runtime_error("failed to get server list");
  }

  return adrList;

  /*
  for (auto it = adrList.begin(); it != adrList.end(); it++)
  {
    std::cout << "debug list index " << it->m_index << " addr " << it->m_addr << std::endl;
    this->m_serverIDToAddr[it->m_index] = it->m_addr;
    auto endpoint = this->m_clientEnginePtr->lookup(it->m_addr);
    this->m_serverToEndpoints[it->m_addr] = endpoint;
  }
  return 0;
  */
}

int ClientRPCWrapper::getServerNum(std::string serverAddr)
{
  tl::remote_procedure remotegetServerNum = this->m_clientEnginePtr->define("getServerNum");
  tl::endpoint serverEndpoint = this->lookup(serverAddr);
  // the parameters here should be consistent with the defination at the server end
  int serverNum = remotegetServerNum.on(serverEndpoint)();
  return serverNum;
}

void ClientRPCWrapper::endTimer(std::string serverAddr)
{
  tl::remote_procedure remotestartTimer =
    this->m_clientEnginePtr->define("endTimer").disable_response();
  // the parameters here should be consistent with the defination at the server end
  tl::endpoint serverEndpoint = this->lookup(serverAddr);
  remotestartTimer.on(serverEndpoint)();
  return;
}

// get MetaAddrWrapper List this contains the coresponding server that overlap with current querybox
// this can be called once for multiple get in different time step, since partition is fixed
std::vector<std::string> ClientRPCWrapper::getmetaServerList(
  std::string serverAddr, size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub)
{

  tl::remote_procedure remotegetmetaServerList =
    this->m_clientEnginePtr->define("getmetaServerList");
  // ask the master server
  tl::endpoint serverEndpoint = this->lookup(serverAddr);
  std::vector<std::string> metaserverList =
    remotegetmetaServerList.on(serverEndpoint)(dims, indexlb, indexub);
  return metaserverList;
}

// put trigger info into specific metadata server
int ClientRPCWrapper::putTriggerInfo(
  std::string serverAddr, std::string triggerName, DynamicTriggerInfo& dti)
{
  tl::remote_procedure remoteputTriggerInfo = this->m_clientEnginePtr->define("putTriggerInfo");
  tl::endpoint serverEndpoint = this->lookup(serverAddr);
  int status = remoteputTriggerInfo.on(serverEndpoint)(triggerName, dti);
  return status;
}

void ClientRPCWrapper::registerWatcher(std::vector<std::string> triggerNameList)
{

  std::string watcherAddr = this->m_clientEnginePtr->self();
  tl::remote_procedure remoteregisterWatchInfo = this->m_clientEnginePtr->define("registerWatcher");

  // register the self address to all the servers in the communication group
  for (auto it = this->m_serverToEndpoints.begin(); it != this->m_serverToEndpoints.end(); ++it)
  {
    tl::endpoint serverEndpoint = it->second;
    int status = remoteregisterWatchInfo.on(serverEndpoint)(watcherAddr, triggerNameList);
  }
  return;
}

// notify the data watcher
// be carefule, there are race condition here, if find the remote procedure dynamically
void ClientRPCWrapper::notifyBack(std::string watcherAddr, BlockSummary& bs)
{
  tl::remote_procedure remotenotifyBack = this->m_clientEnginePtr->define("rcvNotify");
  tl::endpoint serverEndpoint = this->lookup(watcherAddr);
  std::string result = remotenotifyBack.on(serverEndpoint)(bs);
  return;
}

// these will be called by analytics
// put the event into the coresponding master server of the task groups
void ClientRPCWrapper::putEventIntoQueue(
  std::string groupMasterAddr, std::string triggerName, EventWrapper& event)
{
  tl::remote_procedure remoteputEvent =
    this->m_clientEnginePtr->define("putEvent").disable_response();
  tl::endpoint serverEndpoint = this->lookup(groupMasterAddr);
  remoteputEvent.on(serverEndpoint)(triggerName, event);
  return;
}

// get a event from the event queue in task trigger
EventWrapper ClientRPCWrapper::getEventFromQueue(
  std::string groupMasterAddr, std::string triggerName)
{
  tl::remote_procedure remotegetEvent = this->m_clientEnginePtr->define("getEvent");
  tl::endpoint serverEndpoint = this->lookup(groupMasterAddr);
  EventWrapper event = remotegetEvent.on(serverEndpoint)(triggerName);
  return event;
}

void ClientRPCWrapper::deleteMetaStep(size_t step)
{
  // range all server and delete metadata for specific step
  tl::remote_procedure remotedeleteMeta = this->m_clientEnginePtr->define("deleteMetaStep");

  for (auto& kv : m_serverToEndpoints)
  {
    remotedeleteMeta.on(kv.second)(step);
  }
  return;
}

std::vector<double> ClientRPCWrapper::getStageStatus(std::string server)
{
  tl::remote_procedure remotegetEvent = this->m_clientEnginePtr->define("getStageStatus");
  tl::endpoint serverEndpoint = this->lookup(server);
  std::vector<double> status = remotegetEvent.on(serverEndpoint)();
  return status;
}

}