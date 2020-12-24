#include "writer.h"
#include <iostream>
#include <utils/uuid.h>

#include <vtkCenterOfMass.h>
#include <vtkConnectivityFilter.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkMarchingCubes.h>
#include <vtkMassProperties.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLPolyDataWriter.h>

void Writer::writeImageData(const GrayScott& sim, std::string fileName)
{

  std::vector<double> u = sim.u_noghost();
  std::array<int, 3> indexlb = { { (int)sim.offset_x, (int)sim.offset_y, (int)sim.offset_z } };
  std::array<int, 3> indexub = { { (int)(sim.offset_x + sim.size_x - 1),
    (int)(sim.offset_y + sim.size_y - 1), (int)(sim.offset_z + sim.size_z - 1) } };

  auto importer = vtkSmartPointer<vtkImageImport>::New();
  importer->SetDataSpacing(1, 1, 1);
  importer->SetDataOrigin(0, 0, 0);
  // from 0 to the shape -1 or from lb to the ub??
  importer->SetWholeExtent(indexlb[0], indexub[0], indexlb[1], indexub[1], indexlb[2], indexub[2]);
  importer->SetDataExtentToWholeExtent();
  importer->SetDataScalarTypeToDouble();
  importer->SetNumberOfScalarComponents(1);
  importer->SetImportVoidPointer((double*)(u.data()));
  importer->Update();

  // Write the file by vtkXMLDataSetWriter
  vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
  writer->SetFileName(fileName.data());

  // get the specific polydata and check the results
  writer->SetInputConnection(importer->GetOutputPort());
  // writer->SetInputData(importer->GetOutputPort());
  // Optional - set the mode. The default is binary.
  writer->SetDataModeToBinary();
  // writer->SetDataModeToAscii();
  writer->Write();
}

vtkSmartPointer<vtkPolyData> Writer::getPoly(const GrayScott& sim, int rank, double iso, int step)
{
  std::vector<double> u = sim.u_noghost();
  std::array<int, 3> indexlb = { { (int)sim.offset_x, (int)sim.offset_y, (int)sim.offset_z } };
  std::array<int, 3> indexub = { { (int)(sim.offset_x + sim.size_x - 1),
    (int)(sim.offset_y + sim.size_y - 1), (int)(sim.offset_z + sim.size_z - 1) } };

  auto importer = vtkSmartPointer<vtkImageImport>::New();
  importer->SetDataSpacing(1, 1, 1);
  importer->SetDataOrigin(0, 0, 0);

  // from 0 to the shape -1 or from lb to the ub??
  importer->SetWholeExtent(indexlb[0], indexub[0], indexlb[1], indexub[1], indexlb[2], indexub[2]);
  importer->SetDataExtentToWholeExtent();
  importer->SetDataScalarTypeToDouble();
  importer->SetNumberOfScalarComponents(1);
  importer->SetImportVoidPointer((double*)(u.data()));

  // Run the marching cubes algorithm
  auto mcubes = vtkSmartPointer<vtkMarchingCubes>::New();
  mcubes->SetInputConnection(importer->GetOutputPort());
  mcubes->ComputeNormalsOn();
  mcubes->SetValue(0, iso);
  mcubes->Update();

  // return the poly data
  return mcubes->GetOutput();
}

void Writer::polyProcess(vtkSmartPointer<vtkPolyData> polyData, int step)
{
  int numCells = polyData->GetNumberOfPolys();
  std::cout << "step " << step << " cell number " << numCells << std::endl;

  if (numCells > 0)
  {
    // surface area
    auto connectivityFilter = vtkSmartPointer<vtkConnectivityFilter>::New();
    connectivityFilter->SetInputData(polyData);
    connectivityFilter->SetExtractionModeToAllRegions();
    connectivityFilter->ColorRegionsOn();
    connectivityFilter->Update();

    int nBlobs = connectivityFilter->GetNumberOfExtractedRegions();

    std::cout << "step " << step << " found " << nBlobs << " blobs" << std::endl;

    // get the largetst surface
    connectivityFilter->SetInputData(polyData);
    connectivityFilter->SetExtractionModeToLargestRegion();
    connectivityFilter->ColorRegionsOn();

    auto massProperties = vtkSmartPointer<vtkMassProperties>::New();
    massProperties->SetInputConnection(connectivityFilter->GetOutputPort());

    std::cout << "Surface area of largest blob is " << massProperties->GetSurfaceArea()
              << std::endl;

    // get the center of the region
    // Compute the center of mass
    vtkSmartPointer<vtkCenterOfMass> centerOfMassFilter = vtkSmartPointer<vtkCenterOfMass>::New();
    centerOfMassFilter->SetInputConnection(connectivityFilter->GetOutputPort());
    centerOfMassFilter->SetUseScalarsAsWeights(false);
    centerOfMassFilter->Update();

    double center[3];
    centerOfMassFilter->GetCenter(center);

    std::cout << "Center of mass for largest blob is " << center[0] << " " << center[1] << " "
              << center[2] << std::endl;
  }
}

