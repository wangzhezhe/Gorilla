#include "functionManagerRaw.h"
#include <spdlog/spdlog.h>

#include <vtkCenterOfMass.h>
#include <vtkConnectivityFilter.h>
#include <vtkMassProperties.h>
#include <vtkAppendPolyData.h>
#include <vtkCleanPolyData.h>

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

std::string FunctionManagerRaw::aggregateProcess(ClientForStaging* uniclient, std::string blockIDSuffix,
  std::string fiunctionName, const std::vector<std::string>& parameters)
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
    //std::cout << "block " << i << " cell number: " << polyList[i]->GetNumberOfPolys() << std::endl;
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