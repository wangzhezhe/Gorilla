#include <mpi.h>
#include <unistd.h>
#include <stdio.h>
#include "../common/datameta.h"

#include <vector>
#include <iostream>
#include <map>
#include <string>
#include <array>
#include <cstring>
#include <thallium.hpp>

namespace tl = thallium;

void dspaces_client_get(tl::engine &myEngine, std::string serverAddr, int ts, int rank)
{
    //TODO put them at separate class
    tl::remote_procedure dsget = myEngine.define("dsget");
    //tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
    tl::endpoint globalServerEndpoint = myEngine.lookup(serverAddr);

    //write to server
    std::string varName = "testVar";
    size_t blockID = rank;

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

void dspaces_client_put(tl::engine &myEngine, std::string serverAddr, int ts, int rank)
{
    //TODO put them at separate class
    tl::remote_procedure dsput = myEngine.define("dsput");
    //tl::remote_procedure putMetaData = myEngine.define("putMetaData").disable_response();
    tl::endpoint globalServerEndpoint = myEngine.lookup(serverAddr);

    //write to server
    std::string varName = "testVar";
    size_t blockID = rank;
    size_t dimention = 1;
    std::array<size_t, 3> lowbound = {0, 0, 0};
    std::array<size_t, 3> shape = {10, 0, 0};

    //create the object
    DataMeta datameta = DataMeta(varName, ts, dimention, typeid(double).name(), sizeof(double), lowbound, shape);

    //send the data block to the server
    //putMetaData.on(globalServerEndpoint)(datameta);

    //generating the data for testing
    std::vector<double> dummyData;
    for (int i = 0; i < 10; i++)
    {
        dummyData.push_back(i * 0.01);
    }

    std::vector<std::pair<void *, std::size_t>> segments(1);
    segments[0].first = (void *)(dummyData.data());
    segments[0].second = datameta.getDataMallocSize();

    tl::bulk myBulk = myEngine.expose(segments, tl::bulk_mode::read_only);

    int status = dsput.on(globalServerEndpoint)(datameta, blockID, myBulk);

    std::cout << "put status is " << status << std::endl;

    return;
}

int main(int argc, char **argv)
{

    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <address>" << std::endl;
        exit(0);
    }

    //init the server before the client starting

    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Print off a hello world message
    printf("rank %d start\n", rank);

    //tl::endpoint server = dspaces_client_init(argv[1]);

    std::string serverAddr = argv[1];

    //TODO, put this into separate class
    //for client, just input tcp://...
    //tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);
    //tl::engine myEngine("na+sm", THALLIUM_CLIENT_MODE);
    tl::engine myEngine("verbs", THALLIUM_CLIENT_MODE);
    
    //generate data

    int dsize = 10;
    std::vector<double> inputData;
    for (int i = 0; i < dsize; i++)
    {
        inputData.push_back(rank + i * 0.1);
    }

    //only use one time step for the proof of concepts
    int ts = 0;

    dspaces_client_put(myEngine, serverAddr, ts, rank);

    std::cout << "ok to put data for rank " << rank << std::endl;

    std::cout << "start to get data: " << std::endl;

    sleep(0.5);

    dspaces_client_get(myEngine, serverAddr, ts, rank);

    std::cout << "ok to get data for rank " << rank << std::endl;

    //read from the server

    //call the rpc server to get the data from the server

    // Finalize the MPI environment.
    MPI_Finalize();
}