

#ifndef RAWMEMOBJ_H
#define RAWMEMOBJ_H

#include <blockManager/blockManager.h>
#include <iostream>

namespace GORILLA
{

struct RawMemObj : public DataBlockInterface
{

  RawMemObj(const char* blockid)
    : DataBlockInterface(blockid){
      // std::cout << "RawMemObj is initialised" << std::endl;
    };

  BlockSummary getData(void*& dataContainer);

  // put data into coresponding data structure for specific implementation
  int putData(void* dataSourcePtr);

  int eraseData();

  BlockSummary getDataSubregion(size_t dims, std::array<int, 3> subregionlb,
    std::array<int, 3> subregionub, void*& dataContainer);

  void* getrawMemPtr() { return this->m_rawMemPtr; };

  int putArray(ArraySummary& as, void* dataSourcePtr)
  {
    throw std::runtime_error("unsupported yet for RawMemObj");
  }

  int getArray(ArraySummary& as, void*& dataContainer)
  {
    throw std::runtime_error("unsupported yet for RawMemObj");
  }

  void* m_rawMemPtr = NULL;

  virtual ~RawMemObj()
  {
    if (m_rawMemPtr != NULL)
    {
      free(m_rawMemPtr);
    }
  };
};
}
#endif