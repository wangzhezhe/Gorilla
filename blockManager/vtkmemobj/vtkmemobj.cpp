#include "vtkmemobj.h"
namespace GORILLA
{

BlockSummary VTKMemObj::getData(void*& dataContainer)
{
  // return the vtk data set
  if (this->m_dataset==NULL)
  {
    throw std::runtime_error("failed to getData for RawMemObj");
  }
  dataContainer = (void*)(this->m_dataset);
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

  if (this->m_dataset!=NULL)
  {
    throw std::runtime_error("vtk objects exists");
  }

  this->m_dataset = (vtkDataSet*)dataSourcePtr;
  return 0;
}

}