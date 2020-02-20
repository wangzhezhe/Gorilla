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
#include <thallium.hpp>

namespace tl = thallium;

struct UniClient
{
    UniClient(){};
    //for server enginge, the client ptr is the pointer to the server engine
    //for the client code, the client engine is the pointer to the engine with the client mode
    UniClient(tl::engine *clientEnginePtr, std::string masterConfigFile)
    {
        m_clientEnginePtr = clientEnginePtr;
        m_masterAddr = loadMasterAddr(masterConfigFile);
        std::cout << "load master Addr: " << m_masterAddr << std::endl;
    };
    //set the m_masterAddr separately
    UniClient(tl::engine *clientEnginePtr)
    {
        m_clientEnginePtr = clientEnginePtr;
    };
    
    std::string m_masterAddr;
    tl::engine *m_clientEnginePtr = NULL;
    ~UniClient(){};

    int updateDHT(std::string serverAddr, std::vector<MetaAddrWrapper> metaAddrWrapperList);

    //get the address of the server by round roubin pattern
    std::string getServerAddrByRRbin();

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
};

#endif