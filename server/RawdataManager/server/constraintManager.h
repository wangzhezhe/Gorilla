#ifndef CONSTRAINTMANAGER_H
#define CONSTRAINTMANAGER_H

#include <map>
#include <set>
#include <vector>
#include <string>

//contains the instance of the callable function

typedef bool (*containtFilterPtr)(void*data);

// one profile is corresponding to a constraint manager
struct constraintManager
{
    constraintManager(std::string stepConstriaintsName, 
    std::string blockConstraintsName,
    std::string contentConstrains);

    ~constraintManager(){};
	
    bool (*stepConstraintsPtr)(size_t)=NULL;
	bool (*blockConstraintsPtr)(size_t)=NULL;
    bool (*contentConstraintPtr)(void*data)=NULL;

    //execute all the binding filter for current data to see if it return true
    bool execute (size_t step, size_t blockID, void*data);
    
    void addSubscriber(std::string subscriberAddr){subscriberAddrSet.insert(subscriberAddr);}

    //list of the subscriber corresponding with this constraint manager
    //this is corresponding to one varaiale
    std::set<std::string> subscriberAddrSet;
};


#endif