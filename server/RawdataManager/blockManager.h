

#ifndef RAWDATAMANAGER_H
#define RAWDATAMANAGER_H

#include "../../commondata/metadata.h"

//#include "filterManager.h"
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <thallium.hpp>

namespace tl = thallium;

// the abstraction that manage the storage for one data block
struct DataBlockInterface {
  // the data structure of block summary is same for different data block
  // but the storage part is different
  BlockSummary m_blockSummary;

  DataBlockInterface(BlockSummary &blockSummary)
      : m_blockSummary(blockSummary) {
    std::cout << "DataBlockInterface is initialized" << std::endl;
  };

  virtual BlockSummary getData(void *&dataContainer) = 0;

  // put data into coresponding data structure for specific implementation
  virtual int putData(void *dataSourcePtr) = 0;

  virtual BlockSummary getDataSubregion(std::array<size_t, 3> subregionlb,
                                        std::array<size_t, 3> subregionub,
                                        void *&dataContainer) = 0;

  // TODO generate RawDataEndpointFromBlockSummary

  ~DataBlockInterface(){};
};

class BlockManager {
public:
  // constructor, initilize the thread pool
  // and start to check the thread pool after initialization
  BlockManager(){};

  // put/get data by Object
  // parse the interface by the defination

  int putBlock(size_t blockID,
               BlockSummary &m_blockSummary, void *dataPointer);

  // this function can be called when the blockid is accuired from the metadata
  // service this is just get the summary information of block data
  size_t getBlockSize(size_t blockID);
  BlockSummary getBlockSummary(size_t blockID);
  BlockSummary getBlock(size_t blockID, void *&dataContainer);
  BlockSummary getBlockSubregion(size_t blockID,
                                   std::array<size_t, 3> subregionlb,
                                   std::array<size_t, 3> subregionub,
                                   void *&dataContainer);

  // execute the data checking service
  // void doChecking(DataMeta &dataMeta, size_t blockID);
  // void loadFilterManager(FilterManager*
  // fmanager){m_filterManager=fmanager;return;}

  // add thread pool here, after the data put, get a thread from the thread pool
  // to check filtered data there is a filter List for every block

  ~BlockManager() {}

private:
  tl::mutex m_DataBlockMapMutex;
  // map the block id into the
  std::map<size_t, DataBlockInterface *> DataBlockMap;
  int checkDataExistance(size_t blockID);
};

#endif