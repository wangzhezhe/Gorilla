#include "../../blockManager/blockManager.h"
#include "../../commondata/metadata.h"
#include <cstdlib>

// for sphere
#include <vector>
#include <vtkCharArray.h>
#include <vtkCommunicator.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkXMLPolyDataWriter.h>

using namespace GORILLA;

void test_explicitput(BlockManager& bm)
{
  std::cout << "---test " << __FUNCTION__ << std::endl;
  vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->SetThetaResolution(8);
  sphereSource->SetPhiResolution(8);
  sphereSource->SetStartTheta(0.0);
  sphereSource->SetEndTheta(360.0);
  sphereSource->SetStartPhi(0.0);
  sphereSource->SetEndPhi(180.0);
  sphereSource->LatLongTessellationOff();

  sphereSource->Update();
  vtkSmartPointer<vtkPolyData> polyData = sphereSource->GetOutput();

  int numCells = polyData->GetNumberOfPolys();
  int numPoints = polyData->GetNumberOfPoints();

  // get specific array and put it into the block obj
  std::vector<double> bufPoints(numPoints * 3);
  std::vector<double> bufNormals(numPoints * 3);
  std::vector<int> bufCells(numCells * 3); // Assumes that cells are triangles

  double coords[3];

  auto cellArray = polyData->GetPolys();

  cellArray->InitTraversal();

  // Iterate through cells
  for (int i = 0; i < numCells; i++)
  {
    auto idList = vtkSmartPointer<vtkIdList>::New();

    cellArray->GetNextCell(idList);

    // Iterate through points of a cell
    for (int j = 0; j < idList->GetNumberOfIds(); j++)
    {
      auto id = idList->GetId(j);

      bufCells[i * 3 + j] = id;

      polyData->GetPoint(id, coords);

      bufPoints[id * 3 + 0] = coords[0];
      bufPoints[id * 3 + 1] = coords[1];
      bufPoints[id * 3 + 2] = coords[2];
    }
  }

  auto normalArray = polyData->GetPointData()->GetNormals();

  // Extract normals
  for (int i = 0; i < normalArray->GetNumberOfTuples(); i++)
  {
    normalArray->GetTuple(i, coords);

    bufNormals[i * 3 + 0] = coords[0];
    bufNormals[i * 3 + 1] = coords[1];
    bufNormals[i * 3 + 2] = coords[2];
  }

  // create the block manager
  std::string blockid = "testblockid";
  std::vector<ArraySummary> aslist;
  std::array<int, 3> indexlb = { { 0, 0, 0 } };
  std::array<int, 3> indexub = { { 10, 0, 0 } };
  BlockSummary bs(aslist, DATATYPE_VTKEXPLICIT, blockid, 1, indexlb, indexub);
  // assume there is transfer process
  int nPoints = bufPoints.size() / 3;
  int nCells = bufCells.size() / 3;

  // new array that needs to store
  auto points = vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints(nPoints);
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    points->SetPoint(i, &bufPoints[i * 3]);
  }

  std::cout << "put points array" << std::endl;
  // the elem size can be the original buff poly data
  ArraySummary aspoints("points", sizeof(double), numPoints * 3);
  int status = bm.putArray(bs, aspoints, BACKEND::MEMVTKEXPLICIT, &points);
  if (status != 0)
  {
    throw std::runtime_error("failed to put points");
  }

  // new array that needs to store
  auto polys = vtkSmartPointer<vtkCellArray>::New();
  for (vtkIdType i = 0; i < nCells; i++)
  {
    vtkIdType a = bufCells[i * 3 + 0];
    vtkIdType b = bufCells[i * 3 + 1];
    vtkIdType c = bufCells[i * 3 + 2];

    polys->InsertNextCell(3);
    polys->InsertCellPoint(a);
    polys->InsertCellPoint(b);
    polys->InsertCellPoint(c);
  }

  std::cout << "put cell array" << std::endl;
  ArraySummary aspolys("polys", sizeof(int), numCells * 3);
  status = bm.putArray(bs, aspolys, BACKEND::MEMVTKEXPLICIT, &polys);
  if (status != 0)
  {
    throw std::runtime_error("failed to put polys");
  }

  // new array that need to store
  auto normals = vtkSmartPointer<vtkDoubleArray>::New();
  normals->SetNumberOfComponents(3);
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    normals->InsertNextTuple(&bufNormals[i * 3]);
  }

  std::cout << "put normals array" << std::endl;
  ArraySummary asnormals("normals", sizeof(double), numPoints * 3);
  status = bm.putArray(bs, asnormals, BACKEND::MEMVTKEXPLICIT, &normals);
  if (status != 0)
  {
    throw std::runtime_error("failed to put asnormals");
  }
}

void test_explicitget(BlockManager& bm)
{
  std::cout << "---test " << __FUNCTION__ << std::endl;
  // get array one by one and organize it into the vtk objects

  void* pointArrayptr = NULL;
  void* polyArrayPtr = NULL;
  void* normalArrayPtr = NULL;

  std::string blockid = "testblockid";

  // check the bm status
  BlockSummary bs = bm.getBlockSummary(blockid);

  bs.printSummary();

  int status = bm.getArray(blockid, "points", BACKEND::MEMVTKEXPLICIT, pointArrayptr);
  if (status != 0)
  {
    throw std::runtime_error("failed to extract points");
  }
  status = bm.getArray(blockid, "polys", BACKEND::MEMVTKEXPLICIT, polyArrayPtr);
  if (status != 0)
  {
    throw std::runtime_error("failed to extract polys");
  }
  status = bm.getArray(blockid, "normals", BACKEND::MEMVTKEXPLICIT, normalArrayPtr);
  if (status != 0)
  {
    throw std::runtime_error("failed to extract normals");
  }

  if (pointArrayptr == NULL || polyArrayPtr == NULL || normalArrayPtr == NULL)
  {
    throw std::runtime_error("failed to extract array");
  }

  // assembel extracted array into the vtkdata object
  auto polyData = vtkSmartPointer<vtkPolyData>::New();

  //use the common pointer, since the smart pointer will degenerate into the common ptr
  polyData->SetPoints((vtkPoints*)pointArrayptr);
  polyData->SetPolys((vtkCellArray*)polyArrayPtr);
  polyData->GetPointData()->SetNormals((vtkDoubleArray*)normalArrayPtr);

  polyData->PrintSelf(std::cout, vtkIndent(5));
  return;
}

int main()
{
  tl::abt scope;
  BlockManager bm;

  test_explicitput(bm);
  test_explicitget(bm);
}