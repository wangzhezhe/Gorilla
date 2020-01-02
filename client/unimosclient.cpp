
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

/*

int UniClient::getqawDataEndpointList(std::vector<std::string> metaServerList, size_t step,
                                      std::string varName,
                                      size_t dim,
                                      std::array<int, 3> indexlb,
                                      std::array<int, 3> indexub)
{
    //go through the metaserver list and get the endpointlist for every server
}

//get the raw data according to the endpoint list
int UniClient::getrawdata(std::string serverAddr, RawDataEndpoint &bs, void *dataContainer)
{

    //assume that the memory is allocated successfully for the dataContainer pointer

    //get a list of RawDataEndpoint
}
*/
