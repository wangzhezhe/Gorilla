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

    std::cout << "load server addr: " << serverAddr << std::endl;
    //TODO, put this into separate class
    //for client, just input tcp://...
    //tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);
    //tl::engine myEngine("na+sm", THALLIUM_CLIENT_MODE);
    //tl::engine myEngine("verbs", THALLIUM_CLIENT_MODE);

    tl::engine clientEngine(networkingType, THALLIUM_CLIENT_MODE);

    //generate data

    size_t elemNum = 10;
    std::vector<double> inputData;
    for (int i = 0; i < elemNum; i++)
    {
        inputData.push_back(rank + i * 0.1);
    }
    std::string varName = "testVar";
    std::array<size_t, 3> shape = {elemNum, 0, 0};
    DataMeta dataMeta = DataMeta(varName, 0, 1, typeid(double).name(), sizeof(double), shape);
    size_t blockID = (size_t)rank;
    
    for (int ts = 0; ts < 10; ts++)
    {
        
        std::string slaveAddr = dspaces_client_getaddr(clientEngine, serverAddr, varName, ts);
        std::cout << "the slave server addr for ds put is " << slaveAddr << std::endl;
        dataMeta.m_iteration = ts;
        
        dspaces_client_put(clientEngine, slaveAddr, dataMeta, blockID, inputData);
    }
    
    sleep(0.5);

    //simulate the behavious of the client

    for (int ts = 0; ts < 10; ts++)
    {
        std::string slaveAddr = dspaces_client_getaddr(clientEngine, serverAddr, varName, ts);
        std::cout << "the slave server addr for ds get is " << slaveAddr << std::endl;  
        
        //get the metadata
        BlockMeta blockmeta = dspaces_client_getblockMeta(clientEngine, slaveAddr, varName, ts, blockID);
        blockmeta.printMeta();


        //allocate the memroy based on metadata
        //get meta data
        //dspaces_client_get(clientEngine, addr, ts, rank, inputData);
    }

    /*
    

    std::cout << "ok to put data for rank " << rank << std::endl;

    std::cout << "start to get data: " << std::endl;

    //sleep(0.5);

    dspaces_client_get(clientEngine, serverAddr, ts, rank);

    std::cout << "ok to get data for rank " << rank << std::endl;
*/

    // Finalize the MPI environment.
    MPI_Finalize();
}