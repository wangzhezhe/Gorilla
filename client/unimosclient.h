/*
this is deprecated and we divide the large client into several smaller one
*/
#ifndef __UNIMOSCLIENT_H__
#define __UNIMOSCLIENT_H__

#include "../commondata/metadata.h"
#include "../utils/matrixtool.h"
#include <blockManager/blockManager.h>

#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <thallium.hpp>
#include <vector>

#include <vtkCharArray.h>
#include <vtkCommunicator.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

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
 * @brief The client that call the rpc services
 * this is usded by both user and the services
 */
struct UniClient
{
  UniClient(){};

  void initMaster(std::string masterAddr)
  {
    this->m_mygroupMasterEndpoint = this->m_clientEnginePtr->lookup(masterAddr);
    return;
  }



  // for server enginge, the client ptr is the pointer to the server engine
  // for the client code, the client engine is the pointer to the engine with the client mode
  UniClient(tl::engine* clientEnginePtr, std::string masterConfigFile, int rrbStartPosition)
  {
    m_clientEnginePtr = clientEnginePtr;
    m_mygroupMasterAddr = loadMasterAddr(masterConfigFile);
    m_position = rrbStartPosition;
    DEBUG("debug my position " << m_position << " rrbStartPosition " << rrbStartPosition);
    // std::cout << "load master Addr: " << m_masterAddr << std::endl;
    this->initMaster(m_mygroupMasterAddr);
  };

  // set the m_masterAddr separately when use this
  UniClient(tl::engine* clientEnginePtr, int rrbStartPosition)
  {
    m_clientEnginePtr = clientEnginePtr;
    m_position = rrbStartPosition;
  };

  // from the id to the string (this may used by the writer to fix the specific server)
  std::map<int, std::string> m_serverIDToAddr;

  // from the string to the endpoints, this is used by all clients
  // caculate this at the beginning when init the client to avoid the potential mercury mistakes
  std::map<std::string, tl::endpoint> m_serverToEndpoints;

  // the master addr of the current group
  std::string m_mygroupMasterAddr;
  tl::endpoint m_mygroupMasterEndpoint;

  // if there are multiple associated servers
  // store them into this set
  std::set<std::string> m_associatedAddrSet;

  // the associated address of other group(when there is one data server)
  std::string m_associatedDataServer = "";

  // the ptr that store the thallium client engine
  tl::engine* m_clientEnginePtr = NULL;

  // the block manager in memory
  BlockManager b_manager;

  // the in manager catch
  size_t m_bulkSize = 0;
  // this part manages the memory space for data transferring
  // it is similar to the load and unload platform in the port
  std::vector<std::pair<void*, std::size_t> > m_segments;
  tl::bulk m_dataBulk;

  // this value is used for round roubin
  int m_position = 0;
  // this value is set by writer
  int m_totalServerNum = 0;

  ~UniClient()
  {
    for (int i = 0; i < m_segments.size(); i++)
    {
      if (m_segments[i].first != nullptr)
      {
        free(m_segments[i].first);
      }
    }

    // std::cout << "destroy UniClient"<<std::endl;
  };

  tl::endpoint lookup(std::string& address);

  int getIDByRandom()
  {
    if (m_totalServerNum == 0)
    {
      throw std::runtime_error("total serverNum should not be zero");
    }

    srand(time(NULL));

    int serverId = rand() % m_totalServerNum;

    return serverId;
  };

  int updateDHT(std::string serverAddr, std::vector<MetaAddrWrapper> metaAddrWrapperList);

  // get the address of the server by round roubin pattern
  std::string getServerAddr();

  // put the raw data
  int putrawdata(size_t step, std::string varName, BlockSummary& dataSummary, void* dataContainer);

  // put the meta data to specific server addr
  int putmetadata(std::string serverAddr, size_t step, std::string varName, BlockDescriptor& rde);

  // get the address of the master server according
  std::string loadMasterAddr(std::string masterConfigFile);

  // get the coresponding list from any server
  // this metnod is unnecessary if the server end execute the data get operation
  std::vector<std::string> getmetaServerList(
    size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub);

  std::vector<BlockDescriptor> getblockDescriptorList(std::string serverAddr, size_t step,
    std::string varName, size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub);

  int getSubregionData(std::string serverAddr, std::string blockID, size_t dataSize, size_t dims,
    std::array<int, 3> indexlb, std::array<int, 3> indexub, void* dataContainer);

  MATRIXTOOL::MatrixView getArbitraryData(size_t step, std::string varName, size_t elemSize,
    size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub);

  int putTriggerInfo(std::string serverAddr, std::string triggerName, DynamicTriggerInfo& dti);

  std::string registerTrigger(size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub,
    std::string triggerName, DynamicTriggerInfo& dti);

  int getServerNum();

  int getAllServerAddr();

  void startTimer();

  void endTimer();

  void initPutRawData(size_t dataMallocSize);

  std::string executeRawFunc(std::string serverAddr, std::string blockID, std::string functionName,
    std::vector<std::string>& funcParameters);

  void registerWatcher(std::vector<std::string> triggerNameList);

  void notifyBack(std::string watcherAddr, BlockSummary& bs);

  void putEventIntoQueue(std::string groupMasterAddr, std::string triggerName, EventWrapper& event);

  EventWrapper getEventFromQueue(std::string groupMasterAddr, std::string triggerName);

  void eraseRawData(std::string serverAddr, std::string blockID);

  void deleteMetaStep(size_t step);

  void executeAsyncExp(int step, std::string blockid);

  std::vector<vtkSmartPointer<vtkPolyData> > aggregatePolyBySuffix(std::string blockIDSuffix);
};

}
#endif