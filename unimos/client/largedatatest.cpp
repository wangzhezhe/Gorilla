#include "unimosclient.hpp"
#include <mpi.h>
#include <cmath>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#define BILLION 1000000000L

bool AreSame(double a, double b)
{
    return std::fabs(a - b) < 0.000001;
}

int main(int argc, char **argv)
{

    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <networkingType> <dimsize>" << std::endl;
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

    std::string networkingType = argv[1];

    std::string serverAddr = loadMasterAddr();

    std::cout << "load server addr: " << serverAddr << std::endl;
    //TODO, put this into separate class
    //for client, just input tcp://...
    //tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);
    //tl::engine myEngine("na+sm", THALLIUM_CLIENT_MODE);
    //tl::engine myEngine("verbs", THALLIUM_CLIENT_MODE);

    tl::engine clientEngine(networkingType, THALLIUM_CLIENT_MODE);

    //generate data

    size_t elemNum = size_t(std::stoi(argv[2]));
    std::cout << "grid size for testing is " << elemNum << std::endl;
    std::vector<double> inputData;
    for (int i = 0; i < elemNum; i++)
    {
        for (int j = 0; j < elemNum; j++)
        {
            for (int k = 0; k < elemNum; k++)
            {
                inputData.push_back(rank + i * 0.1 + j + 0.01 + k * 0.001);
            }
        }
    }
    std::string varName = "testVar";
    std::array<size_t, 3> shape = {elemNum, elemNum, elemNum};
    DataMeta dataMeta = DataMeta(varName, 3, typeid(double).name(), sizeof(double), shape);
    size_t blockID = (size_t)rank;

    struct timespec start, end;
    double diff;
    clock_gettime(CLOCK_REALTIME, &start); /* mark start time */
    for (int ts = 0; ts < 10; ts++)
    {
        std::string slaveAddr = dspaces_client_getaddr(clientEngine, serverAddr, varName, ts);
        std::cout << "the slave server addr for ds put is " << slaveAddr << std::endl;
        dataMeta.m_steps = ts;
        dspaces_client_put(clientEngine, slaveAddr, dataMeta, blockID, inputData);
    }
    clock_gettime(CLOCK_REALTIME, &end); /* mark end time */
    diff = (end.tv_sec - start.tv_sec) * 1.0 + (end.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
    std::cout << " time is " << diff << " seconds" << std::endl;

    MPI_Finalize();
}