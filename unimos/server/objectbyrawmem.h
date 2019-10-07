
#ifndef OBJBYRAWMEM_H
#define OBJBYRAWMEM_H

#include "memcache.h"

struct DataBlockByRawMem
{
    int m_blockID;
    void* m_rawMemPtr=NULL;
    
    DataBlockByRawMem(){};
    DataBlockByRawMem(int blockID,  size_t dataBlockSize, void*dataSourcePtr)
    {
        m_blockID = blockID;
        m_rawMemPtr = malloc(dataBlockSize);
        std::memcpy(m_rawMemPtr, dataSourcePtr,  dataBlockSize);
        if(m_rawMemPtr==NULL){
            throw std::runtime_error("failed to allocate the memroy for dataBlock");
        }
    }
    ~DataBlockByRawMem() {
        std::cout << "destructor of DataBlock is called" <<std::endl;
        free(m_rawMemPtr);
    }
};


//do not divide by type, divide by inner implementation mechanism
template <typename T>
struct DataObjectByRawMem : public DataObjectInterface
{
    
    //TODO, add more method for initializing, only initialize the data meta, without the data adding operation
    DataObjectByRawMem(DataMeta dataMeta,
               size_t blockID,
               std::vector<T> &dataArray) : DataObjectInterface(dataMeta)
    {
        std::cout << "init the data object by vector, blockid is " << blockID << std::endl;
        //assume the data is sorted by blockid, first is block0 secnd is block1...
        //TODO check here
        DataBlockByRawMem*dataBlockPtr = new DataBlockByRawMem(blockID, sizeof(T)*dataArray.size(), dataArray.data());
        dataBlockMap[blockID]=dataBlockPtr;
/*
        std::cout << "check the value at the end of DataObject"<<std::endl;
        int* rawdata = dataBlockPtr->DataBlockValue.data();
        for(int i=0;i<10;i++){
            std::cout << "index " << i << " value "<<*rawdata <<std::endl;
            rawdata++;
        }
*/

    }
        
    DataObjectByRawMem(DataMeta dataMeta,
               size_t blockID,
               void* rawDataPtr) : DataObjectInterface(dataMeta)
    {
        std::cout << "init the data object by raw pointer, blockid is " << blockID << std::endl;
        size_t dataMallocSize = dataMeta.getDataMallocSize();
        if(dataMallocSize==0){
            throw std::runtime_error("failed to putData, dataMallocSize is 0");
        }
        putData(blockID, dataMallocSize, rawDataPtr);
    }




    int getData(int blockID, void* &dataContainer)
    {
        std::cout << "implement the getData by DataObject" << std::endl;
        //dataContainer = (void*)(dataBlockMap[blockID]->DataBlockValue.data());
        dataContainer = (void*)(dataBlockMap[blockID]->m_rawMemPtr);
       /*
        std::cout << "check value at getData " <<std::endl;
        int * rawData = (int*)dataContainer;
        for(int i=0;i<10;i++){
            std::cout << "index " << i<< "value "<< *rawData << std::endl;
            rawData++;
        }
        */   
        return 0;      
    };

    //this method should be called when there is no block data
    int putData(int blockID, size_t dataMallocSize,  void *dataContainer){

        

        DataBlockByRawMem*dataBlockPtr = new DataBlockByRawMem(blockID, dataMallocSize, dataContainer);
        dataBlockMap[blockID]=dataBlockPtr;
    }


    bool ifBlockIdExist(size_t blockID) {
        if(dataBlockMap.find(blockID)!=dataBlockMap.end()){
            return true;
        }
        return false;
    }

    ~DataObjectByRawMem(){};

private:
    //this is used to index the blockData by block ID
    //TODO add lock
    //std::map<size_t,DataBlock<T>*> dataBlockMap;
    std::map<size_t,DataBlockByRawMem*> dataBlockMap;
};

template class DataObjectByRawMem<int>;
template class DataObjectByRawMem<double>;

#endif