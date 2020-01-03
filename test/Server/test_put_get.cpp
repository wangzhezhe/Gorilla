
#include <vector>
#include "../../client/unimosclient.h"
#include "../../commondata/metadata.h"
#include <thallium.hpp>
#include "../../utils/bbxtool.h"
#include "../../utils/matrixtool.h"

namespace tl = thallium;

void test_get_meta()
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

    std::cout << "---tests1---" << std::endl;
    for (int i = 0; i < metaList.size(); i++)
    {
        std::cout << "server " << metaList[i] << std::endl;
    }

    indexlb = {{10, 10, 0}};
    indexub = {{99, 12, 0}};

    metaList = uniclient->getmetaServerList(2, indexlb, indexub);

    std::cout << "---tests2---" << std::endl;
    for (int i = 0; i < metaList.size(); i++)
    {
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
    size_t elemSize = sizeof(double);
    size_t elemNum = elemInOneDim * elemInOneDim;
    std::string driverType = DRIVERTYPE_RAWMEM;
    size_t dims = 2;
    std::array<int, 3> indexlb = {{10, 10, 0}};
    std::array<int, 3> indexub = {{99, 99, 0}};
    std::string varName = "testVar";

    //allocate space
    std::vector<double> rawdata;
    for (int i = 0; i < elemNum - 1; i++)
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
    for (size_t step = 0; step < 5; step++)
    {
        int status = uniclient->putrawdata(step, varName, bs, rawdata.data());
        if (status != 0)
        {
            throw std::runtime_error("failed to put data for step " + std::to_string(step));
        }
    }

    //check the output at the server end manually
}

void test_get_rawDatList()
{

    //put some data

    test_put();

    //init client engine
    tl::engine clientEngine("tcp", THALLIUM_CLIENT_MODE);
    UniClient *uniclient = new UniClient(&clientEngine, "./unimos_server.conf");

    //although we use same config here, if the max length at the metaserver is different
    //the number of returned metaserver is different
    //for example, if we use the 511 at the setting.json, there is only 1 value in the vector
    //if we use the 127 at the setting.json, there is 4 value in the vector
    std::array<int, 3> indexlb = {{10, 10, 0}};
    std::array<int, 3> indexub = {{99, 99, 0}};

    std::vector<std::string> metaList = uniclient->getmetaServerList(2, indexlb, indexub);

    std::vector<MATRIXTOOL::MatrixView> matrixViewList;

    for (auto it = metaList.begin(); it != metaList.end(); it++)
    {
        std::string metaServerAddr = *it;
        std::cout << "metadata server " << metaServerAddr << std::endl;

        std::vector<RawDataEndpoint> rweList = uniclient->getrawDataEndpointList(metaServerAddr, 0, "testVar", 2, indexlb, indexub);

        for (auto iit = rweList.begin(); iit != rweList.end(); iit++)
        {
            iit->printInfo();

            //get the subrigion according to the information at the list
            //it is better to use the multithread here

            //get subrigion of the data
            //allocate size (use differnet strategy is the vtk object is used)
            BBXTOOL::BBX *bbx = new BBXTOOL::BBX(2, iit->m_indexlb, iit->m_indexub);
            size_t allocSize = sizeof(double) * bbx->getElemNum();
            std::cout << "alloc size is " << allocSize << std::endl;

            void *subDataContainer = (void *)malloc(allocSize);

            //get the data by subregion api

            int status = uniclient->getSubregionData(iit->m_rawDataServerAddr,
                                                     iit->m_rawDataID,
                                                     allocSize, 2,
                                                     iit->m_indexlb,
                                                     iit->m_indexub,
                                                     subDataContainer);

            if (status != 0)
            {
                std::cout << "failed for get subrigion data for current iit" << std::endl;
                exit(0);
            }

            //check resutls
            std::cout << "check subregion value " << std::endl;
            double *temp = (double *)subDataContainer;
            for (int i = 0; i < 5; i++)
            {
                double value = *(temp + i);
                std::cout << "value " << value << std::endl;
            }

            //put it into the Matrix View
            MATRIXTOOL::MatrixView mv(bbx, subDataContainer);
            matrixViewList.push_back(mv);
        }
    }

    //assemble the matrix View
    BBX *intactbbx = new BBX(2, indexlb, indexub);
    MATRIXTOOL::MatrixView mvassemble = MATRIXTOOL::matrixAssemble(sizeof(double), matrixViewList, intactbbx);
    double *temp = (double *)(mvassemble.m_data);
    std::cout << "check whole region " << std::endl;
    for (int i = 0; i < (90 * 90)-1; i++)
    {
        double value = *(temp + i);
        if(value!=i*0.1){
            std::cout << "index " << i << " value " << value << std::endl;
            throw std::runtime_error("failed to check the assemble data value");
        }
    }

    //free matrixViewList manually according to different usecases
}

int main()
{
    test_get_meta();

    //test_put();

    test_get_rawDatList();

    //test_get_metaList();
}