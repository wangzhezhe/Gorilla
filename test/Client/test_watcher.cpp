
#include "../../client/watcher.hpp"
#include "../../client/unimosclient.h"
#include <thallium.hpp>
#include <vector>
#include <string>

namespace tl = thallium;

int main(int argc, char **argv)
{

    if (argc < 2)
    {
        std::cerr << "Too few arguments" << std::endl;
        std::cerr << "Usage: <binary> protocol" << std::endl;
        exit(-1);
    }

    std::string protocol = argv[1];

    //start a client
    tl::engine clientEngine(protocol, THALLIUM_SERVER_MODE);

    //test watcher
    Watcher *testwatcher = new Watcher();
    std::vector<std::string> triggerList;

    UniClient *m_uniclient = new UniClient(&clientEngine, "unimos_server.conf", 0);
    m_uniclient->getAllServerAddr();

    //put the coresponding trigger name
    triggerList.push_back("testTrigger1");
    //register wather before starting it
    m_uniclient->registerWatcher(triggerList);
    testwatcher->startWatch(&clientEngine);
    return 0;
}