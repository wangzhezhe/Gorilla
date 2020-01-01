#ifndef __UNIMOSCLIENT_H__
#define __UNIMOSCLIENT_H__


#include "../commondata/metadata.h"

#include <vector>
#include <iostream>
#include <string>
#include <array>
#include <fstream>
#include <string>
#include <thallium.hpp>
#include <spdlog/spdlog.h>

namespace tl = thallium;

struct UniClient{
    UniClient(){};
    //for server enginge, the client ptr is the pointer to the server engine
    //for the client code, the client engine is the pointer to the engine with the client mode
    UniClient(tl::engine * clientEnginePtr, std::string masterConfigFile){
        m_clientEnginePtr = clientEnginePtr;
        m_masterAddr = loadMasterAddr(masterConfigFile);
    };
    std::string m_masterAddr;
    tl::engine* m_clientEnginePtr=NULL;
    ~UniClient(){};

    int updateDHT(std::string serverAddr, std::vector<MetaAddrWrapper> metaAddrWrapperList);
    

    //get the address of the server by round roubin pattern
    std::string getServerAddrByRRbin();

    //get the raw data to the specific addr 
    int putrawdata(size_t step, std::string varName, BlockSummary &dataSummary,  void* dataContainer);


    //get the address of the master server according
    std::string loadMasterAddr(std::string masterConfigFile);
};


/*

    tl::remote_procedure sum = myEngine.define("sum");
    tl::endpoint server = myEngine.lookup(argv[1]);
    //attention, the return value here shoule be same with the type defined at the server end
    int ret = sum.on(server)(42,63);
*/



/*
BlockMeta dspaces_client_getblockMeta(tl::engine &myEngine, std::string serverAddr, std::string varName, int ts, size_t blockID);

std::string dspaces_client_getaddr(tl::engine &myEngine, std::string serverAddr, std::string varName, int ts, size_t blockid);

void dspaces_client_get(tl::engine &myEngine,
                        std::string serverAddr,
                        std::string varName,
                        int ts,
                        size_t blockID,
                        std::vector<double> &dataContainer);

//todo add template here
void dspaces_client_put(tl::engine &myEngine,
                        std::string serverAddr,
                        DataMeta &datameta,
                        size_t &blockID,
                        std::vector<double> &putVector);

int dssubscribe(tl::engine &myEngine, std::string serverAddr, std::string varName, FilterProfile& fp);

int dssubscribe_broadcast(tl::engine &myEngine, std::vector<std::string> serverList, std::string varName, FilterProfile &fp);

int dsnotify_subscriber(tl::engine &myEngine, std::string serverAddr, size_t& step, size_t &blockID);


std::string loadMasterAddr();
*/





#endif