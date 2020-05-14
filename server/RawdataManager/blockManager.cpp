

#include "blockManager.h"
//this should be put at the .cpp file instead of the .h file
#include "rawmemobj/rawmemobj.h"
#include "vtkobj/vtkobj.hpp"
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
  else if (blockSummary.m_drivertype.compare(DRIVERTYPE_VTKDATASET) == 0)
  {
    DataBlockInterface *dbi = new VTKObj(blockSummary);
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
                                             size_t dims,
                                             std::array<int, 3> subregionlb,
                                             std::array<int, 3> subregionub,
                                             void *&dataContainer)
{

  this->m_DataBlockMapMutex.lock();
  BlockSummary bs = DataBlockMap[blockID]->getDataSubregion(dims, subregionlb, subregionub, dataContainer);
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

bool BlockManager::checkDataExistance(std::string blockID)
{
  if (this->DataBlockMap.find(blockID) != this->DataBlockMap.end())
  {
    return true;
  }
  else
  {
    return false;
  }
}

int BlockManager::eraseBlock(std::string blockID)
{
  this->m_DataBlockMapMutex.lock();
  DataBlockMap[blockID]->eraseData();
  DataBlockMap.erase(blockID);
  this->m_DataBlockMapMutex.unlock();
  return 0;
}