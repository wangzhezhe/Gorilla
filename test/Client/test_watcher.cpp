
#include "../../client/ClientForSim.hpp"
#include <string>
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

int main(int argc, char** argv)
{

  if (argc < 2)
  {
    std::cerr << "Too few arguments" << std::endl;
    std::cerr << "Usage: <binary> protocol" << std::endl;
    exit(-1);
  }

  std::string protocol = argv[1];

  // start a client
  tl::engine clientEngine(protocol, THALLIUM_SERVER_MODE);

  // test watcher
  Watcher* testwatcher = new Watcher();
  std::vector<std::string> triggerList;

  std::string addrServer = loadMasterAddr("unimos_server.conf");

  ClientForSim* m_uniclient = new ClientForSim(&clientEngine, addrServer, 0);
  m_uniclient->getAllServerAddr();

  // put the coresponding trigger name
  triggerList.push_back("testTrigger1");
  // register wather before starting it
  m_uniclient->registerWatcher(triggerList);
  testwatcher->startWatch(&clientEngine);
  return 0;
}