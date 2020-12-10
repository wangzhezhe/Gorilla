#include "vtkmemexplicit.h"

namespace GORILLA
{

BlockSummary VTKMemExplicitObj::getData(void*& dataContainer)
{
  throw std::runtime_error("BlockSummary is not suitable for vtk multi array");
  return this->m_blockSummary;
}

// put data into coresponding data structure for specific implementation
BlockSummary VTKMemExplicitObj::getDataSubregion(
  size_t dims, std::array<int, 3> subregionlb, std::array<int, 3> subregionub, void*& dataContainer)
{
  throw std::runtime_error("getDataSubregion is not suitable for vtk multi array");
  return this->m_blockSummary;
}

int VTKMemExplicitObj::eraseData()
{
  // do nothing, the smart point will release the mem obj
  return 0;
}

// existing ptr into the shared pointer
// the user is supposed to know which type of vtk data they use
int VTKMemExplicitObj::putData(void* dataSourcePtr)
{

  throw std::runtime_error("putData is not suitable for vtk multi array");
  return 0;
}

// in this case, the datasourcePtr is the ptr to the smart data object
// but how to carry the type information?
// here we basically remove the type information
int VTKMemExplicitObj::putArray(ArraySummary& as, void* dataSourcePtr)
{
  // add array into the map
  m_m_arrayMapMutex.lock();
  int tempcount = this->m_arrayMap.count(as);
  m_m_arrayMapMutex.unlock();

  if (tempcount != 0)
  {
    throw std::runtime_error("array summary exist for " + std::string(as.m_arrayName));
  }

  m_m_arrayMapMutex.lock();
  this->m_arrayMap[as] = *(static_cast<vtkSmartPointer<vtkDataObject>*>(dataSourcePtr));
  m_m_arrayMapMutex.unlock();
  return 0;
}

int VTKMemExplicitObj::getArray(ArraySummary& as, void*& dataContainer)
{

  //std::cout << "debug VTKMemExplicitObj::getArray is called" <<std::endl; 
  m_m_arrayMapMutex.lock();
  int tempcount = this->m_arrayMap.count(as);
  m_m_arrayMapMutex.unlock();

  if (tempcount == 0)
  {
    throw std::runtime_error("array summary not exist for " + std::string(as.m_arrayName));
  }

  m_m_arrayMapMutex.lock();
  dataContainer = (void*)(this->m_arrayMap[as]);
  m_m_arrayMapMutex.unlock();
  return 0;
}

}