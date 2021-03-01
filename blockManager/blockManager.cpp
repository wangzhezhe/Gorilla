

#include "blockManager.h"
// this should be put at the .cpp file instead of the .h file
#include "fileobj/fileobj.h"
#include "rawmemobj/rawmemobj.h"
#include "vtkmemexplicit/vtkmemexplicit.h"
#include "vtkmemobj/vtkmemobj.h"
#include <iostream>

namespace GORILLA
{

// for the rawfile, the client such as analytics maybe call getblock without
// call the put firstly, therefore, it also needs to cache the id
// return instance if it is not null
DataBlockInterface* BlockManager::getBlockHandle(std::string blockID, int backend)
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
      if (backend == BACKEND::MEM)
      {
        DataBlockInterface* dbi = new RawMemObj(blockID.data());
        // assign block id to the new allocated handle
        // this is set when init the interface
        // strcpy(dbi->m_blockid, blockID.data());
        this->DataBlockMap[blockID] = dbi;
        handle = dbi;
      }
      else if (backend == BACKEND::FILE)
      {
        DataBlockInterface* dbi = new FileObj(blockID.data());
        // assign block id to the new allocated handle
        strcpy(dbi->m_blockid, blockID.data());
        this->DataBlockMap[blockID] = dbi;
        handle = dbi;
      }
      else if (backend == BACKEND::MEMVTKPTR)
      {
        DataBlockInterface* dbi = new VTKMemObj(blockID.data());
        // strcpy(dbi->m_blockid, blockID.data());
        this->DataBlockMap[blockID] = dbi;
        handle = dbi;
      }
      else if (backend == BACKEND::MEMVTKEXPLICIT)
      {
        DataBlockInterface* dbi = new VTKMemExplicitObj(blockID.data());
        // strcpy(dbi->m_blockid, blockID.data());
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

int BlockManager::putArray(
  BlockSummary& blockSummary, ArraySummary& as, int backend, void* dataPointer)
{

  DataBlockInterface* handle = getBlockHandle(std::string(blockSummary.m_blockid), backend);

  // if the block summary exist
  if (strcmp(handle->m_blockSummary.m_blockid, "") == 0)
  {
    // this is empty and not initilized
    // update the block summary
    handle->m_blockSummary = blockSummary;
  }

  // add the arraySummary into the block
  handle->m_blockSummary.addArraySummary(as);

  int status = handle->putArray(as, dataPointer);
  return status;
}

ArraySummary BlockManager::getArray(
  std::string blockName, std::string arrayName, int backend, void*& dataPointer)
{

  if (this->DataBlockMap.count(blockName) == 0)
  {
    throw std::runtime_error("the block summary not exist in DataBlockMap");
  }

  // exist
  this->m_DataBlockMapMutex.lock();
  BlockSummary bs = this->DataBlockMap[blockName]->m_blockSummary;
  this->m_DataBlockMapMutex.unlock();

  // todo check the length of the arrayname
  ArraySummary as = bs.getArraySummary(arrayName.data());

  DataBlockInterface* handle = getBlockHandle(blockName, backend);
  // get summary from the block array
  int status = handle->getArray(as, dataPointer);
  if(status!=0){
    throw std::runtime_error("failed to get array with arrayName");
  }
  return as;
}

int BlockManager::putBlock(BlockSummary& blockSummary, int backend, void* dataPointer)
{
  DataBlockInterface* handle = getBlockHandle(std::string(blockSummary.m_blockid), backend);
  // assign actual value to block summay, this information will be used for data put operation
  handle->m_blockSummary = blockSummary;
  // check if the copy is successfuly
  // it might be the shallow copy? since we used typedef
  if (handle->m_blockSummary.equals(blockSummary) == false)
  {
    throw std::runtime_error("copy failed");
    return -1;
  }
  int status = handle->putData(dataPointer);
  return status;
}

bool BlockManager::blockSummaryExist(std::string blockID)
{
  this->m_DataBlockMapMutex.lock();
  int count = this->DataBlockMap.count(blockID);
  this->m_DataBlockMapMutex.unlock();

  if (count == 0)
  {
    return false;
  }
  return true;
}

// This only works for mem obj currently
BlockSummary BlockManager::getBlockSummary(std::string blockID)
{
  // sometimes, we execute the get direaclty for the file driver, in this case, the get block
  // summary is invalid just return the empty block summary and the user need to decide if it is
  // empty and further using

  if (this->DataBlockMap.count(blockID) == 0)
  {
    // throw std::runtime_error("the block summary not exist in DataBlockMap");
    // return an empty blocksummary
    // the block id is empty
    // the caller need to check if the blocksummary is a valid one
    return BlockSummary();
  }

  // exist
  this->m_DataBlockMapMutex.lock();
  BlockSummary bs = this->DataBlockMap[blockID]->m_blockSummary;
  this->m_DataBlockMapMutex.unlock();
  return bs;
}

// this might be called with without put operation firstly
BlockSummary BlockManager::getBlock(std::string blockID, int backend, void*& dataContainer)
{
  // if it is the backend is VTK
  // the data put should be executed firstly
  if (backend == MEMVTKPTR || backend == MEMVTKEXPLICIT || backend == MEM)
  {
    //check if the data exist
    this->m_DataBlockMapMutex.lock();
    int count = this->DataBlockMap.count(blockID);
    this->m_DataBlockMapMutex.unlock();
    if (count == 0)
    {
      // return an empty block summary if it is empty
      return BlockSummary();
    }
  }
  //for the file backend, it is ok if the data does not exist previously
  //we may need to load data from the file
  DataBlockInterface* handle = getBlockHandle(blockID, backend);
  BlockSummary bs = handle->getData(dataContainer);
  return bs;
}

// this function is only useful for the image data
// since we reshape the returned data
BlockSummary BlockManager::getBlockSubregion(std::string blockID, int backend, size_t dims,
  std::array<int, 3> subregionlb, std::array<int, 3> subregionub, void*& dataContainer)
{

  DataBlockInterface* handle = getBlockHandle(blockID, backend);
  BlockSummary bs =
    DataBlockMap[blockID]->getDataSubregion(dims, subregionlb, subregionub, dataContainer);
  return bs;
}
/* deprecated, it should be the getArraySize in specific block
size_t BlockManager::getBlockSize(std::string blockID, int backend)
{
  if (backend == BACKEND::FILE)
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
*/
bool BlockManager::checkDataExistance(std::string blockID, int backend)
{
  if (backend == BACKEND::FILE)
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

int BlockManager::eraseBlock(std::string blockID, int backend)
{
  DataBlockInterface* handle = getBlockHandle(blockID, backend);
  this->m_DataBlockMapMutex.unlock();
  handle->eraseData();
  // the destructor is called here
  // the data will be remoevd when destructor is called
  delete handle;
  DataBlockMap.erase(blockID);
  this->m_DataBlockMapMutex.unlock();
  return 0;
}

}