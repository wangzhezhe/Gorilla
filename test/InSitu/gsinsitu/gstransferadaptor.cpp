#include <iostream>
#include <utils/uuid.h>

#include "gstransferadaptor.hpp"

#include <vtkCenterOfMass.h>
#include <vtkConnectivityFilter.h>
#include <vtkFlyingEdges3D.h>
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

// write image data to disk

/*
  auto importer = vtkSmartPointer<vtkImageImport>::New();
  importer->SetDataSpacing(1, 1, 1);
  importer->SetDataOrigin(0, 0, 0);

  // from 0 to the shape -1 or from lb to the ub??
  importer->SetWholeExtent(indexlb[0], indexub[0], indexlb[1], indexub[1], indexlb[2], indexub[2]);
  importer->SetDataExtentToWholeExtent();
  importer->SetDataScalarTypeToDouble();
  importer->SetNumberOfScalarComponents(1);
  importer->SetImportVoidPointer((double*)(blockData));

  // Run the marching cubes algorithm
  auto mcubes = vtkSmartPointer<vtkMarchingCubes>::New();
  mcubes->SetInputConnection(importer->GetOutputPort());
  mcubes->ComputeNormalsOn();
  mcubes->SetValue(0, 0.5);
  mcubes->Update();

  // caculate the number of polygonals
  vtkSmartPointer<vtkPolyData> polyData = mcubes->GetOutput();
*/
using namespace GORILLA;

// get image data from the gs data
vtkSmartPointer<vtkImageData> GSTransferAdaptor::getImageData(const GrayScott& sim)
{
  std::array<int, 3> indexlb = { { (int)sim.offset_x, (int)sim.offset_y, (int)sim.offset_z } };
  std::array<int, 3> indexub = { { (int)(sim.offset_x + sim.size_x - 1),
    (int)(sim.offset_y + sim.size_y - 1), (int)(sim.offset_z + sim.size_z - 1) } };

  auto importer = vtkSmartPointer<vtkImageImport>::New();
  importer->SetDataSpacing(1, 1, 1);
  importer->SetDataOrigin(0, 0, 0);

  // still not sure the difference between the whole extent and the buffered extent
  importer->SetWholeExtent(indexlb[0], indexub[0], indexlb[1], indexub[1], indexlb[2], indexub[2]);
  importer->SetDataExtentToWholeExtent();
  importer->SetDataScalarTypeToDouble();
  importer->SetNumberOfScalarComponents(1);
  // u_nonghost is a function that takes long time
  // it basically reoranize the data
  // get the same value here direactly to avoid the copy
  importer->SetImportVoidPointer((double*)(sim.u_noghost().data()));
  // !!!the actual image data of the image is empty if we do not execute the update
  importer->Update();
  vtkSmartPointer<vtkImageData> imagedata = importer->GetOutput();

  return imagedata;
}

// get poly data from the gs data
vtkSmartPointer<vtkPolyData> GSTransferAdaptor::getPolyData(
  const GrayScott& sim, double iso, int rank)
{

  // Run the marching cubes algorithm
  // auto mcubes = vtkSmartPointer<vtkMarchingCubes>::New();
  // fly edge is much faster then the marching cube case
  // it can avoid the long execution for the first step
  // refer to
  // https://discourse.vtk.org/t/it-takes-long-time-for-the-iso-surface-extraction-at-first-step/5456
  // get image data from sim firstly
  vtkSmartPointer<vtkImageData> imageData = getImageData(sim);

  auto isoExtraction = vtkSmartPointer<vtkFlyingEdges3D>::New();
  isoExtraction->SetInputData(imageData);
  isoExtraction->ComputeNormalsOn();
  isoExtraction->SetValue(0, iso);
  isoExtraction->Update();
  vtkSmartPointer<vtkPolyData> poly = isoExtraction->GetOutput();
  // double anaStep3 = tl::timer::wtime();

  // the step3 takes the longest time for particular process at the first step
  // std::cout << "rank " << rank << " anastep0 " << anaStep1 - anaStep0 << " anastep1 "
  //          << anaStep2 - anaStep1 << " anastep2 " << anaStep3 - anaStep2 << std::endl;

  // return the poly data
  return poly;
}

