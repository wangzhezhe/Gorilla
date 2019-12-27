

#include "blockManager.h"
#include "rawmemobj/rawmemobj.h"
#include <iostream>


BlockManager::BlockManager(size_t poolSize)
{
    //this->m_threadPool = new ThreadPool(poolSize);
    this->m_threadPool = new ArgoThreadPool(poolSize);
    return;
}

int BlockManager::putBlock(std::string driverType, size_t blockID,
                           BlockSummary &blockSummary, void *dataPointer) {

  if (driverType.compare(DRIVERTYPE_RAWMEM)==0) {

    DataBlockInterface * dbi = new RawMemObj(blockSummary);
    dbi->putData(dataPointer);
    this->m_DataBlockMapMutex.lock();
    DataBlockMap[blockID] = dbi;
    this->m_DataBlockMapMutex.unlock();

  } else {
    throw std::runtime_error("unsuported driver type " + driverType);
  }
}

BlockSummary BlockManager::getBlockSummary(std::string driverType,
                                           size_t blockID) {return BlockSummary();}

BlockSummary BlockManager::getBlock(std::string driverType, size_t blockID,
                                    void *&rawData) {return BlockSummary();}
BlockSummary BlockManager::getSubregionalBlock(
    std::string driverType, size_t blockID, std::array<size_t, 3> baseOffset,
    std::array<size_t, 3> regionShape, void *&rawData) {
        return BlockSummary();
    }