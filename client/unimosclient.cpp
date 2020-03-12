
#include "unimosclient.h"
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#define BILLION 1000000000L

/*

    tl::remote_procedure sum = myEngine.define("sum");
    tl::endpoint server = myEngine.lookup(argv[1]);
    //attention, the return value here shoule be same with the type defined at the server end
    int ret = sum.on(server)(42,63);
*/

void UniClient::startTimer()
{
    tl::remote_procedure remotestartTimer = this->m_clientEnginePtr->define("startTimer").disable_response();
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(this->m_masterAddr);
    //the parameters here should be consistent with the defination at the server end
    remotestartTimer.on(serverEndpoint)();
    return;
}

void UniClient::endTimer()
{
    tl::remote_procedure remotestartTimer = this->m_clientEnginePtr->define("endTimer").disable_response();
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(this->m_masterAddr);
    //the parameters here should be consistent with the defination at the server end
    remotestartTimer.on(serverEndpoint)();
    return;
}

int UniClient::getAllServerAddr()
{
    if (this->m_uniCache == nullptr)
    {
        throw std::runtime_error("the cache shouled not be nullptr when call getAllServerAddr");
    }
    tl::remote_procedure remotegetAllServerAddr = this->m_clientEnginePtr->define("getAllServerAddr");
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(this->m_masterAddr);
    //the parameters here should be consistent with the defination at the server end
    std::vector<MetaAddrWrapper> adrList = remotegetAllServerAddr.on(serverEndpoint)();
    if (adrList.size() == 0)
    {
        throw std::runtime_error("failed to get server list");
        return -1;
    }

    for (auto it = adrList.begin(); it != adrList.end(); it++)
    {
        this->m_uniCache->addAddr(it->m_index, it->m_addr);
    }
    return 0;
}

int UniClient::getServerNum()
{
    if (this->m_masterAddr.compare("") == 0)
    {
        throw std::runtime_error("the master addr should be initilized before getServerNum");
        return 0;
    }

    //std::cout << "debug updateDHT server addr " << serverAddr << std::endl;
    tl::remote_procedure remotegetServerNum = this->m_clientEnginePtr->define("getServerNum");
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(this->m_masterAddr);
    //the parameters here should be consistent with the defination at the server end
    int serverNum = remotegetServerNum.on(serverEndpoint)();
    return serverNum;
}
//this is called by server node, the address is already known
int UniClient::updateDHT(std::string serverAddr, std::vector<MetaAddrWrapper> metaAddrWrapperList)
{
    //std::cout << "debug updateDHT server addr " << serverAddr << std::endl;
    tl::remote_procedure remoteupdateDHT = this->m_clientEnginePtr->define("updateDHT");
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(serverAddr);
    //the parameters here should be consistent with the defination at the server end
    int status = remoteupdateDHT.on(serverEndpoint)(metaAddrWrapperList);
    return status;
}

std::string UniClient::getServerAddr()
{

    //check the cache, return if exist
    int serverId = this->m_position % this->m_totalServerNum;
    //std::cout << "use server ID " << serverId << std::endl;
    if (this->m_uniCache == nullptr)
    {
        throw std::runtime_error("m_uniCache should not be null");
    }

    if (this->m_uniCache->ifAddrIDExist(serverId) == false)
    {
        throw std::runtime_error("server id not exist");
    }

    return this->m_uniCache->m_serverIDToAddr[serverId];
}

/*

    //if not exist, get the addr from the server according to specific id
    tl::remote_procedure remoteGetAddrByID = this->m_clientEnginePtr->define("getaddrbyID");
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(this->m_masterAddr);
    //the parameters here should be consistent with the defination at the server end
    std::string serverAddr = remoteGetAddrByID.on(serverEndpoint)(serverId);
    std::cout << "get server addr by explicit call: " << serverAddr << std::endl;
    //store the data into the cache
    if (this->m_uniCache != nullptr)
    {
        this->m_uniCache->addAddr(serverId, serverAddr);
    }

    return serverAddr;
}
*/

