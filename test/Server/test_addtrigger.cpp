
#include <vector>
#include "../../client/unimosclient.h"
#include "../../commondata/metadata.h"
#include <thallium.hpp>

namespace tl = thallium;

void add_basictrigger()
{
    //client engine
    tl::engine clientEngine("tcp", THALLIUM_CLIENT_MODE);
    UniClient *uniclient = new UniClient(&clientEngine, "./unimos_server.conf",0);

    std::string triggerNameInit = "InitTrigger";
    std::string triggerNameCustomized = "customizedTrigger";

    //declare the function and the parameters
    std::vector<std::string> checkParameters;
    std::vector<std::string> comparisonParameters;
    comparisonParameters.push_back("5");
    std::vector<std::string> actionParameters;

    //start the trigger when the condition is satisfied
    actionParameters.push_back(triggerNameCustomized);
    DynamicTriggerInfo tgInfo("defaultCheckGetStep", checkParameters, 
    "defaultComparisonStep", comparisonParameters, 
    "defaultActionSartDt", actionParameters);

    //register the trigger
    std::array<int, 3> indexlb = {{0, 0, 0}};
    std::array<int, 3> indexub = {{63, 63, 63}};
    uniclient->registerTrigger(3, indexlb, indexub, triggerNameInit, tgInfo);

    //another trigger
    std::vector<std::string> customizedParameter;
    customizedParameter.push_back("customized default parameter list");
    DynamicTriggerInfo tgInfoCustomized("defaultCheck", customizedParameter, "defaultComparison", customizedParameter, "defaultAction", customizedParameter);

    indexlb = {{0, 0, 0}};
    indexub = {{9, 9, 9}};
    uniclient->registerTrigger(3, indexlb, indexub, triggerNameCustomized, tgInfoCustomized);
}

int main()
{

    add_basictrigger();
}