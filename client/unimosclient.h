#ifndef __UNIMOSCLIENT_H__
#define __UNIMOSCLIENT_H__

#include "../utils/matrixtool.h"
#include "../commondata/metadata.h"

#include <vector>
#include <iostream>
#include <string>
#include <array>
#include <string>
#include <fstream>
#include <map>
#include <thallium.hpp>

namespace tl = thallium;

struct UniClient
{
    UniClient(){};

    void initMaster(std::string masterAddr)
    {
        this->m_masterEndpoint = this->m_clientEnginePtr->lookup(masterAddr);
        return;
    }

    //this is called if the addr of all the server is already known
    //otherwise, it will initilized by getAllServerAddr
    void initEndPoints(std::vector<std::string> &serverList)
    {
        for (auto it = serverList.begin(); it != serverList.end(); it++)
        {
            auto endpoint = this->m_clientEnginePtr->lookup(*it);
            m_serverToEndpoints[*it] = endpoint;
        }
        return;
    }

    //for server enginge, the client ptr is the pointer to the server engine
    //for the client code, the client engine is the pointer to the engine with the client mode
    UniClient(tl::engine *clientEnginePtr, std::string masterConfigFile, int rrbStartPosition)
    {
        m_clientEnginePtr = clientEnginePtr;
        m_masterAddr = loadMasterAddr(masterConfigFile);
        m_position = rrbStartPosition;
        //std::cout << "load master Addr: " << m_masterAddr << std::endl;
        this->initMaster(m_masterAddr);
    };

    //set the m_masterAddr separately when use this
    UniClient(tl::engine *clientEnginePtr, int rrbStartPosition)
    {
        m_clientEnginePtr = clientEnginePtr;
        m_position = rrbStartPosition;
    };

    //from the id to the string (this may used by the writer to fix the specific server)
    std::map<int, std::string> m_serverIDToAddr;

    //from the string to the endpoints, this is used by all clients
    //caculate this at the beginning when init the client to avoid the potential mercury mistakes
    std::map<std::string, tl::endpoint> m_serverToEndpoints;

    tl::endpoint lookup(std::string &address);

    std::string m_masterAddr;
    tl::endpoint m_masterEndpoint;
    std::string m_associatedDataServer = "";
    tl::engine *m_clientEnginePtr = NULL;

    size_t m_bulkSize = 0;
    void *m_dataContainer = nullptr;
    std::vector<std::pair<void *, std::size_t>> m_segments;
    tl::bulk m_dataBulk;

    int m_position = 0;
    int m_totalServerNum = 0;

    ~UniClient()
    {
        if (m_dataContainer != nullptr)
        {
            free(m_dataContainer);
        }
        std::cout << "destroy UniClient"<<std::endl;
    };

    int getIDByRandom()
    {
        if (m_totalServerNum == 0)
        {
            throw std::runtime_error("total serverNum should not be zero");
        }

        srand(time(NULL));

        int serverId = rand() % m_totalServerNum;

        return serverId;
    };

    int updateDHT(std::string serverAddr, std::vector<MetaAddrWrapper> metaAddrWrapperList);

    //get the address of the server by round roubin pattern
    std::string getServerAddr();

    //put the raw data
    int putrawdata(size_t step, std::string varName, BlockSummary &dataSummary, void *dataContainer);

    //put the meta data to specific server addr
    int putmetadata(std::string serverAddr, size_t step, std::string varName, RawDataEndpoint &rde);

    //get the address of the master server according
    std::string loadMasterAddr(std::string masterConfigFile);

    //get the coresponding list from any server
    //this metnod is unnecessary if the server end execute the data get operation
    std::vector<std::string> getmetaServerList(size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub);

    std::vector<RawDataEndpoint> getrawDataEndpointList(std::string serverAddr,
                                                        size_t step,
                                                        std::string varName,
                                                        size_t dims,
                                                        std::array<int, 3> indexlb,
                                                        std::array<int, 3> indexub);

    int getSubregionData(std::string serverAddr, std::string blockID, size_t dataSize,
                         size_t dims,
                         std::array<int, 3> indexlb,
                         std::array<int, 3> indexub,
                         void *dataContainer);

    MATRIXTOOL::MatrixView getArbitraryData(
        size_t step,
        std::string varName,
        size_t elemSize,
        size_t dims,
        std::array<int, 3> indexlb,
        std::array<int, 3> indexub);

    int putTriggerInfo(
        std::string serverAddr,
        std::string triggerName,
        DynamicTriggerInfo &dti);

    std::string registerTrigger(
        size_t dims,
        std::array<int, 3> indexlb,
        std::array<int, 3> indexub,
        std::string triggerName,
        DynamicTriggerInfo &dti);

    int getServerNum();

    int getAllServerAddr();

    void startTimer();

    void endTimer();

    void initPutRawData(size_t dataMallocSize);

    std::string executeRawFunc(
        std::string serverAddr,
        std::string blockID,
        std::string functionName,
        std::vector<std::string> &funcParameters);

    void registerWatcher(std::vector<std::string> triggerNameList);

    void notifyBack(std::string watcherAddr, BlockSummary& bs);

    void putEventIntoQueue(std::string groupMasterAddr, std::string triggerName, EventWrapper &event);

    EventWrapper getEventFromQueue(std::string groupMasterAddr, std::string triggerName);

    void eraseRawData(std::string serverAddr, std::string blockID);
};

#endif