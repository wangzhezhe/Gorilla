

#ifndef RAWMEMOBJ_H
#define RAWMEMOBJ_H

#include <blockManager/blockManager.h>
#include <iostream>

struct RawMemObj : public DataBlockInterface
{

  RawMemObj(const char* blockid) : DataBlockInterface(blockid){
                                              //std::cout << "RawMemObj is initialised" << std::endl;
                                          };

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
    return this->m_rawMemPtr;
  };

  void *m_rawMemPtr = NULL;

  ~RawMemObj()
  {
    if (m_rawMemPtr != NULL)
    {
      free(m_rawMemPtr);
    }
  };
};

#endif