
#ifndef VTKOBJ_H
#define VTKOBJ_H

#include <vtkDataSet.h>
#include "../blockManager.h"

//store the generalized vtkDataSet
//https://public.kitware.com/pipermail/vtkusers/2016-August/096186.html
//https://itk.org/Wiki/VTK/Examples/Developers/Broken/MultipleOutputConnections
//use SafeDownCast to get the DataSet
struct VTKObj : public DataBlockInterface
{
    VTKObj(BlockSummary &blockSummary) : DataBlockInterface(blockSummary){
                                             //std::cout << "RawMemObj is initialised" << std::endl;
                                         };

    BlockSummary getData(void *&dataContainer);

    // put data into coresponding data structure for specific implementation
    int putData(void *dataSourcePtr)
    {
        m_vtkDataSet = (vtkDataSet *)dataSourcePtr;
        return 0;
    };

    BlockSummary getDataSubregion(
        size_t dims,
        std::array<int, 3> subregionlb,
        std::array<int, 3> subregionub,
        void *&dataContainer);

    void *getrawMemPtr();

    vtkDataSet *m_vtkDataSet = NULL;

    ~VTKObj()
    {
        std::cout << "destroy VTKObj" << std::endl;
        if (m_vtkDataSet != NULL)
        {
            ///home/zw241/cworkspace/src/vtk/Common/DataModel/vtkDataSet.h:477:3: error: 'virtual vtkDataSet::~vtkDataSet()' is protected
            //~vtkDataSet() override;
            //delete m_vtkDataSet;
        }
    }
};

#endif