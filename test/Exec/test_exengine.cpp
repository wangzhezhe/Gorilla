
#include "../server/ExecutionEngine/executionengine.h"
#include "../server/MetadataManager/metadataManager.h"
#include "../server/ExecutionEngine/defaultFunctions/defaultfunc.h"
#include <thallium.hpp>
#include <unistd.h>

void test_exengineraw()
{
    tl::abt scope;
    ExecutionEngineRaw *exengine = new ExecutionEngineRaw();
    BlockSummary bs;
    std::string funcName = "test";
    exengine->execute(funcName, bs, NULL);
    //make sure the delete operation is called
    //then the destructor will be called;
    delete exengine;
}

void test_executeVTK_raw()
{
    tl::abt scope;
    ExecutionEngineRaw *exengine = new ExecutionEngineRaw();

    std::vector<double> rawdata;

    int len = 10;

    for (int i = 0; i < len * len * len; i++)
    {
        rawdata.push_back(i * 0.1);
    }

    /*
  BlockSummary(std::string typeName, size_t elemSize, size_t elemNum,
               std::string driverType, std::array<size_t, DRIVERTYPE_RAWMEM> indexlb,
               std::array<size_t, DRIVERTYPE_RAWMEM> indexub)
    */

    BlockSummary bs(sizeof(double), len * len * len, DRIVERTYPE_RAWMEM, {{0, 0, 0}}, {{9, 9, 9}});
    std::string funcName = "testvtk";
    exengine->execute(funcName, bs, rawdata.data());
    //make sure the delete operation is called
    //then the destructor will be called;
    delete exengine;
}

int main()
{

    test_exengineraw();

    test_executeVTK_raw();
}