#ifndef VTKMEMOBJ_H
#define VTKMEMOBJ_H

#include <blockManager/blockManager.h>
#include <iostream>
#include <vtkDataSet.h>
#include <vtkSmartPointer.h>

namespace GORILLA
{

struct VTKMemObj : public DataBlockInterface
{

  VTKMemObj(const char* blockid)
    : DataBlockInterface(blockid){
      std::cout << "VTKMemObj is initialised" << std::endl;
    };

  BlockSummary getData(void*& dataContainer);

  // put data into coresponding data structure for specific implementation
  int putData(void* dataSourcePtr);

  int eraseData();

  void* getrawMemPtr() { return (void*)this->m_dataset; };

  BlockSummary getDataSubregion(size_t dims, std::array<int, 3> subregionlb,
    std::array<int, 3> subregionub, void*& dataContainer);

  vtkDataSet* m_dataset = NULL;

  virtual ~VTKMemObj()
  {
    // we may choose to use the pure null ptr
    // refer to https://vtk.org/Wiki/VTK/Tutorials/SmartPointers
    if (m_dataset != NULL)
    {
      m_dataset->Delete();
    }
    std::cout << "VTKMemObj is erased" << std::endl;
  };
};
}
#endif