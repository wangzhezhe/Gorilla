#ifndef EPMANAGEMENT_H
#define EPMANAGEMENT_H


#include <string>
#include <iostream>
#include <vector>


struct endPointsManager
{
    int m_serverNum;
    std::vector<std::string> m_endPointsLists;

    endPointsManager(){};
    endPointsManager(int totalServerNum):m_serverNum(totalServerNum){};

    bool ifAllRegister(int registeredSize){
        if(m_serverNum==0){
            return false;
        }

        //std::cout << "check server num " << m_serverNum 
        //<< " registered size " << registeredSize << " lists size " << m_endPointsLists.size() << std::endl;
        //the master is not included into the endpoint list
        return m_serverNum==m_endPointsLists.size()+1;    
    }

    bool ifMaster = false;

    std::string nodeAddr = "";
        
    //hash the varName + ts
    std::string getByVarTs(std::string varName, int ts);
};




#endif