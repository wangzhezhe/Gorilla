
#include "unimosclient.h"

/*

    tl::remote_procedure sum = myEngine.define("sum");
    tl::endpoint server = myEngine.lookup(argv[1]);
    //attention, the return value here shoule be same with the type defined at the server end
    int ret = sum.on(server)(42,63);
*/

//TODO add the namespace here


int UniClient::updateDHT(std::string serverAddr, std::vector<MetaAddrWrapper> metaAddrWrapperList)
{
    std::cout << "debug updateDHT server addr " << serverAddr << std::endl;
    tl::remote_procedure remoteupdateDHT = this->m_clientEnginePtr->define("updateDHT");
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(serverAddr);
    //the parameters here should be consistent with the defination at the server end
    int status = remoteupdateDHT.on(serverEndpoint)(metaAddrWrapperList);
    return status;
}

std::string UniClient::getServerAddrByRRbin(){
    
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
    spdlog::debug ("load master server: {}", content);

    return content;
}


int UniClient::putrawdata(size_t step, std::string varName, BlockSummary &dataSummary,  void* dataContainer){
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
    int status=remotePutRawData.on(serverEndpoint)(step, varName, dataSummary, dataBulk);

    return status;

}


/*
BlockMeta dspaces_client_getblockMeta(tl::engine &myEngine, std::string serverAddr, std::string varName, int ts, size_t blockID)
{
    //TODO put them at separate class
    tl::remote_procedure dsgetBlockMeta = myEngine.define("dsgetBlockMeta");
    //tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
    tl::endpoint globalServerEndpoint = myEngine.lookup(serverAddr);
    BlockMeta blockmeta = dsgetBlockMeta.on(globalServerEndpoint)(varName, ts, blockID);
    return blockmeta;
}

std::string dspaces_client_getaddr(tl::engine &myEngine, std::string serverAddr, std::string varName, int ts, size_t blockid)
{
    //TODO put them at separate class
    tl::remote_procedure dsgetaddr = myEngine.define("getaddr");
    //tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
    tl::endpoint globalServerEndpoint = myEngine.lookup(serverAddr);
    std::string returnAddr = dsgetaddr.on(globalServerEndpoint)(varName, ts, blockid);
    return returnAddr;
}

void dspaces_client_get(tl::engine &myEngine,
                        std::string serverAddr,
                        std::string varName,
                        int ts,
                        size_t blockID,
                        std::vector<double> &dataContainer)
{

    tl::remote_procedure dsget = myEngine.define("dsget");
    //tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
    tl::endpoint globalServerEndpoint = myEngine.lookup(serverAddr);

    std::vector<std::pair<void *, std::size_t>> segments(1);
    segments[0].first = (void *)(dataContainer.data());
    segments[0].second = dataContainer.size() * sizeof(double);

    tl::bulk clientBulk = myEngine.expose(segments, tl::bulk_mode::write_only);

    int status = dsget.on(globalServerEndpoint)(varName, ts, blockID, clientBulk);

    if (status != 0)
    {
        throw std::runtime_error("failed to get the data " + varName + " ts " + std::to_string(ts) + " blockid " + std::to_string(blockID) + " status " + std::to_string(status));
    }
    //std::cout << "status of the dsget is " << status << std::endl;

    //std::cout << "check data at the client end:" << std::endl;

    //for (int i = 0; i < 10; i++)
    //{
    //    std::cout << "index " << i << " value " << dataContainer[i] << std::endl;
    //}

    return;
}

//todo add template here
void dspaces_client_put(tl::engine &myEngine,
                        std::string serverAddr,
                        DataMeta &datameta,
                        size_t &blockID,
                        std::vector<double> &putVector)
{
    //TODO put them at separate class
    tl::remote_procedure dsput = myEngine.define("dsput");
    //tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
    tl::endpoint globalServerEndpoint = myEngine.lookup(serverAddr);

    std::vector<std::pair<void *, std::size_t>> segments(1);
    segments[0].first = (void *)(putVector.data());
    segments[0].second = datameta.extractBlockMeta().getBlockMallocSize();

    tl::bulk myBulk = myEngine.expose(segments, tl::bulk_mode::read_only);

    //TODO, use async I/O ? store the request to check if the i/o finish when sim finish
    //the data only could be updated when the previous i/o finish
    //auto request = dsput.on(globalServerEndpoint).async(datameta, blockID, myBulk);
    int status=dsput.on(globalServerEndpoint)(datameta, blockID, myBulk);
    return;
}

int dsnotify_subscriber(tl::engine &myEngine, std::string serverAddr, size_t &step, size_t &blockID)
{
    tl::remote_procedure notify = myEngine.define("notify");
    //tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
    tl::endpoint globalServerEndpoint = myEngine.lookup(serverAddr);
    notify.on(globalServerEndpoint)(step, blockID);
    return 0;
}

int dssubscribe(tl::engine &myEngine, std::string serverAddr, std::string varName, FilterProfile &fp)
{
    tl::remote_procedure subscribe = myEngine.define("subscribeProfile");
    //tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
    tl::endpoint globalServerEndpoint = myEngine.lookup(serverAddr);
    int status = subscribe.on(globalServerEndpoint)(varName, fp);
    return status;
}

//set request to all the workers
int dssubscribe_broadcast(tl::engine &myEngine, std::vector<std::string> serverList, std::string varName, FilterProfile &fp)
{
    tl::remote_procedure subscribe = myEngine.define("subscribeProfile");

    for (int i = 0; i < serverList.size(); i++)
    {
        tl::endpoint globalServerEndpoint = myEngine.lookup(serverList[i]);
        int status = subscribe.on(globalServerEndpoint)(varName, fp);
    }

    return 0;
}
*/



