
#ifndef OBJBYRAWMEM_H
#define OBJBYRAWMEM_H

#include "memcache.h"

struct DataBlockByRawMem
{
    //is it necessray to use this??
    //int m_blockID;
    void *m_rawMemPtr = NULL;

    DataBlockByRawMem(){};
    DataBlockByRawMem(size_t dataBlockSize){
        //init empty memory space
        m_rawMemPtr = malloc(dataBlockSize);
        if (m_rawMemPtr == NULL)
        {
            throw std::runtime_error("failed to allocate the memroy for dataBlock");
        }
    }
    DataBlockByRawMem(size_t dataBlockSize, void *dataSourcePtr)
    {
        //m_blockID = blockID;
        m_rawMemPtr = malloc(dataBlockSize);
        std::memcpy(m_rawMemPtr, dataSourcePtr, dataBlockSize);
        if (m_rawMemPtr == NULL)
        {
            throw std::runtime_error("failed to allocate the memroy for dataBlock");
        }
    }

    ~DataBlockByRawMem()
    {
        std::cout << "destructor of DataBlock is called" << std::endl;
        free(m_rawMemPtr);
    }
};

//do not divide by type, divide by inner implementation mechanism

struct DataObjectByRawMem : public DataObjectInterface
{

public:

    DataObjectByRawMem(){};

    DataObjectByRawMem(DataMeta dataMeta,size_t blockID,void *rawDataPtr);

    template <typename dataType>
    void setDataObjectByVector(DataMeta dataMeta,size_t blockID,std::vector<dataType> &dataArray);
    
    //TODO use size_t instead of int
    int getData(int blockID, void *&dataContainer);

    //get data in specific region
    //the region is labeled by lb and ub  
    int getDataRegion(size_t blockID, std::array<size_t, 3> baseOffset, std::array<size_t, 3> dataShape, void *&dataContainer);

    //this method should be called when there is no block data
    int putData(int blockID, size_t dataMallocSize, void *dataContainer);

    bool ifBlockIdExist(size_t blockID);

    ~DataObjectByRawMem(){};

private:
    //this is used to index the blockData by block ID
    //TODO add lock
    //std::map<size_t,DataBlock<T>*> dataBlockMap;
    std::map<size_t, DataBlockByRawMem *> dataBlockMap;
};

#endif