#ifndef MEMCACHE_H
#define MEMCACHE_H

#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <array>
#include <cstring>
#include "../common/datameta.h"


struct DataObjectInterface
{
    //the meta data of the object
    //the real data is maintaind by the inherited class
    DataMeta m_dataMeta;
    //todo update the blockid to size_t
    virtual int getData(int blockID, void *&dataContainer) = 0;
    virtual int putData(int blockID, size_t dataMallocSize, void *dataContainer) = 0;

    virtual int getDataRegion(size_t blockID, std::array<size_t, 3> baseOffset, std::array<size_t, 3> dataShape, void *&dataContainer) = 0;
    virtual bool ifBlockIdExist(size_t blockID) = 0;
    
    DataObjectInterface(){};
    DataObjectInterface(DataMeta dataMeta) : m_dataMeta(dataMeta){};
    ~DataObjectInterface(){};
};


enum CACHESTATUS
{
    BLOCKEXIST,
    BLOCKNOTEXIST,
    TSNOTEXIST,
    VARNOTEXIST
};

class MemCache
{
public:
    //put/get data by Object
    //parse the interface by the defination
    template <typename dataType>
    int putIntoCache(DataMeta dataMeta, size_t blockID, std::vector<dataType> &dataArray);
    int putIntoCache(DataMeta dataMeta, size_t blockID, void* dataPointer);

    DataMeta* getFromCache(std::string varName, size_t ts, size_t blockID, void *&rawData);
    DataMeta getRegionFromCache(std::string varName, size_t ts, size_t blockID, std::array<size_t, 3> baseOffset, std::array<size_t, 3> regionShape, void *&rawData);

private:
    //first index is the variable name
    //second index is the time step
    //TODO add lock [when modify the element in the map such as delete elements, it will be not thread safety, add lock at that time]
    std::map<std::string, std::map<int, DataObjectInterface *>> dataObjectMap;
    CACHESTATUS checkDataExistance(std::string varName, size_t timeStep, size_t blockID);
};



#endif