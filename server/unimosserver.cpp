// the unified in memory server that contains both metadata manager and raw data
// manager
#include <spdlog/spdlog.h>

#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <string>
#include <thallium.hpp>
#include <thread>
#include <typeinfo>
#include <vector>

#include "mpi.h"

//#include <uuid/uuid.h>

#include "../client/ClientForStaging.hpp"
#include "../commondata/metadata.h"
#include "../utils/bbxtool.h"
#include "../utils/stringtool.h"
#include "../utils/uuid.h"
#include "settings.h"
#include "statefulConfig.h"
#include "unimosserver.hpp"

// for vtk
#include <vtkCharArray.h>
#include <vtkCommunicator.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataWriter.h>

// timer information
//#include "../putgetMeta/metaclient.h"

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#define BILLION 1000000000L

#ifdef USE_GNI
extern "C"
{
#include <rdmacred.h>
}
#include <margo.h>
#include <mercury.h>
#define DIE_IF(cond_expr, err_fmt, ...)                                                            \
  do                                                                                               \
  {                                                                                                \
    if (cond_expr)                                                                                 \
    {                                                                                              \
      fprintf(stderr, "ERROR at %s:%d (" #cond_expr "): " err_fmt "\n", __FILE__, __LINE__,        \
        ##__VA_ARGS__);                                                                            \
      exit(1);                                                                                     \
    }                                                                                              \
  } while (0)
#endif

namespace tl = thallium;

namespace GORILLA
{

// global variables

// The pointer to the enginge should be set as global element
// this shoule be initilized after the initilization of the argobot
tl::engine* globalServerEnginePtr = nullptr;
tl::engine* globalClientEnginePtr = nullptr;
ClientForStaging* clientStaging = nullptr;
UniServer* uniServer = nullptr;
statefulConfig* globalSconfig = nullptr;

// name of configure file, the write one line, the line is the rank0 addr
// std::string masterConfigFile = "./unimos_server.conf";
Settings gloablSettings;

// rank & proc number for current MPI process
int globalRank = 0;
int globalProc = 0;

const std::string serverCred = "Gorila_cred_conf";

// the endpoint is the self addr for specific server
void gatherIP(std::string endpoint)
{
  // spdlog::info ("current rank is: {}, current ip is: {}", globalRank,
  // endpoint);

  if (uniServer->m_addrManager == nullptr)
  {
    throw std::runtime_error("addrManager should not be null");
  }
  if (globalRank == 0)
  {
    uniServer->m_addrManager->ifMaster = true;
  }

  uniServer->m_addrManager->nodeAddr = endpoint;

  // maybe it is ok that only write the masternode's ip and provide the
  // interface of getting ipList for other clients

  // attention: there number should be changed if the endpoint is not ip
  // padding to 20 only for ip
  // for the ip, the longest is 15 add the start label and the end label
  // this number should longer than the address
  int msgPaddingLen = 200;
  if (endpoint.size() > msgPaddingLen)
  {
    throw std::runtime_error("current addr is longer than msgPaddingLen, reset the addr buffer to "
                             "make it larger");
    return;
  }
  int sendLen = msgPaddingLen;
  int sendSize = sendLen * sizeof(char);
  char* sendipStr = (char*)malloc(sendSize);
  sprintf(sendipStr, "H%sE", endpoint.c_str());

  std::cout << "check send ip: " << std::string(sendipStr) << std::endl;

  int rcvLen = sendLen;

  char* rcvString = NULL;

  // if (globalRank == 0)
  //{
  // it is possible that some ip are 2 digits and some are 3 digits
  // add extra space to avoid message truncated error
  // the logest ip is 15 digit plus one comma
  int rcvSize = msgPaddingLen * globalProc * sizeof(char);
  // std::cout << "sendSize: " << sendSize << ", rcvSize:" << rcvSize <<
  // std::endl;
  rcvString = (char*)malloc(rcvSize);
  {
    if (rcvString == NULL)
    {
      MPI_Abort(MPI_COMM_WORLD, 1);
      return;
    }
  }
  //}

  /*
  MPI_Gather(void* send_data,
  int send_count,
  MPI_Datatype send_datatype,
  void* recv_data,
  int recv_count,
  MPI_Datatype recv_datatype,
  int root,
  MPI_Comm communicator)
  */

  // attention, the recv part is the size of the buffer recieved from the each
  // thread instead of all the str refer to
  // https://stackoverflow.com/questions/37993214/segmentation-fault-on-mpi-gather-with-2d-arrays
  int error_code =
    MPI_Gather(sendipStr, sendLen, MPI_CHAR, rcvString, rcvLen, MPI_CHAR, 0, MPI_COMM_WORLD);
  if (error_code != MPI_SUCCESS)
  {
    std::cout << "error for rank " << globalRank << " get MPI_GatherError: " << error_code
              << std::endl;
  }
  // write to file for ip list json file if it is necessary
  // or expose the list by the rpc

  // broadcast the results to all the services
  // the size is different compared with the MPI_Gather
  error_code = MPI_Bcast(rcvString, rcvSize, MPI_CHAR, 0, MPI_COMM_WORLD);
  if (error_code != MPI_SUCCESS)
  {
    std::cout << "error for rank " << globalRank << " get MPI_BcastError: " << error_code
              << std::endl;
  }

  std::vector<std::string> ipList = IPTOOL::split(rcvString, msgPaddingLen * globalProc, 'H', 'E');

  for (int i = 0; i < ipList.size(); i++)
  {
    // store all server addrs
    spdlog::debug("rank {} add raw data server {}", globalRank, ipList[i]);
    uniServer->m_addrManager->m_endPointsLists.push_back(ipList[i]);
  }
  free(sendipStr);
  free(rcvString);
}

void hello(const tl::request& req)
{
  std::cout << "Hello World!" << std::endl;
}

void hello2(const tl::request& req)
{
  std::cout << "Hello World!" << std::endl;
}

void updateEndpoints(const tl::request& req, std::vector<std::string>& serverLists)
{
  // put the metadata into current epManager
  int size = serverLists.size();

  for (int i = 0; i < size; i++)
  {
    uniServer->m_addrManager->m_endPointsLists.push_back(serverLists[i]);
  }

  req.respond(0);
  return;
}

// currently we use the global dht for all the variables
// this can also be binded with specific variables
// when the global mesh changes, the dht changes
void updateDHT(const tl::request& req, std::vector<MetaAddrWrapper>& datawrapperList)
{
  // put the metadata into current epManager
  int size = datawrapperList.size();

  for (int i = 0; i < size; i++)
  {
    // TODO init dht
    spdlog::debug("rank {} add meta server, index {} and addr {}", globalRank,
      datawrapperList[i].m_index, datawrapperList[i].m_addr);
    // TODO add lock here
    uniServer->m_dhtManager->metaServerIDToAddr[datawrapperList[i].m_index] =
      datawrapperList[i].m_addr;
  }

  req.respond(0);
  return;
}

// this should be the addr of the compute nodes
// the meta nodes is invisiable for the data writer
void getAllServerAddr(const tl::request& req)
{
  std::vector<MetaAddrWrapper> adrList;

  int i = 0;

  for (auto it = uniServer->m_addrManager->m_endPointsLists.begin();
       it != uniServer->m_addrManager->m_endPointsLists.end(); it++)
  {
    spdlog::debug("getAllServerAddr, check all server endpoint id {} value {}", i, *it);
    MetaAddrWrapper mar(i, *it);
    adrList.push_back(mar);
    i++;
  }

  req.respond(adrList);
  return;
}

void getServerNum(const tl::request& req)
{
  // return number of the servers to the client
  int serNum = uniServer->m_addrManager->m_endPointsLists.size();
  req.respond(serNum);
}

void getaddrbyID(const tl::request& req, int serverID)
{
  std::string serverAddr = "";
  if (uniServer->m_dhtManager->metaServerIDToAddr.find(serverID) ==
    uniServer->m_dhtManager->metaServerIDToAddr.end())
  {
    req.respond(serverAddr);
  }
  serverAddr = uniServer->m_dhtManager->metaServerIDToAddr[serverID];
  req.respond(serverAddr);
}

// get server address by round roubin
void getaddrbyrrb(const tl::request& req)
{
  if (uniServer->m_addrManager->ifMaster == false)
  {
    req.respond(std::string("NOTMASTER"));
  }

  std::string serverAddr = uniServer->m_addrManager->getByRRobin();
  req.respond(serverAddr);
}

// remove the metadata of specific step
// even if it is undeletable
void forceEraseMetaAndRawManually(size_t step)
{
  // delete the raw data forcelly, it works for any types
  // delete coresponding rawdata in async way
  uniServer->m_metaManager->m_metaDataMapMutex.lock();

  // traverse the map and release memory for old data
  std::cout << "rank " << globalRank << " delete metadata step by manual " << step << std::endl;

  for (auto& kv : uniServer->m_metaManager->m_metaDataMap[step])
  {
    std::string varName = kv.first;
    // std::cout << "delete metadata varName " << varName << std::endl;

    for (auto& kvinner : kv.second.m_metadataBlock)
    {
      std::string varType = kvinner.first;
      // std::cout << "delete metadata varType " << varType << " size " <<
      // kvinner.second.size() << std::endl;
      for (auto& it : kvinner.second)
      {
        BlockDescriptor rde = it;
        // TODO chcek the status here
        clientStaging->eraseRawData(rde.m_rawDataServerAddr, rde.m_rawDataID);
      }
    }
  }

  // remove the outlayer information from the map
  uniServer->m_metaManager->m_metaDataMap.erase(step);
  uniServer->m_metaManager->m_metaDataMapMutex.unlock();

  return;
}

// delete the step stored on this server
void deleteMetaStep(const tl::request& req, size_t step)
{
  forceEraseMetaAndRawManually(step);
  req.respond(0);
}

// TODO some data is undeletable, add another parameter, if there is important
// data, but it needs long time to be consumed, this need to be keeped for some
// time be careful about the setting the lb and the ub of the window here this
// step is current step, that is the latest step
void eraseMetaAndRaw(size_t step)
{
  uniServer->m_metaManager->m_boundMutex.lock();
  uniServer->m_metaManager->m_windowub = step;

  bool ifOutOfBuffer =
    ((uniServer->m_metaManager->m_windowub - uniServer->m_metaManager->m_windowlb + 1) >
      uniServer->m_metaManager->m_windowSize);
  size_t currentlb = uniServer->m_metaManager->m_windowlb;
  size_t currentub = uniServer->m_metaManager->m_windowub;
  if (ifOutOfBuffer)
  {
    uniServer->m_metaManager->m_windowlb =
      uniServer->m_metaManager->m_windowlb + uniServer->m_metaManager->m_deletedNum;
    std::cout << "debug windowlb windowub " << uniServer->m_metaManager->m_windowlb << " "
              << uniServer->m_metaManager->m_windowub << std::endl;
  }
  uniServer->m_metaManager->m_boundMutex.unlock();

  // erase the lower bound data when step window is larger than threshold
  // only modify lb when the window is larger than buffer
  if (ifOutOfBuffer)
  {
    for (int i = 0; i < uniServer->m_metaManager->m_deletedNum; i++)
    {
      // TODO make sure do not delete key data, use better way here
      // if (currentlb % 5 == 1 || currentlb % 5 == 2 || currentlb % 5 == 3)
      //{
      //    continue;
      //}

      // delete coresponding rawdata in async way
      uniServer->m_metaManager->m_metaDataMapMutex.lock();

      // traverse the map and release memory for old data
      std::cout << "rank " << globalRank << " delete metadata step " << currentlb << std::endl;

      for (auto& kv : uniServer->m_metaManager->m_metaDataMap[currentlb])
      {
        std::string varName = kv.first;
        // std::cout << "delete metadata varName " << varName << std::endl;

        for (auto& kvinner : kv.second.m_metadataBlock)
        {
          std::string varType = kvinner.first;
          // std::cout << "delete metadata varType " << varType << " size " <<
          // kvinner.second.size() << std::endl; use the vector conditional
          // iterate and erase
          auto iter = kvinner.second.begin();
          while (iter != kvinner.second.end())
          {
            BlockDescriptor rde = *iter;
            // check the status
            // raw or after process , delete
            // std::cout << "curr status " << rde.m_metaStatus << std::endl;
            while (true)
            {
              if (rde.m_metaStatus == MetaStatus::BEFOREPROCESS ||
                rde.m_metaStatus == MetaStatus::AFTERPROCESS)
              {
                // erase metadata
                // TODO update the data structure to make it more efficient here

                // erase rawdata, and make it points to the next element
                iter = kvinner.second.erase(iter);
                clientStaging->eraseRawData(rde.m_rawDataServerAddr, rde.m_rawDataID);
                break;
              }
              else if (rde.m_metaStatus == MetaStatus::BEFOREPROCESS ||
                rde.m_metaStatus == MetaStatus::INPROCESS)
              {
                // std::cout << "debug wait the finish of the data with id " <<
                // rde.m_rawDataID << std::endl; usleep(500000); continue;
                // TODO neglect these data for this step
                // if we use sleep and continue, there will be a deadlock for
                // the trigger process since it could not acquire the lock
                ++iter;
                break;
              }
              else if (rde.m_metaStatus == MetaStatus::UNDELETABLE)
              {
                // can not be deleted
                ++iter;
                break;
              }
            }
          }
        }

        // erase coresponding metadata and the flag if it becomes empty
        // the current deleted step is i
        int blockSize =
          uniServer->m_metaManager->m_metaDataMap[currentlb][varName].getBlockNumberByVersion(
            DATATYPE_CARGRID);
        // std::cout << "debug metablock size for step: " << currentlb << "
        // variable: " << varName << " size: " << blockSize << std::endl;
        if (blockSize == 0)
        {
          // TODO, only remove the inner data if there are multiple versions in
          // furture
          uniServer->m_metaManager->m_metaDataMap[currentlb][varName].eraseBlocks(DATATYPE_CARGRID);
        }
        // std::cout << "debug m_metaDataMap size for step currentlb " <<
        // uniServer->m_metaManager->m_metaDataMap.count(currentlb) << std::endl;
      }

      // uniServer->m_metaManager->m_metaDataMap[currentlb] and
      // uniServer->m_metaManager->m_metaDataMap[currentlb][varName] can not be
      // deleted since they might be accessed by multiple threads if we want to
      // check if there are enough data before put opearation, we can use
      // coresponding data in the moritor manager [todo]
      currentlb++;

      uniServer->m_metaManager->m_metaDataMapMutex.unlock();
    }
  }
  return;
}

// the manager of the matadata controller should be maintained at this level
void putmetadata(const tl::request& req, size_t& step, std::string& varName, BlockDescriptor rde)
{
  // TODO update the status of the RDE here
  try
  {
    if (gloablSettings.addTrigger == true)
    {
      rde.m_metaStatus = MetaStatus::BEFOREPROCESS;
    }
    spdlog::debug("server {} put meta", uniServer->m_addrManager->nodeAddr);
    if (gloablSettings.logLevel > 0)
    {
      rde.printInfo();
      uniServer->m_metaManager->printInfo(globalRank, step, varName);
    }
    uniServer->m_metaManager->updateMetaData(step, varName, rde);
  }
  catch (const std::exception& e)
  {
    spdlog::info("exception for meta data put step {} varname {} server {}", step, varName,
      uniServer->m_addrManager->nodeAddr);
    req.respond(-1);
    return;
  }

  try
  {
    // execute init trigger
    // if the trigger is true
    // TODO, add a lable to say if the metadata can be deleted
    // the trigger need to access the metadata in this way
    // maybe to notify it by send an RPC call by action operation
    if (gloablSettings.addTrigger == true)
    {
      // create thread on a particular and put it into the pool associated with
      // margo instance, mix it with the pool to process the rpc
      // put the timer here, start to put the task
      std::string timerNameWait = rde.m_rawDataID + "_wait";
      uniServer->m_frawmanager->m_statefulConfig->startTimer(timerNameWait);
      globalServerEnginePtr->get_handler_pool().make_thread(
        [=]() {
          // time it
          // if (globalRank == 0)
          //{
          // time it to get the wait time
          uniServer->m_frawmanager->m_statefulConfig->endTimer(timerNameWait);

          std::string timerNameExecute = rde.m_rawDataID + "_execute";
          uniServer->m_frawmanager->m_statefulConfig->startTimer(timerNameExecute);
          //}
          uniServer->m_dtmanager->initstart("InitTrigger", step, varName, rde);
          uniServer->m_frawmanager->m_statefulConfig->endTimer(timerNameExecute);
        },
        tl::anonymous());
      // it is unnecessary to store the thread by using the anonymous pattern
      spdlog::debug("start trigger for var {} step {} data id {}", varName, step, rde.m_rawDataID);
    }
  }
  catch (std::exception& e)
  {
    spdlog::info(
      "exception for init trigger step {} varname {}: {}", step, varName, std::string(e.what()));
    rde.printInfo();
  }
  req.respond(0);

  // the server runs asyncrounously to delete the metadata, make it as separate
  // manager check meta to figure out if there is enough space by calling
  // eraseMetaAndRaw(); delete raw data related with current metaserver
  // eraseMetaAndRaw(step);

  return;
}

void eraserawdata(const tl::request& req, std::string& blockID)
{
  uniServer->m_blockManager->eraseBlock(blockID, BACKEND::MEM);
  req.respond(0);
  return;
}

// make sure if there is avalible memory
// and return the metadataWrapperList list
// TODO use the double bbx for next step
void getinfoForput(const tl::request& req, size_t step, size_t objSize, size_t bbxdim,
  std::array<int, 3> indexlb, std::array<int, 3> indexub)
{
  // if there is enough mem, return memobj write
  // if there is not enough mem space, return disk obj write
  // TODO add strategies value here
  // use the dummy version currently
  std::string putMethod;
  bool okForMem = uniServer->m_schedulerManager->oktoPutMem(objSize);

  if (okForMem)
  {
    putMethod = RDMAPUT;
  }
  else
  {
    putMethod = FILEPUT;
  }

  // get the overlapped metaserver with the bbx
  // get the meta server according to bbx
  BBXTOOL::BBX BBXQuery(bbxdim, indexlb, indexub);

  // if the query bbx is accoss mamy partitions
  // return multiple servers
  std::vector<ResponsibleMetaServer> metaserverList =
    uniServer->m_dhtManager->getMetaServerID(BBXQuery);

  // get the addr of the metaserver list
  InfoForPut ifp;
  ifp.m_putMethod = putMethod;
  for (auto it = metaserverList.begin(); it != metaserverList.end(); it++)
  {
    int metaServerId = it->m_metaServerID;
    if (uniServer->m_dhtManager->metaServerIDToAddr.find(metaServerId) ==
      uniServer->m_dhtManager->metaServerIDToAddr.end())
    {
      req.respond(ifp);
      throw std::runtime_error("faild to get the coresponding server id in dhtManager");
      return;
    }
    std::string destAddr = uniServer->m_dhtManager->metaServerIDToAddr[metaServerId];
    ifp.m_metaServerList.push_back(destAddr);
  }
  req.respond(ifp);
}

vtkSmartPointer<vtkPolyData> read_mesh2(const std::vector<float>& bufPoints,
  const std::vector<long>& bufOffsetCells, const std::vector<long>& bufConnectCells,
  const std::vector<float>& bufNormals)
{

  int nPoints = bufPoints.size() / 3;

  // get points
  auto points = vtkSmartPointer<vtkPoints>::New();

  points->SetNumberOfPoints(nPoints);

  for (vtkIdType i = 0; i < nPoints; i++)
  {
    // std::cout << "set point value " << *(tempp + (i * 3)) << " " << *(tempp + (i * 3) + 1) << " "
    //          << *(tempp + (i * 3) + 2) << std::endl;
    points->SetPoint(i, &bufPoints[i * 3]);
  }

  // get cells
  auto polys = vtkSmartPointer<vtkCellArray>::New();

  auto newoffsetArray = vtkSmartPointer<vtkTypeInt64Array>::New();
  newoffsetArray->SetNumberOfComponents(1);
  newoffsetArray->SetNumberOfTuples(bufOffsetCells.size());

  for (vtkIdType i = 0; i < bufOffsetCells.size(); i++)
  {
    newoffsetArray->SetValue(i, bufOffsetCells[i]);
  }

  auto newconnectivityArray = vtkSmartPointer<vtkTypeInt64Array>::New();
  newconnectivityArray->SetNumberOfComponents(1);
  newconnectivityArray->SetNumberOfTuples(bufConnectCells.size());

  for (vtkIdType i = 0; i < bufConnectCells.size(); i++)
  {
    newconnectivityArray->SetValue(i, bufConnectCells[i]);
  }

  polys->SetData(newoffsetArray, newconnectivityArray);

  // get normal
  auto normals = vtkSmartPointer<vtkFloatArray>::New();
  normals->SetNumberOfComponents(3);

  for (vtkIdType i = 0; i < nPoints; i++)
  {
    normals->InsertNextTuple(&bufNormals[i * 3]);
  }

  // generate poly data
  auto newpolyData = vtkSmartPointer<vtkPolyData>::New();
  newpolyData->SetPoints(points);
  newpolyData->SetPolys(polys);
  newpolyData->GetPointData()->SetNormals(normals);

  return newpolyData;
}

vtkSmartPointer<vtkPolyData> read_mesh(const std::vector<double>& bufPoints,
  const std::vector<int>& bufCells, const std::vector<double>& bufNormals)
{
  int nPoints = bufPoints.size() / 3;
  int nCells = bufCells.size() / 3;

  auto points = vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints(nPoints);
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    points->SetPoint(i, &bufPoints[i * 3]);
  }

  auto polys = vtkSmartPointer<vtkCellArray>::New();
  for (vtkIdType i = 0; i < nCells; i++)
  {
    vtkIdType a = bufCells[i * 3 + 0];
    vtkIdType b = bufCells[i * 3 + 1];
    vtkIdType c = bufCells[i * 3 + 2];

    polys->InsertNextCell(3);
    polys->InsertCellPoint(a);
    polys->InsertCellPoint(b);
    polys->InsertCellPoint(c);
  }

  auto normals = vtkSmartPointer<vtkDoubleArray>::New();
  normals->SetNumberOfComponents(3);
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    normals->InsertNextTuple(&bufNormals[i * 3]);
  }

  auto polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);
  polyData->SetPolys(polys);
  polyData->GetPointData()->SetNormals(normals);

  return polyData;
}

