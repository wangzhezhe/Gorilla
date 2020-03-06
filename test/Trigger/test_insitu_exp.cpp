
#include <vector>
#include "../../client/unimosclient.h"
#include "../../commondata/metadata.h"
#include <thallium.hpp>

namespace tl = thallium;

void add_insitu_exp()
{
    //TODO add gni initilization
    //client engine
    tl::engine clientEngine("verbs", THALLIUM_CLIENT_MODE);
    UniClient *uniclient = new UniClient(&clientEngine, "./unimos_server.conf",0);

    //add the init trigger
    std::string triggerNameInit = "InitTrigger";
    std::string triggerNameExp = "InsituTriggerExp";

    //declare the function and the parameters
    std::vector<std::string> initCheckParameters;
    std::vector<std::string> initComparisonParameters;
    std::vector<std::string> initActionParameters;


    initComparisonParameters.push_back("0");
    //start the trigger when the condition is satisfied
    initActionParameters.push_back(triggerNameExp);
    
    DynamicTriggerInfo initTgInfo(
        "defaultCheckGetStep", initCheckParameters,
        "defaultComparisonStep", initComparisonParameters,
        "defaultActionSartDt", initActionParameters);

    //register the trigger
    std::array<int, 3> indexlb = {{0, 0, 0}};
    std::array<int, 3> indexub = {{511, 511, 511}};
    uniclient->registerTrigger(3, indexlb, indexub, triggerNameInit, initTgInfo);

    std::string anaTime = std::to_string(0.28 * 0.1);
    //declare the function and the parameters
    std::vector<std::string> checkParameters;
    std::vector<std::string> comparisonParameters;
    std::vector<std::string> actionParameters;

    checkParameters.push_back(anaTime);
    comparisonParameters.push_back("8");
    actionParameters.push_back("adiosWrite");

    //start the trigger when the condition is satisfied
    //InsituExpCheck is used to get step and execute the ana, input parameter
    //is the time for ana execution
    //InsituExpCompare is used to check the ana results
    //InsituExpAction is used to write the data by ADIOS
    DynamicTriggerInfo tgInfo(
        "InsituExpCheck", checkParameters,
        "InsituExpCompare", comparisonParameters,
        "InsituExpAction", actionParameters);

    //register the trigger
    //std::array<int, 3> indexlb = {{0, 0, 0}};
    //std::array<int, 3> indexub = {{1023, 1023, 1023}};
    uniclient->registerTrigger(3, indexlb, indexub, triggerNameExp, tgInfo);
}

int main()
{

    add_insitu_exp();
}