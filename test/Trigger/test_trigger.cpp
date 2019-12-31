

#include "../server/ExecutionEngine/executionengine.h"
#include "../server/ExecutionEngine/defaultFunctions/defaultfuncmeta.h"
#include "../server/TriggerManager/dynamictrigger.h"


void test_defaulttrigger()
{
    tl::abt scope;
    std::vector<std::string> parameters(5, "someparameters");
    FuncDescriptor checkFunc("defaultCheck", parameters);
    FuncDescriptor comparisonFunc("defaultComparison", parameters);
    FuncDescriptor actionFunc("defaultAction", parameters);
    DynamicTrigger dt(checkFunc, comparisonFunc, actionFunc);
    ExecutionEngineMeta *exenginemeta = new ExecutionEngineMeta(5);
    dt.start(exenginemeta);
}

int main()
{
    test_defaulttrigger();
}