void putvtkexpzero(const tl::request& req, int clientID, size_t& step, std::string& varName,
  BlockSummary& blockSummary, tl::bulk& dataBulk, std::vector<size_t>& transferSizeList)
{

  struct timespec start, end1, end2, end3;
  double diff1, diff2, diff3;
  clock_gettime(CLOCK_REALTIME, &start);

  // get data and change it into the vtk object
  // the size of the segment should be a list in this case
  // this can be optimized for multiple objects
  // there is also memory leak for this way
  // the server may need to maintain a list of segments for every attached client
  std::vector<std::pair<void*, std::size_t> > segments(transferSizeList.size());
  for (int i = 0; i < transferSizeList.size(); i++)
  {
    segments[i].first = (void*)malloc(transferSizeList[i]);
    segments[i].second = transferSizeList[i];
  }

  spdlog::debug("ok for init segments list size is {} ", transferSizeList.size());

  // transfer data
  tl::bulk currentBulk = globalServerEnginePtr->expose(segments, tl::bulk_mode::write_only);

  clock_gettime(CLOCK_REALTIME, &end1);
  diff1 = (end1.tv_sec - start.tv_sec) * 1.0 + (end1.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
  spdlog::debug("server registermem: {}", diff1);

  tl::endpoint ep = req.get_endpoint();
  // pull the data onto the server
  dataBulk.on(ep) >> currentBulk;

  clock_gettime(CLOCK_REALTIME, &end2);
  diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
  spdlog::debug("server transfer: {}", diff2);

  // extract the data and assemble them into the vtk
  // assume we use the polygonal data
  std::vector<float> bufPoints(transferSizeList[0] / sizeof(float));
  std::vector<float> bufNormals(transferSizeList[1] / sizeof(float));
  std::vector<long> bufOffsetCells(transferSizeList[2] / sizeof(long));
  std::vector<long> bufConnectCells(transferSizeList[3] / sizeof(long));

  // assign transfered value to the vector array
  memcpy(bufPoints.data(), segments[0].first, transferSizeList[0]);
  memcpy(bufNormals.data(), segments[1].first, transferSizeList[1]);
  memcpy(bufOffsetCells.data(), segments[2].first, transferSizeList[2]);
  memcpy(bufConnectCells.data(), segments[3].first, transferSizeList[3]);

  auto polydata = read_mesh2(bufPoints, bufOffsetCells, bufConnectCells, bufNormals);

  clock_gettime(CLOCK_REALTIME, &end3);
  diff3 = (end3.tv_sec - end2.tv_sec) * 1.0 + (end3.tv_nsec - end2.tv_nsec) * 1.0 / BILLION;
  spdlog::debug("server unmarshal: {}", diff3);

  // test if poly data ok
  polydata->PrintSelf(std::cout, vtkIndent(5));

  // write the data for futher checking
  // vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  // writer->SetFileName("./serverrecvpoly.vtp");
  // get the specific polydata and check the results
  // writer->SetInputData(polydata);
  // writer->Write();
  // release the memory space of the segments
  for (int i = 0; i < segments.size(); i++)
  {
    if (segments[i].first != NULL)
    {
      free(segments[i].first);
    }
  }
  req.respond(0);
  return;
}

// some issues to use this
// maintain more segments, this will increase the complexity of managing the transfer process
// the number of the segments is hard to know in advance
// the size of the segments is hard to know
void putvtkexp(const tl::request& req, int clientID, size_t& step, std::string& varName,
  BlockSummary& blockSummary, tl::bulk& dataBulk, std::vector<size_t>& transferSizeList)
{

  struct timespec start, end1, end2, end3;
  double diff1, diff2, diff3;
  clock_gettime(CLOCK_REALTIME, &start);

  // get data and change it into the vtk object
  // the size of the segment should be a list in this case
  // this can be optimized for multiple objects
  // there is also memory leak for this way
  // the server may need to maintain a list of segments for every attached client
  std::vector<std::pair<void*, std::size_t> > segments(transferSizeList.size());
  for (int i = 0; i < transferSizeList.size(); i++)
  {
    segments[i].first = (void*)malloc(transferSizeList[i]);
    segments[i].second = transferSizeList[i];
  }

  spdlog::debug("ok for init segments list size is {} ", transferSizeList.size());

  // transfer data
  tl::bulk currentBulk = globalServerEnginePtr->expose(segments, tl::bulk_mode::write_only);

  clock_gettime(CLOCK_REALTIME, &end1);
  diff1 = (end1.tv_sec - start.tv_sec) * 1.0 + (end1.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
  spdlog::debug("server registermem: {}", diff1);

  tl::endpoint ep = req.get_endpoint();
  // pull the data onto the server
  dataBulk.on(ep) >> currentBulk;

  clock_gettime(CLOCK_REALTIME, &end2);
  diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
  spdlog::debug("server transfer: {}", diff2);

  // extract the data and assemble them into the vtk
  // assume we use the polygonal data
  std::vector<double> bufPoints(transferSizeList[0] / sizeof(double));
  std::vector<double> bufNormals(transferSizeList[1] / sizeof(double));
  std::vector<int> bufCells(transferSizeList[2] / sizeof(int));

  // assign transfered value to the vector array
  memcpy(bufPoints.data(), segments[0].first, transferSizeList[0]);
  memcpy(bufNormals.data(), segments[1].first, transferSizeList[1]);
  memcpy(bufCells.data(), segments[2].first, transferSizeList[2]);

  auto polydata = read_mesh(bufPoints, bufCells, bufNormals);

  clock_gettime(CLOCK_REALTIME, &end3);
  diff3 = (end3.tv_sec - end2.tv_sec) * 1.0 + (end3.tv_nsec - end2.tv_nsec) * 1.0 / BILLION;
  spdlog::debug("server unmarshal: {}", diff3);

  // test if poly data ok
  polydata->PrintSelf(std::cout, vtkIndent(5));

  // write the data for futher checking
  // vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  // writer->SetFileName("./serverrecvpoly.vtp");
  // get the specific polydata and check the results
  // writer->SetInputData(polydata);
  // writer->Write();
  // release the memory space of the segments
  for (int i = 0; i < segments.size(); i++)
  {
    if (segments[i].first != NULL)
    {
      free(segments[i].first);
    }
  }
  req.respond(0);
  return;
}

void expcheckdata(const tl::request& req, BlockSummary& blockSummary)
{
  // extract the necessary info to assemble the full data
  // the data might not regonized well in this case
  // since we use the async call

  void* pointArrayptr = NULL;
  void* cellArrayPtr = NULL;
  void* normalArrayPtr = NULL;

  ArraySummary arrayPoints = uniServer->m_blockManager->getArray(
    blockSummary.m_blockid, "points", BACKEND::MEMVTKEXPLICIT, pointArrayptr);
  ArraySummary arrayCells = uniServer->m_blockManager->getArray(
    blockSummary.m_blockid, "cells", BACKEND::MEMVTKEXPLICIT, cellArrayPtr);
  ArraySummary arrayNormals = uniServer->m_blockManager->getArray(
    blockSummary.m_blockid, "normals", BACKEND::MEMVTKEXPLICIT, normalArrayPtr);

  // after normal put (the last one)
  // try to assemble the data into the poly

  // generate the points array
  int nPoints = arrayPoints.m_elemNum / 3;
  auto points = vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints(nPoints);
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    const double* tempp = (double*)pointArrayptr;
    points->SetPoint(i, tempp + (i * 3));
  }

  // generate normal array
  auto normals = vtkSmartPointer<vtkDoubleArray>::New();
  normals->SetNumberOfComponents(3);
  const double* tempn = (double*)normalArrayPtr;

  for (vtkIdType i = 0; i < nPoints; i++)
  {
    normals->InsertNextTuple(tempn + (i * 3));
  }

  // generate cell array
  int nCells = arrayCells.m_elemNum / 3;
  auto polys = vtkSmartPointer<vtkCellArray>::New();
  const int* tempc = (int*)cellArrayPtr;

  for (vtkIdType i = 0; i < nCells; i++)
  {
    vtkIdType a = *(tempc + (i * 3 + 0));
    vtkIdType b = *(tempc + (i * 3 + 1));
    vtkIdType c = *(tempc + (i * 3 + 2));

    polys->InsertNextCell(3);
    polys->InsertCellPoint(a);
    polys->InsertCellPoint(b);
    polys->InsertCellPoint(c);
  }

  auto polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);
  polyData->SetPolys(polys);
  polyData->GetPointData()->SetNormals(normals);

  // polyData->PrintSelf(std::cout, vtkIndent(5));
  std::cout << "expcheckdata cell number: " << polyData->GetNumberOfPolys() << std::endl;
  return;
}

