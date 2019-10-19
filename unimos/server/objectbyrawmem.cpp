#include "objectbyrawmem.h"

//it is hard to set the template constructor in non template class
//just use explicit dataArray type in this case

DataObjectByRawMem::DataObjectByRawMem(DataMeta dataMeta,
                                       size_t blockID,
                                       void *rawDataPtr) : DataObjectInterface(dataMeta)
{
    std::cout << "init the data object by raw pointer, blockid is " << blockID << std::endl;
    size_t dataMallocSize = dataMeta.getDataMallocSize();
    if (dataMallocSize == 0)
    {
        throw std::runtime_error("failed to putData, dataMallocSize is 0");
    }
    putData(blockID, dataMallocSize, rawDataPtr);
}


template <typename T>
void DataObjectByRawMem::setDataObjectByVector(DataMeta dataMeta,
                                          size_t blockID,
                                          std::vector<T> &dataArray)
{
    std::cout << "init the data object by vector, blockid is " << blockID << std::endl;
    //assume the data is sorted by blockid, first is block0 secnd is block1...
    //TODO check here

    this->m_dataMeta=dataMeta;
    DataBlockByRawMem *dataBlockPtr = new DataBlockByRawMem(blockID, sizeof(T) * dataArray.size(), dataArray.data());
    dataBlockMap[blockID] = dataBlockPtr;
    /*
        std::cout << "check the value at the end of DataObject"<<std::endl;
        int* rawdata = dataBlockPtr->DataBlockValue.data();
        for(int i=0;i<10;i++){
            std::cout << "index " << i << " value "<<*rawdata <<std::endl;
            rawdata++;
        }

    */

}

int DataObjectByRawMem::getData(int blockID, void *&dataContainer)
{
    std::cout << "implement the getData by DataObject" << std::endl;
    //dataContainer = (void*)(dataBlockMap[blockID]->DataBlockValue.data());
    dataContainer = (void *)(dataBlockMap[blockID]->m_rawMemPtr);
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
int DataObjectByRawMem::putData(int blockID, size_t dataMallocSize, void *dataContainer)
{

    DataBlockByRawMem *dataBlockPtr = new DataBlockByRawMem(blockID, dataMallocSize, dataContainer);
    dataBlockMap[blockID] = dataBlockPtr;
}


bool DataObjectByRawMem::ifBlockIdExist(size_t blockID)
{
    if (dataBlockMap.find(blockID) != dataBlockMap.end())
    {
        return true;
    }
    return false;
}


template void DataObjectByRawMem::setDataObjectByVector<int>(DataMeta dataMeta,size_t blockID,std::vector<int> &dataArray);
template void DataObjectByRawMem::setDataObjectByVector<double>(DataMeta dataMeta,size_t blockID,std::vector<double> &dataArray);