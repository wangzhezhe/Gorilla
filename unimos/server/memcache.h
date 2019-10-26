#ifndef MEMCACHE_H
#define MEMCACHE_H

#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <array>
#include <cstring>
#include "../common/datameta.h"

//the abstraction that mamage the multiple block object for one step
//todo, update the name here
struct DataObjectInterface
{
    //the meta data of the object
    //the real data is maintaind by the inherited class
    std::string m_varName;
    size_t m_steps;
    //data summary
    //this summary is for the all the block for specific timestep

    //todo update the blockid to size_t
    
    virtual BlockMeta getData(size_t blockID, void *&dataContainer) = 0;
    virtual int putData(size_t blockID, BlockMeta blockMeta, void *dataContainer) = 0;

    virtual BlockMeta getDataRegion(size_t blockID, std::array<size_t, 3> baseOffset, std::array<size_t, 3> regionShape, void *&rawData)= 0;
    virtual bool ifBlockIdExist(size_t blockID) = 0;
    virtual BlockMeta getBlockMeta(size_t blockID) = 0;

    DataObjectInterface(std::string varName, size_t steps):m_varName(varName),m_steps(steps){};
    DataObjectInterface(DataMeta dataMeta) : m_varName(dataMeta.m_varName),m_steps(dataMeta.m_steps){};
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

    BlockMeta getBlockMeta(std::string varName, size_t iteration, size_t blockID);
    
    BlockMeta getFromCache(std::string varName, size_t ts, size_t blockID, void *&rawData);
    BlockMeta getRegionFromCache(std::string varName, size_t ts, size_t blockID, std::array<size_t, 3> baseOffset, std::array<size_t, 3> regionShape, void *&rawData);
    
private:
    //first index is the variable name
    //second index is the time step
    //TODO add lock [when modify the element in the map such as delete elements, it will be not thread safety, add lock at that time]
    std::map<std::string, std::map<int, DataObjectInterface *>> dataObjectMap;
    CACHESTATUS checkDataExistance(std::string varName, size_t timeStep, size_t blockID);
};



#endif