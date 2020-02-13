
#include "../server/FunctionManager/functionManager.h"
#include "../server/FunctionManager/defaultFunctions/defaultfuncraw.h"
#include <thallium.hpp>
#include <unistd.h>

void test_exengineraw()
{
    tl::abt scope;
    FunctionManagerRaw *exengine = new FunctionManagerRaw();
    BlockSummary bs;
    std::string funcName = "test";
    std::vector<std::string> parameters;
    exengine->execute(bs, NULL, funcName, parameters);
    //make sure the delete operation is called
    //then the destructor will be called;
    delete exengine;
}

void test_executeVTK_raw()
{
    tl::abt scope;
    FunctionManagerRaw *exengine = new FunctionManagerRaw();

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

    BlockSummary bs(sizeof(double), len * len * len, DRIVERTYPE_RAWMEM, 3, {{0, 0, 0}}, {{9, 9, 9}});
    std::string funcName = "testvtk";
    std::vector<std::string> parameters;
    exengine->execute(bs, rawdata.data(), funcName, parameters);
    //make sure the delete operation is called
    //then the destructor will be called;
    delete exengine;
}

int main()
{

    test_exengineraw();

    test_executeVTK_raw();
}