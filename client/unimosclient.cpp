
#include "unimosclient.h"

/*

    tl::remote_procedure sum = myEngine.define("sum");
    tl::endpoint server = myEngine.lookup(argv[1]);
    //attention, the return value here shoule be same with the type defined at the server end
    int ret = sum.on(server)(42,63);
*/

//TODO add the namespace here

//this is called by server node, the address is already known
int UniClient::updateDHT(std::string serverAddr, std::vector<MetaAddrWrapper> metaAddrWrapperList)
{
    std::cout << "debug updateDHT server addr " << serverAddr << std::endl;
    tl::remote_procedure remoteupdateDHT = this->m_clientEnginePtr->define("updateDHT");
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(serverAddr);
    //the parameters here should be consistent with the defination at the server end
    int status = remoteupdateDHT.on(serverEndpoint)(metaAddrWrapperList);
    return status;
}

std::string UniClient::getServerAddrByRRbin()
{

    tl::remote_procedure remoteGetServerRRB = this->m_clientEnginePtr->define("getaddrbyrrb");
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(this->m_masterAddr);
    //the parameters here should be consistent with the defination at the server end
    std::string serverAddr = remoteGetServerRRB.on(serverEndpoint)();
    return serverAddr;
}

std::string UniClient::loadMasterAddr(std::string masterConfigFile)
{

    std::ifstream infile(masterConfigFile);
    std::string content;
    std::getline(infile, content);
    spdlog::debug("load master server: {}", content);

    return content;
}

int UniClient::putrawdata(size_t step, std::string varName, BlockSummary &dataSummary, void *dataContainer)
{
    //get the server by round roubin
    std::string rrbServerAddr = this->getServerAddrByRRbin();

    tl::remote_procedure remotePutRawData = this->m_clientEnginePtr->define("putrawdata");
    //tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(rrbServerAddr);

    size_t dataMallocSize = dataSummary.getTotalSize();
    std::vector<std::pair<void *, std::size_t>> segments(1);
    segments[0].first = (void *)(dataContainer);
    segments[0].second = dataMallocSize;

    tl::bulk dataBulk = this->m_clientEnginePtr->expose(segments, tl::bulk_mode::read_only);

    //TODO, use async I/O ? store the request to check if the i/o finish when sim finish
    //the data only could be updated when the previous i/o finish
    //auto request = dsput.on(globalServerEndpoint).async(datameta, blockID, myBulk);
    int status = remotePutRawData.on(serverEndpoint)(step, varName, dataSummary, dataBulk);

    return status;
}

//this is called by the server node, the adress is already known
int UniClient::putmetadata(std::string serverAddr, size_t step, std::string varName, RawDataEndpoint &rde)
{

    tl::remote_procedure remotePutMetaData = this->m_clientEnginePtr->define("putmetadata");
    //tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(serverAddr);
    int status = remotePutMetaData.on(serverEndpoint)(step, varName, rde);
    return status;
}

//get MetaAddrWrapper List this contains the coresponding server that overlap with current querybox
//this can be called once for multiple get in different time step, since partition is fixed
std::vector<std::string> UniClient::getmetaServerList(size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub)
{

    std::string rrbServerAddr = this->getServerAddrByRRbin();
    tl::remote_procedure remotegetmetaServerList = this->m_clientEnginePtr->define("getmetaServerList");
    //tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(rrbServerAddr);
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
            itrwe->printInfo();

            //get the subrigion according to the information at the list
            //it is better to use the multithread here

            //get subrigion of the data
            //allocate size (use differnet strategy is the vtk object is used)
            BBXTOOL::BBX *bbx = new BBXTOOL::BBX(dims, itrwe->m_indexlb, itrwe->m_indexub);
            size_t allocSize = sizeof(double) * bbx->getElemNum();
            std::cout << "alloc size is " << allocSize << std::endl;

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
            std::cout << "check subregion value " << std::endl;
            double *temp = (double *)subDataContainer;
            for (int i = 0; i < 5; i++)
            {
                double value = *(temp + i);
                std::cout << "value " << value << std::endl;
            }

            //put it into the Matrix View
            MATRIXTOOL::MatrixView mv(bbx, subDataContainer);
            matrixViewList.push_back(mv);
        }
    }

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
        if(status!=0){
            throw std::runtime_error("failed to putTriggerInfo for metaServer " + metaServerAddr);
        }
    }
    return;
}