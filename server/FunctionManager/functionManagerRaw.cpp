#include "functionManagerRaw.h"
#include <spdlog/spdlog.h>

#include <thread>
#include <vtkAppendPolyData.h>
#include <vtkCenterOfMass.h>
#include <vtkCleanPolyData.h>
#include <vtkConnectivityFilter.h>
#include <vtkFlyingEdges3D.h>
#include <vtkImageImport.h>
#include <vtkMarchingCubes.h>
#include <vtkMassProperties.h>
using namespace std::chrono_literals;

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

void FunctionManagerRaw::testisoExec(
  std::string blockCompleteName, const std::vector<std::string>& parameters)
{
  // std::cout << "debug testisoExec start" << std::endl;

  // std::cout << "testisoExec for block: " << blockCompleteName << std::endl;
  // get block summary
  BlockSummary bs = this->m_blockManager->getBlockSummary(blockCompleteName);
  // process get the block summary
  // print the message here
  // bs.printSummary();
  void* blockData = nullptr;
  this->m_blockManager->getBlock(blockCompleteName, bs.m_backend, blockData);

  // maybe block not exist here, since put process not finish? this might be an issue anyway
  if (blockData == nullptr)
  {
    std::cout << "testisoExec error, blockCompleteName: " << blockCompleteName << " not exist"
              << std::endl;
    return;
  }

  // std::cout << "debug testisoExec ok get block" << std::endl;

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
  auto isoExtraction = vtkSmartPointer<vtkFlyingEdges3D>::New();
  isoExtraction->SetInputConnection(importer->GetOutputPort());
  isoExtraction->ComputeNormalsOn();
  isoExtraction->SetValue(0, 0.5);
  isoExtraction->Update();

  vtkSmartPointer<vtkPolyData> polyData = isoExtraction->GetOutput();

  // int numCells = polyData->GetNumberOfPolys();
  // std::cout << "blockCompleteName " << blockCompleteName << " cell number " << numCells
  //          << std::endl;

  // get the largest region

  this->m_insituana.polyProcess(polyData);
  // std::cout << "debug testisoExec finish" << std::endl;

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

  this->m_insituana.polyProcess2(mergedPolydata, blockIDSuffix);

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