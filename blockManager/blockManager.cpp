

#include "blockManager.h"
// this should be put at the .cpp file instead of the .h file
#include "fileobj/fileobj.h"
#include "rawmemobj/rawmemobj.h"
#include <iostream>

// for the rawfile, the client such as analytics maybe call getblock without
// call the put firstly, therefore, it also needs to cache the id
// return instance if it is not null
DataBlockInterface* BlockManager::getBlockHandle(std::string blockID, std::string backend)
{
  if (blockID.compare("") == 0)
  {
    throw std::runtime_error("blockID should not but null in blockSummary");
    return NULL;
  }

  DataBlockInterface* handle = NULL;
  {
    std::lock_guard<tl::mutex> lck(this->m_DataBlockMapMutex);
    // cache block if it is not in the map
    // if the block exist, return
    if (this->DataBlockMap.count(blockID) != 0)
    {
      handle = this->DataBlockMap[blockID];
    }
    else
    {
      // this is the new block id
      if (backend.compare(BACKENDMEM) == 0)
      {
        DataBlockInterface* dbi = new RawMemObj(blockID.data());
        // assign block id to the new allocated handle
        strcpy(dbi->m_blockid, blockID.data());
        this->DataBlockMap[blockID] = dbi;
        handle = dbi;
      }
      else if (backend.compare(BACKENDFILE) == 0)
      {
        DataBlockInterface* dbi = new FileObj(blockID.data());
        // assign block id to the new allocated handle
        strcpy(dbi->m_blockid, blockID.data());
        this->DataBlockMap[blockID] = dbi;
        handle = dbi;
      }
      else
      {
        throw std::runtime_error("unsupported block type to get block handle");
      }
    }
  }
  return handle;
}

int BlockManager::putBlock(BlockSummary& blockSummary, std::string backend, void* dataPointer)
{
  DataBlockInterface* handle = getBlockHandle(std::string(blockSummary.m_blockid), backend);
  // assign actual value to block summay, this information will be used for data put operation
  handle->m_blockSummary = blockSummary;
  int status = handle->putData(dataPointer);
  return status;
}

// This only works for mem obj currently
BlockSummary BlockManager::getBlockSummary(std::string blockID)
{
  // sometimes, we execute the get direaclty for the file driver, in this case, the get block
  // summary is invalid just return the empty block summary and the user need to decide if it is
  // empty and further using

  if (this->DataBlockMap.count(blockID) == 0)
  {
    throw std::runtime_error("the block summary not exist in DataBlockMap");
  }

  // exist
  this->m_DataBlockMapMutex.lock();
  BlockSummary bs = this->DataBlockMap[blockID]->m_blockSummary;
  this->m_DataBlockMapMutex.unlock();
  return bs;
}

// this might be called with without put operation firstly
BlockSummary BlockManager::getBlock(std::string blockID, std::string backend, void*& dataContainer)
{
  DataBlockInterface* handle = getBlockHandle(blockID, backend);
  BlockSummary bs = handle->getData(dataContainer);
  return bs;
}

// this function is only useful for the image data
// since we reshape the returned data
BlockSummary BlockManager::getBlockSubregion(std::string blockID, std::string backend, size_t dims,
  std::array<int, 3> subregionlb, std::array<int, 3> subregionub, void*& dataContainer)
{

  DataBlockInterface* handle = getBlockHandle(blockID, backend);
  BlockSummary bs =
    DataBlockMap[blockID]->getDataSubregion(dims, subregionlb, subregionub, dataContainer);
  return bs;
}

size_t BlockManager::getBlockSize(std::string blockID, std::string backend)
{
  if (backend == BACKENDFILE)
  {
    throw std::runtime_error("not support getBlockSize for file backend currently");
  }
  if (this->DataBlockMap.count(blockID) == 0)
  {
    throw std::runtime_error("block not exist");
  }
  this->m_DataBlockMapMutex.lock();
  BlockSummary bs = this->DataBlockMap[blockID]->m_blockSummary;
  this->m_DataBlockMapMutex.unlock();
  return bs.m_elemNum * bs.m_elemSize;
}

bool BlockManager::checkDataExistance(std::string blockID, std::string backend)
{
  if (backend == BACKENDFILE)
  {
    throw std::runtime_error("not support checkDataExistance for file backend currently");
  }
  if (this->DataBlockMap.find(blockID) != this->DataBlockMap.end())
  {
    return true;
  }
  else
  {
    return false;
  }
}

int BlockManager::eraseBlock(std::string blockID, std::string backend)
{
  DataBlockInterface* handle = getBlockHandle(blockID, backend);
  handle->eraseData();
  this->m_DataBlockMapMutex.unlock();
  DataBlockMap.erase(blockID);
  this->m_DataBlockMapMutex.unlock();
  return 0;
}