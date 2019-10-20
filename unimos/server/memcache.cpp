//the linked list that store the data object
//this structure is same with the dataspaces conceptually

#include "memcache.h"
#include "objectbyrawmem.h"

enum CACHESTATUS MemCache::checkDataExistance(std::string varName, size_t timeStep, size_t blockID)
{
    if (this->dataObjectMap.find(varName) != dataObjectMap.end())
    {

        if (this->dataObjectMap[varName].find(timeStep) != dataObjectMap[varName].end())
        {
            //check block
            if (this->dataObjectMap[varName][timeStep]->ifBlockIdExist(blockID))
            {
                return CACHESTATUS::BLOCKEXIST;
            }
            else
            {
                return CACHESTATUS::BLOCKNOTEXIST;
            }
        }
        else
        {
            return CACHESTATUS::TSNOTEXIST;
        }
    }

    return CACHESTATUS::VARNOTEXIST;
}

template <typename dataType>
int MemCache::putIntoCache(DataMeta dataMeta, size_t blockID, std::vector<dataType> &dataArray)
{

    //create a interface

    std::cout << "implement the putintoCache" << std::endl;

    //check existance

    enum CACHESTATUS cacheStatus = checkDataExistance(dataMeta.m_varName, dataMeta.m_timeStep, blockID);

    switch (cacheStatus)
    {
    case BLOCKEXIST:
        std::cout << "failed to insert data, varname " << dataMeta.m_varName << " with ts " << dataMeta.m_timeStep << " and block " << blockID << "exists " << std::endl;
        throw std::runtime_error("putIntoCache failed");
        break;
    case BLOCKNOTEXIST:
    {
        //put value into the original DataObjectByRawMem
        size_t dataMallocSize = dataMeta.getDataMallocSize();
        if (dataMallocSize == 0)
        {
            throw std::runtime_error("failed to putData, dataMallocSize is 0");
        }
        dataObjectMap[dataMeta.m_varName][dataMeta.m_timeStep]->putData(blockID, dataMallocSize, (void *)dataArray.data());
        break;
    }

    case TSNOTEXIST:
    {
        //DataObjectByRawMem *tempptr = new DataObjectByRawMem(DataMeta());
        DataObjectByRawMem *tempptr = new DataObjectByRawMem();
        //set actual value by template function
        tempptr->setDataObjectByVector<dataType>(dataMeta, blockID, dataArray);
        DataObjectInterface *objInterfrace = tempptr;
        dataObjectMap[dataMeta.m_varName][dataMeta.m_timeStep] = objInterfrace;
        break;
    }

    case VARNOTEXIST:
    {
        DataObjectByRawMem *tempptr = new DataObjectByRawMem();
        //set actual value by template function
        tempptr->setDataObjectByVector<dataType>(dataMeta, blockID, dataArray);
        DataObjectInterface *objInterfrace = tempptr;

        std::map<int, DataObjectInterface *> innerMap;
        innerMap[dataMeta.m_timeStep] = objInterfrace;
        dataObjectMap[dataMeta.m_varName] = innerMap;
        break;
    }
    }

    //TODO use the error code to label the status of put operation
    return 0;
}

int MemCache::putIntoCache(DataMeta dataMeta, size_t blockID, void *dataPointer)
{

    //check existance

    enum CACHESTATUS cacheStatus = checkDataExistance(dataMeta.m_varName, dataMeta.m_timeStep, blockID);
    //put value into the original DataObjectByRawMem

    switch (cacheStatus)
    {
    case BLOCKEXIST:
        std::cout << "failed to insert data, varname " << dataMeta.m_varName << " with ts " << dataMeta.m_timeStep << " and block " << blockID << "exists " << std::endl;
        throw std::runtime_error("putIntoCache failed");
        break;
    case BLOCKNOTEXIST:
    {
        size_t dataMallocSize = dataMeta.getDataMallocSize();
        if (dataMallocSize == 0)
        {
            throw std::runtime_error("failed to putData, dataMallocSize is 0");
        }
        dataObjectMap[dataMeta.m_varName][dataMeta.m_timeStep]->putData(blockID, dataMallocSize, dataPointer);
        break;
    }

    case TSNOTEXIST:
    {

        DataObjectInterface *objInterfrace = new DataObjectByRawMem(dataMeta, blockID, dataPointer);
        dataObjectMap[dataMeta.m_varName][dataMeta.m_timeStep] = objInterfrace;
        break;
    }

    case VARNOTEXIST:
    {
        DataObjectInterface *objInterfrace = new DataObjectByRawMem(dataMeta, blockID, dataPointer);
        std::map<int, DataObjectInterface *> innerMap;
        innerMap[dataMeta.m_timeStep] = objInterfrace;
        dataObjectMap[dataMeta.m_varName] = innerMap;
        break;
    }
    }

    //TODO use the error code to label the status of put operation
    return 0;
}

