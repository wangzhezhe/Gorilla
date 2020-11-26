#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <string>
#include <iostream>
#include <vector>

/*
{
    "lenArray":[512,512,512],
    "partitionMethod":"manual",    
    "partitionLayout":[2,2,2],
    "metaserverNum": 8,
    "protocol": "verbs",
    "masterInfo":"./unimos_server.conf",
    "addTrigger": false,
    "logLevel":1
}
*/

struct Settings {
    Settings();
    //the element here represent the number of the elements at each dimention
    //this is not the index, this is the number of the elements
    std::vector<int> lenArray;
    std::string partitionMethod;
    std::vector<int> partitionLayout;
    size_t metaserverNum;
    std::string protocol;
    std::string masterInfo;
    bool addTrigger;
    std::string memLimit;
    int logLevel;
    
    //add the copy function or use the new operator
    static Settings from_json(const std::string &fname);

    void printsetting(){
    std::cout<<"------settings------"<<std::endl;
    int arraySize = this->lenArray.size();
    std::cout << "lenArray:              ";
    for(int i=0;i<arraySize;i++){
        std::cout << this->lenArray[i];
        if(i!=arraySize-1){
            std::cout << ",";
        }else{
            std::cout << std::endl;
        }
    }
    

    std::cout << "partitionMethod:       " << this->partitionMethod<< std::endl;
    std::cout << "partitionLayout:       ";
    arraySize = this->partitionLayout.size();
    for(int i=0;i<arraySize;i++){
        std::cout << this->partitionLayout[i];
        if(i!=arraySize-1){
            std::cout << ",";
        }else{
            std::cout << std::endl;
        }
    }
    
    std::cout << "metaserverNum:         " << this->metaserverNum << std::endl;
    std::cout << "addTrigger:            " << this->addTrigger << std::endl;
    std::cout << "memLimit:              " << this->memLimit << std::endl;
    std::cout << "logLevel:              " << this->logLevel << std::endl;
    std::cout<<"--------------------"<<std::endl;
    }

    Settings& operator=(Settings& other)
    {
        lenArray=other.lenArray;
        partitionMethod=other.partitionMethod;
        partitionLayout=other.partitionLayout;
        metaserverNum=other.metaserverNum;
        protocol=other.protocol;
        masterInfo=other.masterInfo;
        memLimit=other.memLimit;
        addTrigger=other.addTrigger;
        logLevel=other.logLevel;
        return *this;
    }

    ~Settings(){};
};

#endif
