#ifndef __UNIMOSCLIENT_H__
#define __UNIMOSCLIENT_H__


#include <mpi.h>
#include <unistd.h>
#include <stdio.h>
#include "../common/datameta.h"

#include <vector>
#include <iostream>
#include <map>
#include <string>
#include <array>
#include <fstream>
#include <cstring>
#include <thallium.hpp>

namespace tl = thallium;

namespace UNICLIENT{
    
extern std::string masterConfig;

/*

    tl::remote_procedure sum = myEngine.define("sum");
    tl::endpoint server = myEngine.lookup(argv[1]);
    //attention, the return value here shoule be same with the type defined at the server end
    int ret = sum.on(server)(42,63);
*/



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

}



#endif