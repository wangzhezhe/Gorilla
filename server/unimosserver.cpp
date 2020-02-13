
//the unified in memory server that contains both metadata manager and raw data manager

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <typeinfo>
#include <string>
#include <iostream>
#include <vector>
#include <csignal>

#include <thallium.hpp>

#include <spdlog/spdlog.h>
#include "mpi.h"

//#include <uuid/uuid.h>

#include "../utils/stringtool.h"
#include "../utils/bbxtool.h"
#include "../utils/uuid.h"

#include "addrManager.h"
#include "settings.h"
#include "DHTManager/dhtmanager.h"
#include "../commondata/metadata.h"
#include "../client/unimosclient.h"
#include "RawdataManager/blockManager.h"
#include "MetadataManager/metadataManager.h"
#include "FunctionManager/functionManager.h"
#include "TriggerManager/triggerManager.h"

//timer information
#include "../putgetMeta/metaclient.h"


#include <time.h>
#include <stdio.h>
#include <unistd.h>
#define BILLION 1000000000L

#ifdef USE_GNI
extern "C"
{
#include <rdmacred.h>
}
#include <mercury.h>
#include <margo.h>
#define DIE_IF(cond_expr, err_fmt, ...)                                                                           \
    do                                                                                                            \
    {                                                                                                             \
        if (cond_expr)                                                                                            \
        {                                                                                                         \
            fprintf(stderr, "ERROR at %s:%d (" #cond_expr "): " err_fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
            exit(1);                                                                                              \
        }                                                                                                         \
    } while (0)
#endif

namespace tl = thallium;

//global variables

//The pointer to the enginge should be set as global element
//this shoule be initilized after the initilization of the argobot
tl::engine *globalServerEnginePtr = nullptr;
//tl::engine *globalClientEnginePtr = nullptr;

UniClient *globalClient = nullptr;

//name of configure file, the write one line, the line is the rank0 addr
//std::string masterConfigFile = "./unimos_server.conf";
Settings gloablSettings;

std::string MasterIP = "";
//rank & proc number for current MPI process
int globalRank = 0;
int globalProc = 0;

//the manager for all the server endpoints
AddrManager *addrManager = nullptr;

//the global manager for DHT
DHTManager *dhtManager = nullptr;

//the global manager for raw data
BlockManager *blockManager = nullptr;

//the meta data service
MetaDataManager *metaManager = nullptr;

//the trigger manager
FunctionManagerMeta *fmetamanager = nullptr;
FunctionManagerRaw *frawmanager = nullptr;
DynamicTriggerManager *dtmanager = nullptr;

const std::string serverCred = "Gorila_cred_conf";

//the endpoint is the self addr for specific server
void gatherIP(std::string endpoint)
{
    //spdlog::info ("current rank is: {}, current ip is: {}", globalRank, endpoint);

    if(addrManager==nullptr){
        throw std::runtime_error("addrManager should not be null");
    }
    if (globalRank == 0)
    {
        addrManager->ifMaster = true;
    }

    addrManager->nodeAddr = endpoint;
    

    //maybe it is ok that only write the masternode's ip and provide the interface
    //of getting ipList for other clients


    //attention: there number should be changed if the endpoint is not ip
    //padding to 20 only for ip
    //for the ip, the longest is 15 add the start label and the end label
    //this number should longer than the address
    int msgPaddingLen = 200;
    if (endpoint.size() > msgPaddingLen)
    {
        throw std::runtime_error("current addr is longer than msgPaddingLen, reset the addr buffer to make it larger");
        return;
    }
    int sendLen = msgPaddingLen;
    int sendSize = sendLen * sizeof(char);
    char *sendipStr = (char *)malloc(sendSize);
    sprintf(sendipStr, "H%sE", endpoint.c_str());

    std::cout << "check send ip: "<<std::string(sendipStr) << std::endl;

    int rcvLen = sendLen;

    char *rcvString = NULL;


    if (globalRank == 0)
    {
        //it is possible that some ip are 2 digits and some are 3 digits
        //add extra space to avoid message truncated error
        //the logest ip is 15 digit plus one comma
        int rcvSize = msgPaddingLen * globalProc * sizeof(char);
        //std::cout << "sendSize: " << sendSize << ", rcvSize:" << rcvSize << std::endl;
        rcvString = (char *)malloc(rcvSize);
        {
            if (rcvString == NULL)
            {
                MPI_Abort(MPI_COMM_WORLD, 1);
                return;
            }
        }
    }

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

    //attention, the recv part is the size of the buffer recieved from the each thread instead of all the str
    //refer to https://stackoverflow.com/questions/37993214/segmentation-fault-on-mpi-gather-with-2d-arrays
    int error_code = MPI_Gather(sendipStr, sendLen, MPI_CHAR, rcvString, rcvLen, MPI_CHAR, 0, MPI_COMM_WORLD);
    if (error_code != MPI_SUCCESS)
    {
        std::cout << "error for rank " << globalRank << " get MPI_GatherError: " << error_code << std::endl;
    }
    //write to file for ip list json file if it is necessary
    //or expose the list by the rpc

    if (globalRank == 0)
    {
        //printf("check retuen value: ");
        //char *temp = rcvString;
        //for (int i = 0; i < rcvLen * globalProc; i++)
        //{
        //    printf("%c", *temp);
        //    temp++;
        //}
        //std::cout << '\n';
        //add termination for the last position
        //rcvString[rcvLen * procNum - 1] = '\0';
        //printf("rcv value: %s\n", rcvString);
        //string list = string(rcvString);
        //only fetch the first procNum ip
        std::vector<std::string> ipList = IPTOOL::split(rcvString, msgPaddingLen * globalProc, 'H', 'E');

        //std::cout << "check the ip list:" << std::endl;
        for (int i = 0; i < ipList.size(); i++)
        {
            //only store the worker addr
            //if (epManager->nodeAddr.compare(ipList[i]) != 0)
            //{
            //spdlog::debug("rank {} add raw data server {}", globalRank, ipList[i]);
            addrManager->m_endPointsLists.push_back(ipList[i]);

            //}
        }

        free(rcvString);
    }
}

void hello(const tl::request &req)
{
    std::cout << "Hello World!" << std::endl;
}

void hello2(const tl::request &req)
{
    std::cout << "Hello World!" << std::endl;
}

//currently we use the global dht for all the variables
//this can also be binded with specific variables
//when the global mesh changes, the dht changes
void updateDHT(const tl::request &req, std::vector<MetaAddrWrapper> &datawrapperList)
{

    //put the metadata into current epManager
    int size = datawrapperList.size();

    for (int i = 0; i < size; i++)
    {
        //TODO init dht
        spdlog::debug("rank {} add meta server, index {} and addr {}", globalRank, datawrapperList[i].m_index, datawrapperList[i].m_addr);
        //TODO add lock here
        dhtManager->metaServerIDToAddr[datawrapperList[i].m_index] = datawrapperList[i].m_addr;
    }

    req.respond(0);
    return;
}

//get server address by round roubin
void getaddrbyrrb(const tl::request &req)
{

    if (addrManager->ifMaster == false)
    {
        req.respond(std::string("NOTMASTER"));
    }

    std::string serverAddr = addrManager->getByRRobin();
    req.respond(serverAddr);
}

void putmetadata(const tl::request &req, size_t &step, std::string &varName, RawDataEndpoint &rde)
{
    try
    {
        spdlog::debug("server {} put meta", addrManager->nodeAddr);
        //rde.printInfo();
        metaManager->updateMetaData(step, varName, rde);
        req.respond(0);
    }
    catch (...)
    {
        spdlog::debug("exception for meta data put step {} varname {}", step, varName);
        req.respond(-1);
    }

    try
    {
        //execute init trigger
        //if the trigger is true
        if (gloablSettings.addTrigger)
        {
            dtmanager->initstart("InitTrigger", step, varName, rde);
        }
    }
    catch (std::exception &e)
    {
        spdlog::debug("exception for init trigger step {} varname {}: {}", step, varName, std::string(e.what()));
        rde.printInfo();
    }
}

//put the raw data into the raw data manager
void putrawdata(const tl::request &req, size_t &step, std::string &varName, BlockSummary &blockSummary, tl::bulk &dataBulk)
{
    struct timespec start, end1, end2;
    double diff1, diff2;
    clock_gettime(CLOCK_REALTIME, &start);

    //assume data is different when every rawdataput is called
    //generate the unique id for new data

    spdlog::debug("execute raw data put for server id {} :", globalRank);
    if(gloablSettings.logLevel>0){
        blockSummary.printSummary();  
    }

    //caculate the blockid by uuid
    std::string blockID = UUIDTOOL::generateUUID();

    spdlog::debug("blockID is {} on server {} ", blockID, addrManager->nodeAddr);

    tl::endpoint ep = req.get_endpoint();

    //assign the memory
    size_t mallocSize = blockSummary.m_elemSize * blockSummary.m_elemNum;

    spdlog::debug("malloc size is {}", mallocSize);

    try
    {
        void *localContainer = (void *)malloc(mallocSize);
        if (localContainer == NULL)
        {
            req.respond(-1);
            blockSummary.printSummary();
            spdlog::info ("failed to malloc data");
            return;
        }
        //it is mandatory to use this expression
        std::vector<std::pair<void *, std::size_t>> segments(1);
        segments[0].first = localContainer;
        segments[0].second = mallocSize;

        //create the container for the data and expose it to remote server
        tl::bulk local = globalServerEnginePtr->expose(segments, tl::bulk_mode::write_only);
        //pull the data onto the server
        dataBulk.on(ep) >> local;

        clock_gettime(CLOCK_REALTIME, &end1);
        diff1 = (end1.tv_sec - start.tv_sec) * 1.0 + (end1.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
        spdlog::debug("server put stage 1: {}", diff1);

        //spdlog::debug("Server received bulk, check the contents: ");
        //check the bulk
        //double *rawdata = (double *)localContainer;
        /*check the data contents at server end
        for (int i = 0; i < 10; i++)
        {
           std::cout << "index " << i << " value " << *rawdata << std::endl;
            rawdata++;
        }
        */
        //generate the empty container firstly, then get the data pointer
        //decreaset the data transfer, just keep the pointer instead of memcopy
        int status = blockManager->putBlock(blockID, blockSummary, localContainer);

        spdlog::debug("---put block {} on server {} map size {}", blockID, addrManager->nodeAddr, blockManager->DataBlockMap.size());
        if (status != 0)
        {
            blockSummary.printSummary();
            throw std::runtime_error("failed to put the raw data");
        }

        //put into the Block Manager

        //get the meta server according to bbx
        BBXTOOL::BBX *BBXQuery = new BBXTOOL::BBX(blockSummary.m_dims, blockSummary.m_indexlb, blockSummary.m_indexub);

        std::vector<ResponsibleMetaServer> metaserverList = dhtManager->getMetaServerID(BBXQuery);

        //update the coresponding metadata server, send information to corespond meta
        if (gloablSettings.logLevel > 0)
        {
            spdlog::debug("step {} for var {} size of metaserverList {}", step, varName, metaserverList.size());
            for (int i = 0; i < metaserverList.size(); i++)
            {
                std::cout << "metaserver index " << metaserverList[i].m_metaServerID << std::endl;
                metaserverList[i].m_bbx->printBBXinfo();
            }
        }

        //TODO, update the metaManager
        //update the metadata according to the address in tht metamap
        //go through metaserverList, for every id get the address from metaServerIDToAddr
        //send request to metaDataServer

        /*
        RawDataEndpoint(std::string rawDataServerAddr, std::string rawDataID,
                  size_t dims, std::array<int, 3> indexlb,
                  std::array<int, 3> indexub)
        */

        //update the coresponding metaserverList
        clock_gettime(CLOCK_REALTIME, &end2);
        diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
        spdlog::debug("server put stage 2: {}", diff2);


        //update the meta data by async way to improve the performance of the put operation
        for (auto it = metaserverList.begin(); it != metaserverList.end(); it++)
        {
            std::array<int, 3> indexlb = it->m_bbx->getIndexlb();
            std::array<int, 3> indexub = it->m_bbx->getIndexub();

            RawDataEndpoint rde(
                addrManager->nodeAddr,
                blockID,
                blockSummary.m_dims, indexlb, indexub);

            int metaServerId = it->m_metaServerID;
            if (dhtManager->metaServerIDToAddr.find(metaServerId) == dhtManager->metaServerIDToAddr.end())
            {
                throw std::runtime_error("faild to get the coresponding server id in dhtManager");
            }
            status = globalClient->putmetadata(dhtManager->metaServerIDToAddr[metaServerId], step, varName, rde);
            if (status != 0)
            {
                throw std::runtime_error("faild to update the metadata for id " + std::to_string(metaServerId));
            }
            //TODO, add the timer operation here
            std::string recordKey = blockSummary.m_extraInfo;
            if (recordKey.compare("") != 0)
            {
                MetaClient *metaclient = new MetaClient(globalServerEnginePtr);
                metaclient->Recordtime(recordKey);
            }
        }

        free(BBXQuery);
        req.respond(0);

    }
    catch (...)
    {
        spdlog::info("exception for data put");
        req.respond(-1);
    }
}

void putTriggerInfo(const tl::request &req, std::string triggerName, DynamicTriggerInfo &dti)
{

    try
    {
        dtmanager->updateTrigger(triggerName, dti);
        spdlog::debug("add trigger {} for server id {}", triggerName, globalRank);
        req.respond(0);
    }
    catch (...)
    {
        spdlog::debug("exception for putTriggerInfo with trigger name {}", triggerName);
        dti.printInfo();
        req.respond(-1);
    }
}

void getmetaServerList(const tl::request &req, size_t &dims, std::array<int, 3> &indexlb, std::array<int, 3> &indexub)
{
    std::vector<std::string> metaServerAddr;
    try
    {
        BBXTOOL::BBX *BBXQuery = new BBXTOOL::BBX(dims, indexlb, indexub);
        std::vector<ResponsibleMetaServer> metaserverList = dhtManager->getMetaServerID(BBXQuery);
        for (auto it = metaserverList.begin(); it != metaserverList.end(); it++)
        {
            int metaServerId = it->m_metaServerID;
            if (dhtManager->metaServerIDToAddr.find(metaServerId) == dhtManager->metaServerIDToAddr.end())
            {
                throw std::runtime_error("faild to get the coresponding server id in dhtManager for getmetaServerList");
            }
            metaServerAddr.push_back(dhtManager->metaServerIDToAddr[metaServerId]);
        }
        free(BBXQuery);
        req.respond(metaServerAddr);
    }
    catch (std::exception &e)
    {
        spdlog::debug("exception for getmetaServerList: {}", std::string(e.what()));
        req.respond(metaServerAddr);
    }
}

void getRawDataEndpointList(const tl::request &req,
                            size_t &step,
                            std::string &varName,
                            size_t &dims,
                            std::array<int, 3> &indexlb,
                            std::array<int, 3> &indexub)
{
    std::vector<RawDataEndpoint> rawDataEndpointList;
    try
    {
        BBXTOOL::BBX *BBXQuery = new BBXTOOL::BBX(dims, indexlb, indexub);
        rawDataEndpointList = metaManager->getOverlapEndpoints(step, varName, BBXQuery);
        free(BBXQuery);
        req.respond(rawDataEndpointList);
    }
    catch (std::exception &e)
    {
        spdlog::debug("exception for getRawDataEndpointList: {}", std::string(e.what()));
        req.respond(rawDataEndpointList);
    }
}

void executeRawFunc(const tl::request &req, std::string &blockID, 
std::string& functionName,
std::vector<std::string>& funcParameters)
{

    //get block summary
    BlockSummary bs = blockManager->getBlockSummary(blockID);

    //check data existance
    if (blockManager->DataBlockMap.find(blockID) == blockManager->DataBlockMap.end())
    {
        req.respond(std::string("NOTEXIST"));
        return;
    }

    DataBlockInterface *dbi = blockManager->DataBlockMap[blockID];
    void* rawDataPtr= dbi->getrawMemPtr();
    std::string  exeResults = frawmanager->execute(bs,rawDataPtr,
    functionName, funcParameters);
    req.respond(exeResults);
    return;
}

void getDataSubregion(const tl::request &req,
                      std::string &blockID,
                      size_t &dims,
                      std::array<int, 3> &subregionlb,
                      std::array<int, 3> &subregionub,
                      tl::bulk &clientBulk)
{

    try
    {
        void *dataContainer = NULL;

        spdlog::debug("map size on server {} is {}", addrManager->nodeAddr, blockManager->DataBlockMap.size());
        if (blockManager->checkDataExistance(blockID) == false)
        {
            throw std::runtime_error("failed to get block id " + blockID + " on server with rank id " + std::to_string(globalRank));
        }
        BlockSummary bs = blockManager->getBlockSummary(blockID);
        size_t elemSize = bs.m_elemSize;
        BBXTOOL::BBX *bbx = new BBXTOOL::BBX(dims, subregionlb, subregionub);
        size_t allocSize = elemSize * bbx->getElemNum();
        spdlog::debug("alloc size at server {} is {}", globalRank, allocSize);

        blockManager->getBlockSubregion(blockID, dims, subregionlb, subregionub, dataContainer);

        std::vector<std::pair<void *, std::size_t>> segments(1);
        segments[0].first = (void *)(dataContainer);
        segments[0].second = allocSize;

        tl::bulk returnBulk = globalServerEnginePtr->expose(segments, tl::bulk_mode::read_only);

        tl::endpoint ep = req.get_endpoint();
        clientBulk.on(ep) << returnBulk;

        free(bbx);
        req.respond(0);
    }
    catch (std::exception &e)
    {
        spdlog::debug("exception for getDataSubregion block id {} lb {},{},{} ub {},{},{}", blockID,
                      subregionlb[0], subregionlb[1], subregionlb[2],
                      subregionub[0], subregionub[1], subregionub[2]);
        spdlog::debug("exception for getDataSubregion: {}", std::string(e.what()));
        req.respond(-1);
    }
}

void initManager()
{
    //init global managers
    addrManager = new AddrManager();

    //config the address manager
    addrManager->m_serverNum = globalProc;
    addrManager->m_metaServerNum = gloablSettings.metaserverNum;

    //the global manager for raw data
    blockManager = new BlockManager();

    //the meta data service
    metaManager = new MetaDataManager();

    //dynamic trigger manager
    if (globalClient != NULL)
    {
        fmetamanager = new FunctionManagerMeta(globalClient);
        dtmanager = new DynamicTriggerManager(fmetamanager, 5);
    }
    else
    {
        spdlog::info("global client need to be initilized before function manager");
        exit(0);
    }

    frawmanager = new FunctionManagerRaw();

}

void runRerver(std::string networkingType)
{

#ifdef USE_GNI
    uint32_t drc_credential_id = 0;
    drc_info_handle_t drc_credential_info;
    uint32_t drc_cookie;
    char drc_key_str[256] = {0};
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
            //dest tag communicator
            MPI_Send(&drc_credential_id, 1, MPI_UINT32_T, dest, 0, MPI_COMM_WORLD);
        }

        //write this cred_id into file that can be shared by clients
        //output the credential id into the config files
        std::ofstream credFile;
        credFile.open(serverCred);
        credFile << drc_credential_id << "\n";
        credFile.close();
    }
    else
    {
        //send rcv is the block call
        //gather the id from the rank 0
        //source tag communicator
        MPI_Recv(&drc_credential_id, 1, MPI_UINT32_T, 0, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
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
    //the number here should same with the number of cores used in test scripts
    mid = margo_init_opt("gni", MARGO_SERVER_MODE, &hii, 0, 8);
    tl::engine serverEnginge(mid);
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
    globalServerEnginePtr->define("getaddrbyrrb", getaddrbyrrb);
    globalServerEnginePtr->define("putrawdata", putrawdata);
    globalServerEnginePtr->define("putmetadata", putmetadata);
    globalServerEnginePtr->define("getmetaServerList", getmetaServerList);
    globalServerEnginePtr->define("getRawDataEndpointList", getRawDataEndpointList);
    globalServerEnginePtr->define("getDataSubregion", getDataSubregion);
    globalServerEnginePtr->define("putTriggerInfo", putTriggerInfo);
    globalServerEnginePtr->define("executeRawFunc", executeRawFunc);

    //for testing
    globalServerEnginePtr->define("hello", hello).disable_response();


    std::string rawAddr = globalServerEnginePtr->self();
    std::string selfAddr = IPTOOL::getClientAdddr(networkingType, rawAddr);
    char tempAddr[200];
    std::string masterAddr;

    if (globalRank == 0)
    {
        spdlog::debug("Start the unimos server with addr for master: {}", selfAddr);

        //globalClientEnginePointer->define("getaddr", getaddr);
        masterAddr = selfAddr;
        //broadcast the master addr to all the servers
        std::copy(selfAddr.begin(), selfAddr.end(), tempAddr);
        tempAddr[selfAddr.size()] = '\0';
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(
        tempAddr,
        200,
        MPI_CHAR,
        0,
        MPI_COMM_WORLD);
    if (globalRank != 0)
    {
        masterAddr = std::string(tempAddr);
        spdlog::info("rank {} load master addr {}", globalRank, masterAddr);
    }


    //the server engine can also be the client engine, only one engine can be declared here
    globalClient = new UniClient(globalServerEnginePtr);
    globalClient->m_masterAddr = masterAddr;

    //the functionManager need the client, it need to be declared after the init of the globalClient
    initManager();

    //this will use the addManager
    gatherIP(selfAddr);

    //bradcaster the ip to all the worker nodes use the thallium api
    if (addrManager->ifMaster)
    {
        //there is gathered address information only for the master node
        addrManager->broadcastMetaServer(globalClient);
    }

    //write master server to file, server init ok
    if (globalRank == 0)
    {
        std::ofstream confFile;
        confFile.open(gloablSettings.masterInfo);
        confFile << masterAddr << "\n";
        confFile.close();
    }

#ifdef USE_GNI
    //destructor will not be called if send mid to engine
    spdlog::debug("call margo wait for rank {}", globalRank);
    margo_wait_for_finalize(mid);
#endif

    //the destructor of the engine will be called when the variable is out of the scope
    return;
}

void signalHandler(int signal_num)
{
    std::cout << "The interrupt signal is (" << signal_num << "),"
              << " call finalize manually.\n";
    globalServerEnginePtr->finalize();
    exit(signal_num);
}

int main(int argc, char **argv)
{

    MPI_Init(NULL, NULL);

    MPI_Comm_rank(MPI_COMM_WORLD, &globalRank);
    MPI_Comm_size(MPI_COMM_WORLD, &globalProc);

    //auto file_logger = spdlog::basic_logger_mt("unimos_server_log", "unimos_server_log.txt");
    //spdlog::set_default_logger(file_logger);

    if (argc != 2)
    {

        std::cerr << "Usage: unimos_server <path of setting.json>" << std::endl;
        exit(0);
    }

    tl::abt scope;
    //this manager should be init at the beginning
    //other managers are inited by intiManager after the init of the engine
    //the trigger manager
    dhtManager = new DHTManager();

    //the copy operator is called here
    Settings tempsetting = Settings::from_json(argv[1]);
    gloablSettings = tempsetting;
    if (globalRank == 0)
    {
        gloablSettings.printsetting();
    }

    std::string networkingType = gloablSettings.protocol;

    int logLevel = gloablSettings.logLevel;

    if (logLevel == 0)
    {
        spdlog::set_level(spdlog::level::info);
    }
    else
    {
        spdlog::set_level(spdlog::level::debug);
    }

    spdlog::debug("debug mode");

    signal(SIGINT, signalHandler);
    signal(SIGQUIT, signalHandler);
    signal(SIGTSTP, signalHandler);

    if (globalRank == 0)
    {
        std::cout << "total process num: " << globalProc << std::endl;
    }

    if (gloablSettings.metaserverNum > globalProc)
    {
        throw std::runtime_error("number of metaserver should less than the number of process");
    }
    int dataDims = gloablSettings.lenArray.size();
    //config the dht manager
    if (gloablSettings.partitionMethod.compare("SFC") == 0)
    {
        int maxLen = 0;
        for (int i = 0; i < dataDims; i++)
        {
            maxLen = std::max(maxLen, gloablSettings.lenArray[i]);
        }

        BBX *globalBBX = new BBX(dataDims);
        for (int i = 0; i < dataDims; i++)
        {
            Bound *b = new Bound(0, maxLen - 1);
            globalBBX->BoundList.push_back(b);
        }
        dhtManager->initDHTBySFC(dataDims, gloablSettings.metaserverNum, globalBBX);
    }
    else if (gloablSettings.partitionMethod.compare("manual") == 0)
    {
        //check the metaserverNum match with the partitionLayout
        int totalPartition = 1;
        for (int i = 0; i < gloablSettings.partitionLayout.size(); i++)
        {
            totalPartition = totalPartition * gloablSettings.partitionLayout[i];
        }
        if (totalPartition != gloablSettings.metaserverNum)
        {
            throw std::runtime_error("metaserverNum should equals to the product of partitionLayout[i]");
        }

        dhtManager->initDHTManually(gloablSettings.lenArray, gloablSettings.partitionLayout);
    }
    else
    {
        throw std::runtime_error("unsuported partition method " + gloablSettings.partitionMethod);
    }

    if (globalRank == 0)
    {
        //print metaServerIDToBBX
        for (auto it = dhtManager->metaServerIDToBBX.begin(); it != dhtManager->metaServerIDToBBX.end(); it++)
        {
            std::cout << "init DHT, meta id " << it->first << std::endl;
            it->second->printBBXinfo();
        }
    }
    try
    {
        //load the filter manager
        bool ifTrigger = gloablSettings.addTrigger;

        if (ifTrigger == true)
        {
            //mcache->loadFilterManager(fmanager);
            spdlog::info("load the filter for rank {}", globalRank);
        }

        runRerver(networkingType);
    }
    catch (const std::exception &e)
    {
        std::cout << "exception for server: " << e.what() << std::endl;
        //release the resource
        return 1;
    }

    std::cout << "server close" << std::endl;

    MPI_Finalize();
    return 0;
}
