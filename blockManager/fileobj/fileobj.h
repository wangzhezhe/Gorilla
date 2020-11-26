#ifndef FILEOBJ_H
#define FILEOBJ_H

#include <blockManager/blockManager.h>
#include <iostream>


//this can be the wrapper for the file based obj
//such as the burst buffer or the parallel file system

struct FileObj : public DataBlockInterface
{

  FileObj(const char* blockid) : DataBlockInterface(blockid){};
  
  BlockSummary getData(void *&dataContainer);

  // put data into coresponding data structure for specific implementation
  int putData(void *dataSourcePtr);

  int eraseData();

  BlockSummary getDataSubregion(
      size_t dims,
      std::array<int, 3> subregionlb,
      std::array<int, 3> subregionub,
      void *&dataContainer);

  void* getrawMemPtr(){
    //since there is not raw mem ptr for file based IO
    return NULL;
  };

  void *m_rawMemPtr = NULL;

  ~FileObj()
  {
    if (m_rawMemPtr != NULL)
    {
      free(m_rawMemPtr);
    }
  };
};

#endif