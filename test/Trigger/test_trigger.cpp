

#include "../server/FunctionManager/defaultFunctions/defaultfuncmeta.h"
#include "../server/FunctionManager/functionManager.h"
#include "../commondata/metadata.h"
#include "../server/TriggerManager/triggerManager.h"
#include <unistd.h>

void test_steptrigger()
{

    std::cout << "------test_steptrigger------" << std::endl;
    tl::abt scope;


    std::string triggerName = "InitTrigger";
    std::string triggerNameb = "defaultTrigger";


    std::vector<std::string> comparisonParameters;
    comparisonParameters.push_back("5");

    std::vector<std::string> actionParameters;
    actionParameters.push_back(triggerNameb);

    FunctionManagerMeta *fmm = new FunctionManagerMeta(NULL);
    DynamicTriggerManager *dtm = new DynamicTriggerManager(fmm, 5);


    DynamicTriggerInfo tgInfo("defaultCheckGetStep", comparisonParameters, 
    "defaultComparisonStep", comparisonParameters, 
    "defaultActionSartDt", actionParameters);
    

    std::vector<std::string> parametersb;
    parametersb.push_back("customized default parameters");
    DynamicTriggerInfo tgInfob("defaultCheck", parametersb, 
    "defaultComparison", parametersb, 
    "defaultAction", parametersb);


    //this is similar to the process of the subscribe
    dtm->updateTrigger(triggerName, tgInfo);
    dtm->updateTrigger(triggerNameb, tgInfob);


    RawDataEndpoint rde;

    //when there is the update of the metadata call Init   
    std::cout << "------test_steptrigger_step_0------" << std::endl;
    dtm->initstart(triggerName, 0, "testVar", rde);
    std::cout << "------test_steptrigger_step_5------" << std::endl;
    dtm->initstart(triggerName, 6, "testVar", rde);
}

int main()
{
    test_steptrigger();
}