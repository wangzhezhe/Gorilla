#include "InSituTransferCommon.hpp"
#include <vtkXMLDataSetWriter.h>
#include <vtkXMLImageDataWriter.h>

namespace GORILLA
{

void InSituTransferCommon::writePolyDataFile(
  vtkSmartPointer<vtkPolyData> polyData, std::string fileName)
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

void InSituTransferCommon::stagePolyData(
  vtkSmartPointer<vtkPolyData> polydata, std::string varName, int step, int rank)
{

  // there is one data block for this case
  std::string blockName = "block_0";
  ArraySummary as(blockName, 1, 1);
  std::vector<ArraySummary> aslist;
  aslist.push_back(as);
  size_t dims = 3;

  //TODO this boundry value might still necessary for index, it may similar to the outline of the polydata
  //we will add this as needed, currently, for the poly data, we set the lb and ub as 0
  std::array<int, 3> indexlb = { { 0, 0, 0 } };
  std::array<int, 3> indexub = { { 0, 0, 0 } };
  std::string dataType = DATATYPE_VTKEXPLICIT;

  //The bs should be updated, the lb and ub might be not useful if we use other format?
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

// TODO this only works for the image with one scalar, we need more versatile method to transfer
// multiple arrays here we need to extend the data transfer method here
void InSituTransferCommon::stageImageData(vtkSmartPointer<vtkImageData> imageData,
  std::string varName, size_t step, int rank, std::string recordInfo)
{
  //  of element
  //  size of every element
  size_t elemSize = imageData->GetScalarSize();
  size_t elemNum = imageData->GetNumberOfPoints();

  // the index is unnecessary for this vtk backend
  int* extentArray = imageData->GetExtent();
  std::array<int, 3> indexlb = { { extentArray[0], extentArray[2], extentArray[4] } };
  std::array<int, 3> indexub = { { extentArray[1], extentArray[3], extentArray[5] } };

  // we can use the varname plus step to filter if the data partition need to be processed
  // and there is only need a thin index to get all the associated blocks
  std::string blockid = varName + "_" + std::to_string(step) + "_" + std::to_string(rank);

  // this part belongs to the common operation
  // generate raw data summary block
  ArraySummary as(blockid, elemSize, elemNum);
  std::vector<ArraySummary> aslist;
  aslist.push_back(as);

  // TODO update this part, we already have the vtk image data with full meta info
  // just send it to the staging service and reconstruct the same vtk image data
  BlockSummary bs(aslist, DATATYPE_CARGRID, blockid, 3, indexlb, indexub, recordInfo);
  int status = m_uniclient->putrawdata(step, varName, bs, imageData->GetScalarPointer());

  if (status != 0)
  {
    throw std::runtime_error("failed to put data for step " + std::to_string(step));
  }
}

void InSituTransferCommon::writeImageDataFile(
  vtkSmartPointer<vtkImageData> imageData, std::string fileName)
{

  // Write the file by vtkXMLDataSetWriter
  vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
  writer->SetFileName(fileName.data());

  // get the specific polydata and check the results
  writer->SetInputData(imageData);
  // writer->SetInputData(importer->GetOutputPort());
  // Optional - set the mode. The default is binary.
  writer->SetDataModeToBinary();
  // writer->SetDataModeToAscii();
  writer->Write();
}

// this is the old capability which is depricated
// we do not use this api trigger api anymore
void InSituTransferCommon::registerRtrigger(int num)
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

}