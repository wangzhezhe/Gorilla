// client for data producer and consumer
// there is only one data for consumer
// if the in-memory cache is used, we also need to access it
// so we put the consumer and producer together
#ifndef __CLIENTFORSIM_H__
#define __CLIENTFORSIM_H__

#include "ClientRPCWrapper.hpp"
#include <blockManager/blockManager.h>

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

struct Watcher
{
  Watcher(){};
  // watch the trigger in the list
  // TODO, add a function handler
  // the function will be called if there is notification
  std::string startWatch(tl::engine* enginePtr);
  // close the server
  // int end(){};
  ~Watcher(){};

  std::string m_watcherAddr;
};

/**
 * @brief this class manage how to get the server address in the format of string
 * and related cache
 */
class ClientForSim : public ClientRPCWrapper
{
public:
  ClientForSim(tl::engine* clientEnginePtr, std::string addrServer, int rank)
    : ClientRPCWrapper(clientEnginePtr, addrServer)
    , m_addrServer(addrServer)
    , m_rank(rank)
  {
    // load all the server addrs
    // we assume the server is started before the clients
    std::vector<MetaAddrWrapper> adrList = this->getAllServerAddr();
    // caculate the total server number
    this->m_totalServerNum = adrList.size();
    // get endpoints and put it into the cache
    for (auto it = adrList.begin(); it != adrList.end(); it++)
    {
      std::cout << "debug list index " << it->m_index << " addr " << it->m_addr << std::endl;
      this->m_serverIDToAddr[it->m_index] = it->m_addr;
      // check the endpoint and put it into the cache
      auto endpoint = this->m_clientEnginePtr->lookup(it->m_addr);
      this->m_serverToEndpoints[it->m_addr] = endpoint;
    }
    getAssociatedServerAddr();
  };

  virtual ~ClientForSim(){};

  // manage all kinds of server address and particular
  // rpc for the simulation program

  // this value is used for round roubin
  int m_rank = 0;
  int m_totalServerNum = 0;
  // from the id to server addr
  std::map<int, std::string> m_serverIDToAddr;

  std::string m_addrServer;
  // this might be different with the addr server
  std::string m_associatedDataServer = "";

  // the cache for the data
  size_t m_bulkSize = 0;
  // this part manages the memory space for data transferring
  // it is similar to the load and unload platform in the port
  std::vector<std::pair<void*, std::size_t> > m_segments;
  tl::bulk m_dataBulk;

  // the block manager in memory
  BlockManager b_manager;

  std::string getAssociatedServerAddr();

  void initPutRawData(size_t dataTransferSize);

  int putCarGrid(
    size_t step, std::string varName, BlockSummary& dataSummary, void* dataContainerSrc);

  int putVTKDataExp(
    size_t step, std::string varName, BlockSummary& dataSummary, void* dataContainerSrc);

  int putVTKDataExpZeroOneRpc(
    size_t step, std::string varName, BlockSummary& dataSummary, void* dataContainerSrc);

  int putArrayIntoBlock(BlockSummary& dataSummary, void* dataContainerSrc);

  int putArrayIntoBlockZero(BlockSummary& dataSummary, void* dataContainerSrc);

  int putVTKData(
    size_t step, std::string varName, BlockSummary& dataSummary, void* dataContainerSrc);

  void executeAsyncExp(int step, std::string blockid);

  int putrawdata(
    size_t step, std::string varName, BlockSummary& dataSummary, void* dataContainerSrc);

  std::string registerTrigger(size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub,
    std::string triggerName, DynamicTriggerInfo& dti);

  MATRIXTOOL::MatrixView getArbitraryData(size_t step, std::string varName, size_t elemSize,
    size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub);
};
}
#endif