DataMeta *MemCache::getFromCache(std::string varName, size_t ts, size_t blockID, void *&rawData)
{
    //use the error code to label the status of get operation
    //check the map
    enum CACHESTATUS cacheStatus = MemCache::checkDataExistance(varName, ts, blockID);
    DataObjectInterface *objInterfrace = nullptr;

    switch (cacheStatus)
    {
    case BLOCKNOTEXIST:
        std::cout << "failed to get data, varname " << varName << " with ts " << ts << " block " << blockID << " not exist " << std::endl;
        return NULL;
    case TSNOTEXIST:
        std::cout << "failed to get data, varname " << varName << " with ts " << ts << " not exist " << std::endl;
        return NULL;
    case VARNOTEXIST:
        std::cout << "failed to get data, varname " << varName << " not exist" << std::endl;
        return NULL;

    case BLOCKEXIST:
        objInterfrace = dataObjectMap[varName][ts];
        break;
    }

    //if exist get datablock and put it into the dataArray
    //send the pointer out to the vector
    int status = objInterfrace->getData(blockID, rawData);

    if (rawData == nullptr)
    {
        std::cout << "rawData is nullptr at getFromCache" << std::endl;
    }

    /*
    std::cout << "check at the getFromCache" << std::endl;
    int *rawTemp = (int*)rawData;
    for (int i = 0; i < 10; i++)
    {
        std::cout << "index " << i << "value " << *rawTemp << std::endl;
        rawTemp++;
    }
    */
    if (status != 0)
    {
        std::cout << "failed to get the getData for varName " << varName << "ts " << ts << "block " << blockID << std::endl;
        return NULL;
    }

    return &objInterfrace->m_dataMeta;
}

DataMeta MemCache::getRegionFromCache(
    std::string varName, size_t ts, size_t blockID, std::array<size_t, 3> baseOffset, std::array<size_t, 3> regionShape, void *&rawData)
{
    //use the error code to label the status of get operation
    //check the map
    enum CACHESTATUS cacheStatus = MemCache::checkDataExistance(varName, ts, blockID);
    DataObjectInterface *objInterfrace = nullptr;

    switch (cacheStatus)
    {
    case BLOCKNOTEXIST:
        std::cout << "failed to get data, varname " << varName << " with ts " << ts << " block " << blockID << " not exist " << std::endl;
        return DataMeta();
    case TSNOTEXIST:
        std::cout << "failed to get data, varname " << varName << " with ts " << ts << " not exist " << std::endl;
        return DataMeta();
    case VARNOTEXIST:
        std::cout << "failed to get data, varname " << varName << " not exist" << std::endl;
        return DataMeta();

    case BLOCKEXIST:
        objInterfrace = dataObjectMap[varName][ts];
        break;
    }

    // if exist get datablock and put it into the dataArray
    // send the pointer out to the vector
    // std::array<size_t, 3> baseOffset, std::array<size_t, 3> regionShape
    int status = objInterfrace->getDataRegion(blockID, baseOffset, regionShape, rawData);

    std::cout << "check at the getFromCache" << std::endl;

    if (status != 0)
    {
        std::cerr << "failed to get the getDataRegion for varName " << varName << " ts " << ts << " block " << blockID << std::endl;
        throw std::runtime_error("getRegionFromCache failed");
        return DataMeta();
    }

    if (rawData == NULL)
    {
        std::cerr << "failed to get the getDataRegion for varName (null) " << varName << " ts " << ts << " block " << blockID << std::endl;
        throw std::runtime_error("getRegionFromCache failed");
        return DataMeta();
    }

    //generate the new metadata
    DataMeta queryMetaData = objInterfrace->m_dataMeta;
    queryMetaData.m_shape = regionShape;

    return queryMetaData;
}

template int MemCache::putIntoCache<int>(DataMeta dataMeta, size_t blockID, std::vector<int> &dataArray);
template int MemCache::putIntoCache<double>(DataMeta dataMeta, size_t blockID, std::vector<double> &dataArray);
