#include "unimosclient.h"
#include <mpi.h>
#include <cmath>

bool AreSame(double a, double b)
{
    return std::fabs(a - b) < 0.000001;
}

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
    size_t step = 0;
    DataMeta dataMeta = DataMeta(varName, step, typeid(double).name(), sizeof(double), shape);
    size_t blockID = (size_t)rank;

    for (int ts = 0; ts < 10; ts++)
    {

        std::string slaveAddr = dspaces_client_getaddr(clientEngine, serverAddr, varName, ts);
        std::cout << "the slave server addr for ds put is " << slaveAddr << std::endl;
        dataMeta.m_steps = ts;

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

        //how to solve this case?
        //void *dataPointer = (void *)malloc(blockmeta.getBlockMallocSize());
        //get meta data

        std::vector<double> dataContainer(blockmeta.m_shape[0]);
        dspaces_client_get(clientEngine, slaveAddr, varName, ts, blockID, dataContainer);

        //check the correctness of the data content
        //if the type is the double for the return value
        //double *temp = (double *)dataPointer;
        for (int i = 0; i < blockmeta.m_shape[0]; i++)
        {
            //std::cout << "index " << i << " value " << *dataValue << std::endl;
            if (!AreSame(dataContainer[i], rank + i * 0.1))
            {
                std::cout << "index " << i << " return value " << dataContainer[i] << std::endl;
                throw std::runtime_error("return value is incorrect for index  " + std::to_string(i));
            }

        }
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