
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
//import this to parse the template
#include <spdlog/spdlog.h>
#include "mpi.h"
#include "../utils/stringtool.h"
#include "addrManagement.h"
#include "settings.h"

namespace tl = thallium;

//global variables
tl::abt scope;

//The pointer to the enginge should be set as global element
//this shoule be initilized after the initilization of the argobot
tl::engine *globalEnginePointer = nullptr;

//name of configure file, the write one line, the line is the rank0 addr
//std::string masterConfigFile = "./unimos_server.conf";
Settings gloablSettings;

std::string MasterIP = "";
//rank & proc number for current MPI process
int globalRank = 0;
int globalProc = 0;

//the manager for all the server endpoints
endPointsManager *epManager = new endPointsManager();

//the global manager DHT
DHTManager *dhtManager = nullptr;

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
            //if (epManager->nodeAddr.compare(ipList[i]) != 0)
            //{
            spdlog::info("rank {} add raw data server {}", globalRank, ipList[i]);
            epManager->m_endPointsLists.push_back(ipList[i]);

            //}
        }

        free(rcvString);
    }

    //TODO bradcaster the ip to all the worker nodes use the thallium api
}

//currently, we use the global dht
//this can also be binded with specific variables
//when the global mesh changes, the dht changes
void updateDHT(const tl::request &req, std::vector<MetaAddtWrapper> &datawrapperList)
{

    //put the metadata into current epManager
    epManager->metaAddtWrapperList = datawrapperList;
    epManager->m_metaServerNum = datawrapperList.size();

    for (int i = 0; i < epManager->m_metaServerNum; i++)
    {
        spdlog::info("rank {} add meta addr")
    }

    //get mesh dim from the setting

    //get global bbx from the setting

    //dhtManager = initDHT(int ndim, int metaServerNum, BBX *globalBBX)

    //put the data wrapper list into the DHT

    return;
}

void runRerver(std::string networkingType)
{

    tl::engine dataMemEnginge(networkingType, THALLIUM_SERVER_MODE);
    globalEnginePointer = &dataMemEnginge;

    globalEnginePointer->define("updateDHT", updateDHT).disable_response();

    //globalClientEnginePointer->define("hello", hello).disable_response();
    //globalClientEnginePointer->define("putDoubleVector", putDoubleVector).disable_response();
    //globalClientEnginePointer->define("putMetaData", putMetaData).disable_response();
    /*
    globalClientEnginePointer->define("dsput", dsput);
    globalClientEnginePointer->define("dsget", dsget);
    globalClientEnginePointer->define("dsgetBlockMeta", dsgetBlockMeta);
    globalClientEnginePointer->define("subscribeProfile", subscribeProfile);
    */

    std::string addr = dataMemEnginge.self();

    if (globalRank == 0)
    {
        spdlog::info("Start the unimos server with addr for master: {}", addr);

        //globalClientEnginePointer->define("getaddr", getaddr);
        std::string masterAddr = IPTOOL::getClientAdddr(networkingType, addr);
        std::ofstream confFile;
        confFile.open(gloablSettings->masterAddr);
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

    bool addFilter = false;
    if (argc != 2)
    {

        gloablSettings = Settings::from_json(argv[1]);
        std::cerr << "Usage: " << argv[0] << "unimos_server <path of setting.json>" << std::endl;
        exit(0);
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

    //TODO, use extra configuration here
    epManager->m_serverNum = globalProc;
    epManager->m_metaServerNum = globalProc;

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
