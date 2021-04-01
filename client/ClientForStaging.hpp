// necessary processings in staging service
// put data to staging server
#ifndef __CLIENTFORSTAGE_H__
#define __CLIENTFORSTAGE_H__

#include "ClientRPCWrapper.hpp"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <unordered_map>

#include <vtkCharArray.h>
#include <vtkCommunicator.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#define BILLION 1000000000L
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
 * @brief this class manage the client used for the data staging service
 */
class ClientForStaging : public ClientRPCWrapper
{
public:
  ClientForStaging(tl::engine* clientEnginePtr, std::string addrServer, int rank)
    : ClientRPCWrapper(clientEnginePtr, addrServer)
    , m_masterAddr(addrServer)
    , m_rank(rank)
  {
    // endpoints are initilized by initlizeMaster separately for the stageclient
  }
  ~ClientForStaging(){};

  // my rank and address server
  int m_rank;
  std::string m_masterAddr;

  // associated clients addresses
  std::vector<std::string> m_associatedClients;

  tl::mutex m_mutex_clientAddrToID;
  std::unordered_map<std::string, int> m_clientAddrToID;
  int m_base = 0;

  std::vector<vtkSmartPointer<vtkPolyData> > aggregatePolyBySuffix(std::string blockIDSuffix);

  void cacheClientAddr(const std::string& clientAddr);

  int getIDFromClientAddr(const std::string& clientAddr);
};

}

#endif