// maybe aggregate first then process
// iso extraction check the polygonal number
// TODO extract the poly extraction and the analytics
void Writer::isosurfacePolyNum(const GrayScott& sim, int rank, double iso, int step)
{

  std::vector<double> u = sim.u_noghost();
  std::array<int, 3> indexlb = { { (int)sim.offset_x, (int)sim.offset_y, (int)sim.offset_z } };
  std::array<int, 3> indexub = { { (int)(sim.offset_x + sim.size_x - 1),
    (int)(sim.offset_y + sim.size_y - 1), (int)(sim.offset_z + sim.size_z - 1) } };

  auto importer = vtkSmartPointer<vtkImageImport>::New();
  importer->SetDataSpacing(1, 1, 1);
  importer->SetDataOrigin(0, 0, 0);

  // from 0 to the shape -1 or from lb to the ub??
  importer->SetWholeExtent(indexlb[0], indexub[0], indexlb[1], indexub[1], indexlb[2], indexub[2]);
  importer->SetDataExtentToWholeExtent();
  importer->SetDataScalarTypeToDouble();
  importer->SetNumberOfScalarComponents(1);
  importer->SetImportVoidPointer((double*)(u.data()));

  // Run the marching cubes algorithm
  auto mcubes = vtkSmartPointer<vtkMarchingCubes>::New();
  mcubes->SetInputConnection(importer->GetOutputPort());
  mcubes->ComputeNormalsOn();
  mcubes->SetValue(0, iso);
  mcubes->Update();

  // caculate the number of polygonals
  vtkSmartPointer<vtkPolyData> polyData = mcubes->GetOutput();

  int numCells = polyData->GetNumberOfPolys();
  std::cout << "step " << step << " rank " << rank << " cell number " << numCells << std::endl;

  if (numCells > 0)
  {
    // surface area
    auto connectivityFilter = vtkSmartPointer<vtkConnectivityFilter>::New();
    connectivityFilter->SetInputData(polyData);
    connectivityFilter->SetExtractionModeToAllRegions();
    connectivityFilter->ColorRegionsOn();
    connectivityFilter->Update();

    int nBlobs = connectivityFilter->GetNumberOfExtractedRegions();

    std::cout << "step " << step << " rank " << rank << " found " << nBlobs << " blobs"
              << std::endl;

    // get the largetst surface
    connectivityFilter->SetInputData(polyData);
    connectivityFilter->SetExtractionModeToLargestRegion();
    connectivityFilter->ColorRegionsOn();

    auto massProperties = vtkSmartPointer<vtkMassProperties>::New();
    massProperties->SetInputConnection(connectivityFilter->GetOutputPort());

    std::cout << "Surface area of largest blob is " << massProperties->GetSurfaceArea()
              << std::endl;

    // get the center of the region
    // Compute the center of mass
    vtkSmartPointer<vtkCenterOfMass> centerOfMassFilter = vtkSmartPointer<vtkCenterOfMass>::New();
    centerOfMassFilter->SetInputConnection(connectivityFilter->GetOutputPort());
    centerOfMassFilter->SetUseScalarsAsWeights(false);
    centerOfMassFilter->Update();

    double center[3];
    centerOfMassFilter->GetCenter(center);

    std::cout << "Center of mass for largest blob is " << center[0] << " " << center[1] << " "
              << center[2] << std::endl;
  }

  /*
auto threshold = vtkSmartPointer<vtkThreshold>::New();
auto massProperties = vtkSmartPointer<vtkMassProperties>::New();
auto surfaceFilter = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();

threshold->SetInputConnection(connectivityFilter->GetOutputPort());
surfaceFilter->SetInputConnection(threshold->GetOutputPort());
massProperties->SetInputConnection(surfaceFilter->GetOutputPort());

for (int i = 0; i < nBlobs; i++)
{
  threshold->ThresholdBetween(i, i);

  std::cout << "Surface area of blob #" << i << " is " << massProperties->GetSurfaceArea()
            << std::endl;
}
*/

  // get the center of the polygonal data
  // refer to https://lorensen.github.io/VTKExamples/site/Java/PolyData/CenterOfMass/
  // try to get the center for specific one
  // or refer to https://discourse.paraview.org/t/center-of-mass-center-of-data/1273/6 in vtk
  // refer to this to set the mode for adding the specific region
}

void Writer::write(const GrayScott& sim, size_t step, int rank, std::string recordInfo)
{
  std::vector<double> u = sim.u_noghost();

  std::string VarNameU = "grascott_u";

  std::array<int, 3> indexlb = { { (int)sim.offset_x, (int)sim.offset_y, (int)sim.offset_z } };
  std::array<int, 3> indexub = { { (int)(sim.offset_x + sim.size_x - 1),
    (int)(sim.offset_y + sim.size_y - 1), (int)(sim.offset_z + sim.size_z - 1) } };

  size_t elemSize = sizeof(double);
  size_t elemNum = sim.size_x * sim.size_y * sim.size_z;
  int blockindex = 0;
  std::string blockid = VarNameU + "_" + std::to_string(step) + "_" + std::to_string(rank) + "_" +
    std::to_string(blockindex);

  // generate raw data summary block
  ArraySummary as(blockid, elemSize, elemNum);
  std::vector<ArraySummary> aslist;
  aslist.push_back(as);

  BlockSummary bs(aslist, DATATYPE_CARGRID, blockid, 3, indexlb, indexub, recordInfo);

  int status = m_uniclient->putrawdata(step, VarNameU, bs, u.data());

  if (status != 0)
  {
    throw std::runtime_error("failed to put data for step " + std::to_string(step));
  }
}
