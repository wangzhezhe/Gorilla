#include "functionManagerRaw.h"
#include <spdlog/spdlog.h>

#include <vtkAppendPolyData.h>
#include <vtkCenterOfMass.h>
#include <vtkCleanPolyData.h>
#include <vtkConnectivityFilter.h>
#include <vtkImageImport.h>
#include <vtkMarchingCubes.h>
#include <vtkMassProperties.h>

namespace GORILLA
{
// if the execute aims to generate the new version of the data
// the return value is the id of original function,
// then at the trigger, replace the original id and use the same index (the data is on the same
// node) we do not need to update the metadata when there is multiple version of the dataduring the
// workflow execution

std::string FunctionManagerRaw::execute(FunctionManagerRaw* fmr, const BlockSummary& bs,
  void* inputData, std::string fiunctionName, const std::vector<std::string>& parameters)
{

  if (this->m_functionMap.find(fiunctionName) == this->m_functionMap.end())
  {
    spdlog::info("the function {} is not registered into the map", fiunctionName);
    return "";
  }
  // since it calles functionPointer
  // this functionPointer is beyond the scope of the class
  // it is better to make it as a static function???
  rawdatafunctionPointer fp = this->m_functionMap[fiunctionName];
  std::string results = fp(this, bs, inputData, parameters);
  return results;
}

bool FunctionManagerRaw::registerFunction(std::string functionName, rawdatafunctionPointer fp)
{

  m_functionMapMutex.lock();
  if (this->m_functionMap.find(functionName) != this->m_functionMap.end())
  {
    return false;
  }

  m_functionMap[functionName] = fp;
  m_functionMapMutex.unlock();

  return true;
}

void polyProcess(vtkSmartPointer<vtkPolyData> polyData, std::string blockIDSuffix)
{
  int numCells = polyData->GetNumberOfPolys();
  std::cout << "blockIDSuffix " << blockIDSuffix << " cell number " << numCells << std::endl;

  if (numCells > 0)
  {
    // surface area
    auto connectivityFilter = vtkSmartPointer<vtkConnectivityFilter>::New();
    connectivityFilter->SetInputData(polyData);
    connectivityFilter->SetExtractionModeToAllRegions();
    connectivityFilter->ColorRegionsOn();
    connectivityFilter->Update();

    int nBlobs = connectivityFilter->GetNumberOfExtractedRegions();

    std::cout << "blockIDSuffix " << blockIDSuffix << " found " << nBlobs << " blobs" << std::endl;

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

void polyProcess(vtkSmartPointer<vtkPolyData> polyData)
{
  int numCells = polyData->GetNumberOfPolys();
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

void FunctionManagerRaw::dummyAna(int step, int totalStep)
{
  // it takes around 0.2s when the workload is 150
  // dummy high
  int workLoad = 120;
  int executeIteration = 1 + 10;

  // dummy low
  // int workLoad = 80;
  // int executeIteration = 1;

  /*
  dummy peak, low high low


  int executeIteration = 1;
  int workLoad = 80;
  // steady
  int bound1 = (totalStep / 3);
  // increase
  int bound2 = (3 * totalStep / 6);
  // steady
  int bound3 = (3 * totalStep / 6);
  // decrease
  int bound4 = (4 * totalStep / 6);

  //0.2 is a good value to make sure it increase gradually
  //try this later
  double rate = 1.0;

  if (step < (bound1)) {
    // first part
    executeIteration = 1;
  }
  else if (step >= bound1 && step < bound2) {
    workLoad = 120 - 40 * (1.0 / (1.0 + rate * (step - bound1)));
    executeIteration = 1 + 10 - 10 * (1.0 / (1.0 + rate * (step - bound1)));
  }
  else if (step >= bound2 && step <= bound3) {
    workLoad = 120;
    executeIteration = 11;
  }
  else if (step > bound3 && step <= bound4) {
    workLoad = 120 - 40 * (1.0 / (1.0 + rate*(bound4 - step)));
    executeIteration = 1 + 10 - 10 * (1.0 / (1.0 + rate * (bound4 - step)));
  }
  else {
    // last part is low
    workLoad = 80;
    executeIteration = 1;
  }
  */
  // workload with fixed time
  /*
  struct timespec start, end;
  clock_gettime(CLOCK_REALTIME, &start);
  while (true)
  {
    double temp = 0.01 * 0.02 * 0.03 * 0.04 * 0.05 / 0.06;
    clock_gettime(CLOCK_REALTIME, &end);

    double timespan =
      (end.tv_sec - start.tv_sec) * 1.0 + (end.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
    if (timespan > executeTime)
    {
      break;
    }
  }
  */
  int num = 600;
  std::vector<double> v(num, 0);
  double results = 0;
  for (int i = 0; i < executeIteration; i++)
  {
    for (int j = 0; j < workLoad; j++)
    {
      for (int k = 0; k < workLoad; k++)
      {
        for (int m = 0; m < num; m++)
        {
          double rf = (double)rand() / RAND_MAX;
          v[i] = 0 + rf * (0.1 * i * k - 0);
        }
        for (int m = 0; m < num; m++)
        {
          results = v[i] + results;
        }
      }
    }
  }
}

void FunctionManagerRaw::testisoExec(
  std::string blockCompleteName, const std::vector<std::string>& parameters)
{
  // std::cout << "testisoExec for block: " << blockCompleteName << std::endl;
  // get block summary
  BlockSummary bs = this->m_blockManager->getBlockSummary(blockCompleteName);
  // process get the block summary
  // print the message here
  // bs.printSummary();
  void* blockData = nullptr;
  this->m_blockManager->getBlock(blockCompleteName, bs.m_backend, blockData);

  std::array<int, 3> indexlb = bs.m_indexlb;
  std::array<int, 3> indexub = bs.m_indexub;

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

  // int numCells = polyData->GetNumberOfPolys();
  // std::cout << "blockCompleteName " << blockCompleteName << " cell number " << numCells
  //          << std::endl;

  // get the largest region
  polyProcess(polyData);

  return;
}

std::string FunctionManagerRaw::aggregateProcess(ClientForStaging* uniclient,
  std::string blockIDSuffix, std::string fiunctionName, const std::vector<std::string>& parameters)
{
  // aggregate poly from different processes
  // blocksuppary contains the var and step info
  std::cout << "debug start to process with suffix " << blockIDSuffix << std::endl;

  // get the block BlockSummary list from all stage servers with current blockIDSuffix
  std::vector<vtkSmartPointer<vtkPolyData> > polyList =
    uniclient->aggregatePolyBySuffix(blockIDSuffix);
  std::cout << "aggregate data for suffix " << blockIDSuffix << std::endl;

  vtkSmartPointer<vtkAppendPolyData> appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();

  for (int i = 0; i < polyList.size(); i++)
  {
    // std::cout << "block " << i << " cell number: " << polyList[i]->GetNumberOfPolys() <<
    // std::endl;
    // try to aggregate into one
    appendFilter->AddInputData(polyList[i]);
  }
  vtkSmartPointer<vtkCleanPolyData> cleanFilter = vtkSmartPointer<vtkCleanPolyData>::New();
  cleanFilter->SetInputConnection(appendFilter->GetOutputPort());
  cleanFilter->Update();

  // generate the merged polydata
  vtkSmartPointer<vtkPolyData> mergedPolydata = cleanFilter->GetOutput();

  std::cout << "merge into the large poly: " << mergedPolydata->GetNumberOfPolys() << std::endl;

  polyProcess(mergedPolydata, blockIDSuffix);

  // start the filters

  // get the data back based on the size and the name

  // assemble them into one poly data

  // process the poly data

  // if the last step, call the timer
  if (blockIDSuffix.find("20") != std::string::npos)
  {
    // end the timer, for the master node
    uniclient->endTimer(uniclient->m_masterAddr);
  }

  return "";
}

}