void getArraysExplicit(const tl::request& req, std::string& blockID, tl::bulk& clientBulk)
{

  // check existance of the block
  void* pointArrayptr = NULL;
  void* normalArrayPtr = NULL;
  void* cellArrayPtr = NULL;

  ArraySummary arrayPoints =
    uniServer->m_blockManager->getArray(blockID, "points", BACKEND::MEMVTKEXPLICIT, pointArrayptr);
  ArraySummary arrayNormals = uniServer->m_blockManager->getArray(
    blockID, "normals", BACKEND::MEMVTKEXPLICIT, normalArrayPtr);
  ArraySummary arrayCells =
    uniServer->m_blockManager->getArray(blockID, "cells", BACKEND::MEMVTKEXPLICIT, cellArrayPtr);

  if (pointArrayptr == NULL || cellArrayPtr == NULL || normalArrayPtr == NULL)
  {
    throw std::runtime_error("failed to extract array for getArraysExplicit");
  }

  // expose the region, points, normals, cells
  std::vector<std::pair<void*, std::size_t> > segments(3);
  segments[0].first = pointArrayptr;
  segments[0].second = arrayPoints.m_elemNum * arrayPoints.m_elemSize;

  segments[1].first = normalArrayPtr;
  segments[1].second = arrayNormals.m_elemNum * arrayNormals.m_elemSize;

  segments[2].first = cellArrayPtr;
  segments[2].second = arrayCells.m_elemNum * arrayCells.m_elemSize;

  // transfer data
  tl::bulk returnBulk = globalServerEnginePtr->expose(segments, tl::bulk_mode::read_only);
  tl::endpoint ep = req.get_endpoint();
  clientBulk.on(ep) << returnBulk;
  req.respond(0);
}

