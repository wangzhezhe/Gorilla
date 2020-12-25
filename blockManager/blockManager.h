

#ifndef RAWDATAMANAGER_H
#define RAWDATAMANAGER_H

#include <commondata/metadata.h>

//#include "filterManager.h"
#include <sys/stat.h>
#include <sys/types.h>

#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <thallium.hpp>
#include <vector>

namespace tl = thallium;
namespace GORILLA
{

const std::string FILEOBJ_PREXIX = "./filedataobj";

// the abstraction that manage the storage for one data block
struct DataBlockInterface
{
  // the data structure of block summary is same for different data block
  // but the storage part is different
  // the block id is also needed, for file based data block, this information is
  // needed
  BlOCKID m_blockid;

  BlockSummary m_blockSummary;
  // this might be used by the file init
  // we only know the blockid for the file driver
  // for different implementaiton, the array is in different format
  // for the most general form such as the data block contains mutiple array
  // there should a map that index the array name into the data pointer

  DataBlockInterface(const char* blockid)
  {
    if (strlen(blockid) >= STRLENLONG)
    {
      throw std::runtime_error("long blockid");
    }
    strcpy(m_blockid, blockid);
  };
  // deprecated, the block summary is supposed to be set separately
  // by the caller
  /*
  DataBlockInterface(BlockSummary& blockSummary)
    : m_blockSummary(blockSummary)
  {
    strcpy(this->m_blockid, blockSummary.m_blockid);
  };
  */
  // maybe add array name here???
  virtual BlockSummary getData(void*& dataContainer) = 0;

  // put data into coresponding data structure for specific implementation
  // when we use the putdata, there is one array in data block
  virtual int putData(void* dataSourcePtr) = 0;

  virtual int eraseData() = 0;

  virtual BlockSummary getDataSubregion(size_t dims, std::array<int, 3> subregionlb,
    std::array<int, 3> subregionub, void*& dataContainer) = 0;

  virtual void* getrawMemPtr() = 0;

  virtual int putArray(ArraySummary& as, void* dataSourcePtr) = 0;

  virtual int getArray(ArraySummary& as, void*& dataContainer) = 0;

  // both the destructor of the parent class and child class should labeled by virtual
  // https://www.quantstart.com/articles/C-Virtual-Destructors-How-to-Avoid-Memory-Leaks/
  // otherwise base class destructor will be called instead of the derived class destructor
  virtual ~DataBlockInterface(){
    // std::cout << "delete DataBlockInterface" << std::endl;
  };
};

class BlockManager
{
public:
  // constructor, initilize the thread pool
  // and start to check the thread pool after initialization
  BlockManager(int rank = 0)
  {
    // create the dir if it is not exist
    if (rank == 0)
    {
      struct stat buffer;
      if (stat(FILEOBJ_PREXIX.c_str(), &buffer) != 0)
      {
        // dir not exist , create
        if (mkdir(FILEOBJ_PREXIX.data(), 0700) != 0)
        {
          throw std::runtime_error("failed to create the objects dir for the block manager");
        }
      }
    }
  };
  DataBlockInterface* getBlockHandle(std::string blockID, int backend);
  // put/get data by Object
  // parse the interface by the defination
  int putBlock(BlockSummary& blockSummary, int backend, void* dataPointer);

  int eraseBlock(std::string blockID, int backend);

  // this function can be called when the blockid is accuired from the metadata
  // service this is just get the summary information of block data
  size_t getBlockSize(std::string blockID, int backend);
  BlockSummary getBlockSummary(std::string blockID);
  BlockSummary getBlock(std::string blockID, int backend, void*& dataContainer);
  BlockSummary getBlockSubregion(std::string blockID, int backend, size_t dims,
    std::array<int, 3> subregionlb, std::array<int, 3> subregionub, void*& dataContainer);

  int putArray(BlockSummary& blockSummary, ArraySummary& as, int backend, void* dataPointer);
  ArraySummary getArray(std::string blockName, std::string arrayName, int backend, void*& dataPointer);
  bool blockSummaryExist(std::string blockID);

  // execute the data checking service
  // void doChecking(DataMeta &dataMeta, size_t blockID);
  // void loadFilterManager(FilterManager*
  // fmanager){m_filterManager=fmanager;return;}

  // add thread pool here, after the data put, get a thread from the thread pool
  // to check filtered data there is a filter List for every block
  bool checkDataExistance(std::string blockID, int backend);

  // void* getBlockPointer(std::string blockID);

  ~BlockManager() {};

  // private:
  tl::mutex m_DataBlockMapMutex;
  // map the block id into the interface
  std::map<std::string, DataBlockInterface*> DataBlockMap;

  // TODO add accounting information, how much resource is avalible for the
  // current process
};
}
#endif