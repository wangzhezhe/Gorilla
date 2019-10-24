
#ifndef OBJBYRAWMEM_H
#define OBJBYRAWMEM_H

#include "memcache.h"

// This abstraction manage one specific data blocks
struct DataBlockByRawMem
{

public:

    DataBlockByRawMem(){};
    DataBlockByRawMem(size_t dataBlockSize,BlockMeta blockmeta):m_blockData(blockmeta){
        //init empty memory space
        m_rawMemPtr = malloc(dataBlockSize);
        if (m_rawMemPtr == NULL)
        {
            throw std::runtime_error("failed to allocate the memroy for dataBlock");
        }
    }
    DataBlockByRawMem(BlockMeta blockmeta):m_blockData(blockmeta){
        //init empty memory space
        size_t dataBlockSize=m_blockData.getBlockMallocSize();
        m_rawMemPtr = malloc(dataBlockSize);
        if (m_rawMemPtr == NULL)
        {
            throw std::runtime_error("failed to allocate the memroy for dataBlock");
        }
    }
    DataBlockByRawMem(BlockMeta blockmeta, void *dataSourcePtr):m_blockData(blockmeta)
    {
        //m_blockID = blockID;
        size_t dataBlockSize=m_blockData.getBlockMallocSize();
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


    BlockMeta m_blockData = BlockMeta();
    void *m_rawMemPtr = NULL;

};

// This abstraction manage multiple data blocks
struct DataObjectByRawMem : public DataObjectInterface
{

public:

    DataObjectByRawMem(std::string varName, size_t iteration):DataObjectInterface(varName,iteration){};
    
    //the super class need to be initialised
    //it is necessary to use the ObjectMeta instead of blockMeta
    DataObjectByRawMem(size_t blockID, DataMeta datameta, void *rawDataPtr);

    template <typename dataType>
    void setDataObjectByVector(size_t blockID, DataMeta datameta,std::vector<dataType> &dataArray);
    
    BlockMeta getDataBlock(size_t blockID);

    BlockMeta getData(size_t blockID, void *&dataContainer);

    //get data in specific region
    //the region is labeled by lb and ub  
    BlockMeta getDataRegion(size_t blockID, std::array<size_t, 3> baseOffset, std::array<size_t, 3> regionShape, void *&dataContainer);

    //this method should be called when there is no block data
    int putData(size_t blockID, BlockMeta blockMeta, void *dataContainer);

    bool ifBlockIdExist(size_t blockID);

    BlockMeta getBlockMeta(size_t blockID);

    //size_t getBlockSize(size_t blockID){return this->m_blockSize;}

    ~DataObjectByRawMem(){};

private:
    //this is used to index the blockData by block ID
    //the first value is the blockid and the second value is the real data
    //TODO add lock
    //std::map<size_t,DataBlock<T>*> dataBlockMap;
    std::map<size_t, DataBlockByRawMem *> dataBlockMap;
};

#endif