// step and varname are not necessary here
// it only useful to update the metadata
void putArrayIntoBlock(const tl::request& req, BlockSummary& blockSummary,
  ArraySummary& arraySummary, tl::bulk& dataBulk)
{

  // get the size of the data
  size_t transferSize = arraySummary.m_elemNum * arraySummary.m_elemSize;
  // operator new only allocate memory without calling constructor
  void* memspace = (void*)::operator new(transferSize);

  // get the transfered data by exposing the mem space
  std::vector<std::pair<void*, std::size_t> > segments(1);
  segments[0].first = memspace;
  segments[0].second = transferSize;

  spdlog::debug("recv array {} size {}", std::string(arraySummary.m_arrayName), transferSize);

  // transfer data
  tl::bulk currentBulk = globalServerEnginePtr->expose(segments, tl::bulk_mode::write_only);

  tl::endpoint ep = req.get_endpoint();
  // pull the data onto the server
  dataBulk.on(ep) >> currentBulk;

  uniServer->m_blockManager->putArray(blockSummary, arraySummary, blockSummary.m_backend, memspace);
  // the memspace will be deleted when the object is deleted
  req.respond(0);
  return;
}

// put the raw data into the raw data manager
void putrawdata(const tl::request& req, int clientID, size_t& step, std::string& varName,
  BlockSummary& blockSummary, tl::bulk& dataBulk)
{
  struct timespec start, end1, end2, end3;
  double diff1, diff2, diff3;
  clock_gettime(CLOCK_REALTIME, &start);

  if (blockSummary.m_backend != BACKEND::MEM)
  {
    throw std::runtime_error("the backend is supposed to be mem for putrawdata");
  }

  // assume data is different when every rawdataput is called
  // generate the unique id for new data
  spdlog::debug("execute raw data put for server id {}", globalRank);
  if (gloablSettings.logLevel > 0)
  {
    blockSummary.printSummary();
  }

  spdlog::debug(
    "blockID is {} on server {} ", blockSummary.m_blockid, uniServer->m_addrManager->nodeAddr);

  // assign the memory
  // use the same array id with the block summary
  size_t transferSize = blockSummary.getArraySize(blockSummary.m_blockid);

  // the space are allocated previously in order to save data put time
  // TODO, the client can choose how many channel are exposed at the same time
  // currently we assume there is one channel per client
  // create the new memory space when the clientid is not exist

  uniServer->m_bulkMapmutex.lock();
  if (uniServer->m_bulkMap.find(clientID) == uniServer->m_bulkMap.end())
  {

    // update the accouting info for mem space
    uniServer->m_schedulerManager->assignMem(transferSize);
    uniServer->m_dataContainerMapmutex.lock();

    uniServer->m_dataContainerMap[clientID] = (void*)malloc(transferSize);

    std::vector<std::pair<void*, std::size_t> > segments(1);
    segments[0].first = uniServer->m_dataContainerMap[clientID];
    segments[0].second = transferSize;
    uniServer->m_dataContainerMapmutex.unlock();

    uniServer->m_bulkMap[clientID] =
      globalServerEnginePtr->expose(segments, tl::bulk_mode::write_only);

    spdlog::info("allocate new bulk for clientID {} serverRank {} size {}", clientID, globalRank,
      transferSize);
  }
  else
  {
    // TODO need to be updated here, even for the same client
    // the data size can also change？how to process this?
    size_t oldSize = uniServer->m_bulkMap[clientID].size();
    // if old size not equal with the previous one
    // reallocate the space
    if (oldSize != transferSize)
    {
      spdlog::debug("old size {} new size {}", oldSize, transferSize);
      uniServer->m_dataContainerMapmutex.lock();
      // create the new mem when the client exists but the memsize is not enough
      if (uniServer->m_dataContainerMap[clientID] != nullptr)
      {
        free(uniServer->m_dataContainerMap[clientID]);
        uniServer->m_schedulerManager->releaseMem(oldSize);
      }

      uniServer->m_schedulerManager->assignMem(transferSize);
      uniServer->m_dataContainerMap[clientID] = (void*)malloc(transferSize);
      spdlog::info("resize the new memsize of client {} to {}", clientID, transferSize);
      uniServer->m_dataContainerMapmutex.unlock();
    }

    // if the size is not equal with the previous one
    // just resize it(we need to create a new bulk), no matter it is larger or smaller
    uniServer->m_dataContainerMapmutex.lock();
    std::vector<std::pair<void*, std::size_t> > segments(1);
    segments[0].first = uniServer->m_dataContainerMap[clientID];
    segments[0].second = transferSize;
    uniServer->m_dataContainerMapmutex.unlock();

    uniServer->m_bulkMap[clientID] =
      globalServerEnginePtr->expose(segments, tl::bulk_mode::write_only);
  }

  tl::bulk currentBulk = uniServer->m_bulkMap[clientID];
  uniServer->m_bulkMapmutex.unlock();

  clock_gettime(CLOCK_REALTIME, &end1);
  diff1 = (end1.tv_sec - start.tv_sec) * 1.0 + (end1.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
  spdlog::debug("server put registermem : {}", diff1);

  tl::endpoint ep = req.get_endpoint();
  // pull the data onto the server
  dataBulk.on(ep) >> currentBulk;

  clock_gettime(CLOCK_REALTIME, &end2);
  diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
  spdlog::debug("server put transfer : {}", diff2);
  // spdlog::debug("Server received bulk, check the contents: ");
  // check the bulk
  // double *rawdata = (double *)localContainer;
  /*check the data contents at server end
  for (int i = 0; i < 10; i++)
  {
     std::cout << "index " << i << " value " << *rawdata << std::endl;
      rawdata++;
  }
  */

  // For the raw mem object, copy from the original place
  // For the vtkptr, we need to create a separate object before the put operation
  // since for put, we only store a pointer

  // generate the empty container firstly, then get the data pointer
  // the data will be stored into the memory by put block operation
  // what if another request comes during the copy operation from the transferblock to real
  // block?? one transfer block only associated with one client currently
  // for the vtk objects, we need to marshal the data before put operation
  size_t elemSize = blockSummary.getArrayElemSize(blockSummary.m_blockid);
  size_t elemNum = blockSummary.getArrayElemNum(blockSummary.m_blockid);
  if (std::string(blockSummary.m_dataType) == DATATYPE_VTKPTR)
  {

    // try to recv the data
    vtkSmartPointer<vtkCharArray> recvbuffer = vtkSmartPointer<vtkCharArray>::New();

    // set key properties, type, numTuples, numComponents, name,
    spdlog::debug("component {} tuples {} in recvbuffer", elemSize, elemNum);
    recvbuffer->SetNumberOfComponents(elemSize);
    recvbuffer->SetNumberOfTuples(elemNum);

    // try to simulate the data recv process
    // when the memory of the vtkchar array is allocated???
    // TODO add the condition varible here
    // start to memcopy when the datacontainer is not used
    uniServer->m_dataContainerMapmutex.lock();
    memcpy(recvbuffer->GetPointer(0), uniServer->m_dataContainerMap[clientID], transferSize);
    uniServer->m_dataContainerMapmutex.unlock();

    // check the data content
    // recvbuffer->PrintSelf(std::cout, vtkIndent(5));
    // std::cout << "------check recvbuffer content: ------" << std::endl;
    // for (int i = 0; i < recvSize; i++)
    //{
    //  std::cout << recvbuffer->GetValue(i);
    // }
    // std::cout << "------" << std::endl;

    // check the recvarray

    // unmarshal
    vtkSmartPointer<vtkDataObject> recvbj = vtkCommunicator::UnMarshalDataObject(recvbuffer);
    spdlog::debug("---finish to unmarshal vtk obj");
    // if the origianl data is poly data
    vtkDataObject* recvptr = recvbj;
    // how to decide what data it is here???maybe contains the data type???
    // TODO, maybe implements the array or block expression, and let the data type to be more
    // specific
    // ((vtkPolyData*)recvptr)->PrintSelf(std::cout, vtkIndent(5));
    // (recvptr)->PrintSelf(std::cout, vtkIndent(5));
    clock_gettime(CLOCK_REALTIME, &end3);
    diff3 = (end3.tv_sec - end2.tv_sec) * 1.0 + (end3.tv_nsec - end2.tv_nsec) * 1.0 / BILLION;
    spdlog::debug("server put unmarshal : {}", diff3);

    // this vtkDataObject should be managemet by the datablock manager
    // we use void* as a bridge to transfer it to the vtksmartPointer managed by the blockmanager
    int status = uniServer->m_blockManager->putBlock(blockSummary, BACKEND::MEMVTKPTR, &recvbj);
    uniServer->m_schedulerManager->assignMem(blockSummary.getArraySize(blockSummary.m_blockid));

    spdlog::debug("---put vtk block {} on server {} map size {}", blockSummary.m_blockid,
      uniServer->m_addrManager->nodeAddr, uniServer->m_blockManager->DataBlockMap.size());
    if (status != 0)
    {
      blockSummary.printSummary();
      req.respond(status);
      throw std::runtime_error("failed to put the vtk data");
      return;
    }

    // put into the Block Manager
    req.respond(status);
  }
  else
  {
    // supposed to be the cargird
    spdlog::debug("---put block {} on server {} map size {}", blockSummary.m_blockid,
      uniServer->m_addrManager->nodeAddr, uniServer->m_blockManager->DataBlockMap.size());

    // persist the data, and put it into the blockManager
    uniServer->m_dataContainerMapmutex.lock();
    int status = uniServer->m_blockManager->putBlock(
      blockSummary, BACKEND::MEM, uniServer->m_dataContainerMap[clientID]);
    uniServer->m_dataContainerMapmutex.unlock();

    spdlog::debug("--- ok put block {} on server {}", blockSummary.m_blockid,
      uniServer->m_addrManager->nodeAddr);

    if (status != 0)
    {
      blockSummary.printSummary();
      throw std::runtime_error("failed to put the raw data");
      req.respond(status);
      return;
    }

    uniServer->m_schedulerManager->assignMem(blockSummary.getArraySize(blockSummary.m_blockid));
    req.respond(status);
  }

  return;
}

void registerWatcher(
  const tl::request& req, std::string watcherAddr, std::vector<std::string>& triggerName)
{
  // range the triggerName, if current trigger name is registered
  // put the watcherAddr into the triggerManger
  for (auto it = triggerName.begin(); it != triggerName.end(); ++it)
  {
    std::string triggerName = *it;
    spdlog::debug("register watcher for trigger {}", triggerName);
    // if trigger is on this node
    if (uniServer->m_dtmanager->m_dynamicTrigger.find(triggerName) !=
      uniServer->m_dtmanager->m_dynamicTrigger.end())
    {
      // register the watcher if it is not empty
      uniServer->m_dtmanager->m_watcherSetMutex.lock();
      uniServer->m_dtmanager->m_registeredWatcherSet.insert(watcherAddr);

      auto endpoint = clientStaging->m_clientEnginePtr->lookup(watcherAddr);
      clientStaging->m_serverToEndpoints[watcherAddr] = endpoint;

      uniServer->m_dtmanager->m_watcherSetMutex.unlock();
    }
    else
    {
      spdlog::info("try to watch unregistered trigger {}", triggerName);
    }
  }
  req.respond(0);
}
// TODO, the trigger should also know the master among all knows that holds this
// trigger every trigger is a computing group essentially it needs to know the
// master addr of this group
void putTriggerInfo(const tl::request& req, std::string triggerName, DynamicTriggerInfo& dti)
{
  try
  {
    uniServer->m_dtmanager->updateTrigger(triggerName, dti);
    spdlog::info("add trigger {} for server id {}", triggerName, globalRank);
    req.respond(0);
    return;
  }
  catch (...)
  {
    spdlog::info("exception for putTriggerInfo with trigger name {}", triggerName);
    dti.printInfo();
    req.respond(-1);
    return;
  }
}

void getmetaServerList(
  const tl::request& req, size_t& dims, std::array<int, 3>& indexlb, std::array<int, 3>& indexub)
{
  std::vector<std::string> metaServerAddr;
  try
  {
    BBXTOOL::BBX BBXQuery(dims, indexlb, indexub);
    std::vector<ResponsibleMetaServer> metaserverList =
      uniServer->m_dhtManager->getMetaServerID(BBXQuery);
    for (auto it = metaserverList.begin(); it != metaserverList.end(); it++)
    {
      int metaServerId = it->m_metaServerID;
      if (uniServer->m_dhtManager->metaServerIDToAddr.find(metaServerId) ==
        uniServer->m_dhtManager->metaServerIDToAddr.end())
      {
        throw std::runtime_error("faild to get the coresponding server id in dhtManager for "
                                 "getmetaServerList");
      }
      metaServerAddr.push_back(uniServer->m_dhtManager->metaServerIDToAddr[metaServerId]);
    }

    req.respond(metaServerAddr);
  }
  catch (std::exception& e)
  {
    spdlog::info("exception for getmetaServerList: {}", std::string(e.what()));
    req.respond(metaServerAddr);
  }
}

// TODO one issue here
// this blockmap is supposed to be cleaned periodically
// otherwise, it will getting slower for the large step
// since we basically range the map to get if there are required variable
// oneway is to put the data into the metadata, retrieve metadata server to get required data
// firstly another way is to add another layer (step) when storing the rawdata maybe use a doube map
// here? map<step, map<varname,interface >> if current strategy is not efficient
void getBlockSummayBySuffix(const tl::request& req, std::string& varSuffix)
{
  std::vector<BlockSummary> blockSummaryList;
  bool needCheck = false;
  uniServer->m_blockManager->m_DataBlockMapMutex.lock();
  // TODO how to update this?
  // if there are multiple data blocks, the operation of iterating map is time consuming
  for (auto& kv : uniServer->m_blockManager->DataBlockMap)
  {
    // put the blockSummary into the list
    if (kv.first.find(varSuffix) != std::string::npos)
    {
      // the key contains the suffix
      blockSummaryList.push_back(kv.second->m_blockSummary);
      if (kv.second->m_blockSummary.m_arrayListLen != 3)
      {
        needCheck = true;
      }
    }
  }
  uniServer->m_blockManager->m_DataBlockMapMutex.unlock();

  // check the array len
  // if it is less then three
  // the data might not put into the block since the all threads are executed asynronously
  // require these data
  if (needCheck)
  {

    for (int i = 0; i < blockSummaryList.size(); i++)
    {
      if (blockSummaryList[i].m_arrayListLen != 3)
      {

        std::string blockid = blockSummaryList[i].m_blockid;
        while (uniServer->m_blockManager->DataBlockMap[blockid]->m_blockSummary.m_arrayListLen != 3)
        {
          tl::thread::sleep(*globalServerEnginePtr, 200);
        }
        blockSummaryList[i] = uniServer->m_blockManager->DataBlockMap[blockid]->m_blockSummary;
      }
    }
  }

  req.respond(blockSummaryList);
  return;
}

void getBlockDescriptorList(const tl::request& req, size_t& step, std::string& varName,
  size_t& dims, std::array<int, 3>& indexlb, std::array<int, 3>& indexub)
{
  std::vector<BlockDescriptor> blockDescriptorList;
  try
  {
    BBXTOOL::BBX BBXQuery(dims, indexlb, indexub);

    // check if all required partition is avalible, get raw endpoint first then
    // execute check operation
    std::vector<BlockDescriptor> rdeplist =
      uniServer->m_metaManager->getRawEndpoints(step, varName);

    // get overlap between the query
    // and the boundry that this process respond to
    BBX queryBBX(dims, indexlb, indexub);

    BBX* overlap =
      getOverlapBBX(queryBBX, *(uniServer->m_dhtManager->metaServerIDToBBX[globalRank]));
    if (overlap == NULL)
    {
      req.respond(blockDescriptorList);
      throw std::runtime_error("dht error, there should overlap between queried bbx and current "
                               "metadata partition\n");
      return;
    }
    // it needs some time to update the metadata after the data put operation
    // it only makes sense to get all registered data when data endpoints for a particular
    // step.var.datatype are all registered into the metadata server
    // other wise, we might miss some data partitions
    bool ifcover = uniServer->m_metaManager->ifCovered(rdeplist, *overlap);
    if (ifcover == false)
    {
      // return a none array if it is not covered
      if (overlap != NULL)
      {
        delete overlap;
      }
      req.respond(blockDescriptorList);
      return;
    }
    blockDescriptorList = uniServer->m_metaManager->getOverlapEndpoints(step, varName, BBXQuery);
    if (overlap != NULL)
    {
      delete overlap;
    }
    req.respond(blockDescriptorList);
    return;
  }
  catch (std::exception& e)
  {
    spdlog::info("exception for getBlockDescriptorList: {}", std::string(e.what()));
    req.respond(blockDescriptorList);
  }
  return;
}

void startTimer(const tl::request& req)
{
  std::string timerName = "main";
  uniServer->m_frawmanager->m_statefulConfig->startTimer(timerName);
  spdlog::info("start timer for rank {}", globalRank);
  return;
}

void endTimer(const tl::request& req)
{
  std::string timerName = "main";
  uniServer->m_frawmanager->m_statefulConfig->endTimer(timerName);
  return;
}

void tickTimer(const tl::request& req)
{
  std::string timerName = "main";
  uniServer->m_frawmanager->m_statefulConfig->tickTimer(timerName);
  return;
}

void getStageStatus(const tl::request& req)
{
  // get the current schedule time and execution time
  // return value is a vector, first is schedule time second is execution time
  std::vector<double> stageStatus(2);
  // TODO adjust if the key exist, if not exist, return 0
  if (uniServer->m_metricManager->metricExist("default_schedule") == false)
  {
    stageStatus[0] = 0;
  }
  else
  {
    stageStatus[0] = uniServer->m_metricManager->getLastNmetrics("default_schedule", 1)[0];
  }
  if (uniServer->m_metricManager->metricExist("default_ana") == false)
  {
    stageStatus[1] = 0;
  }
  else
  {
    stageStatus[1] = uniServer->m_metricManager->getLastNmetrics("default_ana", 1)[0];
  }

  req.respond(stageStatus);
  return;
}

// execute raw func in async way
// this function will trigger one function
// this function will gather all polydata firstly and then execute the filter action
void executeAsyncExp(const tl::request& req, std::string& blockIDSuffix, int& blockIndex,
  std::string& functionName, std::vector<std::string>& funcParameters, bool ifLastStep)
{
  spdlog::debug("---server rank {} start executeAsyncExp for block {}", globalRank, blockIDSuffix);

  std::string blockCompleteName = blockIDSuffix + "_" + std::to_string(blockIndex);
  std::string timerNameSchedule = "schedule_" + blockCompleteName;
  std::string executeNameSchedule = "execute_" + blockCompleteName;

  globalSconfig->startTimer(timerNameSchedule);

  globalServerEnginePtr->get_handler_pool().make_thread(
    [=]() {
      double timeSpan = globalSconfig->endTimer(timerNameSchedule);
      // put data into the metric manager
      uniServer->m_metricManager->putMetric("default_schedule", timeSpan);

      // it is posible that this server contians data with same idsuffix (varname+step)

      globalSconfig->startTimer(executeNameSchedule);

      // record the task scheduling time
      if (functionName == "testaggrefunc")
      {
        uniServer->m_frawmanager->aggregateProcess(
          clientStaging, blockIDSuffix, functionName, funcParameters);
      }
      else if (functionName == "testisoExec")
      {

        uniServer->m_frawmanager->testisoExec(blockCompleteName, funcParameters);
      }
      else
      {
        std::cout << "unsupported function name " << functionName << std::endl;
      }

      timeSpan = globalSconfig->endTimer(executeNameSchedule);
      // put data into the metric manager
      uniServer->m_metricManager->putMetric("default_ana", timeSpan);

      // remove the current data if it is processed
      uniServer->m_blockManager->eraseBlock(blockCompleteName, BACKEND::MEM);
      // tick timer
      std::string timerName = "main";
      //TODO the clientStaging send request to the master server
      //if contians the last step when ifLastStep is true
      
    },
    tl::anonymous());

  return;
}

// get the schedule time of the current staging service
void getScheduleTime() {}

// this basic function that is called on the node
// with the block data mamager
// it basically extract the necessary data and execute particular function on it
// it looks this should be the syncronous call (we also provide the async version)
// since we need the direct results here
void executeRawFunc(const tl::request& req, std::string& blockID, std::string& functionName,
  std::vector<std::string>& funcParameters)
{
  // get block summary
  BlockSummary bs = uniServer->m_blockManager->getBlockSummary(blockID);
  // TODO check if the blockSummary is empty
  // NO id
  // check data existance
  if (uniServer->m_blockManager->DataBlockMap.find(blockID) ==
    uniServer->m_blockManager->DataBlockMap.end())
  {
    req.respond(std::string("NOTEXIST"));
    return;
  }

  DataBlockInterface* dbi = uniServer->m_blockManager->DataBlockMap[blockID];
  // get particular data
  void* rawDataPtr = dbi->getrawMemPtr();

  std::string timerNameExecute = blockID + "_execute";
  uniServer->m_frawmanager->m_statefulConfig->startTimer(timerNameExecute);

  // for testing, use the thred pool
  std::string exeResults = uniServer->m_frawmanager->execute(
    uniServer->m_frawmanager, bs, rawDataPtr, functionName, funcParameters);
  uniServer->m_frawmanager->m_statefulConfig->endTimer(timerNameExecute);

  // it is unnecessary to store the thread by using the anonymous pattern
  spdlog::debug("finish execution blockID {}", blockID);

  req.respond(exeResults);
  return;
}

// std::string blockID, int backend
void getPolyMeta()
{
  // check if reguired data is in the current server
  // return the array size in the vtk object

  // get block
  // return ArraySummary
  // return 0 size if there is no poly data
}

// This function is only called for the mem backend
void getDataSubregion(const tl::request& req, std::string& blockID, size_t& dims,
  std::array<int, 3>& subregionlb, std::array<int, 3>& subregionub, tl::bulk& clientBulk)
{
  try
  {
    void* dataContainer = NULL;

    spdlog::debug("map size on server {} is {}", uniServer->m_addrManager->nodeAddr,
      uniServer->m_blockManager->DataBlockMap.size());
    if (uniServer->m_blockManager->checkDataExistance(blockID, BACKEND::MEM) == false)
    {
      throw std::runtime_error("failed to get block id " + blockID + " on server with rank id " +
        std::to_string(globalRank));
    }
    BlockSummary bs = uniServer->m_blockManager->getBlockSummary(blockID);
    // TODO check if the summary is empty (no data id)
    size_t elemSize = bs.getArrayElemSize(bs.m_blockid);
    BBXTOOL::BBX bbx(dims, subregionlb, subregionub);
    size_t allocSize = elemSize * bbx.getElemNum();
    spdlog::debug("alloc size at server {} is {}", globalRank, allocSize);

    uniServer->m_blockManager->getBlockSubregion(
      blockID, BACKEND::MEM, dims, subregionlb, subregionub, dataContainer);

    // TODO, this can still be optimized
    // reuse the segments for data put instead of allocate new segment every time
    std::vector<std::pair<void*, std::size_t> > segments(1);
    segments[0].first = (void*)(dataContainer);
    segments[0].second = allocSize;

    tl::bulk returnBulk = globalServerEnginePtr->expose(segments, tl::bulk_mode::read_only);

    tl::endpoint ep = req.get_endpoint();
    clientBulk.on(ep) << returnBulk;

    req.respond(0);
  }
  catch (std::exception& e)
  {
    spdlog::info("exception for getDataSubregion block id {} lb {},{},{} ub {},{},{}", blockID,
      subregionlb[0], subregionlb[1], subregionlb[2], subregionub[0], subregionub[1],
      subregionub[2]);
    spdlog::info("exception for getDataSubregion: {}", std::string(e.what()));
    req.respond(-1);
  }
}

void putEvent(const tl::request& req, std::string& triggerName, EventWrapper& event)
{
  uniServer->m_dtmanager->putEvent(triggerName, event);
  return;
}

void getEvent(const tl::request& req, std::string& triggerName)
{
  EventWrapper event = uniServer->m_dtmanager->getEvent(triggerName);
  req.respond(event);
  return;
}

void initDHT()
{
  int dataDims = gloablSettings.lenArray.size();
  // config the dht manager
  if (gloablSettings.partitionMethod.compare("SFC") == 0)
  {
    int maxLen = 0;
    for (int i = 0; i < dataDims; i++)
    {
      maxLen = std::max(maxLen, gloablSettings.lenArray[i]);
    }

    BBX* globalBBX = new BBX(dataDims);
    for (int i = 0; i < dataDims; i++)
    {
      Bound tempb(0, maxLen - 1);
      globalBBX->BoundList.push_back(tempb);
    }
    uniServer->m_dhtManager->initDHTBySFC(dataDims, gloablSettings.metaserverNum, globalBBX);
  }
  else if (gloablSettings.partitionMethod.compare("manual") == 0)
  {
    // check the metaserverNum match with the partitionLayout
    int totalPartition = 1;
    for (int i = 0; i < gloablSettings.partitionLayout.size(); i++)
    {
      totalPartition = totalPartition * gloablSettings.partitionLayout[i];
    }
    if (totalPartition != gloablSettings.metaserverNum)
    {
      throw std::runtime_error("metaserverNum should equals to the product of partitionLayout[i]");
    }

    uniServer->m_dhtManager->initDHTManually(
      gloablSettings.lenArray, gloablSettings.partitionLayout);
  }
  else
  {
    throw std::runtime_error("unsuported partition method " + gloablSettings.partitionMethod);
  }

  if (globalRank == 0)
  {
    // print metaServerIDToBBX
    for (auto it = uniServer->m_dhtManager->metaServerIDToBBX.begin();
         it != uniServer->m_dhtManager->metaServerIDToBBX.end(); it++)
    {
      std::cout << "init DHT, meta id " << it->first << std::endl;
      it->second->printBBXinfo();
    }
  }
  return;
}

void runRerver(std::string networkingType)
{
#ifdef USE_GNI
  uint32_t drc_credential_id = 0;
  drc_info_handle_t drc_credential_info;
  uint32_t drc_cookie;
  char drc_key_str[256] = { 0 };
  int ret;

  struct hg_init_info hii;
  memset(&hii, 0, sizeof(hii));

  if (globalRank == 0)
  {
    ret = drc_acquire(&drc_credential_id, DRC_FLAGS_FLEX_CREDENTIAL);
    DIE_IF(ret != DRC_SUCCESS, "drc_acquire");

    ret = drc_access(drc_credential_id, 0, &drc_credential_info);
    DIE_IF(ret != DRC_SUCCESS, "drc_access");
    drc_cookie = drc_get_first_cookie(drc_credential_info);
    sprintf(drc_key_str, "%u", drc_cookie);
    hii.na_init_info.auth_key = drc_key_str;

    ret = drc_grant(drc_credential_id, drc_get_wlm_id(), DRC_FLAGS_TARGET_WLM);
    DIE_IF(ret != DRC_SUCCESS, "drc_grant");

    spdlog::debug("grant the drc_credential_id: {}", drc_credential_id);
    spdlog::debug("use the drc_key_str {}", std::string(drc_key_str));
    for (int dest = 1; dest < globalProc; dest++)
    {
      // dest tag communicator
      MPI_Send(&drc_credential_id, 1, MPI_UINT32_T, dest, 0, MPI_COMM_WORLD);
    }

    // write this cred_id into file that can be shared by clients
    // output the credential id into the config files
    std::ofstream credFile;
    credFile.open(serverCred);
    credFile << drc_credential_id << "\n";
    credFile.close();
  }
  else
  {
    // send rcv is the block call
    // gather the id from the rank 0
    // source tag communicator
    MPI_Recv(&drc_credential_id, 1, MPI_UINT32_T, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    spdlog::debug("rank {} recieve cred key {}", globalRank, drc_credential_id);

    if (drc_credential_id == 0)
    {
      throw std::runtime_error("failed to rcv drc_credential_id");
    }
    ret = drc_access(drc_credential_id, 0, &drc_credential_info);
    DIE_IF(ret != DRC_SUCCESS, "drc_access %u", drc_credential_id);
    drc_cookie = drc_get_first_cookie(drc_credential_info);

    sprintf(drc_key_str, "%u", drc_cookie);
    hii.na_init_info.auth_key = drc_key_str;
  }

  margo_instance_id mid;
  // the number here should same with the number of cores used in test scripts
  // the loop process is 0, the main thread will work on it
  // mid = margo_init_opt("gni", MARGO_SERVER_MODE, &hii, 0, 8);
  // tl::engine serverEnginge(mid);

  // the latest engine can be created based on hii
  tl::engine serverEnginge("ofi+gni", THALLIUM_SERVER_MODE, false, 8, &hii);
  globalServerEnginePtr = &serverEnginge;

#else
  if (globalRank == 0)
  {
    spdlog::debug("use the protocol other than gni: {}", networkingType);
  }
  tl::engine serverEnginge(networkingType, THALLIUM_SERVER_MODE);
  globalServerEnginePtr = &serverEnginge;

#endif

  globalServerEnginePtr->define("updateDHT", updateDHT);
  globalServerEnginePtr->define("getAllServerAddr", getAllServerAddr);
  // globalServerEnginePtr->define("getServerNum", getServerNum);
  // globalServerEnginePtr->define("getaddrbyID", getaddrbyID);
  globalServerEnginePtr->define("getaddrbyrrb", getaddrbyrrb);
  globalServerEnginePtr->define("getinfoForput", getinfoForput);
  globalServerEnginePtr->define("putrawdata", putrawdata);
  globalServerEnginePtr->define("eraserawdata", eraserawdata);
  globalServerEnginePtr->define("putmetadata", putmetadata);
  globalServerEnginePtr->define("getmetaServerList", getmetaServerList);
  globalServerEnginePtr->define("getBlockDescriptorList", getBlockDescriptorList);
  globalServerEnginePtr->define("getDataSubregion", getDataSubregion);
  globalServerEnginePtr->define("putTriggerInfo", putTriggerInfo);
  globalServerEnginePtr->define("executeRawFunc", executeRawFunc);
  globalServerEnginePtr->define("registerWatcher", registerWatcher);
  globalServerEnginePtr->define("putEvent", putEvent).disable_response();
  globalServerEnginePtr->define("getEvent", getEvent);
  globalServerEnginePtr->define("deleteMetaStep", deleteMetaStep);
  globalServerEnginePtr->define("putvtkexp", putvtkexp);
  globalServerEnginePtr->define("putvtkexpzero", putvtkexpzero);

  globalServerEnginePtr->define("putArrayIntoBlock", putArrayIntoBlock);
  globalServerEnginePtr->define("expcheckdata", expcheckdata).disable_response();
  globalServerEnginePtr->define("executeAsyncExp", executeAsyncExp).disable_response();
  globalServerEnginePtr->define("getBlockSummayBySuffix", getBlockSummayBySuffix);
  globalServerEnginePtr->define("getArraysExplicit", getArraysExplicit);
  globalServerEnginePtr->define("getStageStatus", getStageStatus);

  globalServerEnginePtr->define("startTimer", startTimer).disable_response();
  globalServerEnginePtr->define("endTimer", endTimer).disable_response();

  // for testing
  globalServerEnginePtr->define("hello", hello).disable_response();

  std::string rawAddr = globalServerEnginePtr->self();
  std::string selfAddr = IPTOOL::getClientAdddr(networkingType, rawAddr);
  char tempAddr[200];
  std::string masterAddr;

  if (globalRank == 0)
  {
    spdlog::debug("Start the unimos server with addr for master: {}", selfAddr);

    masterAddr = selfAddr;
    // broadcast the master addr to all the servers
    std::copy(selfAddr.begin(), selfAddr.end(), tempAddr);
    tempAddr[selfAddr.size()] = '\0';
  }

  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Bcast(tempAddr, 200, MPI_CHAR, 0, MPI_COMM_WORLD);
  if (globalRank != 0)
  {
    masterAddr = std::string(tempAddr);
    spdlog::info("rank {} load master addr {}", globalRank, masterAddr);
  }

  // the server engine can also be the client engine, only one engine can be
  // declared here
  globalClientEnginePtr = &serverEnginge;
  clientStaging = new ClientForStaging(globalClientEnginePtr, masterAddr, globalRank);

  // init stateful config (this may use MPI, do not mix it with mochi)
  globalSconfig = new statefulConfig();
  // init all the important manager of the server
  uniServer = new UniServer();
  uniServer->initManager(globalProc, globalRank, gloablSettings.metaserverNum,
    gloablSettings.memLimit, clientStaging, true);

  // init the DHT
  // this is initilized based on the partition layout
  // if there are large amount of the nodes in cluster
  // but the meta server is 1, it is also ok
  initDHT();

  // gather IP to the rank0 and broadcaster the IP to all the services
  // address manager knows all the server info after this operation
  gatherIP(selfAddr);

  // write master server to file, server init ok
  if (globalRank == 0)
  {
    std::ofstream confFile;
    spdlog::debug("master info file: {}", gloablSettings.masterInfo);
    confFile.open(gloablSettings.masterInfo);

    if (!confFile.is_open())
    {
      spdlog::info("Could not open file: {}", gloablSettings.masterInfo);
      exit(-1);
    }
    confFile << masterAddr << "\n";
    confFile.close();
  }

  // bradcaster the ip to all the worker nodes use the thallium api
  // this should be after the init of the server
  clientStaging->initEndPoints(uniServer->m_addrManager->m_endPointsLists);

  if (uniServer->m_addrManager->ifMaster)
  {
    // there is gathered address information only for the master node
    // master will broad cast the list to all the servers
    // init the endpoint for all the slave node
    uniServer->m_addrManager->broadcastMetaServer(clientStaging);
  }

  spdlog::info("init server ok, call margo wait for rank {}", globalRank);

  // call the ADIOS init explicitly
  uniServer->m_frawmanager->m_statefulConfig = globalSconfig;
  // test if the engine is normal

  // std::cout << "---debug adios io name in in server: " <<
  // uniServer->m_frawmanager->m_statefulConfig->m_io.Name() << std::endl;
  // std::cout << "--- debug engine type in server " <<
  // uniServer->m_frawmanager->m_statefulConfig->m_engine.Type() << std::endl;
  spdlog::info("init globalSconfig ok, call margo wait for rank {}", globalRank);

#ifdef USE_GNI
  // destructor will not be called if send mid to engine
  // do not use this if we start server by tahllium
  // margo_wait_for_finalize(mid);
#endif

  // the destructor of the engine will be called when the variable is out of the
  // scope
  return;
}

void signalHandler(int signal_num)
{
  std::cout << "The interrupt signal is (" << signal_num << "),"
            << " call finalize manually.\n";

  // finalize client and server
  // if (uniServer != nullptr)
  //{
  //  delete uniServer;
  //}
  // if (uniClient != nullptr)
  //{
  //  delete uniClient;
  //}
  // globalServerEnginePtr->finalize();
  exit(signal_num);
}

} // namespace gorilla

int main(int argc, char** argv)
{
  MPI_Init(NULL, NULL);

  MPI_Comm_rank(MPI_COMM_WORLD, &GORILLA::globalRank);
  MPI_Comm_size(MPI_COMM_WORLD, &GORILLA::globalProc);

  // auto file_logger = spdlog::basic_logger_mt("unimos_server_log",
  // "unimos_server_log.txt"); spdlog::set_default_logger(file_logger);

  if (argc != 2)
  {
    std::cerr << "Usage: unimos_server <path of setting.json>" << std::endl;
    exit(0);
  }

  tl::abt scope;

  // the copy operator is called here
  Settings tempsetting = Settings::from_json(argv[1]);
  GORILLA::gloablSettings = tempsetting;
  if (GORILLA::globalRank == 0)
  {
    GORILLA::gloablSettings.printsetting();
  }

  std::string networkingType = GORILLA::gloablSettings.protocol;

  int logLevel = GORILLA::gloablSettings.logLevel;

  if (logLevel == 0)
  {
    spdlog::set_level(spdlog::level::info);
  }
  else
  {
    spdlog::set_level(spdlog::level::debug);
  }

  spdlog::debug("debug mode");

  signal(SIGINT, GORILLA::signalHandler);
  signal(SIGQUIT, GORILLA::signalHandler);
  signal(SIGTSTP, GORILLA::signalHandler);

  if (GORILLA::globalRank == 0)
  {
    std::cout << "total process num: " << GORILLA::globalProc << std::endl;
  }

  if (GORILLA::gloablSettings.metaserverNum > GORILLA::globalProc)
  {
    throw std::runtime_error("number of metaserver should less than the number of process");
  }

  try
  {
    GORILLA::runRerver(networkingType);
  }
  catch (const std::exception& e)
  {
    std::cout << "exception for server: " << e.what() << std::endl;
    // release the resource
    return 1;
  }

  std::cout << "server close" << std::endl;

  MPI_Finalize();
  return 0;
}
