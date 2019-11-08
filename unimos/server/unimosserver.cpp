
//the mini data space server that provide the get and put api

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
//import this to parse the template
#include <spdlog/spdlog.h>
#include "mpi.h"
#include "memcache.h"
#include "endpointManagement.h"
#include "../utils/stringtool.h"
#include "../client/unimosclient.h"
#include "filterManager.h"

namespace tl = thallium;

//The pointer to the enginge should be set as global element
//this shoule be initilized after the initilization of the argobot

tl::engine *globalClientEnginePointer = nullptr;
//TBD tl::engine *globalInnerEnginePointer = nullptr;

//init memory cache
MemCache *mcache = new MemCache(4);
//init filter manager
FilterManager *fmanager = new FilterManager();
//load the filter manager

//the manager for all the server endpoints
endPointsManager *epManager = new endPointsManager();

//name of configure file, the write one line, the line is the rank0 addr
std::string masterConfigFile = "./unimos_server.conf";

std::string MasterIP = "";
//rank & proc number for current MPI process
int globalRank = 0;
int globalProc = 0;

/*
the rpc call for testing using
void hello(const tl::request &req, const std::string &name)
{
    spdlog::debug("Hello {}", name);
}

void putDoubleVector(const tl::request &req, const std::vector<double> &inputVector)
{
    spdlog::debug("check vector at the server:");
    int size = inputVector.size();
    for (int i = 0; i < size; i++)
    {
        spdlog::debug("index {} value {} inputVector[i]");
    }
    return;
}

void putMetaData(const tl::request &req, DataMeta &datameta)
{
    spdlog::debug("check putMetaData at the server:");
    datameta.printMeta();
    return;
}
*/

//get the shape of the data and the client could allocate the memory
//the sammary is only need to be called once for each time step
//when the data is distributed among multiple blocks, it is better to start query arbitrary region until all data block is ready
void dsgetBlockMeta(const tl::request &req, const std::string &varName, const int &ts, size_t &blockID)
{
    BlockMeta blockmeta = mcache->getBlockMeta(varName, ts, blockID);
    std::cout << "check get results at server end ts " << ts << std::endl;
    blockmeta.printMeta();

    req.respond(blockmeta);
}

void getaddr(const tl::request &req, const std::string &varName, const int &ts)
{
    if (epManager->ifAllRegister(globalProc))
    {
        std::string serverAddr = epManager->getByVarTs(varName, ts);
        //spdlog::debug("varname {} and ts {} getaddr  {}", varName, ts, serverAddr);
        req.respond(serverAddr);
    }
    else
    {
        //other wise, return the ip
        req.respond(std::string("NOREGISTER"));
    }
}

//return the error code
//be careful with the parameters, they should match with the type used at the client end
//otherwise, there are some template issues

void dsput(const tl::request &req, DataMeta &datameta, size_t &blockID, tl::bulk &dataBulk)
{
    spdlog::debug("execute dataspace put:");
    datameta.printMeta();
    spdlog::debug("blockID is {} ", blockID);

    //get the bulk value
    tl::endpoint ep = req.get_endpoint();

    //assign the memory
    size_t mallocSize = datameta.extractBlockMeta().getBlockMallocSize();

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

        //create the container for the data
        tl::bulk local = globalClientEnginePointer->expose(segments, tl::bulk_mode::write_only);
        dataBulk.on(ep) >> local;

        spdlog::debug("Server received bulk, check the contents: ");

        //check the bulk
        /*
        double *rawdata = (double *)localContainer;
        for (int i = 0; i < 10; i++)
        {
            std::cout << "index " << i << " value " << *rawdata << std::endl;
            rawdata++;
        }
        */

        //TODO get the pointer wisely to decrease the data copy
        //generate the empty container firstly, then get the data pointer
        int status = mcache->putIntoCache(datameta, blockID, localContainer);

        //TODO update the return value
        req.respond(status);

        spdlog::debug("ok to put the data");
    }
    catch (...)
    {
        spdlog::debug("exception for data put");
        req.respond(-1);
    }

    return;
}

//get the whole object
void dsget(const tl::request &req, std::string &varName, int &ts, size_t &blockID, tl::bulk &clientBulk)
{
    //get variable type from the data
    void *data = nullptr;

    spdlog::debug("dsget by varName: {} ts {} blockID {}", varName, ts, blockID);

    try
    {
        BlockMeta blockMeta = mcache->getFromCache(varName, ts, blockID, data);
        //get the data value
        if (blockMeta.getValidDimention() == 0)
        {
            spdlog::debug("failed to get the data at the server end");
            //return empty bulk info if it is failed to get data
            //how to adjust it is empty at the client end?
            req.respond(-1);
            return;
        }
        std::vector<std::pair<void *, std::size_t>> segments(1);
        segments[0].first = (void *)(data);
        segments[0].second = blockMeta.getBlockMallocSize();

        tl::bulk returnBulk = globalClientEnginePointer->expose(segments, tl::bulk_mode::read_only);

        tl::endpoint ep = req.get_endpoint();
        clientBulk.on(ep) << returnBulk;

        req.respond(0);
    }
    catch (...)
    {
        spdlog::debug("exception for get");
        req.respond(-1);
    }

    /*
    double *rawdata = (double *)data;
    std::cout << "check the get data at the server end" << std::endl;

    for (int i = 0; i < 10; i++)
    {
        std::cout << "index " << i << " value " << *rawdata << std::endl;
        rawdata++;
    }
    */
}

