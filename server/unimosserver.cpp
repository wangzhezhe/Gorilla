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

#include "settings.h"
#include "../commondata/metadata.h"
#include "../client/unimosclient.h"

#include "unimosserver.hpp"
#include "statefulConfig.h"

//timer information
//#include "../putgetMeta/metaclient.h"

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
tl::engine *globalClientEnginePtr = nullptr;
UniClient *uniClient = nullptr;
UniServer *uniServer = nullptr;

//name of configure file, the write one line, the line is the rank0 addr
//std::string masterConfigFile = "./unimos_server.conf";
Settings gloablSettings;

//rank & proc number for current MPI process
int globalRank = 0;
int globalProc = 0;

const std::string serverCred = "Gorila_cred_conf";

//the endpoint is the self addr for specific server
void gatherIP(std::string endpoint)
{
    //spdlog::info ("current rank is: {}, current ip is: {}", globalRank, endpoint);

    if (uniServer->m_addrManager == nullptr)
    {
        throw std::runtime_error("addrManager should not be null");
    }
    if (globalRank == 0)
    {
        uniServer->m_addrManager->ifMaster = true;
    }

    uniServer->m_addrManager->nodeAddr = endpoint;

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

    std::cout << "check send ip: " << std::string(sendipStr) << std::endl;

    int rcvLen = sendLen;

    char *rcvString = NULL;

    //if (globalRank == 0)
    //{
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

    //attention, the recv part is the size of the buffer recieved from the each thread instead of all the str
    //refer to https://stackoverflow.com/questions/37993214/segmentation-fault-on-mpi-gather-with-2d-arrays
    int error_code = MPI_Gather(sendipStr, sendLen, MPI_CHAR, rcvString, rcvLen, MPI_CHAR, 0, MPI_COMM_WORLD);
    if (error_code != MPI_SUCCESS)
    {
        std::cout << "error for rank " << globalRank << " get MPI_GatherError: " << error_code << std::endl;
    }
    //write to file for ip list json file if it is necessary
    //or expose the list by the rpc

    //broadcast the results to all the services
    //the size is different compared with the MPI_Gather
    error_code = MPI_Bcast(rcvString, rcvSize, MPI_CHAR, 0, MPI_COMM_WORLD);
    if (error_code != MPI_SUCCESS)
    {
        std::cout << "error for rank " << globalRank << " get MPI_BcastError: " << error_code << std::endl;
    }

    std::vector<std::string> ipList = IPTOOL::split(rcvString, msgPaddingLen * globalProc, 'H', 'E');

    for (int i = 0; i < ipList.size(); i++)
    {
        //store all server addrs

        spdlog::debug("rank {} add raw data server {}", globalRank, ipList[i]);
        uniServer->m_addrManager->m_endPointsLists.push_back(ipList[i]);
    }
    free(sendipStr);
    free(rcvString);
}

void hello(const tl::request &req)
{
    std::cout << "Hello World!" << std::endl;
}

void hello2(const tl::request &req)
{
    std::cout << "Hello World!" << std::endl;
}

void updateEndpoints(const tl::request &req, std::vector<std::string> &serverLists)
{

    //put the metadata into current epManager
    int size = serverLists.size();

    for (int i = 0; i < size; i++)
    {
        uniServer->m_addrManager->m_endPointsLists.push_back(serverLists[i]);
    }

    req.respond(0);
    return;
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
        uniServer->m_dhtManager->metaServerIDToAddr[datawrapperList[i].m_index] = datawrapperList[i].m_addr;
    }

    req.respond(0);
    return;
}

//this should be the addr of the compute nodes
//the meta nodes is invisiable for the data writer
void getAllServerAddr(const tl::request &req)
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

void getServerNum(const tl::request &req)
{
    //return number of the servers to the client
    int serNum = uniServer->m_addrManager->m_endPointsLists.size();
    req.respond(serNum);
}

void getaddrbyID(const tl::request &req, int serverID)
{
    std::string serverAddr = "";
    if (uniServer->m_dhtManager->metaServerIDToAddr.find(serverID) == uniServer->m_dhtManager->metaServerIDToAddr.end())
    {
        req.respond(serverAddr);
    }
    serverAddr = uniServer->m_dhtManager->metaServerIDToAddr[serverID];
    req.respond(serverAddr);
}

