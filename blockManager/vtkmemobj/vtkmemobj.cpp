#include "vtkmemobj.h"
namespace GORILLA
{

BlockSummary VTKMemObj::getData(void*& dataContainer)
{
  // return the vtk data set
  if (this->m_vtkobject == NULL)
  {
    throw std::runtime_error("the vtk obj is null for VTKMemObj::getData, failed to getData for RawMemObj");
  }
  dataContainer = (void*)(this->m_vtkobject);
  // the block summary is the member of the parent interface
  return this->m_blockSummary;
}

// put data into coresponding data structure for specific implementation
BlockSummary VTKMemObj::getDataSubregion(
  size_t dims, std::array<int, 3> subregionlb, std::array<int, 3> subregionub, void*& dataContainer)
{
  throw std::runtime_error("getDataSubregion is not suitable for vtkdataset");
  return this->m_blockSummary;
}

int VTKMemObj::eraseData()
{
  // do nothing, the smart point will release the mem obj
  return 0;
}

// existing ptr into the shared pointer
// the user is supposed to know which type of vtk data they use
int VTKMemObj::putData(void* dataSourcePtr)
{

  if (this->m_vtkobject != NULL)
  {
    throw std::runtime_error("vtk objects exists");
  }

  // the datasource is the address of the smartpointer in this case
  // use the addr of the smart pointer to transfer it
  // instead of transfer it into the normal pointer
  // related issue
  // https://discourse.paraview.org/t/question-about-getpointer-of-the-vktsmartpointer/5952
  // be careful, there is sigault if the original type is not wrapped by the vtkSmartPointer
  // maybe try to use the template when this part become more complicated in future
  // https://stackoverflow.com/questions/49830867/check-if-void-to-object-with-static-cast-is-successful
  this->m_vtkobject = *(static_cast<vtkSmartPointer<vtkDataObject>*>(dataSourcePtr));
  return 0;
}

}