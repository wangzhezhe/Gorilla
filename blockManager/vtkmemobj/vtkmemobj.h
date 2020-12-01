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
      //std::cout << "VTKMemObj is initialised" << std::endl;
    };

  BlockSummary getData(void*& dataContainer);

  // put data into coresponding data structure for specific implementation
  int putData(void* dataSourcePtr);

  int eraseData();

  void* getrawMemPtr() { return (void*)this->m_vtkobject; };

  BlockSummary getDataSubregion(size_t dims, std::array<int, 3> subregionlb,
    std::array<int, 3> subregionub, void*& dataContainer);
  
  //we use a vtksmart pointer here
  //otherwise, the data will be deleted if the outer smart pointer out of the scope
  //vtkDataObject* m_vtkobject = NULL;
  //the common pointer and smart pointer can be assigned to each other transparently
  vtkSmartPointer<vtkDataObject> m_vtkobject = NULL;

  virtual ~VTKMemObj()
  {
    // we may choose to use the pure null ptr
    // refer to https://vtk.org/Wiki/VTK/Tutorials/SmartPointers
    if (m_vtkobject != NULL)
    {
      //refer to
      //https://stackoverflow.com/questions/16497930/how-to-free-a-vtksmartpointer
      //previous data will be deleted if we assign it to null
      m_vtkobject=NULL;
    }
    //std::cout << "VTKMemObj is erased" << std::endl;
  };
};
}
#endif