//get server address by round roubin
void getaddrbyrrb(const tl::request &req)
{

    if (uniServer->m_addrManager->ifMaster == false)
    {
        req.respond(std::string("NOTMASTER"));
    }

    std::string serverAddr = uniServer->m_addrManager->getByRRobin();
    req.respond(serverAddr);
}

//remove the metadata of specific step
//even if it is undeletable
void forceEraseMetaAndRawManually(size_t step)
{
    //delete the raw data forcelly, it works for any types
    //delete coresponding rawdata in async way
    uniServer->m_metaManager->m_metaDataMapMutex.lock();

    //traverse the map and release memory for old data
    std::cout << "rank " << globalRank << " delete metadata step by manual " << step << std::endl;

    for (auto &kv : uniServer->m_metaManager->m_metaDataMap[step])
    {
        std::string varName = kv.first;
        //std::cout << "delete metadata varName " << varName << std::endl;

        for (auto &kvinner : kv.second.m_metadataBlock)
        {
            std::string varType = kvinner.first;
            //std::cout << "delete metadata varType " << varType << " size " << kvinner.second.size() << std::endl;
            for (auto &it : kvinner.second)
            {
                RawDataEndpoint rde = it;
                //TODO chcek the status here
                uniClient->eraseRawData(rde.m_rawDataServerAddr, rde.m_rawDataID);
            }
        }
    }

    //remove the outlayer information from the map
    uniServer->m_metaManager->m_metaDataMap.erase(step);
    uniServer->m_metaManager->m_metaDataMapMutex.unlock();

    return;
}

//delete the step stored on this server
void deleteMetaStep(const tl::request &req, size_t step)
{
    forceEraseMetaAndRawManually(step);
    req.respond(0);
}

//TODO some data is undeletable, add another parameter, if there is important data, but it needs long time to be consumed, this need to be keeped for some time
//be careful about the setting the lb and the ub of the window here
//this step is current step, that is the latest step
void eraseMetaAndRaw(size_t step)
{

    uniServer->m_metaManager->m_boundMutex.lock();
    uniServer->m_metaManager->m_windowub = step;

    bool ifOutOfBuffer = ((uniServer->m_metaManager->m_windowub - uniServer->m_metaManager->m_windowlb + 1) > uniServer->m_metaManager->m_windowSize);
    size_t currentlb = uniServer->m_metaManager->m_windowlb;
    size_t currentub = uniServer->m_metaManager->m_windowub;
    if (ifOutOfBuffer)
    {
        uniServer->m_metaManager->m_windowlb = uniServer->m_metaManager->m_windowlb + uniServer->m_metaManager->m_deletedNum;
        std::cout << "debug windowlb windowub " << uniServer->m_metaManager->m_windowlb << " " << uniServer->m_metaManager->m_windowub << std::endl;
    }
    uniServer->m_metaManager->m_boundMutex.unlock();

    // erase the lower bound data when step window is larger than threshold
    // only modify lb when the window is larger than buffer
    if (ifOutOfBuffer)
    {
        for (int i = 0; i < uniServer->m_metaManager->m_deletedNum; i++)
        {
            //TODO make sure do not delete key data, use better way here
            //if (currentlb % 5 == 1 || currentlb % 5 == 2 || currentlb % 5 == 3)
            //{
            //    continue;
            //}

            //delete coresponding rawdata in async way
            uniServer->m_metaManager->m_metaDataMapMutex.lock();

            //traverse the map and release memory for old data
            std::cout << "rank " << globalRank << " delete metadata step " << currentlb << std::endl;

            for (auto &kv : uniServer->m_metaManager->m_metaDataMap[currentlb])
            {
                std::string varName = kv.first;
                //std::cout << "delete metadata varName " << varName << std::endl;

                for (auto &kvinner : kv.second.m_metadataBlock)
                {
                    std::string varType = kvinner.first;
                    //std::cout << "delete metadata varType " << varType << " size " << kvinner.second.size() << std::endl;
                    //use the vector conditional iterate and erase
                    auto iter = kvinner.second.begin();
                    while (iter != kvinner.second.end())
                    {
                        RawDataEndpoint rde = *iter;
                        //check the status
                        //raw or after process , delete
                        //std::cout << "curr status " << rde.m_metaStatus << std::endl;
                        while (true)
                        {
                            if (rde.m_metaStatus == MetaStatus::RAW || rde.m_metaStatus == MetaStatus::AFTERPROCESS)
                            {
                                //erase metadata
                                //TODO update the data structure to make it more efficient here

                                //erase rawdata, and make it points to the next element
                                iter = kvinner.second.erase(iter);
                                uniClient->eraseRawData(rde.m_rawDataServerAddr, rde.m_rawDataID);
                                break;
                            }
                            else if (rde.m_metaStatus == MetaStatus::BEFOREPROCESS || rde.m_metaStatus == MetaStatus::INPROCESS)
                            {
                                //std::cout << "debug wait the finish of the data with id " << rde.m_rawDataID << std::endl;
                                //usleep(500000);
                                //continue;
                                //TODO neglect these data for this step
                                //if we use sleep and continue, there will be a deadlock for the trigger process
                                //since it could not acquire the lock
                                ++iter;
                                break;
                            }
                            else if (rde.m_metaStatus == MetaStatus::UNDELETABLE)
                            {
                                //can not be deleted
                                ++iter;
                                break;
                            }
                        }
                    }
                }

                //erase coresponding metadata and the flag if it becomes empty
                //the current deleted step is i
                int blockSize = uniServer->m_metaManager->m_metaDataMap[currentlb][varName].getBlockNumberByVersion(DRIVERTYPE_RAWMEM);
                //std::cout << "debug metablock size for step: " << currentlb << " variable: " << varName << " size: " << blockSize << std::endl;
                if (blockSize == 0)
                {
                    //TODO, only remove the inner data if there are multiple versions in furture
                    uniServer->m_metaManager->m_metaDataMap[currentlb][varName].eraseBlocks(DRIVERTYPE_RAWMEM);
                }
                //std::cout << "debug m_metaDataMap size for step currentlb " << uniServer->m_metaManager->m_metaDataMap.count(currentlb) << std::endl;
            }

            //uniServer->m_metaManager->m_metaDataMap[currentlb] and  uniServer->m_metaManager->m_metaDataMap[currentlb][varName] can not be deleted
            //since they might be accessed by multiple threads
            //if we want to check if there are enough data before put opearation, we can use
            //coresponding data in the moritor manager [todo]
            currentlb++;

            uniServer->m_metaManager->m_metaDataMapMutex.unlock();
        }
    }
    return;
}

