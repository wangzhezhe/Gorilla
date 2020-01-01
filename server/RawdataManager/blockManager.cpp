

#include "blockManager.h"
#include "rawmemobj/rawmemobj.h"
#include <iostream>

int BlockManager::putBlock(std::string blockID,
                           BlockSummary &blockSummary, void *dataPointer)
{

  if (blockSummary.m_drivertype.compare(DRIVERTYPE_RAWMEM) == 0)
  {

    DataBlockInterface *dbi = new RawMemObj(blockSummary);
    dbi->putData(dataPointer);
    this->m_DataBlockMapMutex.lock();
    DataBlockMap[blockID] = dbi;
    this->m_DataBlockMapMutex.unlock();
  }
  else
  {
    throw std::runtime_error("unsuported driver type " + blockSummary.m_drivertype);
  }
  return 0;
}

BlockSummary BlockManager::getBlockSummary(std::string blockID)
{
  this->m_DataBlockMapMutex.lock();
  BlockSummary bs = this->DataBlockMap[blockID]->m_blockSummary;
  this->m_DataBlockMapMutex.unlock();
  return bs;
}

BlockSummary BlockManager::getBlock(std::string blockID, void *&dataContainer)
{
  this->m_DataBlockMapMutex.lock();
  BlockSummary bs = DataBlockMap[blockID]->getData(dataContainer);
  this->m_DataBlockMapMutex.unlock();
  return bs;
}

BlockSummary BlockManager::getBlockSubregion(std::string blockID,
                                             std::array<size_t, 3> subregionlb,
                                             std::array<size_t, 3> subregionub,
                                             void *&dataContainer)
{

  this->m_DataBlockMapMutex.lock();
  BlockSummary bs = DataBlockMap[blockID]->getDataSubregion(
      subregionlb, subregionub, dataContainer);
  this->m_DataBlockMapMutex.unlock();

  return BlockSummary();
}

size_t BlockManager::getBlockSize(std::string blockID)
{
  this->m_DataBlockMapMutex.lock();
  BlockSummary bs = DataBlockMap[blockID]->m_blockSummary;
  this->m_DataBlockMapMutex.unlock();
  return bs.m_elemNum * bs.m_elemSize;
}
