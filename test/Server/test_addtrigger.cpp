
#include "../../client/unimosclient.h"
#include "../../commondata/metadata.h"
#include <thallium.hpp>
#include <vector>

namespace tl = thallium;
using namespace GORILLA;
std::string loadMasterAddr(std::string masterConfigFile)
{

  std::ifstream infile(masterConfigFile);
  std::string content = "";
  std::getline(infile, content);
  // spdlog::debug("load master server conf {}, content -{}-", masterConfigFile,content);
  if (content.compare("") == 0)
  {
    std::getline(infile, content);
    if (content.compare("") == 0)
    {
      throw std::runtime_error("failed to load the master server\n");
    }
  }
  return content;
}

void add_basictrigger()
{
  // client engine
  tl::engine clientEngine("tcp", THALLIUM_CLIENT_MODE);
  std::string addrServer = loadMasterAddr("./unimos_server.conf");
  ClientForSim* uniclient = new UniClient(&clientEngine, addrServer, 0);

  std::string triggerNameInit = "InitTrigger";
  std::string triggerNameCustomized = "customizedTrigger";

  // declare the function and the parameters
  std::vector<std::string> checkParameters;
  std::vector<std::string> comparisonParameters;
  comparisonParameters.push_back("5");
  std::vector<std::string> actionParameters;

  // start the trigger when the condition is satisfied
  actionParameters.push_back(triggerNameCustomized);
  DynamicTriggerInfo tgInfo("defaultCheckGetStep", checkParameters, "defaultComparisonStep",
    comparisonParameters, "defaultActionSartDt", actionParameters);

  // register the trigger
  std::array<int, 3> indexlb = { { 0, 0, 0 } };
  std::array<int, 3> indexub = { { 63, 63, 63 } };
  uniclient->registerTrigger(3, indexlb, indexub, triggerNameInit, tgInfo);

  // another trigger
  std::vector<std::string> customizedParameter;
  customizedParameter.push_back("customized default parameter list");
  DynamicTriggerInfo tgInfoCustomized("defaultCheck", customizedParameter, "defaultComparison",
    customizedParameter, "defaultAction", customizedParameter);

  indexlb = { { 0, 0, 0 } };
  indexub = { { 9, 9, 9 } };
  uniclient->registerTrigger(3, indexlb, indexub, triggerNameCustomized, tgInfoCustomized);
}

int main()
{

  add_basictrigger();
}