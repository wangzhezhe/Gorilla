#include <iostream>
#include <utils/uuid.h>

#include "InSitu.hpp"

#include <vtkCenterOfMass.h>
#include <vtkConnectivityFilter.h>
#include <vtkDataSetSurfaceFilter.h>
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

void InSitu::writePolyDataFile(vtkSmartPointer<vtkPolyData> polyData, std::string fileName)
{
  vtkSmartPointer<vtkXMLDataSetWriter> writer = vtkSmartPointer<vtkXMLDataSetWriter>::New();
  writer->SetFileName(fileName.data());
  // get the specific polydata and check the results
  writer->SetInputData(polyData);
  // Optional - set the mode. The default is binary.
  writer->SetDataModeToBinary();
  // writer->SetDataModeToAscii();
  writer->Write();
}

void InSitu::writeImageData(const GrayScott& sim, std::string fileName)
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
vtkSmartPointer<vtkPolyData> InSitu::getPoly(const GrayScott& sim, double iso, int rank)
{
  // double anaStep0 = tl::timer::wtime();

  std::array<int, 3> indexlb = { { (int)sim.offset_x, (int)sim.offset_y, (int)sim.offset_z } };
  std::array<int, 3> indexub = { { (int)(sim.offset_x + sim.size_x - 1),
    (int)(sim.offset_y + sim.size_y - 1), (int)(sim.offset_z + sim.size_z - 1) } };

  auto importer = vtkSmartPointer<vtkImageImport>::New();
  importer->SetDataSpacing(1, 1, 1);
  importer->SetDataOrigin(0, 0, 0);
  // double anaStep1 = tl::timer::wtime();

  // from 0 to the shape -1 or from lb to the ub??
  importer->SetWholeExtent(indexlb[0], indexub[0], indexlb[1], indexub[1], indexlb[2], indexub[2]);
  importer->SetDataExtentToWholeExtent();
  importer->SetDataScalarTypeToDouble();
  importer->SetNumberOfScalarComponents(1);
  // u_nonghost is a function that takes long time
  // it basically reoranize the data
  // get the same value here direactly to avoid the copy
  importer->SetImportVoidPointer((double*)(sim.u_noghost().data()));
  // double anaStep2 = tl::timer::wtime();

  // Run the marching cubes algorithm
  // auto mcubes = vtkSmartPointer<vtkMarchingCubes>::New();
  // fly edge is much faster then the marching cube case
  // it can avoid the long execution for the first step
  // refer to
  // https://discourse.vtk.org/t/it-takes-long-time-for-the-iso-surface-extraction-at-first-step/5456
  auto isoExtraction = vtkSmartPointer<vtkFlyingEdges3D>::New();
  isoExtraction->SetInputConnection(importer->GetOutputPort());
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

void InSitu::stagePolyData(
  vtkSmartPointer<vtkPolyData> polydata, std::string varName, int step, int rank)
{

  // there is one data block for this case
  std::string blockName = "block_0";
  ArraySummary as(blockName, 1, 1);
  std::vector<ArraySummary> aslist;
  aslist.push_back(as);
  size_t dims = 3;
  // the index is unnecessary for this vtk backend
  std::array<int, 3> indexlb = { { 0, 0, 0 } };
  std::array<int, 3> indexub = { { 0, 0, 0 } };
  std::string dataType = DATATYPE_VTKEXPLICIT;

  BlockSummary bs(aslist, dataType, blockName, dims, indexlb, indexub);
  bs.m_backend = BACKEND::MEMVTKEXPLICIT;
  // complete name = varName_dataid
  std::string completevarName = varName + "_" + std::to_string(rank);
  int status = this->m_uniclient->putrawdata(step, completevarName, bs, polydata.GetPointer());
  if (status != 0)
  {
    throw std::runtime_error("failed to put data for step " + std::to_string(0));
  }
}

void InSitu::polyProcess(vtkSmartPointer<vtkPolyData> polyData, int step)
{
  int numCells = polyData->GetNumberOfPolys();
  // std::cout << "step " << step << " cell number " << numCells << std::endl;

  if (numCells > 0)
  {
    // surface area
    auto connectivityFilter = vtkSmartPointer<vtkConnectivityFilter>::New();
    connectivityFilter->SetInputData(polyData);
    connectivityFilter->SetExtractionModeToAllRegions();
    connectivityFilter->ColorRegionsOn();
    connectivityFilter->Update();

    int nBlobs = connectivityFilter->GetNumberOfExtractedRegions();

    // std::cout <<" found " << nBlobs << " blobs" << std::endl;
    // get the largetst surface
    connectivityFilter->SetInputData(polyData);
    connectivityFilter->SetExtractionModeToLargestRegion();
    connectivityFilter->ColorRegionsOn();

    auto massProperties = vtkSmartPointer<vtkMassProperties>::New();
    massProperties->SetInputConnection(connectivityFilter->GetOutputPort());

    // std::cout << "Surface area of largest blob is " << massProperties->GetSurfaceArea()
    //          << std::endl;
  }
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

void InSitu::write(
  const GrayScott& sim, std::string varName, size_t step, int rank, std::string recordInfo)
{
  std::vector<double> u = sim.u_noghost();

  std::string VarNameU = varName;

  std::array<int, 3> indexlb = { { (int)sim.offset_x, (int)sim.offset_y, (int)sim.offset_z } };
  std::array<int, 3> indexub = { { (int)(sim.offset_x + sim.size_x - 1),
    (int)(sim.offset_y + sim.size_y - 1), (int)(sim.offset_z + sim.size_z - 1) } };

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

// extract the data into the polydata and then write data into the staging
// return the blockid for future use
std::string InSitu::extractAndwrite(
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

  clock_gettime(CLOCK_REALTIME, &end); /* mark end time */
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

void InSitu::registerRtrigger(int num)
{
  // add the init trigger
  std::string triggerNameInit = "InitTrigger";

  // declare the function and the parameters
  std::vector<std::string> initCheckParameters;
  std::vector<std::string> initComparisonParameters;
  std::vector<std::string> initActionParameters;

  initComparisonParameters.push_back("0");

  // how many seconds
  int anaTimeint = 0 * 1000000;
  std::string anaTime = std::to_string(anaTimeint);
  // declare the function and the parameters
  std::vector<std::string> checkParameters;
  std::vector<std::string> comparisonParameters;
  std::vector<std::string> actionParameters;

  checkParameters.push_back(anaTime);
  comparisonParameters.push_back("0");
  actionParameters.push_back("adiosWrite");

  // register the trigger
  std::array<int, 3> indexlb = { { 0, 0, 0 } };
  std::array<int, 3> indexub = { { 1287, 1287, 1287 } };

  // register multiple in-staging executions
  for (int i = 0; i < num; i++)
  {
    std::string triggerNameExp = "InsituTriggerExp_" + std::to_string(i);
    initActionParameters.push_back(triggerNameExp);
    DynamicTriggerInfo tgInfo("InsituExpCheck", checkParameters, "InsituExpCompare",
      comparisonParameters, "InsituExpAction", actionParameters);

    m_uniclient->registerTrigger(3, indexlb, indexub, triggerNameExp, tgInfo);
  }

  DynamicTriggerInfo initTgInfo("defaultCheckGetStep", initCheckParameters, "defaultComparisonStep",
    initComparisonParameters, "defaultActionSartDt", initActionParameters);
  m_uniclient->registerTrigger(3, indexlb, indexub, triggerNameInit, initTgInfo);
}

MetricsSet InSitu::naiveGet()
{
  MetricsSet mset;
  mset.T = this->m_metricManager.getLastNmetrics("T", 1)[0];
  mset.At = this->m_metricManager.getLastNmetrics("At", 1)[0];
  mset.S = this->m_metricManager.getLastNmetrics("S", 1)[0];
  mset.Al = this->m_metricManager.getLastNmetrics("Al", 1)[0];
  mset.W = this->m_metricManager.getLastNmetrics("W", 1)[0];
  return mset;
}

MetricsSet InSitu::estimationGet(std::string lastDecision, int currStep)
{
  // total step can be acquired from the in-situ code
  MetricsSet mset = this->naiveGet();
  // let the initial value equals with each other and then update particular one
  MetricsSet emset = mset;
  double p = 0;
  double Tmin;

  if (lastDecision == "tightly")
  {
    // loosely coupled case is outdated
    if (mset.Al < mset.At)
    {
      emset.Al = mset.At;
    }
    p = mset.At;
    this->m_Tmin = std::min(this->m_Tmin, mset.T);
    emset.T = m_Tmin;
  }
  else if (lastDecision == "loosely")
  {
    // tightly coupled values are outdated
    // if the al is 0, the staging task did not finish and we should still use the old values
    if (mset.Al < mset.At && mset.Al != 0)
    {
      emset.At = mset.Al;
    }
    p = mset.T;
  }
  else
  {
    throw std::runtime_error("lastDecision is invalid");
  }
  this->m_savg = this->m_savg + 1.0 * (mset.S - this->m_savg) / (1.0 * currStep);
  this->m_pavg = this->m_pavg + 1.0 * (p - this->m_pavg) / (1.0 * currStep);
  double esim = (this->m_totalStep - currStep) * (this->m_savg + this->m_pavg);
  emset.S = esim;

  return emset;
}

void InSitu::decideTaskPlacement(
  int step, std::string strategy, bool& ifTCAna, bool& ifWriteToStage)
{

  if (step <= 1)
  {
    ifTCAna = true;
    ifWriteToStage = false;

    return;
  }
  if (step == 2)
  {
    ifTCAna = false;
    ifWriteToStage = true;

    return;
  }

  // get the last situation
  std::string lastDecision;
  if (ifTCAna == true && ifWriteToStage == false)
  {
    lastDecision = "tightly";
  }
  else if (ifTCAna == false && ifWriteToStage == true)
  {
    lastDecision = "loosely";
  }
  else
  {
    throw std::runtime_error("last decisions are in valid");
  }

  // these two variables need to be restet every time
  ifTCAna = false;
  ifWriteToStage = false;
  double currentsavedTime = 0;

  MetricsSet mset;
  if (strategy == "dynamicNaive")
  {
    mset = this->naiveGet();
  }
  else if (strategy.find("dynamicEstimation") != std::string::npos)
  {
    // we assume the freq is 1 and currSimStep equals to the currInSituStep
    mset = this->estimationGet(lastDecision, step);
  }
  else
  {
    throw std::runtime_error("unsupprted strategy");
  }

  if (mset.S >= (mset.W + mset.Al))
  {
    if (mset.At >= mset.T)
    {
      ifWriteToStage = true;
    }
    else
    {
      ifTCAna = true;
    }
    // currentsavedTime = abs(At - T);
  }
  else
  {
    if (mset.At + mset.S >= (mset.T + mset.W + mset.Al))
    {
      ifWriteToStage = true;
    }
    else
    {
      ifTCAna = true;
    }
    // currentsavedTime = abs((At + S) - (T + W + Al));
  }

  // totalsavedTime = totalsavedTime + currentsavedTime;
  // std::string metricName = "Saved";
  // gsinsitu.m_metricManager.putMetric(metricName, currentsavedTime);
}

// stationalry high
void shigh()
{
  int workLoad = 1500;
  int num = 500;
  std::vector<double> v(num, 0);
  double results = 0;
  for (int i = 0; i < workLoad; i++)
  {
    for (int j = 0; j < num; j++)
    {
      double rf = (double)rand() / RAND_MAX;
      v[j] = 0 + rf * (0.1 * i - 0);
    }
    for (int j = 0; j < num; j++)
    {
      results = v[j] + results;
    }
    std::this_thread::sleep_for(1ms);
  }
  return;
}

void slow()
{
  int workLoad = 60;
  int num = 500;
  std::vector<double> v(num, 0);
  double results = 0;
  for (int i = 0; i < workLoad; i++)
  {
    for (int j = 0; j < num; j++)
    {
      double rf = (double)rand() / RAND_MAX;
      v[j] = 0 + rf * (0.1 * i - 0);
    }
    for (int j = 0; j < num; j++)
    {
      results = v[j] + results;
    }
    std::this_thread::sleep_for(1ms);
  }
  return;
}
// varied low high low
void vlhl(int step, int totalStep)
{
  int workLoad = 60;
  int workLoadhigh = 1500;

  // 0-bound1 steady
  int bound1 = (totalStep / 6) + 5;
  // bound1-bound2 increase
  int bound2 = (2 * totalStep / 6) + 5;
  // bound2-bound3 steady
  int bound3 = (3 * totalStep / 6) + 5;
  // bound2-bound4 decrease
  int bound4 = (4 * totalStep / 6) + 5;
  // bound4-last steady

  // 0.2 is a good value to make sure it increase gradually
  // try this later
  double rate = 0.2;

  if (step < (bound1))
  {
    // first part
  }
  else if (step > bound1 && step < bound2)
  {
    workLoad = workLoadhigh - workLoadhigh * (1.0 / (1.0 + rate * (step - bound1)));
  }
  else if (step >= bound2 && step <= bound3)
  {
    workLoad = workLoadhigh;
  }
  else if (step > bound3 && step < bound4)
  {
    workLoad = workLoadhigh - workLoadhigh * (1.0 / (1.0 + rate * (bound4 - step)));
  }
  else
  {
    // last part is low
    workLoad = 60;
  }

  int num = 500;
  std::vector<double> v(num, 0);
  double results = 0;
  for (int i = 0; i < workLoad; i++)
  {
    for (int j = 0; j < num; j++)
    {
      double rf = (double)rand() / RAND_MAX;
      v[j] = 0 + rf * (0.1 * i - 0);
    }
    for (int j = 0; j < num; j++)
    {
      results = v[j] + results;
    }
    std::this_thread::sleep_for(1ms);
  }

  return;
}

void vhlh(int step, int totalStep)
{
  int workLoadhigh = 1500;
  int workLoadlow = 60;
  int workLoad = 0;

  // 0-bound1 steady
  int bound1 = (totalStep / 5) - 6;
  // bound1-bound2 increase
  int bound2 = (2 * totalStep / 5) - 6;
  // bound2-bound3 steady
  int bound3 = (3 * totalStep / 5) + 6;
  // bound2-bound4 decrease
  int bound4 = (4 * totalStep / 5) + 6;
  // bound4-last steady

  // 0.2 is a good value to make sure it increase gradually
  // try this later
  double rate = 0.2;

  if (step < (bound1))
  {
    workLoad = workLoadhigh;
    // first part
  }
  else if (step > bound1 && step < bound2)
  {
    workLoad = workLoadhigh - workLoadhigh * (1.0 / (1.0 + rate * (bound2 - step)));
  }
  else if (step >= bound2 && step <= bound3)
  {
    workLoad = workLoadlow;
  }
  else if (step > bound3 && step < bound4)
  {
    workLoad = workLoadhigh - workLoadhigh * (1.0 / (1.0 + rate * (step - bound3)));
  }
  else
  {
    // last part
    workLoad = workLoadhigh;
  }

  int num = 500;
  std::vector<double> v(num, 0);
  double results = 0;
  for (int i = 0; i < workLoad; i++)
  {
    for (int j = 0; j < num; j++)
    {
      double rf = (double)rand() / RAND_MAX;
      v[j] = 0 + rf * (0.1 * i - 0);
    }
    for (int j = 0; j < num; j++)
    {
      results = v[j] + results;
    }
    std::this_thread::sleep_for(1ms);
  }
  return;
}

void vmultiple(int step, int totalStep)
{

  int workLoadhigh = 250;
  int workLoadMiddle = 100;
  int workLoadlow = 60;
  int workLoad = 0;
  int num = 500;
  std::vector<double> v(num, 0);
  double results = 0;

  if (step % 8 == 0 || step % 8 == 1 || step % 8 == 2)
  {
    workLoad = workLoadlow;
  }
  if (step % 8 == 3 || step % 8 == 7)
  {
    workLoad = workLoadMiddle;
  }
  if (step % 8 == 4 || step % 8 == 5 || step % 8 == 6)
  {
    workLoad = workLoadhigh;
  }

  for (int i = 0; i < workLoad; i++)
  {
    for (int j = 0; j < num; j++)
    {
      double rf = (double)rand() / RAND_MAX;
      v[j] = 0 + rf * (0.1 * i - 0);
    }
    for (int j = 0; j < num; j++)
    {
      results = v[j] + results;
    }
    std::this_thread::sleep_for(1ms);
  }
  return;
}

void InSitu::dummyAna(int step, int totalStep, std::string anatype)
{

  if (anatype == "S_HIGH")
  {
    shigh();
  }
  else if (anatype == "S_LOW")
  {
    slow();
  }
  else if (anatype == "V_LHL")
  {
    vlhl(step, totalStep);
  }
  else if (anatype == "V_HLH")
  {
    vhlh(step, totalStep);
  }
  else if (anatype == "V_MULTIPLE")
  {
    vmultiple(step, totalStep);
  }
  else
  {
    std::cout << "unsupported cases for tightly coupled in-situ" << std::endl;
  }
  return;
}