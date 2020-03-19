

#include "../../commondata/metadata.h"
#include "../../server/RawdataManager/blockManager.h"

#include "vtkDataObject.h"
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>

void test_vtkobj_basic()
{
    tl::abt scope;
    BlockManager bm;
    BlockSummary bs;
    bs.m_drivertype = DRIVERTYPE_VTKDATASET;
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    bm.putBlock("123", bs, (void *)vtkDataSet::SafeDownCast(sphereSource->GetOutput()));
    return;
}

void test_vtkobj_putget()
{
    tl::abt scope;
    BlockManager bm;
    BlockSummary bs;
    bs.m_drivertype = DRIVERTYPE_VTKDATASET;
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetThetaResolution(8);
    sphereSource->SetPhiResolution(8);
    sphereSource->SetStartTheta(0.0);
    sphereSource->SetEndTheta(360.0);
    sphereSource->SetStartPhi(0.0);
    sphereSource->SetEndPhi(180.0);
    sphereSource->LatLongTessellationOff();

    //original data
    sphereSource->Update();
    vtkSmartPointer<vtkPolyData> polyData = sphereSource->GetOutput();

    // send this poly data
    // vtkDataObject *data, int remoteId, int tag
    std::cout << "---generate sphere source---" << std::endl;
    polyData->PrintSelf(std::cout, vtkIndent(0));

    bm.putBlock("12345", bs, (void *)vtkDataSet::SafeDownCast(polyData));

    //get block and check
    void *dataContainer = NULL;
    bm.getBlock("12345", dataContainer);

    std::cout << "---check the results of data get---" << std::endl;
    vtkPolyData *newpolydata = (vtkPolyData *)dataContainer;
    newpolydata->Print(std::cout);
    return;
}

int main()
{
    test_vtkobj_basic();
    test_vtkobj_putget();
}