//get the object in specific region
void dsgetregion(const tl::request &req, std::string &varName, int &ts, std::array<size_t, 3> baseoffset,
                 std::array<size_t, 3> shape, tl::bulk &clientBulk)
{

    //todo
}

//todo, this api should be divided into two parts
//the first is for master, all other clients will subscribe to the master
//the second is between the master and other work node, the master will transfer the profile to all other node
void subscribeProfile(const tl::request &req, std::string &varName, FilterProfile &fp)
{
    //subscribe profile to the server
    //call the subscribe of the filter manager
    //tl::endpoint ep = req.get_endpoint();
    //std::string subscriberAddr = std::string(ep);
    //spdlog::debug("subscribe the profile from {}", subscriberAddr);
    //update the subscriber position of the profile
    if (globalRank == 0)
    {
        //subscribe to all the worker
        //get serverList
        dssubscribe_broadcast(*globalClientEnginePointer, epManager->m_endPointsLists, varName, fp);
        spdlog::debug("master send subscribe to worker");
        req.respond(0);
    }
    else
    {

        int status = fmanager->profileSubscribe(varName, fp);
        spdlog::debug("worker node {} subscribe", globalRank);
        req.respond(status);
    }
}

void gatherIP(std::string engineName, std::string endpoint)
{

    std::string ip = IPTOOL::getClientAdddr(engineName, endpoint);

    if (globalRank == 0)
    {
        epManager->ifMaster = true;
    }

    epManager->nodeAddr = ip;

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

        std::cout << "check the ip list:" << std::endl;
        for (int i = 0; i < ipList.size(); i++)
        {
            std::cout << ipList[i] << std::endl;
            //only store the worker addr
            if (epManager->nodeAddr.compare(ipList[i]) != 0)
            {
                spdlog::info("push back addr {}", ipList[i]);
                epManager->m_endPointsLists.push_back(ipList[i]);
            }
        }

        free(rcvString);
    }
}

void runRerver(std::string networkingType)
{

    tl::engine dataMemEnginge(networkingType, THALLIUM_SERVER_MODE);

    //another option is to use the unique pointer to point to the entity of the engine
    //use the unique pointer to make sure the destructor is called when the variable is out of the scope
    globalClientEnginePointer = &dataMemEnginge;

    //globalClientEnginePointer = new tl::engine(networkingType, THALLIUM_SERVER_MODE);
    //std::string addr = globalClientEnginePointer->self();

    //globalClientEnginePointer->define("hello", hello).disable_response();
    //globalClientEnginePointer->define("putDoubleVector", putDoubleVector).disable_response();
    //globalClientEnginePointer->define("putMetaData", putMetaData).disable_response();
    globalClientEnginePointer->define("dsput", dsput);
    globalClientEnginePointer->define("dsget", dsget);
    globalClientEnginePointer->define("dsgetBlockMeta", dsgetBlockMeta);
    globalClientEnginePointer->define("subscribeProfile", subscribeProfile);

    std::string addr = dataMemEnginge.self();

    if (globalRank == 0)
    {
        spdlog::info("Start the unimos server with addr for master: {}", addr);

        globalClientEnginePointer->define("getaddr", getaddr);
        std::string masterAddr = IPTOOL::getClientAdddr(networkingType, addr);
        std::ofstream confFile;
        confFile.open(masterConfigFile);
        confFile << masterAddr << "\n";
        confFile.close();
    }

    //write the addr out here

    gatherIP(networkingType, addr);

    //the destructor of the engine will be called when the variable is out of the scope

    return;
}

void signalHandler(int signal_num)
{
    std::cout << "The interrupt signal is (" << signal_num << "),"
              << " call finalize manually.\n";
    globalClientEnginePointer->finalize();
    exit(signal_num);
}

int main(int argc, char **argv)
{

    MPI_Init(NULL, NULL);

    MPI_Comm_rank(MPI_COMM_WORLD, &globalRank);
    MPI_Comm_size(MPI_COMM_WORLD, &globalProc);

    //auto file_logger = spdlog::basic_logger_mt("unimos_server_log", "unimos_server_log.txt");
    //spdlog::set_default_logger(file_logger);

    //default value
    int logLevel = 0;
    bool addFilter = false;
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <networkingType> filter <loglevel>" << std::endl;
        exit(0);
    }
    std::string networkingType = argv[1];

    if (argc > 2)
    {
        std::string filterKey = argv[2];
        if (filterKey.compare(std::string("filter")) == 0)
        {
            addFilter = true;
        }
    }

    if (argc == 4)
    {
        logLevel = atoi(argv[3]);
    }

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

    epManager->m_serverNum = globalProc;

    try
    {
        //load the filter manager

        if (addFilter == true)
        {
            mcache->loadFilterManager(fmanager);
            spdlog::info("load the filter for rank {}", globalRank);
        }

        //this is used for the notification
        tl::engine myclientEngine(networkingType, THALLIUM_CLIENT_MODE);
        //fmanager->m_Engine=&dataMemEnginge;
        fmanager->m_Engine = &myclientEngine;

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