void GSTransferAdaptor::stageGsData(
  const GrayScott& sim, std::string varName, size_t step, int rank, std::string recordInfo)
{
  // get image data
  vtkSmartPointer<vtkImageData> gsimage = this->getImageData(sim);

  // stage the image data
  this->stageImageData(gsimage, varName, step, rank, recordInfo);
}

// maybe aggregate first then process
// iso extraction check the polygonal number
// TODO extract the poly extraction and the analytics
/*
void InSitu::isosurfacePolyNum(const GrayScott& sim, int rank, double iso, int step)
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
//}

/*
void InSitu::stageImageData(
  const GrayScott& sim, std::string varName, size_t step, int rank, std::string recordInfo)
{
  std::vector<double> u = sim.u_noghost();

  std::string VarNameU = varName;

  std::array<int, 3> indexlb = { { (int)sim.offset_x, (int)sim.offset_y, (int)sim.offset_z } };
  std::array<int, 3> indexub = { { (int)(sim.offset_x + sim.size_x - 1),
    (int)(sim.offset_y + sim.size_y - 1), (int)(sim.offset_z + sim.size_z - 1) } };

  //  of element
  //  size of every element
  size_t elemSize = sizeof(double);
  size_t elemNum = sim.size_x * sim.size_y * sim.size_z;

  // we can use the varname plus step to filter if the data partition need to be processed
  // and there is only need a thin index to get all the associated blocks
  std::string blockid = VarNameU + "_" + std::to_string(step) + "_" + std::to_string(rank);

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
*/

// extract the data into the polydata and then write data into the staging
// return the blockid for future use
/*
std::string GSTransferAdaptor::extractAndwrite(
  const GrayScott& sim, size_t step, int rank, std::string recordInfo)
{
  struct timespec start, end;
  double diff;
  clock_gettime(CLOCK_REALTIME, &start);

  std::array<int, 3> indexlb = { { (int)sim.offset_x, (int)sim.offset_y, (int)sim.offset_z } };
  std::array<int, 3> indexub = { { (int)(sim.offset_x + sim.size_x - 1),
    (int)(sim.offset_y + sim.size_y - 1), (int)(sim.offset_z + sim.size_z - 1) } };

  int blockindex = 0;
  std::string VarNameU = "grascott_u";

  // contains varname and step
  std::string blockidSuffix = VarNameU + "_" + std::to_string(step);

  std::string blockid =
    blockidSuffix + "_" + std::to_string(rank) + "_" + std::to_string(blockindex);

  // extract poly
  auto polyData = this->getPoly(sim, 0.5, rank);

  // only transfer data when polydata cell is larger than 0
  int polyNum = polyData->GetNumberOfPolys();

  clock_gettime(CLOCK_REALTIME, &end);
  diff = (end.tv_sec - start.tv_sec) * 1.0 + (end.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
  if (rank == 0)
  {
    std::cout << "poly extraction time: " << diff << std::endl;
  }

  // caculate extract time
  if (polyNum != 0)
  {
    // generate raw data summary block
    // the elemnum and elemsize is 1 in this case
    // since the actual data is self contained
    // ArraySummary as(blockid, 1, 1);
    std::vector<ArraySummary> aslist;
    // aslist.push_back(as);

    BlockSummary bs(aslist, DATATYPE_VTKEXPLICIT, blockid, 3, indexlb, indexub, recordInfo);
    bs.m_backend = BACKEND::MEMVTKEXPLICIT;
    int status = m_uniclient->putrawdata(step, VarNameU, bs, polyData.GetPointer());
    if (status != 0)
    {
      throw std::runtime_error("failed to put data for step " + std::to_string(step));
    }
  }
  // we assume that the interesting data contains in every step in this exp
  // but actually, there might not interesting data in specific step
  // maybe one aggregate operation needed here
  // to make sure if there is actual data
  return blockidSuffix;
}
*/
