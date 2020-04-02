#include <vector>
#include <string>
#include <thallium.hpp>

namespace tl = thallium;

//the watcher at the client is a server
//this server only have one RPC, the recvNotify
struct Watcher {
    Watcher(){};
    //watch the trigger in the list
    //TODO, add a function handler
    //the function will be called if there is notification
    std::string startWatch(tl::engine *enginePtr);
    //close the server
    int end(){};
    ~Watcher(){};


    std::string m_watcherAddr;
};