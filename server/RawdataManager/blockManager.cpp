

#include "blockManager.h"
#include "rawmemobj/rawmemobj.h"
#include <iostream>

BlockManager::BlockManager(size_t poolSize) {
  // this->m_threadPool = new ThreadPool(poolSize);
  this->m_threadPool = new ArgoThreadPool(poolSize);
  return;
}

int BlockManager::putBlock(std::string driverType, size_t blockID,
                           BlockSummary &blockSummary, void *dataPointer) {

  if (driverType.compare(DRIVERTYPE_RAWMEM) == 0) {

    DataBlockInterface *dbi = new RawMemObj(blockSummary);
    dbi->putData(dataPointer);
    this->m_DataBlockMapMutex.lock();
    DataBlockMap[blockID] = dbi;
    this->m_DataBlockMapMutex.unlock();

  } else {
    throw std::runtime_error("unsuported driver type " + driverType);
  }
}

BlockSummary BlockManager::getBlockSummary(size_t blockID) {
  this->m_DataBlockMapMutex.lock();
  BlockSummary bs = this->DataBlockMap[blockID]->m_blockSummary;
  this->m_DataBlockMapMutex.unlock();
  return bs;
}

BlockSummary BlockManager::getBlock(size_t blockID, void *&dataContainer) {
  this->m_DataBlockMapMutex.lock();
  BlockSummary bs = DataBlockMap[blockID]->getData(dataContainer);
  this->m_DataBlockMapMutex.unlock();
  return bs;
}

BlockSummary BlockManager::getBlockSubregion(size_t blockID,
                                             std::array<size_t, 3> subregionlb,
                                             std::array<size_t, 3> subregionub,
                                             void *&dataContainer) {

  this->m_DataBlockMapMutex.lock();
  BlockSummary bs = DataBlockMap[blockID]->getDataSubregion(
      subregionlb, subregionub, dataContainer);
  this->m_DataBlockMapMutex.unlock();

  return BlockSummary();
}