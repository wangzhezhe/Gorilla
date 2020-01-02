
#include <vector>
#include "../../client/unimosclient.h"
#include "../../commondata/metadata.h"
#include <thallium.hpp>

namespace tl = thallium;

void test_get_metaListEmpty()
{

    //client engine
    tl::engine clientEngine("tcp", THALLIUM_CLIENT_MODE);
    UniClient *uniclient = new UniClient(&clientEngine, "./unimos_server.conf");

    //although we use same config here, if the max length at the metaserver is different
    //the number of returned metaserver is different
    //for example, if we use the 511 at the setting.json, there is only 1 value in the vector
    //if we use the 127 at the setting.json, there is 4 value in the vector
    std::array<int, 3> indexlb = {{10, 10, 0}};
    std::array<int, 3> indexub = {{99, 99, 0}};

    std::vector<std::string> metaList = uniclient->getmetaServerList(2, indexlb, indexub);

    for(int i=0;i<metaList.size();i++){
        std::cout << "server " << metaList[i] << std::endl;
    }
}

//assume that the server is alreasy started normally
void test_put()
{

    //client engine
    tl::engine clientEngine("tcp", THALLIUM_CLIENT_MODE);
    UniClient *uniclient = new UniClient(&clientEngine, "./unimos_server.conf");

    //there is data screw if the length of the data is not the 2^n
    //there is data screw if the input data is not in the shape of the cubic
    size_t elemInOneDim = (99 - 10 + 1);
    size_t elemSize = elemInOneDim * elemInOneDim;
    size_t elemNum = sizeof(double);
    std::string driverType = DRIVERTYPE_RAWMEM;
    size_t dims = 2;
    std::array<int, 3> indexlb = {{10, 10, 0}};
    std::array<int, 3> indexub = {{99, 99, 0}};
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

    //check the output at the server end manually
}

int main()
{
    test_get_metaListEmpty();

    //test_put();

    //test_get_metaList();
}