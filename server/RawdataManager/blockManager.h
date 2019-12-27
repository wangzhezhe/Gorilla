

#ifndef RAWDATAMANAGER_H
#define RAWDATAMANAGER_H

#include "../../commondata/metadata.h"
#include "../../utils/ArgothreadPool.h"
//#include "filterManager.h"
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace tl = thallium;


// the abstraction that manage the storage for one data block
struct DataBlockInterface {
  // the data structure of block summary is same for different data block
  // but the storage part is different
  BlockSummary m_blockSummary;

  DataBlockInterface(BlockSummary& blockSummary)
      : m_blockSummary(blockSummary){std::cout << "DataBlockInterface is initialized"  << std::endl;};

  virtual BlockSummary getData(void *&dataContainer) = 0;
  
  //put data into coresponding data structure for specific implementation
  virtual int putData(void *dataContainer) = 0;

  virtual BlockSummary getDataRegion(std::array<size_t, 3> baseOffset,
                                     std::array<size_t, 3> regionShape,
                                     void *&rawData) = 0;

  ~DataBlockInterface(){};
};



class BlockManager {
public:
  // constructor, initilize the thread pool
  // and start to check the thread pool after initialization
  BlockManager(size_t poolSize);

  // put/get data by Object
  // parse the interface by the defination

  int putBlock(std::string driverType, size_t blockID, BlockSummary& m_blockSummary, void *dataPointer);

  // this function can be called when the blockid is accuired from the metadata
  // service this is just get the summary information of block data
  BlockSummary getBlockSummary(std::string driverType, size_t blockID);
  BlockSummary getBlock(std::string driverType, size_t blockID, void *&rawData);
  BlockSummary getSubregionalBlock(std::string driverType, size_t blockID,
                                   std::array<size_t, 3> baseOffset,
                                   std::array<size_t, 3> regionShape,
                                   void *&rawData);

  // execute the data checking service
  // void doChecking(DataMeta &dataMeta, size_t blockID);
  // void loadFilterManager(FilterManager*
  // fmanager){m_filterManager=fmanager;return;}

  // add thread pool here, after the data put, get a thread from the thread pool
  // to check filtered data there is a filter List for every block

  ~BlockManager() { delete this->m_threadPool; }

private:
  tl::mutex m_DataBlockMapMutex;
  // map the block id into the
  std::map<size_t, DataBlockInterface *> DataBlockMap;
  int checkDataExistance(size_t blockID);

  // the thread pool to check the constraints
  // ThreadPool* m_threadPool=NULL, initilzied at the constructor
  ArgoThreadPool *m_threadPool = NULL;
  // FilterManager
  // FilterManager *m_filterManager = NULL;
};

#endif