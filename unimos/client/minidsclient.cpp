#include "unimosclient.hpp"
#include <mpi.h>

int main(int argc, char **argv)
{

    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <networkingType>" << std::endl;
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

    std::cout << "load server addr: " << serverAddr <<std::endl;
    //TODO, put this into separate class
    //for client, just input tcp://...
    //tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);
    //tl::engine myEngine("na+sm", THALLIUM_CLIENT_MODE);
    //tl::engine myEngine("verbs", THALLIUM_CLIENT_MODE);

    tl::engine clientEngine(networkingType, THALLIUM_CLIENT_MODE);

    //generate data

    int dsize = 10;
    std::vector<double> inputData;
    for (int i = 0; i < dsize; i++)
    {
        inputData.push_back(rank + i * 0.1);
    }

    //only use one time step for the proof of concepts
    int ts = 0;
    std::string varName = "testVar";
    std::string addr = dspaces_client_getaddr(clientEngine, serverAddr, varName, ts);

    std::cout << "the server addr is " << addr << std::endl;

    /*
    dspaces_client_put(clientEngine, serverAddr, ts, rank);

    std::cout << "ok to put data for rank " << rank << std::endl;

    std::cout << "start to get data: " << std::endl;

    //sleep(0.5);

    dspaces_client_get(clientEngine, serverAddr, ts, rank);

    std::cout << "ok to get data for rank " << rank << std::endl;
*/

    // Finalize the MPI environment.
    MPI_Finalize();
}