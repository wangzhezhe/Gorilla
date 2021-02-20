

#include "ClientForSim.hpp"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#define BILLION 1000000000L
namespace tl = thallium;

namespace GORILLA
{

// TODO, reduce the infromaion here, this is recieved by several partition
// consider how to reduce the notification
// send a function pointer here and call this function when there is notification information
void rcvNotify(const tl::request& req, BlockSummary& bs)
{
  // prtint the metadata info
  std::cout << "rcvNotify is called " << std::endl;
  bs.printSummary();
  return;
}

// before starting watch, it is necessary to register the watch operation to the server
// iterate all the server, if register the addr
// if the filter exist, register the watcher
// the clientEnginePtr should be server mode if the watcher is called
std::string Watcher::startWatch(tl::engine* enginePtr)
{

  enginePtr->define("rcvNotify", rcvNotify);
  std::string rawAddr = enginePtr->self();
  // get the margo instance and wait here
  std::cout << "start watcher for addr: " << rawAddr << std::endl;
  margo_wait_for_finalize(enginePtr->get_margo_instance());
  return rawAddr;
}

// get associated addr based on the rank and the total server num
std::string ClientForSim::getAssociatedServerAddr()
{
  if (this->m_totalServerNum == 0)
  {
    throw std::runtime_error("m_totalServerNum is not initilizsed");
  }
  // check the cache, return if exist
  int serverId = this->m_rank % this->m_totalServerNum;

  if (this->m_serverIDToAddr.find(serverId) == this->m_serverIDToAddr.end())
  {
    throw std::runtime_error("server id not exist " + std::to_string(this->m_rank) + " " +
      std::to_string(this->m_totalServerNum));
  }

  return this->m_serverIDToAddr[serverId];
}

// when there is no segment, assign it
// when there is existing segments but new one is larger than it
// allocate the new size
// the databulk will be freeed (margo bulk free) when the assignment or movement constructor is
// called
void ClientForSim::initPutRawData(size_t dataTransferSize)
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

// this should be the sim func
int ClientForSim::putCarGrid(
  size_t step, std::string varName, BlockSummary& dataSummary, void* dataContainerSrc)
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
  if (this->m_associatedDataServer.compare("") == 0)
  {
    this->m_associatedDataServer = this->getAssociatedServerAddr();
    // std::cout << "m_rank " << m_rank << " m_associatedDataServer " <<
    // this->m_associatedDataServer << std::endl; assume the size of the data block is fixed
    this->initPutRawData(dataTransferSize);
  }

  if (dataTransferSize > this->m_bulkSize)
  {
    throw std::runtime_error("the tran of the data size");
  }

