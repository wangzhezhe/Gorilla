
#include <vector>
#include "../../client/unimosclient.h"
#include "../../commondata/metadata.h"
#include <thallium.hpp>

namespace tl = thallium;

//assume that the server is alreasy started normally
void test_put_1d()
{

    //client engine
    tl::engine clientEngine("tcp", THALLIUM_CLIENT_MODE);
    UniClient *uniclient = new UniClient(&clientEngine, "./unimos_server.conf");

    size_t elemSize = 100;
    size_t elemNum = sizeof(double);
    std::string driverType = DRIVERTYPE_RAWMEM;
    size_t dims = 1;
    std::array<size_t, 3> indexlb = {{0, 0, 0}};
    std::array<size_t, 3> indexub = {{elemSize - 1, 0, 0}};
    std::string varName = "testVar";

    //allocate space
    std::vector<double> rawdata;
    for (int i = 0; i < elemSize - 1; i++)
    {
        rawdata.push_back(i * 0.1);
    }

    //generate raw data summary block
    BlockSummary bs(elemSize, elemNum,
                    driverType,
                    dims,
                    indexlb,
                    indexub);

    //generate raw data
    for (size_t step = 0; step < 9; step++)
    {
        int status = uniclient->putrawdata(step, varName, bs, rawdata.data());
        if (status != 0)
        {
            throw std::runtime_error("failed to put data for step " + std::to_string(step));
        }
    }
}

int main()
{
    //test put basic
    test_put_1d();

    //test_put_2d();

    //test_put_3d();
}