//start a server that lisen to the notification 


//when there is notification comes in 


//update the table 

#include <thallium.hpp>
#include "../unimos/common/datameta.h"
#include <string>

namespace tl = thallium;

class TaskController {
public:
    TaskController(){};

    //watch, start a server and send the profile to the server and wait the notification
    //send the watch notification to the master node
    //master node send the watch profile to all other node
    //then return 
    //start the server
    //then send the profile to the master

    void run(std::string& networkingType);
    int subscribe(std::string varName, FilterProfile &fp);
    ~TaskController(){};
};