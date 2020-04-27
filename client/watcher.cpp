

#include "watcher.hpp"
#include "../commondata/metadata.h"
#include <iostream>
//TODO, reduce the infromaion here, this is recieved by several partition
//consider how to reduce the notification
//send a function pointer here and call this function when there is notification information
void rcvNotify(const tl::request &req, BlockSummary& bs){
    //prtint the metadata info
    std::cout << "rcvNotify is called " << std::endl;
    bs.printSummary();
    return;
}

//before starting watch, it is necessary to register the watch operation to the server
//iterate all the server, if register the addr
//if the filter exist, register the watcher
//the clientEnginePtr should be server mode if the watcher is called
std::string Watcher::startWatch(tl::engine *enginePtr){

    enginePtr->define("rcvNotify", rcvNotify);
    std::string rawAddr = enginePtr->self();
    //get the margo instance and wait here
    std::cout << "start watcher for addr: " << rawAddr << std::endl;
    margo_wait_for_finalize(enginePtr->get_margo_instance());
    return rawAddr;
}
