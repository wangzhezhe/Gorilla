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
  double* bufPoints = (double*)::operator new(sizeof(double) * numPoints * 3);
  double* bufNormals = (double*)::operator new(sizeof(double) * numPoints * 3);
  int* bufCells = (int*)::operator new(sizeof(int) * numCells * 3);

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

  std::cout << "put points array" << std::endl;
  // the elem size can be the original buff poly data
  ArraySummary aspoints("points", sizeof(double), numPoints * 3);
  int status = bm.putArray(bs, aspoints, BACKEND::MEMVTKEXPLICIT, (void*)bufPoints);
  if (status != 0)
  {
    throw std::runtime_error("failed to put points");
  }

  std::cout << "put cell array" << std::endl;
  ArraySummary aspolys("cells", sizeof(int), numCells * 3);
  status = bm.putArray(bs, aspolys, BACKEND::MEMVTKEXPLICIT, (void*)bufCells);
  if (status != 0)
  {
    throw std::runtime_error("failed to put polys");
  }

  std::cout << "put normals array" << std::endl;
  ArraySummary asnormals("normals", sizeof(double), numPoints * 3);
  status = bm.putArray(bs, asnormals, BACKEND::MEMVTKEXPLICIT, (void*)bufNormals);
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

  ArraySummary arrayPoints = bm.getArray(blockid, "points", BACKEND::MEMVTKEXPLICIT, pointArrayptr);
  ArraySummary arrayCells = bm.getArray(blockid, "cells", BACKEND::MEMVTKEXPLICIT, polyArrayPtr);
  ArraySummary arrayNormals =
    bm.getArray(blockid, "normals", BACKEND::MEMVTKEXPLICIT, normalArrayPtr);

  if (pointArrayptr == NULL || polyArrayPtr == NULL || normalArrayPtr == NULL)
  {
    throw std::runtime_error("failed to extract array");
  }

  // after normal put (the last one)
  // try to assemble the data into the poly

  // generate the points array
  int nPoints = arrayPoints.m_elemNum / 3;
  auto points = vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints(nPoints);
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    const double* tempp = (double*)pointArrayptr;
    points->SetPoint(i, tempp + (i * 3));
  }

  // generate normal array
  auto normals = vtkSmartPointer<vtkDoubleArray>::New();
  normals->SetNumberOfComponents(3);
  const double* tempn = (double*)normalArrayPtr;

  for (vtkIdType i = 0; i < nPoints; i++)
  {
    normals->InsertNextTuple(tempn + (i * 3));
  }

  // generate cell array
  int nCells = arrayCells.m_elemNum / 3;
  auto polys = vtkSmartPointer<vtkCellArray>::New();
  const int* tempc = (int*)polyArrayPtr;

  for (vtkIdType i = 0; i < nCells; i++)
  {
    vtkIdType a = *(tempc + (i * 3 + 0));
    vtkIdType b = *(tempc + (i * 3 + 1));
    vtkIdType c = *(tempc + (i * 3 + 2));

    polys->InsertNextCell(3);
    polys->InsertCellPoint(a);
    polys->InsertCellPoint(b);
    polys->InsertCellPoint(c);
  }

  auto polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);
  polyData->SetPolys(polys);
  polyData->GetPointData()->SetNormals(normals);

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