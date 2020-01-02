
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
#include <uuid/uuid.h>

#include "../utils/stringtool.h"
#include "../utils/bbxtool.h"
#include "addrManager.h"
#include "settings.h"
#include "DHTManager/dhtmanager.h"
#include "../commondata/metadata.h"
#include "../client/unimosclient.h"
#include "RawdataManager/blockManager.h"
#include "MetadataManager/metadataManager.h"

namespace tl = thallium;

//global variables
tl::abt scope;

//The pointer to the enginge should be set as global element
//this shoule be initilized after the initilization of the argobot
tl::engine *globalServerEnginePtr = nullptr;
UniClient *globalClient = nullptr;

//name of configure file, the write one line, the line is the rank0 addr
//std::string masterConfigFile = "./unimos_server.conf";
Settings gloablSettings;

std::string MasterIP = "";
//rank & proc number for current MPI process
int globalRank = 0;
int globalProc = 0;

//the manager for all the server endpoints
AddrManager *addrManager = new AddrManager();

//the global manager for DHT
DHTManager *dhtManager = new DHTManager();

//the global manager for raw data
BlockManager blockManager;

//the meta data service
MetaDataManager metaManager;

void gatherIP(std::string engineName, std::string endpoint)
{

    std::string ip = IPTOOL::getClientAdddr(engineName, endpoint);

    if (globalRank == 0)
    {
        addrManager->ifMaster = true;
    }

    addrManager->nodeAddr = ip;

    //maybe it is ok that only write the masternode's ip and provide the interface
    //of getting ipList for other clients

    std::cout << "current rank is " << globalRank << " current ip is: " << ip << std::endl;

    //attention: there number should be changed if the endpoint is not ip
    //padding to 20 only for ip
    //for the ip, the longest is 15 add the start label and the end label
    //this number should longer than the address
    int msgPaddingLen = 50;
    if (endpoint.size() > msgPaddingLen)
    {
        throw std::runtime_error("current addr is longer than msgPaddingLen, reset the addr buffer to make it larger");
        return;
    }
    int sendLen = msgPaddingLen;
    int sendSize = sendLen * sizeof(char);
    char *sendipStr = (char *)malloc(sendSize);
    sprintf(sendipStr, "H%sE", ip.c_str());

    //std::cout << "check send ip: "<<string(sendipStr) << std::endl;

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
            spdlog::info("rank {} add raw data server {}", globalRank, ipList[i]);
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
        spdlog::info("rank {} add meta server, index {} and addr {}", globalRank, datawrapperList[i].m_index, datawrapperList[i].m_addr);
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
        spdlog::info("server {} put meta", addrManager->nodeAddr);
        rde.printInfo();
        metaManager.updateMetaData(step, varName, rde);
        req.respond(0);
    }
    catch (...)
    {
        spdlog::debug("exception for meta data put step {} varname {}", step, varName);
        rde.printInfo();
        req.respond(-1);
    }
}

//put the raw data into the raw data manager
void putrawdata(const tl::request &req, size_t &step, std::string &varName, BlockSummary &blockSummary, tl::bulk &dataBulk)
{

    //assume data is different when every rawdataput is called
    //generate the unique id for new data

    spdlog::debug("execute raw data put for server id {} :", globalRank);
    blockSummary.printSummary();

    //caculate the blockid by uuid

    uuid_t uuid;
    char strID[50];

    uuid_generate(uuid);
    uuid_unparse(uuid, strID);

    std::string blockID(strID);

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
        }
        //it is mandatory to use this expression
        std::vector<std::pair<void *, std::size_t>> segments(1);
        segments[0].first = localContainer;
        segments[0].second = mallocSize;

        //create the container for the data and expose it to remote server
        tl::bulk local = globalServerEnginePtr->expose(segments, tl::bulk_mode::write_only);
        //pull the data onto the server
        dataBulk.on(ep) >> local;
        //spdlog::debug("Server received bulk, check the contents: ");

        //check the bulk

        double *rawdata = (double *)localContainer;
        /*check the data contents at server end
        for (int i = 0; i < 10; i++)
        {
           std::cout << "index " << i << " value " << *rawdata << std::endl;
            rawdata++;
        }
        */
        //generate the empty container firstly, then get the data pointer
        int status = blockManager.putBlock(blockID, blockSummary, localContainer);

        spdlog::debug("---put block {} on server {} map size {}",blockID,addrManager->nodeAddr,blockManager.DataBlockMap.size());
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
            spdlog::info("step {} for var {}", step, varName);
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
        }
        
        free(BBXQuery);
        req.respond(0);
    }
    catch (...)
    {
        spdlog::debug("exception for data put");
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
        rawDataEndpointList = metaManager.getOverlapEndpoints(step, varName, BBXQuery);
        free(BBXQuery);
        req.respond(rawDataEndpointList);
    }
    catch (std::exception &e)
    {
        spdlog::debug("exception for getRawDataEndpointList: {}", std::string(e.what()));
        req.respond(rawDataEndpointList);
    }
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

        spdlog::debug ("map size on server {} is {}",addrManager->nodeAddr, blockManager.DataBlockMap.size());        
        if (blockManager.checkDataExistance(blockID) == false)
        {
            throw std::runtime_error("failed to get block id " + blockID + " on server with rank id " + std::to_string(globalRank));
        }
        BlockSummary bs = blockManager.getBlockSummary(blockID);
        size_t elemSize = bs.m_elemSize;
        BBXTOOL::BBX *bbx = new BBXTOOL::BBX(dims, subregionlb, subregionub);
        size_t allocSize = elemSize * bbx->getElemNum();
        spdlog::debug("alloc size at server {} is {}", globalRank, allocSize);


        blockManager.getBlockSubregion(blockID, dims, subregionlb, subregionub, dataContainer);

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
        spdlog::info("exception for getDataSubregion block id {} lb {},{},{} ub {},{},{}", blockID,
                     subregionlb[0], subregionlb[1], subregionlb[2],
                     subregionub[0], subregionub[1], subregionub[2]);
        spdlog::info("exception for getDataSubregion: {}", std::string(e.what()));
        req.respond(-1);
    }
}

