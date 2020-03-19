

#include "vtkobj.hpp"

BlockSummary VTKObj::getData(void *&dataContainer)
{
    //return the vtk data set
    if (this->m_vtkDataSet == NULL)
    {
        throw std::runtime_error("failed to getData for RawMemObj");
    }
    dataContainer = (void *)(this->m_vtkDataSet);
    //the block summary is the member of the parent interface
    return this->m_blockSummary;
}

// put data into coresponding data structure for specific implementation

BlockSummary VTKObj::getDataSubregion(
    size_t dims,
    std::array<int, 3> subregionlb,
    std::array<int, 3> subregionub,
    void *&dataContainer)
{
    throw std::runtime_error("getDataSubregion is not suitable for vtkdataset");
    return this->m_blockSummary;
}

void *VTKObj::getrawMemPtr()
{
    //return the pointer that point to the dataset
    return (void *)m_vtkDataSet;
}