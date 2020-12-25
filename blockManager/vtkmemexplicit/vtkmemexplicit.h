#ifndef VTKMEMEXPLICIT_H
#define VTKMEMEXPLICIT_H

#include <blockManager/blockManager.h>
#include <iostream>
#include <unordered_map>
#include <vtkDataSet.h>
#include <vtkSmartPointer.h>

namespace GORILLA
{

struct VTKMemExplicitObj : public DataBlockInterface
{

  VTKMemExplicitObj(const char* blockid)
    : DataBlockInterface(blockid){
      // std::cout << "VTKMemExplicitObj is initialised" << std::endl;
    };

  BlockSummary getData(void*& dataContainer);

  // put data into coresponding data structure for specific implementation
  int putData(void* dataSourcePtr);

  int eraseData();

  void* getrawMemPtr() { return nullptr; };

  int putArray(ArraySummary& as, void* dataSourcePtr);

  int getArray(ArraySummary& as, void*& dataContainer);

  BlockSummary getDataSubregion(size_t dims, std::array<int, 3> subregionlb,
    std::array<int, 3> subregionub, void*& dataContainer);

  // we use a vtksmart pointer here
  // otherwise, the data will be deleted if the outer smart pointer out of the scope
  // vtkDataObject* m_vtkobject = NULL;
  // the common pointer and smart pointer can be assigned to each other transparently
  tl::mutex m_m_arrayMapMutex;
  // std::unordered_map<ArraySummary, vtkSmartPointer<vtkDataObject>, ArraySummaryHash > m_arrayMap;
  // TODO add the type message into the arraysummary??? such as uint or int, that is different
  // although there is same size, but the data value can be different
  std::unordered_map<ArraySummary, void*, ArraySummaryHash> m_arrayMap;

  virtual ~VTKMemExplicitObj()
  {
    // we may choose to use the pure null ptr
    // refer to https://vtk.org/Wiki/VTK/Tutorials/SmartPointers
    // go through the map and delete pointer in it
    for (auto& kv : this->m_arrayMap)
    {
      if (kv.second != nullptr)
      {
        // release the memory space
        // assume we use operator new to allocate the space
        ::operator delete(kv.second);
      }
    }
  };
};
}
#endif