/*
                                                std::string blockID,
                                             size_t dims,
                                             std::array<int, 3> subregionlb,
                                             std::array<int, 3> subregionub,
                                             void *&dataContainer
*/

void runRerver(std::string networkingType)
{
    std::cout << "---debug networkingType " << networkingType << std::endl;
    tl::engine serverEnginge(networkingType, THALLIUM_SERVER_MODE);
    globalServerEnginePtr = &serverEnginge;

    //the server engine can also be the client engine, only one engine can be declared here
    globalClient = new UniClient(globalServerEnginePtr, gloablSettings.masterInfo);

    globalServerEnginePtr->define("updateDHT", updateDHT);
    globalServerEnginePtr->define("getaddrbyrrb", getaddrbyrrb);
    globalServerEnginePtr->define("putrawdata", putrawdata);
    globalServerEnginePtr->define("putmetadata", putmetadata);
    globalServerEnginePtr->define("getmetaServerList", getmetaServerList);
    globalServerEnginePtr->define("getRawDataEndpointList", getRawDataEndpointList);
    globalServerEnginePtr->define("getDataSubregion", getDataSubregion);

    //for testing
    globalServerEnginePtr->define("hello", hello).disable_response();

    //globalClientEnginePointer->define("putDoubleVector", putDoubleVector).disable_response();
    //globalClientEnginePointer->define("putMetaData", putMetaData).disable_response();
    /*
    globalClientEnginePointer->define("dsput", dsput);
    globalClientEnginePointer->define("dsget", dsget);
    globalClientEnginePointer->define("dsgetBlockMeta", dsgetBlockMeta);
    globalClientEnginePointer->define("subscribeProfile", subscribeProfile);
    */

    std::string addr = serverEnginge.self();

    if (globalRank == 0)
    {
        spdlog::info("Start the unimos server with addr for master: {}", addr);

        //globalClientEnginePointer->define("getaddr", getaddr);
        std::string masterAddr = IPTOOL::getClientAdddr(networkingType, addr);
        std::ofstream confFile;
        confFile.open(gloablSettings.masterInfo);
        confFile << masterAddr << "\n";
        confFile.close();
    }

    //write the addr out here
    gatherIP(networkingType, addr);

    //bradcaster the ip to all the worker nodes use the thallium api
    if (addrManager->ifMaster)
    {
        //there is gathered address information only for the master node
        addrManager->broadcastMetaServer(globalClient);
    }

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

    //the copy operator is called here
    Settings tempset = Settings::from_json(argv[1]);
    gloablSettings = tempset;
    if (globalRank == 0)
    {
        gloablSettings.printsetting();
    }

    std::string networkingType = gloablSettings.protocol;

    std::cout << "debug check networking " << networkingType << std::endl;

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

    //config the address manager
    addrManager->m_serverNum = globalProc;
    addrManager->m_metaServerNum = gloablSettings.metaserverNum;

    //TODO this info need to be initiated by the client
    //config the dht manager
    size_t dataDims = gloablSettings.dims;
    BBX *globalBBX = new BBX(dataDims);
    for (int i = 0; i < dataDims; i++)
    {
        Bound *b = new Bound(0, gloablSettings.maxDimValue);
        globalBBX->BoundList.push_back(b);
    }
    dhtManager->initDHT(dataDims, gloablSettings.metaserverNum, globalBBX);
    std::cout << "---debug init ok " << std::endl;
    try
    {
        //load the filter manager
        bool addFilter = gloablSettings.datachecking;

        if (addFilter == true)
        {
            //mcache->loadFilterManager(fmanager);
            spdlog::info("load the filter for rank {}", globalRank);
        }

        runRerver(networkingType);
    }
    catch (const std::exception &e)
    {
        std::cout << "exception for server" << e.what() << std::endl;
        //release the resource
        return 1;
    }

    std::cout << "server close" << std::endl;

    MPI_Finalize();
    return 0;
}