std::string UniClient::loadMasterAddr(std::string masterConfigFile)
{

    std::ifstream infile(masterConfigFile);
    std::string content = "";
    std::getline(infile, content);
    //spdlog::debug("load master server conf {}, content -{}-", masterConfigFile,content);
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

//this is called by the server node, the adress is already known
//int UniClient::putmetadata(std::string serverAddr, size_t step, std::string varName, RawDataEndpoint &rde)
//{
//tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
//    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(serverAddr);
//    return

//}
/*
    struct timespec start, end;
    double diff;
    clock_gettime(CLOCK_REALTIME, &start); 
    sleep(1.0);
    //some functions here
    clock_gettime(CLOCK_REALTIME, &end); 
    diff = (end.tv_sec - start.tv_sec) * 1.0 + (end.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
*/

void UniClient::initPutRawData(size_t dataMallocSize)
{
    //std::cout << "init the data bulk size " << dataMallocSize << std::endl;
    this->m_bulkSize = dataMallocSize;
    this->m_dataContainer = (void *)malloc(dataMallocSize);
    this->m_segments = std::vector<std::pair<void *, std::size_t>>(1);
    this->m_segments[0].first = (void *)(this->m_dataContainer);
    this->m_segments[0].second = dataMallocSize;
    //register the memory
    this->m_dataBulk = this->m_clientEnginePtr->expose(this->m_segments, tl::bulk_mode::read_only);
    return;
}

int UniClient::putrawdata(size_t step, std::string varName, BlockSummary &dataSummary, void *dataContainerSrc)
{
    //struct timespec start, end1, end2;
    //double diff1, diff2;
    //clock_gettime(CLOCK_REALTIME, &start);
    size_t dataMallocSize = dataSummary.getTotalSize();
    if (this->m_associatedDataServer.compare("") == 0)
    {
        //init the endpoint
        //get the server addr according to the m_position
        if (this->m_uniCache == nullptr)
        {
            throw std::runtime_error("the client cache could not be nullptr");
        }

        this->m_associatedDataServer = this->getServerAddr();
        //std::cout << "m_position " << m_position << " m_associatedDataServer " << this->m_associatedDataServer << std::endl;
        this->m_serverEndpoint = this->m_clientEnginePtr->lookup(this->m_associatedDataServer);
        this->initPutRawData(dataMallocSize);
    }

    if (dataMallocSize != this->m_bulkSize)
    {
        throw std::runtime_error("mismatch of the data size");
    }

    //copy data into the current datacontainer
    memcpy(this->m_dataContainer, dataContainerSrc, dataMallocSize);

    tl::remote_procedure remotePutRawData = this->m_clientEnginePtr->define("putrawdata");
    std::vector<MetaDataWrapper> mdwList = remotePutRawData.on(this->m_serverEndpoint)(this->m_position, step, varName, dataSummary, this->m_dataBulk);
    //auto request = remotePutRawData.on(this->m_serverEndpoint).async(this->m_position ,step, varName, dataSummary, this->m_dataBulk);

    //bool completed = request.received();
    // ...
    // actually wait on the request and get the result out of it
    //std::vector<MetaDataWrapper> mdwList = request.wait();

    //clock_gettime(CLOCK_REALTIME, &end1);
    //diff1 = (end1.tv_sec - start.tv_sec) * 1.0 + (end1.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
    //std::cout << "put stage 1: " << diff1 << std::endl;

    //has been updated (data server and metadata server are same)
    //mdw.printInfo();
    if (mdwList.size() == 0)
    {
        return 0;
    }

    //range mdwList
    tl::remote_procedure remotePutMetaData = this->m_clientEnginePtr->define("putmetadata");
    for (auto it = mdwList.begin(); it != mdwList.end(); it++)
    {
        //get dest server
        //    int status = this->m_remotePutMetaData.on(m_serverEndpoint)(step, varName, rde);
        //   return status;
        //int status = this->putmetadata(it->m_destAddr,
        //                               it->m_step,
        //                               it->m_varName,
        //                               it->m_rde);
        tl::endpoint metaserverEndpoint = this->m_clientEnginePtr->lookup(it->m_destAddr);
        int status = remotePutMetaData.on(metaserverEndpoint)(it->m_step, it->m_varName, it->m_rde);
        if (status != 0)
        {
            std::cerr << "failed to put metadata" << std::endl;
            it->printInfo();
            return -1;
        }
    }

    //clock_gettime(CLOCK_REALTIME, &end2);
    //diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
    //std::cout << "put stage 2: " << diff2 << std::endl;

    return 0;
}

//get MetaAddrWrapper List this contains the coresponding server that overlap with current querybox
//this can be called once for multiple get in different time step, since partition is fixed
std::vector<std::string> UniClient::getmetaServerList(size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub)
{

    std::string serverAddr = this->m_masterAddr;
    tl::remote_procedure remotegetmetaServerList = this->m_clientEnginePtr->define("getmetaServerList");
    //tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(serverAddr);
    std::vector<std::string> metaserverList = remotegetmetaServerList.on(serverEndpoint)(dims, indexlb, indexub);
    return metaserverList;
}

std::vector<RawDataEndpoint> UniClient::getrawDataEndpointList(std::string serverAddr,
                                                               size_t step,
                                                               std::string varName,
                                                               size_t dims,
                                                               std::array<int, 3> indexlb,
                                                               std::array<int, 3> indexub)
{
    tl::remote_procedure remotegetrawDataEndpointList = this->m_clientEnginePtr->define("getRawDataEndpointList");
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(serverAddr);
    std::vector<RawDataEndpoint> rawDataEndpointList = remotegetrawDataEndpointList.on(serverEndpoint)(step, varName, dims, indexlb, indexub);
    return rawDataEndpointList;
}

int UniClient::getSubregionData(std::string serverAddr, std::string blockID, size_t dataSize,
                                size_t dims,
                                std::array<int, 3> indexlb,
                                std::array<int, 3> indexub,
                                void *dataContainer)
{

    tl::remote_procedure remotegetSubregionData = this->m_clientEnginePtr->define("getDataSubregion");
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(serverAddr);

    std::vector<std::pair<void *, std::size_t>> segments(1);
    segments[0].first = (void *)(dataContainer);
    segments[0].second = dataSize;

    tl::bulk clientBulk = this->m_clientEnginePtr->expose(segments, tl::bulk_mode::write_only);

    int status = remotegetSubregionData.on(serverEndpoint)(blockID, dims, indexlb, indexub, clientBulk);
    return status;
}

//this method is the wrapper for the getSubregionData and getrawDataEndpointList
//return the assembled matrix
MATRIXTOOL::MatrixView UniClient::getArbitraryData(
    size_t step,
    std::string varName,
    size_t elemSize,
    size_t dims,
    std::array<int, 3> indexlb,
    std::array<int, 3> indexub)
{
    struct timespec start, end1, end2;
    double diff1, diff2;
    clock_gettime(CLOCK_REALTIME, &start);

    //spdlog::debug("index lb {} {} {}", indexlb[0], indexlb[1], indexlb[2]);
    //spdlog::debug("index ub {} {} {}", indexlb[0], indexlb[1], indexlb[2]);
    std::vector<std::string> metaList = this->getmetaServerList(dims, indexlb, indexub);

    std::vector<MATRIXTOOL::MatrixView> matrixViewList;

    for (auto it = metaList.begin(); it != metaList.end(); it++)
    {
        std::string metaServerAddr = *it;
        std::vector<RawDataEndpoint> rweList = this->getrawDataEndpointList(metaServerAddr, step, varName, dims, indexlb, indexub);
        std::cout << "metadata server " << metaServerAddr << " size of rweList " << rweList.size() << std::endl;
        if (rweList.size() == 0)
        {
            throw std::runtime_error("failed to get the overlap raw data endpoint for " + metaServerAddr);
        }
        for (auto itrwe = rweList.begin(); itrwe != rweList.end(); itrwe++)
        {
            //itrwe->printInfo();

            //get the subrigion according to the information at the list
            //it is better to use the multithread here

            //get subrigion of the data
            //allocate size (use differnet strategy is the vtk object is used)
            BBXTOOL::BBX *bbx = new BBXTOOL::BBX(dims, itrwe->m_indexlb, itrwe->m_indexub);
            size_t allocSize = sizeof(double) * bbx->getElemNum();
            //std::cout << "alloc size is " << allocSize << std::endl;

            void *subDataContainer = (void *)malloc(allocSize);

            //get the data by subregion api

            int status = this->getSubregionData(itrwe->m_rawDataServerAddr,
                                                itrwe->m_rawDataID,
                                                allocSize, dims,
                                                itrwe->m_indexlb,
                                                itrwe->m_indexub,
                                                subDataContainer);

            if (status != 0)
            {
                throw std::runtime_error("failed for get subrigion data for current raw data endpoint");
            }

            //check resutls
            //std::cout << "check subregion value " << std::endl;
            //double *temp = (double *)subDataContainer;
            //for (int i = 0; i < 5; i++)
            //{
            //    double value = *(temp + i);
            //    std::cout << "value " << value << std::endl;
            //}

            //put it into the Matrix View
            MATRIXTOOL::MatrixView mv(bbx, subDataContainer);
            matrixViewList.push_back(mv);
        }
    }

    //stage 1
    //clock_gettime(CLOCK_REALTIME, &end1);
    //diff1 = (end1.tv_sec - start.tv_sec) * 1.0 + (end1.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
    //std::cout<<"get data stage 1: " << diff1 << std::endl;

    //assemble the matrix View
    BBX *intactbbx = new BBX(dims, indexlb, indexub);
    MATRIXTOOL::MatrixView mvassemble = MATRIXTOOL::matrixAssemble(elemSize, matrixViewList, intactbbx);

    //free the element in matrixViewList
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

    //stage 2
    //clock_gettime(CLOCK_REALTIME, &end2);
    //diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
    //std::cout<<"get data stage 2: " << diff2 << std::endl;

    return mvassemble;
}

//void getDataSubregion(const tl::request &req, std::string &blockID, size_t &dims, std::array<int, 3> &subregionlb, std::array<int, 3> &subregionub)

/*
//get the raw data according to the endpoint list
int UniClient::getrawdata(std::string serverAddr, RawDataEndpoint &bs, void *dataContainer)
{

    //assume that the memory is allocated successfully for the dataContainer pointer

    //get a list of RawDataEndpoint
}
*/

//put trigger info into specific metadata server
int UniClient::putTriggerInfo(
    std::string serverAddr,
    std::string triggerName,
    DynamicTriggerInfo &dti)
{
    tl::remote_procedure remoteputTriggerInfo = this->m_clientEnginePtr->define("putTriggerInfo");
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(serverAddr);
    int status = remoteputTriggerInfo.on(serverEndpoint)(triggerName, dti);
    return status;
}

void UniClient::registerTrigger(
    size_t dims,
    std::array<int, 3> indexlb,
    std::array<int, 3> indexub,
    std::string triggerName,
    DynamicTriggerInfo &dti)
{

    std::vector<std::string> metaList = this->getmetaServerList(dims, indexlb, indexub);

    std::vector<MATRIXTOOL::MatrixView> matrixViewList;

    for (auto it = metaList.begin(); it != metaList.end(); it++)
    {
        //send request to coresponding metadata server
        //and add the trigger
        std::string metaServerAddr = *it;
        int status = putTriggerInfo(metaServerAddr, triggerName, dti);
        if (status != 0)
        {
            throw std::runtime_error("failed to putTriggerInfo for metaServer " + metaServerAddr);
        }
    }
    return;
}

/*
std::string &blockID,
                    std::string &functionName,
                    std::vector<std::string> &funcParameters
*/
std::string UniClient::executeRawFunc(
    std::string serverAddr,
    std::string blockID,
    std::string functionName,
    std::vector<std::string> &funcParameters)
{
    tl::remote_procedure remoteexecuteRawFunc = this->m_clientEnginePtr->define("executeRawFunc");
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(serverAddr);
    std::string result = remoteexecuteRawFunc.on(serverEndpoint)(blockID, functionName, funcParameters);
    return result;
}

//start a server that could listen to the notification from the server
//this server can be started by a sepatate xstream by using the lamda expression
//this only need to be started on specific rank such as rank=0
