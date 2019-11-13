#include "unimosclient.h"
#include <thread>
#include <cmath>

int main(int argc, char **argv)
{

    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <networkingType>" << std::endl;
        exit(0);
    }

    //tl::endpoint server = dspaces_client_init(argv[1]);

    std::string networkingType = argv[1];

    std::string serverAddr = loadMasterAddr();

    std::cout << "load server addr: " << serverAddr << std::endl;

    tl::engine clientEngine(networkingType, THALLIUM_CLIENT_MODE);

    //generate data

    size_t elemNum = 10;
    std::vector<double> inputData;
    for (int i = 0; i < elemNum; i++)
    {
        inputData.push_back(i * 0.1);
    }
    std::string varName = "testName1d";
    std::array<size_t, 3> shape = {elemNum, 0, 0};
    size_t steptmp = 0;
    DataMeta dataMeta = DataMeta(varName, steptmp, typeid(double).name(), sizeof(double), shape);
    size_t blockID = 0;

    int step = 0;

    for (step = 0; step < 20; step++)
    {
       
        std::string slaveAddr = dspaces_client_getaddr(clientEngine, serverAddr, varName, step, blockID);

        std::cout << "the slave server addr for ds put is " << slaveAddr <<" for step " << step << std::endl;
        if (slaveAddr.compare("NOREGISTER") == 0)
        {
            break;
        }
        dataMeta.m_steps = step;
        dspaces_client_put(clientEngine, slaveAddr, dataMeta, blockID, inputData);
        //there is problem for margo if the push operation is in high frequency
        //the probelm is occasional
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}