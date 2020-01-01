#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <string>
#include <iostream>

struct Settings {
    Settings();
    std::string protocol;
    std::string masterInfo;
    bool datachecking;
    size_t dims;
    size_t metaserverNum;
    //if the maxDimValue is the 5, there are 6 numbers , namely 0 to 5
    //this position represent the upperbound position not the number of the elements
    size_t maxDimValue;
    int logLevel;
    //add the copy function or use the new operator
    static Settings from_json(const std::string &fname);

    void printsetting(){
    std::cout<<"------settins------"<<std::endl;
    std::cout << "protocol:              " << this->protocol<< std::endl;
    std::cout << "masterInfo:            " << this->masterInfo << std::endl;
    std::cout << "datachecking:          " << this->datachecking << std::endl;
    std::cout << "dims:                  " << this->dims << std::endl;
    std::cout << "metaserverNum:         " << this->metaserverNum << std::endl;
    std::cout << "logLevel:              " << this->logLevel << std::endl;
    std::cout<<"------settins------"<<std::endl;
    }

    Settings& operator=(Settings& other)
    {
        protocol=other.protocol;
        masterInfo=other.masterInfo;
        datachecking=other.datachecking;
        dims=other.dims;
        metaserverNum=other.metaserverNum;
        maxDimValue=other.maxDimValue;
        logLevel=other.logLevel;
        return *this;
    }

    ~Settings(){};
};

#endif