// the manager of the matadata controller should be maintained at this level
void putmetadata(const tl::request &req, size_t &step, std::string &varName, RawDataEndpoint rde)
{
    //TODO update the status of the RDE here
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
    catch (const std::exception &e)
    {
        spdlog::info("exception for meta data put step {} varname {} server {}", step, varName, uniServer->m_addrManager->nodeAddr);
        req.respond(-1);
        return;
    }

    try
    {
        //execute init trigger
        //if the trigger is true
        //TODO, add a lable to say if the metadata can be deleted
        //the trigger need to access the metadata in this way
        //maybe to notify it by send an RPC call by action operation
        if (gloablSettings.addTrigger == true)
        {
            //create thread on a particular ess id by round robin pattern
            int essid = uniServer->m_dtmanager->m_threadPool->getEssId();
            uniServer->m_dtmanager->m_threadPool->m_ess[essid]->make_thread([=]() {
                //the rde here is the copy version of the original rde
                uniServer->m_dtmanager->initstart("InitTrigger", step, varName, rde);
                //time it
                if (globalRank == 0)
                {
                    uniServer->m_frawmanager->m_statefulConfig->timeit();
                }
            },
            tl::anonymous());
            //it is unnecessary to store the thread by using the anonymous pattern
            spdlog::debug("start trigger for var {} step {} data id {}", varName, step, rde.m_rawDataID);
        }
    }
    catch (std::exception &e)
    {
        spdlog::info("exception for init trigger step {} varname {}: {}", step, varName, std::string(e.what()));
        rde.printInfo();
    }
    req.respond(0);
    // check meta to figure out if there is enough space by calling eraseMetaAndRaw();
    //delete raw data related with current metaserver
    eraseMetaAndRaw(step);

    return;
}

void eraserawdata(const tl::request &req, std::string &blockID)
{
    uniServer->m_blockManager->eraseBlock(blockID);
    req.respond(0);
    return;
}

