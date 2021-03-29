#include "unimosclient.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#define BILLION 1000000000L
namespace tl = thallium;

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
    //std::cout << "debug list index " << it->m_index << " addr " << it->m_addr << std::endl;
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
  else if (this->m_bulkSize > dataTransferSize)
  {
    // do nothing if current bulk size is larger then malloc size
    // update the segment value
    DEBUG("new dataTransferSize is" << dataTransferSize);
    this->m_segments[0].second = dataTransferSize;
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
  size_t dataTransferSize = dataSummary.getArraySize(dataSummary.m_blockid);
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
  struct timespec start, end1, end2, end3;
  double diff1, diff2, diff3;
  clock_gettime(CLOCK_REALTIME, &start);

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

  clock_gettime(CLOCK_REALTIME, &end1);
  diff1 = (end1.tv_sec - start.tv_sec) * 1.0 + (end1.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
  DEBUG("stage marshal : " << diff1);

  // std::cout << "------check vtkbuffer content: ------" << std::endl;
  // for (int i = 0; i < dataTransferSize; i++)
  //{
  //  std::cout << vtkbuffer->GetValue(i);
  //}
  // std::cout << "------" << std::endl;

  // the elem size and elem length used in the block summary
  // is the size of the marshaled array, update the block summary here
  dataSummary.addArraySummary(
    ArraySummary(dataSummary.m_blockid, (size_t)numComponents, (size_t)numTuples));

  // assign the server
  if (client->m_associatedDataServer.compare("") == 0)
  {
    client->m_associatedDataServer = client->getServerAddr();
    DEBUG("m_position " << client->m_position << " m_associatedDataServer "
                        << client->m_associatedDataServer);
  }
  // resize the mem space if the datatransfersize change

  client->initPutRawData(dataTransferSize);

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

  clock_gettime(CLOCK_REALTIME, &end2);
  diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
  DEBUG("stage registermem : " << diff2);

  // use the marshaled objests here!!
  // the bulk object is associated with the mem space labeld by the segments
  memcpy(client->m_segments[0].first, vtkbuffer->GetPointer(0), dataTransferSize);
  // instead of copy agagin, just let segments equals to pointer size
  // this can not be done before the bulk
  // client->m_segments[0].first = vtkbuffer->GetPointer(0);

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

  clock_gettime(CLOCK_REALTIME, &end3);
  diff3 = (end3.tv_sec - end2.tv_sec) * 1.0 + (end3.tv_nsec - end2.tv_nsec) * 1.0 / BILLION;
  DEBUG("stage transfer : " << diff3);

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

int putArrayIntoBlockZero(UniClient* client, BlockSummary& dataSummary, void* dataContainerSrc)
{
  struct timespec start, end1;
  double diff1;
  clock_gettime(CLOCK_REALTIME, &start);
  // start to marshal
  // we only support the polydata for this method for experiment
  // vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  // polyData.TakeReference((vtkPolyData*)dataContainerSrc);
  vtkSmartPointer<vtkPolyData> polyData = (vtkPolyData*)dataContainerSrc;
  // extract arrays
  int numCells = polyData->GetNumberOfPolys();
  int numPoints = polyData->GetNumberOfPoints();
  DEBUG("putArrayIntoBlock numCells " << numCells << " numPoints " << numPoints);

  // prepare for transfering data
  if (client->m_associatedDataServer.compare("") == 0)
  {
    client->m_associatedDataServer = client->getServerAddr();
  }
  tl::remote_procedure remoteputArrayIntoBlock =
    client->m_clientEnginePtr->define("putArrayIntoBlock");
  tl::endpoint serverEndpoint = client->lookup(client->m_associatedDataServer);

  // extract cell array
  auto cellArray = polyData->GetPolys();
  vtkDataArray* offsetArray = cellArray->GetOffsetsArray();
  vtkDataArray* connectivyArray = cellArray->GetConnectivityArray();

  vtkTypeInt64Array* arrayoffset64 = cellArray->GetOffsetsArray64();
  void* arrayoffsetptr = (void*)arrayoffset64->GetVoidPointer(0);

  size_t offsetTotalSize =
    arrayoffset64->GetNumberOfTuples() * arrayoffset64->GetNumberOfComponents();

  std::vector<std::pair<void*, std::size_t> > cellOffsetsegments(1);
  cellOffsetsegments[0].first = arrayoffsetptr;
  cellOffsetsegments[0].second = offsetTotalSize * sizeof(long);
  ArraySummary ascellsoffset("cellsOffset", sizeof(long), offsetTotalSize);
  tl::bulk cellsOffsetBulk =
    client->m_clientEnginePtr->expose(cellOffsetsegments, tl::bulk_mode::read_only);
  auto cellOffsetresponse =
    remoteputArrayIntoBlock.on(serverEndpoint).async(dataSummary, ascellsoffset, cellsOffsetBulk);
  // auto cellresponse = remoteputArrayIntoBlock.on(serverEndpoint)(dataSummary, ascells,
  // cellsBulk);

  DEBUG("async cells offset");

  vtkTypeInt64Array* arrayConnectivety64 = cellArray->GetConnectivityArray64();
  void* arrayconnptr = (void*)arrayConnectivety64->GetVoidPointer(0);

  size_t connecTotalSize =
    arrayConnectivety64->GetNumberOfTuples() * arrayConnectivety64->GetNumberOfComponents();

  std::vector<std::pair<void*, std::size_t> > cellConnectsegments(1);
  cellConnectsegments[0].first = arrayconnptr;
  cellConnectsegments[0].second = connecTotalSize * sizeof(long);
  ArraySummary ascellsconnect("cellsConnect", sizeof(long), connecTotalSize);
  tl::bulk cellsConnectBulk =
    client->m_clientEnginePtr->expose(cellConnectsegments, tl::bulk_mode::read_only);
  auto cellConnectresponse =
    remoteputArrayIntoBlock.on(serverEndpoint).async(dataSummary, ascellsconnect, cellsConnectBulk);
  // auto cellresponse = remoteputArrayIntoBlock.on(serverEndpoint)(dataSummary, ascells,
  // cellsBulk);

  DEBUG("async cells connect");

  // extract points array
  vtkPoints* pointsarray = polyData->GetPoints();
  void* pointsPtr = (void*)(pointsarray->GetVoidPointer(0));
  size_t totalPointNum = pointsarray->GetNumberOfPoints() * 3;

  // send the points and cell
  std::vector<std::pair<void*, std::size_t> > pointssegments(1);
  pointssegments[0].first = pointsPtr;
  pointssegments[0].second = totalPointNum * sizeof(float);
  tl::bulk pointBulk = client->m_clientEnginePtr->expose(pointssegments, tl::bulk_mode::read_only);

  ArraySummary aspoints("points", sizeof(float), totalPointNum);
  auto pointresponse =
    remoteputArrayIntoBlock.on(serverEndpoint).async(dataSummary, aspoints, pointBulk);
  // auto pointresponse = remoteputArrayIntoBlock.on(serverEndpoint)(dataSummary, aspoints,
  // pointBulk);

  DEBUG("async points");

  // extract normal array
  auto normalArray = polyData->GetPointData()->GetNormals();
  size_t noramlTotalSize = normalArray->GetNumberOfTuples() * normalArray->GetNumberOfComponents();
  // get the normal array addr
  void* buffernormalArray = (void*)normalArray->GetVoidPointer(0);

  std::vector<std::pair<void*, std::size_t> > normalsegments(1);
  normalsegments[0].first = (void*)buffernormalArray;
  normalsegments[0].second = noramlTotalSize * sizeof(float);
  ArraySummary asnormals("normals", sizeof(float), noramlTotalSize);
  tl::bulk normalsBulk =
    client->m_clientEnginePtr->expose(normalsegments, tl::bulk_mode::read_only);
  auto normalresponse =
    remoteputArrayIntoBlock.on(serverEndpoint).async(dataSummary, asnormals, normalsBulk);

  DEBUG("async normals");


  int ret = cellOffsetresponse.wait();
  if (ret != 0)
  {
    throw std::runtime_error("failed for cellOffsetresponse");
  }

  ret = cellConnectresponse.wait();
  if (ret != 0)
  {
    throw std::runtime_error("failed for cellConnectresponse");
  }

  ret = pointresponse.wait();
  if (ret != 0)
  {
    throw std::runtime_error("failed for pointresponse");
  }

  ret = normalresponse.wait();
  if (ret != 0)
  {
    throw std::runtime_error("failed for normalresponse");
  }

  clock_gettime(CLOCK_REALTIME, &end1);
  diff1 = (end1.tv_sec - start.tv_sec) * 1.0 + (end1.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
  DEBUG("time spent on putarrayintoblock zero copy: " << diff1);

  return 0;
}

// non block version
int putArrayIntoBlock(UniClient* client, BlockSummary& dataSummary, void* dataContainerSrc)
{

  struct timespec start, end1, end2, end3;
  double diff1, diff2, diff3;
  clock_gettime(CLOCK_REALTIME, &start);
  // start to marshal
  // we only support the polydata for this method for experiment
  // vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  // polyData.TakeReference((vtkPolyData*)dataContainerSrc);
  vtkSmartPointer<vtkPolyData> polyData = (vtkPolyData*)dataContainerSrc;
  // extract arrays
  int numCells = polyData->GetNumberOfPolys();
  int numPoints = polyData->GetNumberOfPoints();

  // if there are multiple array
  // refer to
  // https://github.com/pnorbert/adiosvm/blob/master/Tutorial/gray-scott/analysis/isosurface.cpp
  // refer to
  // https://github.com/pnorbert/adiosvm/blob/master/Tutorial/gray-scott/analysis/find_blobs.cpp
  // TODO support this further
  // for the test data, there is only one filed array namely the normal data
  // int numPointArray = polyData->GetPointData()->GetNumberOfArrays();
  // std::string arrayName = polyData->GetPointData()->GetArrayName(0);
  // size_t arrayTuple = polyData->GetPointData()->GetArray(0)->GetNumberOfTuples();
  // size_t arraycomponent = polyData->GetPointData()->GetArray(0)->GetNumberOfComponents();
  DEBUG("putArrayIntoBlock numCells " << numCells << " numPoints " << numPoints);

  std::vector<double> points(numPoints * 3);
  std::vector<double> normals(numPoints * 3);
  std::vector<int> cells(numCells * 3); // Assumes that cells are triangles

  double coords[3];
  auto cellArray = polyData->GetPolys();
  cellArray->InitTraversal();

  // Iterate through cells
  for (int i = 0; i < numCells; i++)
  {
    auto idList = vtkSmartPointer<vtkIdList>::New();

    cellArray->GetNextCell(idList);

    // Iterate through points of a cell
    for (int j = 0; j < idList->GetNumberOfIds(); j++)
    {
      auto id = idList->GetId(j);

      cells[i * 3 + j] = id;

      polyData->GetPoint(id, coords);

      points[id * 3 + 0] = coords[0];
      points[id * 3 + 1] = coords[1];
      points[id * 3 + 2] = coords[2];
    }
  }
  // start to transfer the data
  if (client->m_associatedDataServer.compare("") == 0)
  {
    client->m_associatedDataServer = client->getServerAddr();
  }

  tl::remote_procedure remoteputArrayIntoBlock =
    client->m_clientEnginePtr->define("putArrayIntoBlock");
  tl::endpoint serverEndpoint = client->lookup(client->m_associatedDataServer);
  // send the points and cell
  std::vector<std::pair<void*, std::size_t> > pointssegments(1);
  pointssegments[0].first = points.data();
  pointssegments[0].second = points.size() * sizeof(double);
  tl::bulk pointBulk = client->m_clientEnginePtr->expose(pointssegments, tl::bulk_mode::read_only);

  ArraySummary aspoints("points", sizeof(double), points.size());
  auto pointresponse =
    remoteputArrayIntoBlock.on(serverEndpoint).async(dataSummary, aspoints, pointBulk);
  // auto pointresponse = remoteputArrayIntoBlock.on(serverEndpoint)(dataSummary, aspoints,
  // pointBulk);

  DEBUG("async points");

  std::vector<std::pair<void*, std::size_t> > cellsegments(1);
  cellsegments[0].first = cells.data();
  cellsegments[0].second = cells.size() * sizeof(int);
  ArraySummary ascells("cells", sizeof(int), cells.size());
  tl::bulk cellsBulk = client->m_clientEnginePtr->expose(cellsegments, tl::bulk_mode::read_only);
  auto cellresponse =
    remoteputArrayIntoBlock.on(serverEndpoint).async(dataSummary, ascells, cellsBulk);
  // auto cellresponse = remoteputArrayIntoBlock.on(serverEndpoint)(dataSummary, ascells,
  // cellsBulk);

  DEBUG("async cells");

  // Extract normals
  auto normalArray = polyData->GetPointData()->GetNormals();
  // TODO we do not really need the normal vector here
  // since we can use the zero copy for normal array, this is the continuous array
  for (int i = 0; i < normalArray->GetNumberOfTuples(); i++)
  {
    normalArray->GetTuple(i, coords);

    normals[i * 3 + 0] = coords[0];
    normals[i * 3 + 1] = coords[1];
    normals[i * 3 + 2] = coords[2];
  }

  std::vector<std::pair<void*, std::size_t> > normalsegments(1);
  normalsegments[0].first = normals.data();
  normalsegments[0].second = normals.size() * sizeof(double);
  ArraySummary asnormals("normals", sizeof(double), normals.size());
  tl::bulk normalsBulk =
    client->m_clientEnginePtr->expose(normalsegments, tl::bulk_mode::read_only);
  auto normalresponse =
    remoteputArrayIntoBlock.on(serverEndpoint).async(dataSummary, asnormals, normalsBulk);

  // auto normalresponse =
  //  remoteputArrayIntoBlock.on(serverEndpoint)(dataSummary, asnormals, normalsBulk);

  DEBUG("async normals");

  // wait transfer to finish

  int ret = pointresponse.wait();
  if (ret != 0)
  {
    throw std::runtime_error("failed for pointresponse");
  }
  ret = cellresponse.wait();
  if (ret != 0)
  {
    throw std::runtime_error("failed for cellresponse");
  }
  ret = normalresponse.wait();
  if (ret != 0)
  {
    throw std::runtime_error("failed for normalresponse");
  }

  clock_gettime(CLOCK_REALTIME, &end1);
  diff1 = (end1.tv_sec - start.tv_sec) * 1.0 + (end1.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
  DEBUG("time spent on putarrayintoblock : " << diff1);

  // check if the data is complete when all put operation finish
  // check the data at the server end when it is necessary
  // tl::remote_procedure remoteexpcheckdata =
  //  client->m_clientEnginePtr->define("expcheckdata").disable_response();
  // remoteexpcheckdata.on(serverEndpoint)(dataSummary);

  return 0;
}

// experimental function with zero copy
int putVTKDataExpZeroOneRpc(UniClient* client, size_t step, std::string varName,
  BlockSummary& dataSummary, void* dataContainerSrc)
{
  struct timespec start, end1, end2, end3;
  double diff1, diff2, diff3;
  clock_gettime(CLOCK_REALTIME, &start);
  // start to marshal
  // we only support the polydata for this method for experiment
  // vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  // polyData.TakeReference((vtkPolyData*)dataContainerSrc);
  // try to downcast to the poly to try to see it satisfy requirments
  // it is illeagle to cast the void* to class by this way
  // this might break up the type safety anyway
  /*
  vtkPolyData* polyptr = dynamic_cast<vtkPolyData*>(()dataContainerSrc);
  if (polyptr == nullptr)
  {
    throw std::runtime_error("only support poly data for putvtkdataexp");
  }
  */
  vtkSmartPointer<vtkPolyData> polyData = (vtkPolyData*)dataContainerSrc;

  // extract cell array
  auto cellArray = polyData->GetPolys();
  vtkDataArray* offsetArray = cellArray->GetOffsetsArray();
  vtkDataArray* connectivyArray = cellArray->GetConnectivityArray();

  vtkTypeInt64Array* arrayoffset64 = cellArray->GetOffsetsArray64();
  long* arrayoffsetptr = (long*)arrayoffset64->GetVoidPointer(0);

  size_t offsetTotalSize =
    arrayoffset64->GetNumberOfTuples() * arrayoffset64->GetNumberOfComponents();

  vtkTypeInt64Array* arrayConnectivety64 = cellArray->GetConnectivityArray64();
  long* arrayconnptr = (long*)arrayConnectivety64->GetVoidPointer(0);

  size_t connecTotalSize =
    arrayConnectivety64->GetNumberOfTuples() * arrayConnectivety64->GetNumberOfComponents();

  // extract points array
  vtkPoints* pointsarray = polyData->GetPoints();
  float* pointsPtr = (float*)(pointsarray->GetVoidPointer(0));
  size_t totalPointNum = pointsarray->GetNumberOfPoints() * 3;

  // extract normal array
  auto normalArray = polyData->GetPointData()->GetNormals();
  size_t noramlTotalSize = normalArray->GetNumberOfTuples() * normalArray->GetNumberOfComponents();
  // get the normal array addr
  float* buffernormalArray = (float*)normalArray->GetVoidPointer(0);

  std::vector<std::pair<void*, std::size_t> > segments(4);
  segments[0].first = pointsPtr;
  segments[0].second = totalPointNum * sizeof(float);
  segments[1].first = buffernormalArray;
  segments[1].second = noramlTotalSize * sizeof(float);
  segments[2].first = arrayoffsetptr;
  segments[2].second = offsetTotalSize * sizeof(long);
  segments[3].first = arrayconnptr;
  segments[3].second = connecTotalSize * sizeof(long);

  std::vector<size_t> transferSizeList;
  for (int i = 0; i < segments.size(); i++)
  {
    transferSizeList.push_back(segments[i].second);
  }

  clock_gettime(CLOCK_REALTIME, &end1);
  diff1 = (end1.tv_sec - start.tv_sec) * 1.0 + (end1.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
  DEBUG("stage marshal : " << diff1);

  DEBUG("point size  " << segments[0].second << " normal size " << segments[1].second
                       << " cell offset size " << segments[2].second << " cell connective size "
                       << segments[3].second);
  // organize it into the bulk and register memory
  tl::bulk expdataBulk = client->m_clientEnginePtr->expose(segments, tl::bulk_mode::read_only);

  if (client->m_associatedDataServer.compare("") == 0)
  {
    client->m_associatedDataServer = client->getServerAddr();
  }

  clock_gettime(CLOCK_REALTIME, &end2);
  diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
  DEBUG("stage registermem : " << diff2);

  DEBUG("server addr is " << client->m_associatedDataServer);
  tl::remote_procedure remoteputvtkexp = client->m_clientEnginePtr->define("putvtkexpzero");
  tl::endpoint serverEndpoint = client->lookup(client->m_associatedDataServer);
  int status = remoteputvtkexp.on(serverEndpoint)(
    client->m_position, step, varName, dataSummary, expdataBulk, transferSizeList);

  if (status != 0)
  {
    throw std::runtime_error("failed to put the exp vtk data");
    return -1;
  }

  clock_gettime(CLOCK_REALTIME, &end3);
  diff3 = (end3.tv_sec - end2.tv_sec) * 1.0 + (end3.tv_nsec - end2.tv_nsec) * 1.0 / BILLION;
  DEBUG("stage transfer : " << diff3);

  // do not update the metadata for the experimental version
  return 0;
}

// this is experimental function, we extract the data from the vtk poly and then
// transfer them to server and assembe back to vtk data there
int putVTKDataExp(UniClient* client, size_t step, std::string varName, BlockSummary& dataSummary,
  void* dataContainerSrc)
{
  struct timespec start, end1, end2, end3;
  double diff1, diff2, diff3;
  clock_gettime(CLOCK_REALTIME, &start);
  // start to marshal
  // we only support the polydata for this method for experiment
  // vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  // polyData.TakeReference((vtkPolyData*)dataContainerSrc);
  // try to downcast to the poly to try to see it satisfy requirments
  // it is illeagle to cast the void* to class by this way
  // this might break up the type safety anyway
  /*
  vtkPolyData* polyptr = dynamic_cast<vtkPolyData*>(()dataContainerSrc);
  if (polyptr == nullptr)
  {
    throw std::runtime_error("only support poly data for putvtkdataexp");
  }
  */
  vtkSmartPointer<vtkPolyData> polyData = (vtkPolyData*)dataContainerSrc;
  // extract arrays
  int numCells = polyData->GetNumberOfPolys();
  int numPoints = polyData->GetNumberOfPoints();

  // if there are multiple array
  // refer to
  // https://github.com/pnorbert/adiosvm/blob/master/Tutorial/gray-scott/analysis/isosurface.cpp
  // refer to
  // https://github.com/pnorbert/adiosvm/blob/master/Tutorial/gray-scott/analysis/find_blobs.cpp
  // TODO support this further
  // for the test data, there is only one filed array namely the normal data
  // int numPointArray = polyData->GetPointData()->GetNumberOfArrays();
  // std::string arrayName = polyData->GetPointData()->GetArrayName(0);
  // size_t arrayTuple = polyData->GetPointData()->GetArray(0)->GetNumberOfTuples();
  // size_t arraycomponent = polyData->GetPointData()->GetArray(0)->GetNumberOfComponents();
  DEBUG("numCells " << numCells << " numPoints " << numPoints);

  std::vector<double> points(numPoints * 3);
  std::vector<double> normals(numPoints * 3);
  std::vector<int> cells(numCells * 3); // Assumes that cells are triangles

  double coords[3];

  auto cellArray = polyData->GetPolys();

  cellArray->InitTraversal();

  // Iterate through cells
  for (int i = 0; i < numCells; i++)
  {
    auto idList = vtkSmartPointer<vtkIdList>::New();

    cellArray->GetNextCell(idList);

    // Iterate through points of a cell
    for (int j = 0; j < idList->GetNumberOfIds(); j++)
    {
      auto id = idList->GetId(j);

      cells[i * 3 + j] = id;

      polyData->GetPoint(id, coords);

      points[id * 3 + 0] = coords[0];
      points[id * 3 + 1] = coords[1];
      points[id * 3 + 2] = coords[2];
    }
  }

  auto normalArray = polyData->GetPointData()->GetNormals();

  // Extract normals
  for (int i = 0; i < normalArray->GetNumberOfTuples(); i++)
  {
    normalArray->GetTuple(i, coords);

    normals[i * 3 + 0] = coords[0];
    normals[i * 3 + 1] = coords[1];
    normals[i * 3 + 2] = coords[2];
  }

  // start to transfer, use the putvtkdataexp
  // for this method, we will not reuse the segments allocated at the server end
  // we expose three segments
  // This number should be equals to the number array plus three (points, normal, cell)
  std::vector<std::pair<void*, std::size_t> > segments(3);
  segments[0].first = points.data();
  segments[0].second = points.size() * sizeof(double);
  segments[1].first = normals.data();
  segments[1].second = normals.size() * sizeof(double);
  segments[2].first = cells.data();
  segments[2].second = cells.size() * sizeof(int);

  std::vector<size_t> transferSizeList;
  for (int i = 0; i < segments.size(); i++)
  {
    transferSizeList.push_back(segments[i].second);
  }

  clock_gettime(CLOCK_REALTIME, &end1);
  diff1 = (end1.tv_sec - start.tv_sec) * 1.0 + (end1.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
  DEBUG("stage marshal : " << diff1);

  DEBUG("point size  " << segments[0].second << " normal size " << segments[1].second
                       << " cell size " << segments[2].second);
  // organize it into the bulk and register memory
  tl::bulk expdataBulk = client->m_clientEnginePtr->expose(segments, tl::bulk_mode::read_only);

  if (client->m_associatedDataServer.compare("") == 0)
  {
    client->m_associatedDataServer = client->getServerAddr();
  }

  clock_gettime(CLOCK_REALTIME, &end2);
  diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
  DEBUG("stage registermem : " << diff2);

  DEBUG("server addr is " << client->m_associatedDataServer);
  tl::remote_procedure remoteputvtkexp = client->m_clientEnginePtr->define("putvtkexp");
  tl::endpoint serverEndpoint = client->lookup(client->m_associatedDataServer);
  int status = remoteputvtkexp.on(serverEndpoint)(
    client->m_position, step, varName, dataSummary, expdataBulk, transferSizeList);

  if (status != 0)
  {
    throw std::runtime_error("failed to put the exp vtk data");
    return -1;
  }

  clock_gettime(CLOCK_REALTIME, &end3);
  diff3 = (end3.tv_sec - end2.tv_sec) * 1.0 + (end3.tv_nsec - end2.tv_nsec) * 1.0 / BILLION;
  DEBUG("stage transfer : " << diff3);

  // do not update the metadata for the experimental version
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
    // check original data
    //((vtkPolyData*)dataContainerSrc)->PrintSelf(std::cout, vtkIndent(5));
    // this is the continuous mem space version
    // int status = putVTKData(this, step, varName, dataSummary, dataContainerSrc);
    // this is the descrete mem space version
    // int status = putVTKDataExp(this, step, varName, dataSummary, dataContainerSrc);
    int status = putVTKDataExpZeroOneRpc(this, step, varName, dataSummary, dataContainerSrc);

    if (status != 0)
    {
      throw std::runtime_error("failed to put the vtk data");
    }
  }
  else if (dataType == DATATYPE_VTKEXPLICIT)
  {
    // int status = putArrayIntoBlock(this, dataSummary, dataContainerSrc);
    int status = putArrayIntoBlockZero(this, dataSummary, dataContainerSrc);
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

void UniClient::executeAsyncExp(int step, std::string blockid)
{
  // get the server id by rrb
  if (this->m_serverIDToAddr.size() == 0)
  {
    throw std::runtime_error("the m_serverIDToAddr is supposed to initilized");
  }
  int serverid = step % m_serverIDToAddr.size();

  std::cout << "---debug executeAsyncExp serverid is " << serverid << std::endl;

  // send the trigger operation
  std::string serverAddr = this->m_serverIDToAddr[serverid];

  tl::remote_procedure remoteexecuteAsyncExp =
    this->m_clientEnginePtr->define("executeAsyncExp").disable_response();
  tl::endpoint serverEndpoint = this->lookup(serverAddr);
  std::string funcName = "testaggrefunc";
  std::vector<std::string> funcparameter;
  remoteexecuteAsyncExp.on(serverEndpoint)(blockid, funcName, funcparameter);
  return;
}

// TODO update this
// currently we basic ask every stage if there are required data
std::vector<vtkSmartPointer<vtkPolyData> > UniClient::aggregatePolyBySuffix(
  std::string blockIDSuffix)
{
  // send request to all stages nodes
  tl::remote_procedure remotegetBlockSummayBySuffix =
    this->m_clientEnginePtr->define("getBlockSummayBySuffix");

  tl::remote_procedure remotegetArraysExplicit =
    this->m_clientEnginePtr->define("getArraysExplicit");

  std::vector<vtkSmartPointer<vtkPolyData> > polylist;

  // a vector based on the poydata

  for (auto& kv : this->m_serverToEndpoints)
  {
    tl::endpoint serverEndpoint = kv.second;
    std::vector<BlockSummary> tempbsList =
      remotegetBlockSummayBySuffix.on(serverEndpoint)(blockIDSuffix);
    // std::cout << "check server: " << kv.first << std::endl;
    for (auto& v : tempbsList)
    {
      // v.printSummary();
      // maybe try to transfer data here
      // assign the space
      // create the bulk
      // get the poly data back
      // points, normals, cells
      // allocate memory space
      // go through array
      if (v.m_arrayListLen != 3)
      {
        throw std::runtime_error(
          "the array len is supposed to be 3, but now it is: " + std::to_string(v.m_arrayListLen));
      }
      std::vector<std::pair<void*, std::size_t> > segments(3);
      for (int i = 0; i < 3; i++)
      {
        size_t transferSize = v.m_arrayList[i].m_elemSize * v.m_arrayList[i].m_elemNum;
        void* memspace = ::operator new(transferSize);

        if (strcmp(v.m_arrayList[i].m_arrayName, "points") == 0)
        {
          segments[0].first = memspace;
          segments[0].second = transferSize;
        }
        else if (strcmp(v.m_arrayList[i].m_arrayName, "normals") == 0)
        {
          segments[1].first = memspace;
          segments[1].second = transferSize;
        }
        else if (strcmp(v.m_arrayList[i].m_arrayName, "cells") == 0)
        {
          segments[2].first = memspace;
          segments[2].second = transferSize;
        }
        else
        {
          throw std::runtime_error(
            "the array list name is unexpected" + std::string(v.m_arrayList[i].m_arrayName));
        }
      }

      // create the bulk
      // transfer the data back
      tl::bulk clientBulk = this->m_clientEnginePtr->expose(segments, tl::bulk_mode::write_only);
      int status = remotegetArraysExplicit.on(serverEndpoint)(std::string(v.m_blockid), clientBulk);
      if (status != 0)
      {
        throw std::runtime_error("failed to get the data back");
      }

      // create the polydata
      // generate the points array
      void* pointArrayptr = segments[0].first;
      void* normalArrayPtr = segments[1].first;
      void* polyArrayPtr = segments[2].first;

      int nPoints = (segments[0].second / sizeof(double)) / 3;
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
      int nCells = (segments[2].second / sizeof(int)) / 3;
      auto polys = vtkSmartPointer<vtkCellArray>::New();
      const int* tempc = (int*)polyArrayPtr;

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
      // std::cout << "aggregate blockid " << v.m_blockid
      //          << " cell number: " << polyData->GetNumberOfPolys() << std::endl;
      polylist.push_back(polyData);

      // segment can be deleted here
      // since we already have the poly data that use the memory space of the poly array
      ::operator delete(segments[0].first);
      ::operator delete(segments[1].first);
      ::operator delete(segments[2].first);
    }
  }

  // getBlockSummayBySuffix
  return polylist;
}

}