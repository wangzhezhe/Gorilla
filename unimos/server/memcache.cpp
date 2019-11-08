//the linked list that store the data object
//this structure is same with the dataspaces conceptually

#include "memcache.h"
#include "objectbyrawmem.h"

MemCache::MemCache(size_t poolSize)
{
    this->m_threadPool = new ThreadPool(poolSize);
    return;
}

enum CACHESTATUS MemCache::checkDataExistance(std::string varName, size_t steps, size_t blockID)
{
    if (this->dataObjectMap.find(varName) != dataObjectMap.end())
    {

        if (this->dataObjectMap[varName].find(steps) != dataObjectMap[varName].end())
        {
            //check block
            if (this->dataObjectMap[varName][steps]->ifBlockIdExist(blockID))
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

    //check existance
    enum CACHESTATUS cacheStatus = checkDataExistance(dataMeta.m_varName, dataMeta.m_steps, blockID);

    switch (cacheStatus)
    {
    case BLOCKEXIST:
        std::cout << "failed to insert data, varname " << dataMeta.m_varName << " with ts " << dataMeta.m_steps << " and block " << blockID << "exists " << std::endl;
        throw std::runtime_error("putIntoCache failed");
        break;
    case BLOCKNOTEXIST:
    {
        //put value into the original DataObjectByRawMem
        BlockMeta bmeta = dataMeta.extractBlockMeta();
        dataObjectMap[dataMeta.m_varName][dataMeta.m_steps]->putData(blockID, bmeta, (void *)dataArray.data());
        break;
    }

    case TSNOTEXIST:
    {
        //DataObjectByRawMem *tempptr = new DataObjectByRawMem(DataMeta());
        DataObjectByRawMem *tempptr = new DataObjectByRawMem(dataMeta.m_varName, dataMeta.m_steps);
        //set actual value by template function
        //put value into the original DataObjectByRawMem
        tempptr->setDataObjectByVector<dataType>(blockID, dataMeta, dataArray);
        DataObjectInterface *objInterfrace = tempptr;
        dataObjectMap[dataMeta.m_varName][dataMeta.m_steps] = objInterfrace;
        break;
    }

    case VARNOTEXIST:
    {
        DataObjectByRawMem *tempptr = new DataObjectByRawMem(dataMeta.m_varName, dataMeta.m_steps);
        //set actual value by template function
        tempptr->setDataObjectByVector<dataType>(blockID, dataMeta, dataArray);
        DataObjectInterface *objInterfrace = tempptr;

        std::map<int, DataObjectInterface *> innerMap;
        innerMap[dataMeta.m_steps] = objInterfrace;
        dataObjectMap[dataMeta.m_varName] = innerMap;
        break;
    }
    }

    this->m_threadPool->enqueue([dataMeta, blockID, this] {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        char str[200];
        sprintf(str, "call the filterManager for varName (%s),step (%d),blockID (%d)\n", dataMeta.m_varName.data(), dataMeta.m_steps, blockID);
        std::cout << str;
        if (this->m_filterManager != NULL)
        {
            //range the map
            std::map<std::string, constraintManager *> tmpcm = this->m_filterManager->constraintManagerMap[dataMeta.m_varName];
            for (auto it = tmpcm.begin(); it != tmpcm.end(); ++it)
            {
                std::cout << it->first << std::endl;
                bool result = it->second->execute(dataMeta.m_steps, blockID, NULL);
                if (result)
                {
                    m_filterManager->notify(dataMeta.m_steps, blockID, it->second->subscriberAddrSet);
                }
            }
        }
        else
        {
            std::cout << "m_filterManager is null" << std::endl;
        }
        return;
    });

    //TODO use the error code to label the status of put operation
    return 0;
}

int MemCache::putIntoCache(DataMeta dataMeta, size_t blockID, void *dataPointer)
{

    //check existance
    enum CACHESTATUS cacheStatus = checkDataExistance(dataMeta.m_varName, dataMeta.m_steps, blockID);
    //put value into the original DataObjectByRawMem

    switch (cacheStatus)
    {
    case BLOCKEXIST:
        std::cout << "failed to insert data, varname " << dataMeta.m_varName << " with ts " << dataMeta.m_steps << " and block " << blockID << "exists " << std::endl;
        throw std::runtime_error("putIntoCache failed");
        break;
    case BLOCKNOTEXIST:
    {
        BlockMeta bmeta = dataMeta.extractBlockMeta();
        size_t dataMallocSize = bmeta.getBlockMallocSize();
        if (dataMallocSize == 0)
        {
            throw std::runtime_error("failed to putData, dataMallocSize is 0");
        }
        dataObjectMap[dataMeta.m_varName][dataMeta.m_steps]->putData(blockID, bmeta, dataPointer);
        break;
    }

    case TSNOTEXIST:
    {
        DataObjectInterface *objInterfrace = new DataObjectByRawMem(blockID, dataMeta, dataPointer);
        //update the data summary and put it into the object interface
        dataObjectMap[dataMeta.m_varName][dataMeta.m_steps] = objInterfrace;
        break;
    }

    case VARNOTEXIST:
    {
        DataObjectInterface *objInterfrace = new DataObjectByRawMem(blockID, dataMeta, dataPointer);
        std::map<int, DataObjectInterface *> innerMap;
        innerMap[dataMeta.m_steps] = objInterfrace;
        dataObjectMap[dataMeta.m_varName] = innerMap;
        break;
    }
    }

    //if the cache is updated, new block comes in
    //decide if this data need to be checked according to the filterManager
    //then start a new thread to exectue the function at in constraints
    //the content need to be check is in map
    //dataMeta.m_varName,dataMeta.m_steps,blockID
    this->m_threadPool->enqueue([dataMeta, blockID, this] {
        //std::this_thread::sleep_for(std::chrono::seconds(5));
        char str[200];
        sprintf(str, "call the filterManager for varName (%s),step (%d),blockID (%d)\n", dataMeta.m_varName.data(), dataMeta.m_steps, blockID);
        std::cout << str << std::endl;
        if (this->m_filterManager != NULL)
        {
            std::map<std::string, constraintManager *> tmpcm = this->m_filterManager->constraintManagerMap[dataMeta.m_varName];
            for (auto it = tmpcm.begin(); it != tmpcm.end(); ++it)
            {
                std::cout << it->first << std::endl;
                bool result = it->second->execute(dataMeta.m_steps, blockID, NULL);
                if (result)
                {
                    m_filterManager->notify(dataMeta.m_steps, blockID, it->second->subscriberAddrSet);
                }
            }
        }

        else
        {
            std::cout << "m_filterManager is null" << std::endl;
        }
    });

    //TODO use the error code to label the status of put operation
    return 0;
}

BlockMeta MemCache::getFromCache(std::string varName, size_t ts, size_t blockID, void *&rawData)
{
    //use the error code to label the status of get operation
    //check the map
    enum CACHESTATUS cacheStatus = MemCache::checkDataExistance(varName, ts, blockID);
    DataObjectInterface *objInterfrace = nullptr;

    switch (cacheStatus)
    {
    case BLOCKNOTEXIST:
        std::cout << "failed to get data, varname " << varName << " with ts " << ts << " block " << blockID << " not exist " << std::endl;
        return BlockMeta();
    case TSNOTEXIST:
        std::cout << "failed to get data, varname " << varName << " with ts " << ts << " not exist " << std::endl;
        return BlockMeta();
    case VARNOTEXIST:
        std::cout << "failed to get data, varname " << varName << " not exist" << std::endl;
        return BlockMeta();

    case BLOCKEXIST:
        objInterfrace = dataObjectMap[varName][ts];
        break;
    }

    //if exist get datablock and put it into the dataArray
    //send the pointer out to the vector
    BlockMeta blockMeta = objInterfrace->getData(blockID, rawData);

    if (rawData == nullptr)
    {
        std::cout << "rawData is nullptr getFromCache" << std::endl;
        std::cout << "failed to get the getData for varName " << varName << "ts " << ts << "block " << blockID << std::endl;
        return BlockMeta();
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

    return blockMeta;
}

BlockMeta MemCache::getRegionFromCache(
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
        return BlockMeta();
    case TSNOTEXIST:
        std::cout << "failed to get data, varname " << varName << " with ts " << ts << " not exist " << std::endl;
        return BlockMeta();
    case VARNOTEXIST:
        std::cout << "failed to get data, varname " << varName << " not exist" << std::endl;
        return BlockMeta();

    case BLOCKEXIST:
        objInterfrace = dataObjectMap[varName][ts];
        break;
    }

    // if exist get datablock and put it into the dataArray
    // send the pointer out to the vector
    // std::array<size_t, 3> baseOffset, std::array<size_t, 3> regionShape
    // this blockData is updated with the new regionShape
    BlockMeta blockMeta = objInterfrace->getDataRegion(blockID, baseOffset, regionShape, rawData);

    std::cout << "check at the getFromCache" << std::endl;

    if (rawData == NULL)
    {
        std::cerr << "failed to get the getDataRegion for varName (null) " << varName << " ts " << ts << " block " << blockID << std::endl;
        throw std::runtime_error("getRegionFromCache failed");
        return BlockMeta();
    }

    return blockMeta;
}

BlockMeta MemCache::getBlockMeta(std::string varName, size_t steps, size_t blockID)
{
    enum CACHESTATUS cacheStatus = MemCache::checkDataExistance(varName, steps, blockID);
    DataObjectInterface *objInterfrace = nullptr;

    switch (cacheStatus)
    {
    case BLOCKNOTEXIST:
        std::cout << "failed to get block meta data, varname " << varName << " with steps " << steps << " block " << blockID << " not exist " << std::endl;
        return BlockMeta();
    case TSNOTEXIST:
        std::cout << "failed to get block meta data, varname " << varName << " with steps " << steps << " not exist " << std::endl;
        return BlockMeta();
    case VARNOTEXIST:
        std::cout << "failed to get block meta data, varname " << varName << " not exist" << std::endl;
        return BlockMeta();

    case BLOCKEXIST:
        objInterfrace = dataObjectMap[varName][steps];
        break;
    }

    //if exist get datablock and put it into the dataArray
    //send the pointer out to the vector
    BlockMeta blockMeta = objInterfrace->getBlockMeta(blockID);
    return blockMeta;
}

template int MemCache::putIntoCache<int>(DataMeta dataMeta, size_t blockID, std::vector<int> &dataArray);
template int MemCache::putIntoCache<double>(DataMeta dataMeta, size_t blockID, std::vector<double> &dataArray);
