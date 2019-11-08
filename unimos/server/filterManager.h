
#ifndef FILTERMANAGER_H
#define FILTERMANAGER_H

#include <map>
#include <set>
#include <string>
#include <thallium.hpp>
#include "constraintManager.h"
#include "../common/datameta.h"

namespace tl = thallium;

//this should be the parameter of the function 
//or the name of the function


//status
//var not exist
//var exist cm not exist
//var exist cm exits

enum FMANAGERSTATUS
{
    CMEXIST,
    CMNOTEXIST,
    FMANAGERNOTEXIST
};

class FilterManager{

public:
    FilterManager(){};

    ~FilterManager(){};

    //subscribe
    //the name is for the variable
    int profileSubscribe(std::string varName, FilterProfile& fp);

    //send the notification to the subscriber
    void notify(size_t step, size_t blockID, std::set<std::string> &subscriberAddrSet);

    enum FMANAGERSTATUS getStatus(std::string varName, std::string cmName);

    //notify
    //from the variable name to the set of the filter
    //one variable name could contains several cm
    //this structure is filled by the subscribe interface
    std::map<std::string, std::map< std::string, constraintManager* > > constraintManagerMap;

    //the pointer to the server enginge
    //is it necessary to use the client mode here???
    tl::engine * m_Engine = nullptr;
};

#endif