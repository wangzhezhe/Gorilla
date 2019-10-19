
#ifndef OBJBYRAWMEM_H
#define OBJBYRAWMEM_H

#include "memcache.h"

struct DataBlockByRawMem
{
    int m_blockID;
    void *m_rawMemPtr = NULL;

    DataBlockByRawMem(){};
    DataBlockByRawMem(int blockID, size_t dataBlockSize, void *dataSourcePtr)
    {
        m_blockID = blockID;
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

    int getData(int blockID, void *&dataContainer);

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