//put the raw data into the raw data manager
void putrawdata(const tl::request &req, int clientID, size_t &step, std::string &varName, BlockSummary &blockSummary, tl::bulk &dataBulk)
{
    struct timespec start, end1, end2;
    double diff1, diff2;
    clock_gettime(CLOCK_REALTIME, &start);

    //assume data is different when every rawdataput is called
    //generate the unique id for new data

    std::vector<MetaDataWrapper> metadataWrapperList;
    spdlog::debug("execute raw data put for server id {}", globalRank);
    if (gloablSettings.logLevel > 0)
    {
        blockSummary.printSummary();
    }

    //caculate the blockid by uuid
    std::string blockID = UUIDTOOL::generateUUID();

    spdlog::debug("blockID is {} on server {} ", blockID, uniServer->m_addrManager->nodeAddr);

    //assign the memory
    size_t mallocSize = blockSummary.m_elemSize * blockSummary.m_elemNum;

    spdlog::debug("malloc size is {}", mallocSize);

    //if clientID is not in map, get avalible addr
    if (uniServer->m_bulkMap.find(clientID) == uniServer->m_bulkMap.end())
    {
        //create new bulk
        void *tempdataContainer = (void *)malloc(mallocSize);
        std::vector<std::pair<void *, std::size_t>> segments(1);

        segments[0].first = tempdataContainer;
        segments[0].second = mallocSize;

        tl::bulk tempBulk = globalServerEnginePtr->expose(segments, tl::bulk_mode::write_only);
        uniServer->m_bulkMapmutex.lock();
        uniServer->m_bulkMap[clientID] = tempBulk;
        uniServer->m_dataContainerMap[clientID] = tempdataContainer;
        uniServer->m_bulkMapmutex.unlock();
        spdlog::info("allocate new bulk for clientID {} serverRank {}", clientID, globalRank);
    }

    tl::bulk currentBulk = uniServer->m_bulkMap[clientID];

    try
    {
        tl::endpoint ep = req.get_endpoint();
        //pull the data onto the server
        dataBulk.on(ep) >> currentBulk;

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
        int status = uniServer->m_blockManager->putBlock(blockID, blockSummary, uniServer->m_dataContainerMap[clientID]);

        spdlog::debug("---put block {} on server {} map size {}", blockID, uniServer->m_addrManager->nodeAddr, uniServer->m_blockManager->DataBlockMap.size());
        if (status != 0)
        {
            blockSummary.printSummary();
            req.respond(metadataWrapperList);
            throw std::runtime_error("failed to put the raw data");
            return;
        }

        //put into the Block Manager

        //get the meta server according to bbx
        BBXTOOL::BBX BBXQuery(blockSummary.m_dims, blockSummary.m_indexlb, blockSummary.m_indexub);

        //TODO, if the query bbx is accoss mamy partitions if there are multiple records in the metaserver
        std::vector<ResponsibleMetaServer> metaserverList = uniServer->m_dhtManager->getMetaServerID(BBXQuery);

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

        //update the coresponding metaserverList[when the actual region is larger than metaserver, use the overlap as the index]
        //update the meta data by async way to improve the performance of the put operation
        for (auto it = metaserverList.begin(); it != metaserverList.end(); it++)
        {
            std::array<int, 3> indexlb = it->m_bbx->getIndexlb();
            std::array<int, 3> indexub = it->m_bbx->getIndexub();

            int metaServerId = it->m_metaServerID;
            if (uniServer->m_dhtManager->metaServerIDToAddr.find(metaServerId) == uniServer->m_dhtManager->metaServerIDToAddr.end())
            {
                req.respond(metadataWrapperList);
                throw std::runtime_error("faild to get the coresponding server id in dhtManager");
                return;
            }
            RawDataEndpoint rde(
                uniServer->m_addrManager->nodeAddr,
                blockID,
                blockSummary.m_dims, indexlb, indexub);

            clock_gettime(CLOCK_REALTIME, &end2);
            diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
            //spdlog::info("server put stage 2: {}", diff2);
            //if dest of server is current one
            std::string destAddr = uniServer->m_dhtManager->metaServerIDToAddr[metaServerId];
            /* update direactly if meta data service and raw data service locate on same node
            if (destAddr.compare(uniServer->m_addrManager->nodeAddr) == 0)
            {
                spdlog::debug("server {} put meta", uniServer->m_addrManager->nodeAddr);
                if (gloablSettings.logLevel > 0)
                {
                    rde.printInfo();
                }
                uniServer->m_metaManager->updateMetaData(step, varName, rde);
                if (gloablSettings.addTrigger == true)
                {

                    int essid = uniServer->m_dtmanager->m_threadPool->getEssId();
                    tl::managed<tl::thread> th = uniServer->m_dtmanager->m_threadPool->m_ess[essid]->make_thread([=]() {
                        uniServer->m_dtmanager->initstart("InitTrigger", step, varName, rde);
                        //time it
                        if (globalRank == 0)
                        {
                            uniServer->m_frawmanager->m_statefulConfig->timeit();
                        }
                    });
                    uniServer->m_dtmanager->m_threadPool->m_threadmutex.lock();
                    uniServer->m_dtmanager->m_threadPool->m_userThreadList.push_back(std::move(th));
                    spdlog::debug("start trigger for var {} step {} data id {}", varName, step, rde.m_rawDataID);
                    uniServer->m_dtmanager->m_threadPool->m_threadmutex.unlock();
                }
                req.respond(metadataWrapperList);
                return;
            }
            else
            {
            */
            MetaDataWrapper mdw(destAddr, step, varName, rde);
            metadataWrapperList.push_back(mdw);
        }
        //when all metadata is wrapped, then send back
        req.respond(metadataWrapperList);
        return;
    }
    catch (const std::exception &e)
    {
        spdlog::info("exception for data put and update metadata server");
        req.respond(metadataWrapperList);
        return;
    }
}

