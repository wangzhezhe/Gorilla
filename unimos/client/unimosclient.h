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

std::string masterConfig = "./unimos_server.conf";

/*

    tl::remote_procedure sum = myEngine.define("sum");
    tl::endpoint server = myEngine.lookup(argv[1]);
    //attention, the return value here shoule be same with the type defined at the server end
    int ret = sum.on(server)(42,63);
*/

std::string dspaces_client_getaddr(tl::engine &myEngine, std::string serverAddr, std::string varName, int ts)
{
    //TODO put them at separate class
    tl::remote_procedure dsgetaddr = myEngine.define("getaddr");
    //tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
    tl::endpoint globalServerEndpoint = myEngine.lookup(serverAddr);
    std::string returnAddr = dsgetaddr.on(globalServerEndpoint)(varName, ts);
    return returnAddr;
}

void dspaces_client_get(tl::engine &myEngine, 
std::string serverAddr, 
std::string varName, 
int ts, 
size_t blockID,
void* getData)
{

    tl::remote_procedure dsget = myEngine.define("dsget");
    //tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
    tl::endpoint globalServerEndpoint = myEngine.lookup(serverAddr);


    //get the size of the data (metadata is stored at the interface)


    //malloc the corresponding size


    //get execute the real get

    std::vector<double> dummyData(10);
    



    std::vector<std::pair<void *, std::size_t>> segments(1);
    segments[0].first = (void *)(dummyData.data());
    segments[0].second = dummyData.size() * sizeof(double);

    tl::bulk clientBulk = myEngine.expose(segments, tl::bulk_mode::write_only);

    int status = dsget.on(globalServerEndpoint)(varName, ts, blockID, clientBulk);

    std::cout << "status of the dsget is " << status << std::endl;

    std::cout << "check data at the client end:" << std::endl;

    for (int i = 0; i < 10; i++)
    {
        std::cout << "index " << i << " value " << dummyData[i] << std::endl;
    }

    return;
}

//todo add template here
void dspaces_client_put(tl::engine &myEngine, 
DataMeta& datameta,
std::vector<double>& putVector)
{
    //TODO put them at separate class
    tl::remote_procedure dsput = myEngine.define("dsput");
    //tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
    tl::endpoint globalServerEndpoint = myEngine.lookup(serverAddr);

    std::vector<std::pair<void *, std::size_t>> segments(1);
    segments[0].first = (void *)(putVector.data());
    segments[0].second = datameta.getDataMallocSize();

    tl::bulk myBulk = myEngine.expose(segments, tl::bulk_mode::read_only);

    int status = dsput.on(globalServerEndpoint)(datameta, blockID, myBulk);

    std::cout << "put status is " << status << std::endl;

    return;
}

std::string loadMasterAddr()
{

    std::ifstream infile(masterConfig);
    std::string content;
    std::getline(infile, content);

    return content;
}