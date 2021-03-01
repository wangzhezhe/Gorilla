

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
  //we use the new delete instead of malloc and free
  //since we want it more general
  //it can not only contain the void array
  //but can also conatain the particular class
  //if there is not constructor
  //we just use the operator new
  //or maybe use template here to represnet different type in future
  void* m_rawMemPtr = NULL;

  virtual ~RawMemObj()
  {
    if (m_rawMemPtr != NULL)
    {
      ::operator delete(m_rawMemPtr);
    }
  };
};
}
#endif