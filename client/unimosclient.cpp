#include "unimosclient.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#define BILLION 1000000000L

#include <vtkCharArray.h>
#include <vtkCommunicator.h>
#include <vtkSmartPointer.h>

namespace GORILLA
{

tl::endpoint UniClient::lookup(std::string& address)
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

void UniClient::startTimer()
{
  tl::remote_procedure remotestartTimer =
    this->m_clientEnginePtr->define("startTimer").disable_response();
  // the parameters here should be consistent with the defination at the server end
  remotestartTimer.on(this->m_masterEndpoint)();
  return;
}

void UniClient::endTimer()
{
  tl::remote_procedure remotestartTimer =
    this->m_clientEnginePtr->define("endTimer").disable_response();
  // the parameters here should be consistent with the defination at the server end
  remotestartTimer.on(m_masterEndpoint)();
  return;
}

int UniClient::getAllServerAddr()
{
  tl::remote_procedure remotegetAllServerAddr = this->m_clientEnginePtr->define("getAllServerAddr");
  // the parameters here should be consistent with the defination at the server end
  std::vector<MetaAddrWrapper> adrList = remotegetAllServerAddr.on(m_masterEndpoint)();

  if (adrList.size() == 0)
  {
    throw std::runtime_error("failed to get server list");
    return -1;
  }

  for (auto it = adrList.begin(); it != adrList.end(); it++)
  {
    std::cout << "debug list index " << it->m_index << " addr " << it->m_addr << std::endl;
    this->m_serverIDToAddr[it->m_index] = it->m_addr;
    auto endpoint = this->m_clientEnginePtr->lookup(it->m_addr);
    this->m_serverToEndpoints[it->m_addr] = endpoint;
  }

  return 0;
}

int UniClient::getServerNum()
{
  tl::remote_procedure remotegetServerNum = this->m_clientEnginePtr->define("getServerNum");
  // the parameters here should be consistent with the defination at the server end
  int serverNum = remotegetServerNum.on(m_masterEndpoint)();
  return serverNum;
}

// this is called by server node, the endpoint is not initilized yet
int UniClient::updateDHT(std::string serverAddr, std::vector<MetaAddrWrapper> metaAddrWrapperList)
{
  // std::cout << "debug updateDHT server addr " << serverAddr << std::endl;
  tl::remote_procedure remoteupdateDHT = this->m_clientEnginePtr->define("updateDHT");
  tl::endpoint serverEndpoint = this->lookup(serverAddr);
  // the parameters here should be consistent with the defination at the server end
  int status = remoteupdateDHT.on(serverEndpoint)(metaAddrWrapperList);
  return status;
}

std::string UniClient::getServerAddr()
{
  if (this->m_totalServerNum == 0)
  {
    throw std::runtime_error("m_totalServerNum is not initilizsed");
  }
  // check the cache, return if exist
  int serverId = this->m_position % this->m_totalServerNum;

  if (this->m_serverIDToAddr.find(serverId) == this->m_serverIDToAddr.end())
  {
    throw std::runtime_error("server id not exist " + std::to_string(this->m_position) + " " +
      std::to_string(this->m_totalServerNum));
  }

  return this->m_serverIDToAddr[serverId];
}

std::string UniClient::loadMasterAddr(std::string masterConfigFile)
{

  std::ifstream infile(masterConfigFile);
  std::string content = "";
  std::getline(infile, content);
  // spdlog::debug("load master server conf {}, content -{}-", masterConfigFile,content);
  if (content.compare("") == 0)
  {
    std::getline(infile, content);
    if (content.compare("") == 0)
    {
      throw std::runtime_error("failed to load the master server\n");
    }
  }
  return content;
}

// when there is no segment, assign it
// when there is existing segments but new one is larger than it
// allocate the new size
// the databulk will be freeed (margo bulk free) when the assignment or movement constructor is
// called
void UniClient::initPutRawData(size_t dataTransferSize)
{
  if (this->m_bulkSize == 0)
  {
    // if the current bulk size is 0 allocate the new bulk
    this->m_bulkSize = dataTransferSize;
    this->m_segments = std::vector<std::pair<void*, std::size_t> >(1);
    this->m_segments[0].first = (void*)malloc(dataTransferSize);
    this->m_segments[0].second = dataTransferSize;
    // register the memory
    this->m_dataBulk = this->m_clientEnginePtr->expose(this->m_segments, tl::bulk_mode::read_only);
  }
  else if (this->m_bulkSize >= dataTransferSize)
  {
    // do nothing if current bulk size is larger then malloc size
    return;
  }
  else
  {
    // assign the new memory space and release the old one
    // the old bulk is released when execute the assign operator
    // server also need to do the similar operations
    DEBUG("resize the bulk from " << this->m_bulkSize << " to " << dataTransferSize);
    if (this->m_segments[0].first != nullptr)
    {
      free(this->m_segments[0].first);
      this->m_segments[0].first == nullptr;
    }
    this->m_bulkSize = dataTransferSize;
    this->m_segments = std::vector<std::pair<void*, std::size_t> >(1);
    this->m_segments[0].first = (void*)malloc(dataTransferSize);
    this->m_segments[0].second = dataTransferSize;
    // register the memory
    this->m_dataBulk = this->m_clientEnginePtr->expose(this->m_segments, tl::bulk_mode::read_only);
  }

  return;
}

int putCarGrid(UniClient* client, size_t step, std::string varName, BlockSummary& dataSummary,
  void* dataContainerSrc)
{
  struct timespec start, end1, end2;
  double diff1, diff2;
  clock_gettime(CLOCK_REALTIME, &start);
  size_t dataTransferSize = dataSummary.getTotalSize();
  // currently, when there is no associated data server
  // the data bulk (memory space) is not assigned
  // when there is assiciated data server, we assume the size of the data bulk is fixed
  // this might works for cargid
  // but for vtk, the data size might change, this need to be solved
  if (client->m_associatedDataServer.compare("") == 0)
  {
    client->m_associatedDataServer = client->getServerAddr();
    // std::cout << "m_position " << m_position << " m_associatedDataServer " <<
    // this->m_associatedDataServer << std::endl; assume the size of the data block is fixed
    client->initPutRawData(dataTransferSize);
  }

  if (dataTransferSize > client->m_bulkSize)
  {
    throw std::runtime_error("the tran of the data size");
  }

  // prepare the info for data put
  // ask the placement way, use raw mem or write out the file
  tl::remote_procedure remotegetInfoForPut = client->m_clientEnginePtr->define("getinfoForput");
  tl::endpoint serverEndpoint = client->lookup(client->m_associatedDataServer);
  // const tl::request& req, size_t step, size_t objSize, size_t bbxdim,
  // std::array<int, 3> indexlb, std::array<int, 3> indexub)
  InfoForPut ifp = remotegetInfoForPut.on(serverEndpoint)(
    step, dataTransferSize, dataSummary.m_dims, dataSummary.m_indexlb, dataSummary.m_indexub);

  // create the raw data endpoint that will be stored at the metadata server
  // the data block is the minimal granularity that stores the data object
  // maybe try to use the explicit id? the data can be generated by different ranks
  // TODO the data type can be set by users in block summary
  // the blocksummary is the metadata of the file, its length need to be fixed, so we use char array
  // for the block descriptor, it will be stored at the index server, we can use the string
  // this is the bdec that will be indexed by the metadata server
  // Attention to the backend, it needs to be set separately
  BlockDescriptor bdesc("", dataSummary.m_blockid, std::string(dataSummary.m_dataType),
    dataSummary.m_dims, dataSummary.m_indexlb, dataSummary.m_indexub);

  // check the data summary
  if (strcmp(dataSummary.m_blockid, "") == 0)
  {
    throw std::runtime_error("dataSummary.m_blockid should not be empty string");
    return -1;
  }

  DEBUG("debug ifp.m_putMethod " << ifp.m_putMethod);

  // store the data obj into the file if the putmethod is file
  // the fileput just provides some functions for data put or get
  // instead of a place that contains the actual data
  // We may want to let the in-transit service become the metaserver
  // and the backend can be flexible, therefore, the client should have the capability to store and
  // process the data objects
  if (ifp.m_putMethod.compare(FILEPUT) == 0)
  {
    if (std::string(dataSummary.m_dataType) == DATATYPE_VTKPTR)
    {
      throw std::runtime_error("not supported for the vtk ptr yet");
    }

    dataSummary.m_backend = BACKEND::FILE;
    bdesc.m_backend = BACKEND::FILE;

    // write the data
    int status = client->b_manager.putBlock(dataSummary, BACKEND::FILE, dataContainerSrc);
    if (status != 0)
    {
      throw std::runtime_error("failed to put the data into the file");
      return -1;
    }
  }
  else if (ifp.m_putMethod.compare(RDMAPUT) == 0)
  {
    // update the server addr in raw data endpoint
    bdesc.m_rawDataServerAddr = client->m_associatedDataServer;
    // reuse the existing memory space
    // attention, if the simulation time is short consider to use the lock here
    // for every process, there are large span between two data write opertaion
    memcpy(client->m_segments[0].first, dataContainerSrc, dataTransferSize);

    tl::remote_procedure remotePutRawData = client->m_clientEnginePtr->define("putrawdata");
    tl::endpoint serverEndpoint = client->lookup(client->m_associatedDataServer);
    int status = remotePutRawData.on(serverEndpoint)(
      client->m_position, step, varName, dataSummary, client->m_dataBulk);
    // example for async put
    // auto request = remotePutRawData.on(this->m_serverEndpoint).async(this->m_position ,step,
    // varName, dataSummary, this->m_dataBulk);

    // bool completed = request.received();
    // ...
    // actually wait on the request and get the result out of it
    // int status = request.wait();
    if (status != 0)
    {
      throw std::runtime_error("failed to put the raw data");
      return -1;
    }

    // update the meta
    clock_gettime(CLOCK_REALTIME, &end1);
    diff1 = (end1.tv_sec - start.tv_sec) * 1.0 + (end1.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
    // std::cout << "put stage 1: " << diff1 << std::endl;

    if (ifp.m_metaServerList.size() == 0)
    {
      throw std::runtime_error("the size of the metadatalist is not supposed to be 0");
      return -1;
    }

    // range metaserver list, and update the metadata
    tl::remote_procedure remotePutMetaData = client->m_clientEnginePtr->define("putmetadata");
    for (auto it = ifp.m_metaServerList.begin(); it != ifp.m_metaServerList.end(); it++)
    {
      // std::cout << "debug mdwlist position" << this->m_position << it->m_destAddr << std::endl;
      tl::endpoint metaserverEndpoint = client->lookup(*it);
      int status = remotePutMetaData.on(metaserverEndpoint)(step, varName, bdesc);
      if (status != 0)
      {
        std::cerr << "failed to put metadata" << std::endl;
        return -1;
      }
    }

    clock_gettime(CLOCK_REALTIME, &end2);
    diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
    // std::cout << "put stage 2: " << diff2 << std::endl;
  }
  else
  {
    throw std::runtime_error("unsupported put method");
    return -1;
  }
  return 0;
}

int putVTKData(UniClient* client, size_t step, std::string varName, BlockSummary& dataSummary,
  void* dataContainerSrc)
{

  DEBUG("vtk marshal data for var " << varName);
  vtkSmartPointer<vtkCharArray> vtkbuffer = vtkSmartPointer<vtkCharArray>::New();
  bool oktoMarshal =
    vtkCommunicator::MarshalDataObject((vtkDataObject*)dataContainerSrc, vtkbuffer);
  if (oktoMarshal == false)
  {
    throw std::runtime_error("failed to marshal vtk data");
  }

  vtkIdType numTuples = vtkbuffer->GetNumberOfTuples();
  int numComponents = vtkbuffer->GetNumberOfComponents();

  size_t dataTransferSize = numTuples * numComponents;

  DEBUG("vtk dataTransferSize: " << dataTransferSize);

  //std::cout << "------check vtkbuffer content: ------" << std::endl;
  //for (int i = 0; i < dataTransferSize; i++)
  //{
  //  std::cout << vtkbuffer->GetValue(i);
  //}
  //std::cout << "------" << std::endl;

  // the elem size and elem length used in the block summary
  // is the size of the marshaled array, update the block summary here
  dataSummary.m_elemSize = numComponents;
  dataSummary.m_elemNum = numTuples;

  // assign the server
  if (client->m_associatedDataServer.compare("") == 0)
  {
    client->m_associatedDataServer = client->getServerAddr();
    DEBUG("m_position " << client->m_position << " m_associatedDataServer "
                        << client->m_associatedDataServer);
    client->initPutRawData(dataTransferSize);
  }

  tl::remote_procedure remotegetInfoForPut = client->m_clientEnginePtr->define("getinfoForput");
  tl::endpoint serverEndpoint = client->lookup(client->m_associatedDataServer);

  InfoForPut ifp = remotegetInfoForPut.on(serverEndpoint)(
    step, dataTransferSize, dataSummary.m_dims, dataSummary.m_indexlb, dataSummary.m_indexub);

  BlockDescriptor bdesc(client->m_associatedDataServer, dataSummary.m_blockid,
    std::string(dataSummary.m_dataType), dataSummary.m_dims, dataSummary.m_indexlb,
    dataSummary.m_indexub);

  // check the data summary
  if (strcmp(dataSummary.m_blockid, "") == 0)
  {
    throw std::runtime_error("dataSummary.m_blockid should not be empty string");
    return -1;
  }
  
  //use the marshaled objests here!!
  //the bulk object is associated with the mem space labeld by the segments
  memcpy(client->m_segments[0].first, vtkbuffer->GetPointer(0), dataTransferSize);

  tl::remote_procedure remotePutRawData = client->m_clientEnginePtr->define("putrawdata");
  serverEndpoint = client->lookup(client->m_associatedDataServer);
  int status = remotePutRawData.on(serverEndpoint)(
    client->m_position, step, varName, dataSummary, client->m_dataBulk);

  // example for async put
  // auto request = remotePutRawData.on(this->m_serverEndpoint).async(this->m_position ,step,
  // varName, dataSummary, this->m_dataBulk);

  // bool completed = request.received();
  // ...
  // actually wait on the request and get the result out of it
  // int status = request.wait();
  if (status != 0)
  {
    throw std::runtime_error("failed to put the raw data");
    return -1;
  }

  if (ifp.m_metaServerList.size() == 0)
  {
    throw std::runtime_error("the size of the metadatalist is not supposed to be 0");
    return -1;
  }

  // range metaserver list, and update the metadata
  tl::remote_procedure remotePutMetaData = client->m_clientEnginePtr->define("putmetadata");
  for (auto it = ifp.m_metaServerList.begin(); it != ifp.m_metaServerList.end(); it++)
  {
    // std::cout << "debug mdwlist position" << this->m_position << it->m_destAddr << std::endl;
    tl::endpoint metaserverEndpoint = client->lookup(*it);
    int status = remotePutMetaData.on(metaserverEndpoint)(step, varName, bdesc);
    if (status != 0)
    {
      std::cerr << "failed to put metadata" << std::endl;
      return -1;
    }
  }

  return 0;
}

// TODO //ask if there is avalible mem resources
// if there is avalible mem, put in mem
// else, put into the disk
// then update the metadata
int UniClient::putrawdata(
  size_t step, std::string varName, BlockSummary& dataSummary, void* dataContainerSrc)
{

  std::string dataType = std::string(dataSummary.m_dataType);

  if (dataType == DATATYPE_CARGRID)
  {

    int status = putCarGrid(this, step, varName, dataSummary, dataContainerSrc);
    if (status != 0)
    {
      throw std::runtime_error("failed to put the cartisian grid");
    }
  }
  else if (dataType == DATATYPE_VTKPTR)
  {
    int status = putVTKData(this, step, varName, dataSummary, dataContainerSrc);
    if (status != 0)
    {
      throw std::runtime_error("failed to put the vtk data");
    }
  }
  else
  {
    throw std::runtime_error("unsupported data type");
  }

  return 0;
}

// get MetaAddrWrapper List this contains the coresponding server that overlap with current querybox
// this can be called once for multiple get in different time step, since partition is fixed
std::vector<std::string> UniClient::getmetaServerList(
  size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub)
{

  tl::remote_procedure remotegetmetaServerList =
    this->m_clientEnginePtr->define("getmetaServerList");
  // ask the master server
  std::vector<std::string> metaserverList =
    remotegetmetaServerList.on(m_masterEndpoint)(dims, indexlb, indexub);
  return metaserverList;
}

std::vector<BlockDescriptor> UniClient::getblockDescriptorList(std::string serverAddr, size_t step,
  std::string varName, size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub)
{
  tl::remote_procedure remotegetblockDescriptorList =
    this->m_clientEnginePtr->define("getBlockDescriptorList");
  tl::endpoint serverEndpoint = this->lookup(serverAddr);
  std::vector<BlockDescriptor> blockDescriptorList =
    remotegetblockDescriptorList.on(serverEndpoint)(step, varName, dims, indexlb, indexub);
  return blockDescriptorList;
}

int UniClient::getSubregionData(std::string serverAddr, std::string blockID, size_t dataSize,
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

// this method is the wrapper for the getSubregionData and getblockDescriptorList
// return the assembled matrix
/*
1 get metadata list, region associated with the returned metadata overlaps with the queried bbx

2 for every metadata server, get the raw data endpoints/descriptor list on that server. In
particular, if the actual data bbx for data put is partially overlap with the query bbx, In the
returned raw data endpoint, use the overlapped bbx. e.g. obj1 (0,0) (3,3) for actual data, the query
bbx is (2,2) (5,5) in the returned raw data endpoint, the bbx for the obj1 is (2,2)(3,3)

3 for every raw data endpoint in the returned list, get actual data from the raw data server with
the subregion rpc put the data according to the matrix view and put it into a list

4 assemble these data into the full complete data according to the shape of the queried bbx
*/
MATRIXTOOL::MatrixView UniClient::getArbitraryData(size_t step, std::string varName,
  size_t elemSize, size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub)
{
  struct timespec start, end1, end2;
  double diff1, diff2;
  clock_gettime(CLOCK_REALTIME, &start);

  // spdlog::debug("index lb {} {} {}", indexlb[0], indexlb[1], indexlb[2]);
  // spdlog::debug("index ub {} {} {}", indexlb[0], indexlb[1], indexlb[2]);
  std::vector<std::string> metaList = this->getmetaServerList(dims, indexlb, indexub);

  std::vector<MATRIXTOOL::MatrixView> matrixViewList;
  for (auto it = metaList.begin(); it != metaList.end(); it++)
  {
    std::vector<BlockDescriptor> bdList;
    std::string metaServerAddr = *it;
    while (true)
    {
      bdList = this->getblockDescriptorList(metaServerAddr, step, varName, dims, indexlb, indexub);
      std::cout << "step " << step << " indexlb " << indexlb[0] << "," << indexlb[1] << ","
                << indexlb[2] << " indexub " << indexub[0] << "," << indexub[1] << "," << indexub[2]
                << std::endl;
      std::cout << "metaServerAddr " << metaServerAddr << " size of bdList " << bdList.size()
                << std::endl;
      if (bdList.size() == 0)
      {
        // the metadata is not updated on this server, waiting
        // sleep 500ms
        tl::thread::sleep(*this->m_clientEnginePtr, 500);
        continue;
      }
      else
      {
        break;
      }
      // TODO. if wait for a long time, throw runtime error
    }
    // for every metadata, get the rawdata
    for (auto bd = bdList.begin(); bd != bdList.end(); bd++)
    {
      bd->printInfo();

      // get the subrigion according to the information at the list
      // it is better to use the multithread here

      // get subrigion of the data
      // allocate size (use differnet strategy is the vtk object is used)
      BBXTOOL::BBX* bbx = new BBXTOOL::BBX(dims, bd->m_indexlb, bd->m_indexub);
      size_t allocSize = sizeof(double) * bbx->getElemNum();
      // std::cout << "alloc size is " << allocSize << std::endl;
      // TODO when to free this?
      void* subDataContainer = (void*)malloc(allocSize);

      // get the data by subregion api
      if (bd->m_backend == BACKEND::MEM)
      {
        // get the subregion by the rdma to call the remote server
        int status = this->getSubregionData(bd->m_rawDataServerAddr, bd->m_rawDataID, allocSize,
          dims, bd->m_indexlb, bd->m_indexub, subDataContainer);

        if (status != 0)
        {
          std::cerr << "error to get data for step " << step << std::endl;
          throw std::runtime_error("failed for get subrigion data for current raw data endpoint");
        }
      }
      else if (bd->m_backend == BACKEND::FILE)
      {
        // get the subregion data from the local block management store for the file backend
        BlockSummary bs = this->b_manager.getBlockSubregion(
          bd->m_rawDataID, BACKEND::FILE, dims, bd->m_indexlb, bd->m_indexub, subDataContainer);
        if (strcmp(bs.m_blockid, "") == 0)
        {
          throw std::runtime_error("failed to get the block summary");
        }
      }
      else
      {
        throw std::runtime_error("unsupported meta format for raw endpoint");
      }

      // check resutls
      // std::cout << "check subregion value " << std::endl;
      // double *temp = (double *)subDataContainer;
      // for (int i = 0; i < 5; i++)
      //{
      //    double value = *(temp + i);
      //    std::cout << "value " << value << std::endl;
      //}

      // put it into the Matrix View
      MATRIXTOOL::MatrixView mv(bbx, subDataContainer);
      matrixViewList.push_back(mv);
    }
  }

  // stage 1
  // clock_gettime(CLOCK_REALTIME, &end1);
  // diff1 = (end1.tv_sec - start.tv_sec) * 1.0 + (end1.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
  // std::cout<<"get data stage 1: " << diff1 << std::endl;

  // assemble the matrix View
  BBX* intactbbx = new BBX(dims, indexlb, indexub);
  MATRIXTOOL::MatrixView mvassemble =
    MATRIXTOOL::matrixAssemble(elemSize, matrixViewList, intactbbx);

  // free the element in matrixViewList
  for (auto it = matrixViewList.begin(); it != matrixViewList.end(); it++)
  {
    if (it->m_bbx != NULL)
    {
      free(it->m_bbx);
      it->m_bbx = NULL;
    }
    if (it->m_data != NULL)
    {
      free(it->m_data);
      it->m_data = NULL;
    }
  }

  // stage 2
  // clock_gettime(CLOCK_REALTIME, &end2);
  // diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
  // std::cout<<"get data stage 2: " << diff2 << std::endl;

  return mvassemble;
}

// put trigger info into specific metadata server
int UniClient::putTriggerInfo(
  std::string serverAddr, std::string triggerName, DynamicTriggerInfo& dti)
{
  tl::remote_procedure remoteputTriggerInfo = this->m_clientEnginePtr->define("putTriggerInfo");
  tl::endpoint serverEndpoint = this->lookup(serverAddr);
  int status = remoteputTriggerInfo.on(serverEndpoint)(triggerName, dti);
  return status;
}

void UniClient::registerWatcher(std::vector<std::string> triggerNameList)
{

  // range all the server
  std::string watcherAddr = this->m_clientEnginePtr->self();
  tl::remote_procedure remoteregisterWatchInfo = this->m_clientEnginePtr->define("registerWatcher");

  // register the watcher
  for (auto it = this->m_serverToEndpoints.begin(); it != this->m_serverToEndpoints.end(); ++it)
  {
    tl::endpoint serverEndpoint = it->second;
    int status = remoteregisterWatchInfo.on(serverEndpoint)(watcherAddr, triggerNameList);
  }
  return;
}
// TODO, add the identity for different group
// only the master will send the notify information back to the client
// when put trigger, it is necessary to put identity
std::string UniClient::registerTrigger(size_t dims, std::array<int, 3> indexlb,
  std::array<int, 3> indexub, std::string triggerName, DynamicTriggerInfo& dti)
{

  std::vector<std::string> metaList = this->getmetaServerList(dims, indexlb, indexub);

  if (metaList.size() == 0)
  {
    throw std::runtime_error("the metadata list should not be empty");
    return 0;
  }

  std::vector<MATRIXTOOL::MatrixView> matrixViewList;
  // TODO currently, use the first one as the master of the task execution group
  // if there are multiple variables
  // then use the hashvalue of the variable name to determine the metaServer
  std::string groupMasterAddr = metaList[0];
  // TODO add information about the metaserver when registering
  for (auto it = metaList.begin(); it != metaList.end(); it++)
  {
    // send request to coresponding metadata server
    // datastruct of the triggerInfo should know its masteraddr
    dti.m_masterAddr = groupMasterAddr;
    int status = putTriggerInfo(*it, triggerName, dti);
    if (status != 0)
    {
      throw std::runtime_error("failed to putTriggerInfo for metaServer " + groupMasterAddr);
    }
  }

  // return number of the metaserve, know how many notification in total for each step
  return groupMasterAddr;
}

std::string UniClient::executeRawFunc(std::string serverAddr, std::string blockID,
  std::string functionName, std::vector<std::string>& funcParameters)
{
  tl::remote_procedure remoteexecuteRawFunc = this->m_clientEnginePtr->define("executeRawFunc");
  tl::endpoint serverEndpoint = this->lookup(serverAddr);
  std::string result =
    remoteexecuteRawFunc.on(serverEndpoint)(blockID, functionName, funcParameters);
  return result;
}

// notify the data watcher
// be carefule, there are race condition here, if find the remote procedure dynamically
void UniClient::notifyBack(std::string watcherAddr, BlockSummary& bs)
{
  tl::remote_procedure remotenotifyBack = this->m_clientEnginePtr->define("rcvNotify");
  tl::endpoint serverEndpoint = this->lookup(watcherAddr);
  std::string result = remotenotifyBack.on(serverEndpoint)(bs);
  return;
}
// these will be called by analytics
// put the event into the coresponding master server of the task groups
void UniClient::putEventIntoQueue(
  std::string groupMasterAddr, std::string triggerName, EventWrapper& event)
{
  tl::remote_procedure remoteputEvent =
    this->m_clientEnginePtr->define("putEvent").disable_response();
  tl::endpoint serverEndpoint = this->lookup(groupMasterAddr);
  remoteputEvent.on(serverEndpoint)(triggerName, event);
  return;
}

// get a event from the event queue in task trigger
EventWrapper UniClient::getEventFromQueue(std::string groupMasterAddr, std::string triggerName)
{
  tl::remote_procedure remotegetEvent = this->m_clientEnginePtr->define("getEvent");
  tl::endpoint serverEndpoint = this->lookup(groupMasterAddr);
  EventWrapper event = remotegetEvent.on(serverEndpoint)(triggerName);
  return event;
}

// todo
// start a server that could listen to the notification from the server
// this server can be started by a sepatate xstream by using the lamda expression
// this only need to be started on specific rank such as rank=0

void UniClient::eraseRawData(std::string serverAddr, std::string blockID)
{
  tl::remote_procedure remoteeraserawdata = this->m_clientEnginePtr->define("eraserawdata");
  tl::endpoint serverEndpoint = this->lookup(serverAddr);
  remoteeraserawdata.on(serverEndpoint)(blockID);
  return;
}

void UniClient::deleteMetaStep(size_t step)
{
  // range all server and delete metadata for specific step
  tl::remote_procedure remotedeleteMeta = this->m_clientEnginePtr->define("deleteMetaStep");

  for (auto& kv : m_serverToEndpoints)
  {
    remotedeleteMeta.on(kv.second)(step);
  }
  return;
}

}