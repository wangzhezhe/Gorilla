#ifndef ADDRMANAGEMENT_H
#define ADDRMANAGEMENT_H


#include <string>
#include <iostream>
#include <vector>
#include <thallium.hpp>
#include "../client/unimosclient.h"

namespace tl = thallium;

struct AddrManager
{
    int m_serverNum;
    //the list of all the servers
    std::vector<std::string> m_endPointsLists;
    
    int m_metaServerNum;


    AddrManager(){};

    bool ifAllRegister(int registeredSize){
        if(m_serverNum==0){
            return false;
        }

        //std::cout << "check server num " << m_serverNum 
        //<< " registered size " << registeredSize << " lists size " << m_endPointsLists.size() << std::endl;
        //the master is not included into the endpoint list
        return m_serverNum==m_endPointsLists.size();    
    }

    bool ifMaster = false;

    std::string nodeAddr = "";
     
    //hash the varName + ts
    std::string getByVarTs(std::string varName, int ts);
    std::string getByVarTsBlockID(std::string varName, int ts, size_t blockID);
    std::string getByRRobin();
    void broadcastMetaServer(UniClient*globalClient);
    
    tl::mutex m_rrbinMutex;

    size_t m_rrobinValue=0;
};




#endif