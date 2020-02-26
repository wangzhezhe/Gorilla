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

//this cache is used to store the information from the server
//this will be used to decrease the interaction between client and server
struct UniClientCache
{
    UniClientCache(){};

    //the addr of the server addr
    //assume the number of the meta server same with the total server
    //TODO add flag to distiguish the meta number and the server number
    //if the number of metaserver is different compared with the data server number
    tl::mutex m_addrMapMutex;
    std::map<int, std::string> m_serverIDToAddr;

    ~UniClientCache(){};

    bool ifAddrIDExist(int id)
    {
        if (this->m_serverIDToAddr.find(id) == this->m_serverIDToAddr.end())
        {
            //this id not exist
            return false;
        }
        return true;
    };

    void addAddr(int addrId, std::string serverAddr)
    {
        this->m_addrMapMutex.lock();
        this->m_serverIDToAddr[addrId] = serverAddr;
        this->m_addrMapMutex.unlock();
        return;
    };
};

struct UniClient
{
    UniClient(){};
    //for server enginge, the client ptr is the pointer to the server engine
    //for the client code, the client engine is the pointer to the engine with the client mode
    UniClient(tl::engine *clientEnginePtr, std::string masterConfigFile, int rrbStartPosition)
    {
        m_clientEnginePtr = clientEnginePtr;
        m_masterAddr = loadMasterAddr(masterConfigFile);
        m_position = rrbStartPosition;
        std::cout << "load master Addr: " << m_masterAddr << std::endl;
    };
    //set the m_masterAddr separately
    UniClient(tl::engine *clientEnginePtr, int rrbStartPosition)
    {
        m_clientEnginePtr = clientEnginePtr;
        m_position = rrbStartPosition;
    };



    std::string m_masterAddr = "";
    std::string m_associatedDataServer = "";
    tl::endpoint m_serverEndpoint;
    tl::engine *m_clientEnginePtr = NULL;

    
    size_t m_bulkSize=0;
    void* m_dataContainer = nullptr;
    std::vector<std::pair<void *, std::size_t>> m_segments;
    tl::bulk m_dataBulk;

    
    int m_position = 0;
    int m_totalServerNum = 0;

    ~UniClient(){
        if(m_dataContainer!=nullptr){
            free(m_dataContainer);
        }
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

    void registerTrigger(
        size_t dims,
        std::array<int, 3> indexlb,
        std::array<int, 3> indexub,
        std::string triggerName,
        DynamicTriggerInfo &dti);

    int getServerNum();

    int getAllServerAddr();

    void initPutRawData(size_t dataMallocSize);

    UniClientCache *m_uniCache = nullptr;
};

#endif