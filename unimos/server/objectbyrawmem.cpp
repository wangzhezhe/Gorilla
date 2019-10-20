#include "objectbyrawmem.h"
#include <stdio.h>

//it is hard to set the template constructor in non template class
//just use explicit dataArray type in this case

DataObjectByRawMem::DataObjectByRawMem(DataMeta dataMeta,
                                       size_t blockID,
                                       void *rawDataPtr) : DataObjectInterface(dataMeta)
{
    //std::cout << "init the data object by raw pointer, blockid is " << blockID << std::endl;
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
    //std::cout << "init the data object by vector, blockid is " << blockID << std::endl;
    //assume the data is sorted by blockid, first is block0 secnd is block1...
    //TODO check here

    this->m_dataMeta = dataMeta;
    DataBlockByRawMem *dataBlockPtr = new DataBlockByRawMem(sizeof(T) * dataArray.size(), dataArray.data());
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

    DataBlockByRawMem *dataBlockPtr = new DataBlockByRawMem(dataMallocSize, dataContainer);
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

int DataObjectByRawMem::getDataRegion(size_t blockID, std::array<size_t, 3> baseOffset, std::array<size_t, 3> regionShape, void *&dataContainer)
{
    if (regionShape[0] == 0)
    {
        throw std::runtime_error("the regionShape for the x axis could not be 0");
    }

    if (regionShape[0] == 0 && regionShape[1] == 0 && regionShape[2] == 0)
    {
        throw std::runtime_error("the regionShape could not be [0,0,0]");
    }

    //the lower bound is always 0 for this case
    std::array<size_t, 3> dataShape = this->m_dataMeta.m_shape;
    size_t dimention = this->m_dataMeta.getValidDimention();
    for (int i = 0; i < dimention; i++)
    {
        if (baseOffset[i] >= dataShape[i] || baseOffset[i] + regionShape[i] - 1 >= dataShape[i])
        {
            std::cout << "base offset " << baseOffset[0] << "," << baseOffset[1] << "," << baseOffset[2]
                      << "data shape " << dataShape[0] << "," << dataShape[1] << "," << dataShape[2] << std::endl;
            throw std::runtime_error("the baseoffset and the (baseOffset[i] + regionShape[i]) should be smaller than the data shape");
            return -1;
        }
    }

    //allocate new space
    size_t entryNum = 1;
    for (int i = 2; i >= 0; i--)
    {
        if (regionShape[i] != 0)
        {
            entryNum = entryNum * regionShape[i];
        }
    }

    size_t elemSize = this->m_dataMeta.m_elemSize;

    char *dataBlockPtr = (char *)malloc(elemSize * entryNum);

    //the pointer to source memory

    DataBlockByRawMem *sourceMemPtr = dataBlockMap[blockID];

    int localIndex = 0;
    //get the data with current blockID, assume the memory layout is the xyz

    /* debug use
    std::cout << "check data shape" << dataShape[0] << "," << dataShape[1] << "," << dataShape[2] << std::endl;

    std::cout << "check data base offset" << baseOffset[0] << "," << baseOffset[1] << "," << baseOffset[2] << std::endl;

    std::cout << "check data query shape" << regionShape[0] << "," << regionShape[1] << "," << regionShape[2] << std::endl;

    std::cout << "check raw value " << std::endl;

    for (int i = 0; i < dataShape[0]; i++)
    {
        std::cout << "index i " << i << "value " << *(double *)sourceMemPtr->m_rawMemPtr + i << std::endl;
    }
    */

    for (int z = 0; z <= dataShape[2]; z++)
    {
        std::cout << "debug z " << z << std::endl;
        if (z == 0 || (z >= baseOffset[2] && z <= baseOffset[2] + regionShape[2] - 1))
        {
            for (int y = 0; y <= dataShape[1]; y++)
            {
                if (y == 0 || (y >= baseOffset[1] && y <= baseOffset[1] + regionShape[1] - 1))
                {
                    std::cout << "debug y " << y << std::endl;

                    //TODO, this part can be copied at once
                    for (int x = 0; x <= dataShape[0]; x++)
                    {
                        if (x >= baseOffset[0] && x <= baseOffset[0] + regionShape[0] - 1)
                        {

                            int index = z * dataShape[1] * dataShape[0] + y * dataShape[0] + x;
                            //copy the dataBlockMap[blockID][index] into  the new allocated space
                            //copy one row one time
                            //use char as the temporary solution
                            std::cout << "debug global index " << index << " local index " << localIndex << std::endl;
                            memcpy(dataBlockPtr + localIndex * elemSize, (char *)sourceMemPtr->m_rawMemPtr + index * elemSize, elemSize);
                            //localIndex += regionShape[0] * elemSize;
                            localIndex = localIndex + 1;
                        }
                    }
                }
            }
        }

        //check the results
        //std::cout << "debug entryNum " << entryNum << std::endl;

        //the fee pointer should be called after return the value to the client
        dataContainer = (void *)dataBlockPtr;

        return 0;
    }
}

template void DataObjectByRawMem::setDataObjectByVector<int>(DataMeta dataMeta, size_t blockID, std::vector<int> &dataArray);
template void DataObjectByRawMem::setDataObjectByVector<double>(DataMeta dataMeta, size_t blockID, std::vector<double> &dataArray);