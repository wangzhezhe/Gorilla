
#include "controller.h"
#include <thread>
#include <iostream>
#include <fstream>
#include "../unimos/client/unimosclient.h"
#include "../utils/stringtool.h"

namespace tl = thallium;

tl::engine *globalEnginePointer = nullptr;

void notify(const tl::request &req, size_t &step, size_t &blockID)
{
    //TODO update the innter structure according to the notified info
    std::cout << "call the notify at the controller" << std::endl;
    req.respond(0);
}

int TaskController::subscribe(std::string varName, FilterProfile &fp)
{

    //run the server
    std::string masterAddr = loadMasterAddr();
    //got the server addr
    dssubscribe(*globalEnginePointer, masterAddr, varName, fp);
    return 0;
}

void TaskController::run(std::string &networkingType)
{

    //send the request to the master
    std::cout << "networking type: " << networkingType <<std::endl;
    tl::engine controllerEnginge(networkingType, THALLIUM_SERVER_MODE);
    //define the notify service
    globalEnginePointer = &controllerEnginge;
    globalEnginePointer->define("notify", notify);
    return;
}

int main(int argc, char **argv)
{

    //run the task by separate process

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <networkingType>" << std::endl;
        exit(0);
    }

    std::string networkingType = std::string(argv[1]);

    TaskController *tc = new TaskController();

    std::thread runController(&TaskController::run, tc, std::ref(networkingType));

    //use the enginge pointer to subscribe the profile
    while (true)
    {
        if (globalEnginePointer == nullptr)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        else
        {
            break;
        }
    }

    FilterProfile fp;

    std::string tcAddr = globalEnginePointer->self();
    std::string filteredAddr = IPTOOL::getClientAdddr(networkingType, tcAddr);
    std::cout << "task controller is started at " << filteredAddr << std::endl;
    fp.m_subscriberAddr = filteredAddr;

    tc->subscribe("testName1d", fp);

    //write file, tell the controller that the sub operation is ok
    std::ofstream subokFile;
    
    //it is better to use the MMserver here
    subokFile.open("./subok.conf");
    subokFile.close();

    runController.join();

    return 0;
}