void registerWatcher(const tl::request &req, std::string watcherAddr, std::vector<std::string> &triggerName)
{
    //range the triggerName, if current trigger name is registered
    //put the watcherAddr into the triggerManger
    for (auto it = triggerName.begin(); it != triggerName.end(); ++it)
    {
        std::string triggerName = *it;
        spdlog::debug("register watcher for trigger {}", triggerName);
        //if trigger is on this node
        if (uniServer->m_dtmanager->m_dynamicTrigger.find(triggerName) != uniServer->m_dtmanager->m_dynamicTrigger.end())
        {
            //register the watcher if it is not empty
            uniServer->m_dtmanager->m_watcherSetMutex.lock();
            uniServer->m_dtmanager->m_registeredWatcherSet.insert(watcherAddr);

            auto endpoint = uniClient->m_clientEnginePtr->lookup(watcherAddr);
            uniClient->m_serverToEndpoints[watcherAddr] = endpoint;

            uniServer->m_dtmanager->m_watcherSetMutex.unlock();
        }
        else
        {
            spdlog::info("try to watch unregistered trigger {}", triggerName);
        }
    }
    req.respond(0);
}
//TODO, the trigger should also know the master among all knows that holds this trigger
//every trigger is a computing group essentially
//it needs to know the master addr of this group
void putTriggerInfo(const tl::request &req, std::string triggerName, DynamicTriggerInfo &dti)
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

void getmetaServerList(const tl::request &req, size_t &dims, std::array<int, 3> &indexlb, std::array<int, 3> &indexub)
{
    std::vector<std::string> metaServerAddr;
    try
    {
        BBXTOOL::BBX BBXQuery(dims, indexlb, indexub);
        std::vector<ResponsibleMetaServer> metaserverList = uniServer->m_dhtManager->getMetaServerID(BBXQuery);
        for (auto it = metaserverList.begin(); it != metaserverList.end(); it++)
        {
            int metaServerId = it->m_metaServerID;
            if (uniServer->m_dhtManager->metaServerIDToAddr.find(metaServerId) == uniServer->m_dhtManager->metaServerIDToAddr.end())
            {
                throw std::runtime_error("faild to get the coresponding server id in dhtManager for getmetaServerList");
            }
            metaServerAddr.push_back(uniServer->m_dhtManager->metaServerIDToAddr[metaServerId]);
        }

        req.respond(metaServerAddr);
    }
    catch (std::exception &e)
    {
        spdlog::info("exception for getmetaServerList: {}", std::string(e.what()));
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
        BBXTOOL::BBX BBXQuery(dims, indexlb, indexub);

        //check if all required partition is avalible, get raw endpoint first then execute check operation
        std::vector<RawDataEndpoint> rdeplist = uniServer->m_metaManager->getRawEndpoints(step, varName);

        //get overlap between the query and the boundry that this process respond to
        BBX queryBBX(dims, indexlb, indexub);

        BBX *overlap = getOverlapBBX(queryBBX, *(uniServer->m_dhtManager->metaServerIDToBBX[globalRank]));
        if (overlap == NULL)
        {
            req.respond(rawDataEndpointList);
            throw std::runtime_error("dht error, there should overlap between queried bbx and current metadata partition\n");
            return;
        }
        bool ifcover = uniServer->m_metaManager->ifCovered(rdeplist, *overlap);
        if (ifcover == false)
        {
            //return a none array if it is not covered
            if (overlap != NULL)
            {
                delete overlap;
            }
            req.respond(rawDataEndpointList);
            return;
        }
        rawDataEndpointList = uniServer->m_metaManager->getOverlapEndpoints(step, varName, BBXQuery);
        if (overlap != NULL)
        {
            delete overlap;
        }
        req.respond(rawDataEndpointList);
        return;
    }
    catch (std::exception &e)
    {
        spdlog::info("exception for getRawDataEndpointList: {}", std::string(e.what()));
        req.respond(rawDataEndpointList);
    }
    return;
}

