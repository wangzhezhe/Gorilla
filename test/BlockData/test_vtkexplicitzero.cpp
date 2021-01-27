#include "../../blockManager/blockManager.h"
#include "../../commondata/metadata.h"
#include <cstdlib>

// for sphere
#include <vector>
#include <vtkCharArray.h>
#include <vtkCommunicator.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkXMLPolyDataWriter.h>

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#define BILLION 1000000000L

using namespace GORILLA;

void test_explicitputzero(BlockManager& bm)
{
  std::cout << "---test " << __FUNCTION__ << std::endl;
  vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->SetThetaResolution(1024);
  sphereSource->SetPhiResolution(1024);
  sphereSource->SetStartTheta(0.0);
  sphereSource->SetEndTheta(360.0);
  sphereSource->SetStartPhi(0.0);
  sphereSource->SetEndPhi(180.0);
  sphereSource->LatLongTessellationOff();

  sphereSource->Update();
  vtkSmartPointer<vtkPolyData> polyData = sphereSource->GetOutput();

  int numCells = polyData->GetNumberOfPolys();
  int numPoints = polyData->GetNumberOfPoints();

  // std::cout << "original data " << std::endl;
  // polyData->PrintSelf(std::cout, vtkIndent(0));

  struct timespec start, end;
  double diff1;
  clock_gettime(CLOCK_REALTIME, &start);

  // extract cell array
  auto cellArray = polyData->GetPolys();
  vtkDataArray* offsetArray = cellArray->GetOffsetsArray();
  vtkDataArray* connectivyArray = cellArray->GetConnectivityArray();

  size_t componentsOffset = offsetArray->GetNumberOfComponents();
  size_t typeSizeOffset = offsetArray->GetElementComponentSize();

  size_t componentsConnectivy = connectivyArray->GetNumberOfComponents();
  size_t typeSizeConnectivy = connectivyArray->GetElementComponentSize();

  std::cout << "componentsOffset " << componentsOffset << " typeSizeOffset " << typeSizeOffset
            << std::endl;

  std::cout << "componentsConnectivy " << componentsConnectivy << " typeSizeConnectivy "
            << typeSizeConnectivy << std::endl;

  vtkTypeInt64Array* arrayoffset64 = cellArray->GetOffsetsArray64();

  std::cout << "tuplenumber " << arrayoffset64->GetNumberOfTuples() << " component number "
            << arrayoffset64->GetNumberOfComponents() << std::endl;

  long* arrayoffsetptr = (long*)arrayoffset64->GetVoidPointer(0);
  size_t offsettotalSize =
    arrayoffset64->GetNumberOfTuples() * arrayoffset64->GetNumberOfComponents();
  long* temp = arrayoffsetptr;
  // for (int i = 0; i < totalSize; i++)
  //{
  //  std::cout << i << "," << *(temp + i) << "  ";
  //}
  std::cout << std::endl;

  vtkTypeInt64Array* arrayConnectivety64 = cellArray->GetConnectivityArray64();

  std::cout << "tuplenumber " << arrayConnectivety64->GetNumberOfTuples() << " component number "
            << arrayConnectivety64->GetNumberOfComponents() << std::endl;

  long* arrayconnptr = (long*)arrayConnectivety64->GetVoidPointer(0);
  size_t connectotalSize =
    arrayConnectivety64->GetNumberOfTuples() * arrayConnectivety64->GetNumberOfComponents();
  temp = arrayconnptr;

  // for (int i = 0; i < totalSize; i++)
  //{
  //  std::cout << i << "," << *(temp + i) << "  ";
  //}
  // std::cout << std::endl;

  // extract points

  vtkPoints* pointsarray = polyData->GetPoints();

  std::cout << "vtkpoints number " << pointsarray->GetNumberOfPoints() << std::endl;

  // check points values
  // the default data type is the float
  float* pointsPtr = (float*)(pointsarray->GetVoidPointer(0));
  // int datalen = pointsarray->GetNumberOfPoints() * 3;
  // for (int i = 0; i < datalen; i++)
  //{
  //  float v = *(ptr+i);
  //  std::cout << "value " << v << std::endl;
  //}

  // extract normal array

  auto normalArray = polyData->GetPointData()->GetNormals();

  size_t normalComponent = normalArray->GetNumberOfComponents();
  size_t normalComponentSize = normalArray->GetElementComponentSize();

  std::cout << "normalArray tuplenumber " << normalArray->GetNumberOfTuples()
            << " component number " << normalArray->GetNumberOfComponents() << std::endl;

  std::cout << "normalArray normalComponent " << normalComponent << " normalComponentSize "
            << normalComponentSize << std::endl;

  // get the normal array addr
  float* buffernormalArray = (float*)normalArray->GetVoidPointer(0);

  size_t size = normalArray->GetNumberOfTuples() * normalArray->GetNumberOfComponents();
  // float* tempd = buffernormalArray;
  // for (int i = 0; i < size; i++)
  //{
  //  std::cout << i << "," << *(tempd + i) << "  ";
  //}

  clock_gettime(CLOCK_REALTIME, &end);
  diff1 = (end.tv_sec - start.tv_sec) * 1.0 + (end.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
  std::cout << "decompose time " << diff1 << std::endl;

  // start to reconstruct the data
  // get normal
  auto normals = vtkSmartPointer<vtkFloatArray>::New();
  normals->SetNumberOfComponents(3);
  float* tempn = buffernormalArray;

  for (vtkIdType i = 0; i < numPoints; i++)
  {
    normals->InsertNextTuple(tempn + (i * 3));
  }

  std::cout << "ok to recreate normals" << std::endl;

  // get points
  auto points = vtkSmartPointer<vtkPoints>::New();
  if (pointsarray->GetNumberOfPoints() != numPoints)
  {
    throw std::runtime_error("wrong point number");
  }
  points->SetNumberOfPoints(numPoints);
  const float* tempp = pointsPtr;

  for (vtkIdType i = 0; i < numPoints; i++)
  {
    // std::cout << "set point value " << *(tempp + (i * 3)) << " " << *(tempp + (i * 3) + 1) << " "
    //          << *(tempp + (i * 3) + 2) << std::endl;
    points->SetPoint(i, tempp + (i * 3));
  }

  std::cout << "ok to recreate points" << std::endl;

  // get cells
  auto polys = vtkSmartPointer<vtkCellArray>::New();

  auto newoffsetArray = vtkSmartPointer<vtkTypeInt64Array>::New();
  newoffsetArray->SetNumberOfComponents(1);
  newoffsetArray->SetNumberOfTuples(offsettotalSize);

  for (vtkIdType i = 0; i < offsettotalSize; i++)
  {
    newoffsetArray->SetValue(i, *(arrayoffsetptr + i));
  }

  std::cout << "ok to recreate cell offset" << std::endl;

  auto newconnectivityArray = vtkSmartPointer<vtkTypeInt64Array>::New();
  newconnectivityArray->SetNumberOfComponents(1);
  newconnectivityArray->SetNumberOfTuples(connectotalSize);

  for (vtkIdType i = 0; i < connectotalSize; i++)
  {
    newconnectivityArray->SetValue(i, *(arrayconnptr + i));
  }

  std::cout << "ok to recreate cell connectivity" << std::endl;

  polys->SetData(newoffsetArray, newconnectivityArray);

  // generate poly data
  auto newpolyData = vtkSmartPointer<vtkPolyData>::New();
  newpolyData->SetPoints(points);
  newpolyData->SetPolys(polys);
  newpolyData->GetPointData()->SetNormals(normals);

  std::cout << "reconstructed data:" << std::endl;
  newpolyData->PrintSelf(std::cout, vtkIndent(0));
}

int main()
{
  tl::abt scope;
  BlockManager bm;
  test_explicitputzero(bm);
}