#ifndef __METACLIENT_H__
#define __METACLIENT_H__

#include <vector>
#include <iostream>
#include <string>
#include <array>
#include <fstream>
#include <string>
#include "common.h"
#include <thallium.hpp>

namespace tl = thallium;

//write server addr into this folder
const std::string metaserverDir = "Metaserver_conf";

struct MetaClient
{
    MetaClient(){};
    //for server enginge, the client ptr is the pointer to the server engine
    //for the client code, the client engine is the pointer to the engine with the client mode
    MetaClient(tl::engine *clientEnginePtr)
    {
        m_clientEnginePtr = clientEnginePtr;
        m_masterAddr = loadMasterAddr(metaserverDir);
    };
    std::string m_masterAddr;
    tl::engine *m_clientEnginePtr = NULL;
    ~MetaClient(){};

    void Recordtime(std::string recordkey);

    void Recordtimestart(std::string recordkey);

    void Recordtimetick(std::string recordkey);

    void hello();

    std::string loadMasterAddr(std::string masterConfigFile);


};

#endif