void startTimer(const tl::request &req)
{

    uniServer->m_frawmanager->m_statefulConfig->initTimer();
    spdlog::info("start timer for rank {}", globalRank);
    return;
}

void endTimer(const tl::request &req)
{

    uniServer->m_frawmanager->m_statefulConfig->endTimer();
    return;
}

void executeRawFunc(const tl::request &req, std::string &blockID,
                    std::string &functionName,
                    std::vector<std::string> &funcParameters)
{

    //get block summary
    BlockSummary bs = uniServer->m_blockManager->getBlockSummary(blockID);

    //check data existance
    if (uniServer->m_blockManager->DataBlockMap.find(blockID) == uniServer->m_blockManager->DataBlockMap.end())
    {
        req.respond(std::string("NOTEXIST"));
        return;
    }

    DataBlockInterface *dbi = uniServer->m_blockManager->DataBlockMap[blockID];
    void *rawDataPtr = dbi->getrawMemPtr();

    //TODO init the adios if need to put

    std::string exeResults = uniServer->m_frawmanager->execute(uniServer->m_frawmanager, bs, rawDataPtr,
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

        spdlog::debug("map size on server {} is {}", uniServer->m_addrManager->nodeAddr, uniServer->m_blockManager->DataBlockMap.size());
        if (uniServer->m_blockManager->checkDataExistance(blockID) == false)
        {
            throw std::runtime_error("failed to get block id " + blockID + " on server with rank id " + std::to_string(globalRank));
        }
        BlockSummary bs = uniServer->m_blockManager->getBlockSummary(blockID);
        size_t elemSize = bs.m_elemSize;
        BBXTOOL::BBX bbx(dims, subregionlb, subregionub);
        size_t allocSize = elemSize * bbx.getElemNum();
        spdlog::debug("alloc size at server {} is {}", globalRank, allocSize);

        uniServer->m_blockManager->getBlockSubregion(blockID, dims, subregionlb, subregionub, dataContainer);

        std::vector<std::pair<void *, std::size_t>> segments(1);
        segments[0].first = (void *)(dataContainer);
        segments[0].second = allocSize;

        tl::bulk returnBulk = globalServerEnginePtr->expose(segments, tl::bulk_mode::read_only);

        tl::endpoint ep = req.get_endpoint();
        clientBulk.on(ep) << returnBulk;

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

void putEvent(const tl::request &req, std::string &triggerName, EventWrapper &event)
{
    uniServer->m_dtmanager->putEvent(triggerName, event);
    return;
}

void getEvent(const tl::request &req, std::string &triggerName)
{
    EventWrapper event = uniServer->m_dtmanager->getEvent(triggerName);
    req.respond(event);
    return;
}

void initDHT()
{
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
            Bound tempb(0, maxLen - 1);
            globalBBX->BoundList.push_back(tempb);
        }
        uniServer->m_dhtManager->initDHTBySFC(dataDims, gloablSettings.metaserverNum, globalBBX);
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

        uniServer->m_dhtManager->initDHTManually(gloablSettings.lenArray, gloablSettings.partitionLayout);
    }
    else
    {
        throw std::runtime_error("unsuported partition method " + gloablSettings.partitionMethod);
    }

    if (globalRank == 0)
    {
        //print metaServerIDToBBX
        for (auto it = uniServer->m_dhtManager->metaServerIDToBBX.begin(); it != uniServer->m_dhtManager->metaServerIDToBBX.end(); it++)
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

    //margo_instance_id client_mid;
    //client_mid = margo_init_opt("gni", MARGO_CLIENT_MODE, &hii, 0, 8);
    //tl::engine clientEnginge(client_mid);

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
    //globalServerEnginePtr->define("getServerNum", getServerNum);
    //globalServerEnginePtr->define("getaddrbyID", getaddrbyID);
    globalServerEnginePtr->define("getaddrbyrrb", getaddrbyrrb);
    globalServerEnginePtr->define("putrawdata", putrawdata);
    globalServerEnginePtr->define("eraserawdata", eraserawdata);
    globalServerEnginePtr->define("putmetadata", putmetadata);
    globalServerEnginePtr->define("getmetaServerList", getmetaServerList);
    globalServerEnginePtr->define("getRawDataEndpointList", getRawDataEndpointList);
    globalServerEnginePtr->define("getDataSubregion", getDataSubregion);
    globalServerEnginePtr->define("putTriggerInfo", putTriggerInfo);
    globalServerEnginePtr->define("executeRawFunc", executeRawFunc);
    globalServerEnginePtr->define("registerWatcher", registerWatcher);
    globalServerEnginePtr->define("putEvent", putEvent).disable_response();
    globalServerEnginePtr->define("getEvent", getEvent);
    globalServerEnginePtr->define("deleteMetaStep", deleteMetaStep);

    globalServerEnginePtr->define("startTimer", startTimer).disable_response();
    globalServerEnginePtr->define("endTimer", endTimer).disable_response();

    //for testing
    globalServerEnginePtr->define("hello", hello).disable_response();

    std::string rawAddr = globalServerEnginePtr->self();
    std::string selfAddr = IPTOOL::getClientAdddr(networkingType, rawAddr);
    char tempAddr[200];
    std::string masterAddr;

    if (globalRank == 0)
    {
        spdlog::debug("Start the unimos server with addr for master: {}", selfAddr);

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
    globalClientEnginePtr = &serverEnginge;
    uniClient = new UniClient(globalClientEnginePtr, globalRank);
    uniClient->initMaster(masterAddr);

    //get the total number of the server
    //init the client Cache
    uniClient->m_totalServerNum = gloablSettings.metaserverNum;

    //init stateful config (this may use MPI, do not mix it with mochi)
    statefulConfig *sconfig = new statefulConfig();
    //init all the important manager of the server
    uniServer = new UniServer();
    uniServer->initManager(globalProc, gloablSettings.metaserverNum, uniClient, true);

    //init the DHT
    //this is initilized based on the partition layout
    //if there are large amount of the nodes in cluster
    //but the meta server is 1, it is also ok
    initDHT();

    //gather IP to the rank0 and broadcaster the IP to all the services
    gatherIP(selfAddr);

    //write master server to file, server init ok
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

    //bradcaster the ip to all the worker nodes use the thallium api
    //this should be after the init of the server
    uniClient->initEndPoints(uniServer->m_addrManager->m_endPointsLists);

    if (uniServer->m_addrManager->ifMaster)
    {
        //there is gathered address information only for the master node
        //master will broad cast the list to all the servers
        //init the endpoint for all the slave node
        uniServer->m_addrManager->broadcastMetaServer(uniClient);
    }

    spdlog::info("init server ok, call margo wait for rank {}", globalRank);

    //call the ADIOS init explicitly
    uniServer->m_frawmanager->m_statefulConfig = sconfig;
    //test if the engine is normal

    //std::cout << "---debug adios io name in in server: " << uniServer->m_frawmanager->m_statefulConfig->m_io.Name() << std::endl;
    //std::cout << "--- debug engine type in server " << uniServer->m_frawmanager->m_statefulConfig->m_engine.Type() << std::endl;
    spdlog::info("init sconfig ok, call margo wait for rank {}", globalRank);

#ifdef USE_GNI
    //destructor will not be called if send mid to engine
    margo_wait_for_finalize(mid);
#endif

    //the destructor of the engine will be called when the variable is out of the scope
    return;
}

void signalHandler(int signal_num)
{
    std::cout << "The interrupt signal is (" << signal_num << "),"
              << " call finalize manually.\n";

    //finalize client and server
    delete uniClient;
    delete uniServer;
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

    try
    {
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