  // prepare the info for data put
  // ask the placement way, use raw mem or write out the file
  tl::remote_procedure remotegetInfoForPut = this->m_clientEnginePtr->define("getinfoForput");
  tl::endpoint serverEndpoint = this->lookup(this->m_associatedDataServer);
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
    int status = this->b_manager.putBlock(dataSummary, BACKEND::FILE, dataContainerSrc);
    if (status != 0)
    {
      throw std::runtime_error("failed to put the data into the file");
      return -1;
    }
  }
  else if (ifp.m_putMethod.compare(RDMAPUT) == 0)
  {
    // update the server addr in raw data endpoint
    bdesc.m_rawDataServerAddr = this->m_associatedDataServer;
    // reuse the existing memory space
    // attention, if the simulation time is short consider to use the lock here
    // for every process, there are large span between two data write opertaion
    memcpy(this->m_segments[0].first, dataContainerSrc, dataTransferSize);

    tl::remote_procedure remotePutRawData = this->m_clientEnginePtr->define("putrawdata");
    tl::endpoint serverEndpoint = this->lookup(this->m_associatedDataServer);
    int status = remotePutRawData.on(serverEndpoint)(
      this->m_rank, step, varName, dataSummary, this->m_dataBulk);
    // example for async put
    // auto request = remotePutRawData.on(this->m_serverEndpoint).async(this->m_rank ,step,
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
    tl::remote_procedure remotePutMetaData = this->m_clientEnginePtr->define("putmetadata");
    for (auto it = ifp.m_metaServerList.begin(); it != ifp.m_metaServerList.end(); it++)
    {
      // std::cout << "debug mdwlist position" << this->m_rank << it->m_destAddr << std::endl;
      tl::endpoint metaserverEndpoint = this->lookup(*it);
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

void ClientForSim::executeAsyncExp(int step, std::string blockid)
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

int ClientForSim::putVTKData(
  size_t step, std::string varName, BlockSummary& dataSummary, void* dataContainerSrc)
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
  if (this->m_associatedDataServer.compare("") == 0)
  {
    this->m_associatedDataServer = this->getAssociatedServerAddr();
    DEBUG("m_rank " << this->m_rank << " m_associatedDataServer " << this->m_associatedDataServer);
  }
  // resize the mem space if the datatransfersize change

  this->initPutRawData(dataTransferSize);

  tl::remote_procedure remotegetInfoForPut = this->m_clientEnginePtr->define("getinfoForput");
  tl::endpoint serverEndpoint = this->lookup(this->m_associatedDataServer);

  InfoForPut ifp = remotegetInfoForPut.on(serverEndpoint)(
    step, dataTransferSize, dataSummary.m_dims, dataSummary.m_indexlb, dataSummary.m_indexub);

  BlockDescriptor bdesc(this->m_associatedDataServer, dataSummary.m_blockid,
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
  memcpy(this->m_segments[0].first, vtkbuffer->GetPointer(0), dataTransferSize);
  // instead of copy agagin, just let segments equals to pointer size
  // this can not be done before the bulk
  // this->m_segments[0].first = vtkbuffer->GetPointer(0);

  tl::remote_procedure remotePutRawData = this->m_clientEnginePtr->define("putrawdata");
  serverEndpoint = this->lookup(this->m_associatedDataServer);
  int status =
    remotePutRawData.on(serverEndpoint)(this->m_rank, step, varName, dataSummary, this->m_dataBulk);

  // example for async put
  // auto request = remotePutRawData.on(this->m_serverEndpoint).async(this->m_rank ,step,
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
  tl::remote_procedure remotePutMetaData = this->m_clientEnginePtr->define("putmetadata");
  for (auto it = ifp.m_metaServerList.begin(); it != ifp.m_metaServerList.end(); it++)
  {
    // std::cout << "debug mdwlist position" << this->m_rank << it->m_destAddr << std::endl;
    tl::endpoint metaserverEndpoint = this->lookup(*it);
    int status = remotePutMetaData.on(metaserverEndpoint)(step, varName, bdesc);
    if (status != 0)
    {
      std::cerr << "failed to put metadata" << std::endl;
      return -1;
    }
  }

  return 0;
}

int ClientForSim::putArrayIntoBlockZero(BlockSummary& dataSummary, void* dataContainerSrc)
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
  if (this->m_associatedDataServer.compare("") == 0)
  {
    this->m_associatedDataServer = this->getAssociatedServerAddr();
  }
  tl::remote_procedure remoteputArrayIntoBlock =
    this->m_clientEnginePtr->define("putArrayIntoBlock");
  tl::endpoint serverEndpoint = this->lookup(this->m_associatedDataServer);

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
    this->m_clientEnginePtr->expose(cellOffsetsegments, tl::bulk_mode::read_only);
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
    this->m_clientEnginePtr->expose(cellConnectsegments, tl::bulk_mode::read_only);
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
  tl::bulk pointBulk = this->m_clientEnginePtr->expose(pointssegments, tl::bulk_mode::read_only);

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
  tl::bulk normalsBulk = this->m_clientEnginePtr->expose(normalsegments, tl::bulk_mode::read_only);
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
int ClientForSim::putArrayIntoBlock(BlockSummary& dataSummary, void* dataContainerSrc)
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
  if (this->m_associatedDataServer.compare("") == 0)
  {
    this->m_associatedDataServer = this->getAssociatedServerAddr();
  }

  tl::remote_procedure remoteputArrayIntoBlock =
    this->m_clientEnginePtr->define("putArrayIntoBlock");
  tl::endpoint serverEndpoint = this->lookup(this->m_associatedDataServer);
  // send the points and cell
  std::vector<std::pair<void*, std::size_t> > pointssegments(1);
  pointssegments[0].first = points.data();
  pointssegments[0].second = points.size() * sizeof(double);
  tl::bulk pointBulk = this->m_clientEnginePtr->expose(pointssegments, tl::bulk_mode::read_only);

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
  tl::bulk cellsBulk = this->m_clientEnginePtr->expose(cellsegments, tl::bulk_mode::read_only);
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
  tl::bulk normalsBulk = this->m_clientEnginePtr->expose(normalsegments, tl::bulk_mode::read_only);
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
  //  this->m_clientEnginePtr->define("expcheckdata").disable_response();
  // remoteexpcheckdata.on(serverEndpoint)(dataSummary);

  return 0;
}

// experimental function with zero copy
int ClientForSim::putVTKDataExpZeroOneRpc(
  size_t step, std::string varName, BlockSummary& dataSummary, void* dataContainerSrc)
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
  tl::bulk expdataBulk = this->m_clientEnginePtr->expose(segments, tl::bulk_mode::read_only);

  if (this->m_associatedDataServer.compare("") == 0)
  {
    this->m_associatedDataServer = this->getAssociatedServerAddr();
  }

  clock_gettime(CLOCK_REALTIME, &end2);
  diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
  DEBUG("stage registermem : " << diff2);

  DEBUG("server addr is " << this->m_associatedDataServer);
  tl::remote_procedure remoteputvtkexp = this->m_clientEnginePtr->define("putvtkexpzero");
  tl::endpoint serverEndpoint = this->lookup(this->m_associatedDataServer);
  int status = remoteputvtkexp.on(serverEndpoint)(
    this->m_rank, step, varName, dataSummary, expdataBulk, transferSizeList);

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
int ClientForSim::putVTKDataExp(
  size_t step, std::string varName, BlockSummary& dataSummary, void* dataContainerSrc)
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
  tl::bulk expdataBulk = this->m_clientEnginePtr->expose(segments, tl::bulk_mode::read_only);

  if (this->m_associatedDataServer.compare("") == 0)
  {
    this->m_associatedDataServer = this->getAssociatedServerAddr();
  }

  clock_gettime(CLOCK_REALTIME, &end2);
  diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
  DEBUG("stage registermem : " << diff2);

  DEBUG("server addr is " << this->m_associatedDataServer);
  tl::remote_procedure remoteputvtkexp = this->m_clientEnginePtr->define("putvtkexp");
  tl::endpoint serverEndpoint = this->lookup(this->m_associatedDataServer);
  int status = remoteputvtkexp.on(serverEndpoint)(
    this->m_rank, step, varName, dataSummary, expdataBulk, transferSizeList);

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
int ClientForSim::putrawdata(
  size_t step, std::string varName, BlockSummary& dataSummary, void* dataContainerSrc)
{

  std::string dataType = std::string(dataSummary.m_dataType);

  if (dataType == DATATYPE_CARGRID)
  {

    int status = putCarGrid(step, varName, dataSummary, dataContainerSrc);
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
    int status = putVTKDataExpZeroOneRpc(step, varName, dataSummary, dataContainerSrc);

    if (status != 0)
    {
      throw std::runtime_error("failed to put the vtk data");
    }
  }
  else if (dataType == DATATYPE_VTKEXPLICIT)
  {
    // int status = putArrayIntoBlock(this, dataSummary, dataContainerSrc);
    int status = putArrayIntoBlockZero(dataSummary, dataContainerSrc);
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

// TODO, add the identity for different group
// only the master will send the notify information back to the client
// when put trigger, it is necessary to put identity
std::string ClientForSim::registerTrigger(size_t dims, std::array<int, 3> indexlb,
  std::array<int, 3> indexub, std::string triggerName, DynamicTriggerInfo& dti)
{

  std::vector<std::string> metaList =
    this->getmetaServerList(this->m_associatedDataServer, dims, indexlb, indexub);

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
MATRIXTOOL::MatrixView ClientForSim::getArbitraryData(size_t step, std::string varName,
  size_t elemSize, size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub)
{
  struct timespec start, end1, end2;
  double diff1, diff2;
  clock_gettime(CLOCK_REALTIME, &start);

  // spdlog::debug("index lb {} {} {}", indexlb[0], indexlb[1], indexlb[2]);
  // spdlog::debug("index ub {} {} {}", indexlb[0], indexlb[1], indexlb[2]);
  std::vector<std::string> metaList = this->getmetaServerList(this->m_associatedDataServer, dims, indexlb, indexub);

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

}