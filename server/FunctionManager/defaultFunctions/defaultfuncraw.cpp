#include "defaultfuncraw.h"
#include <adios2.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkMarchingCubes.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLPolyDataWriter.h>

#include <time.h>
#include <unistd.h>

#define BILLION 1000000000L

namespace GORILLA
{

std::string test(FunctionManagerRaw* fmr, const BlockSummary& bs, void* inputData,
  const std::vector<std::string>& parameters)
{
  // bs.printSummary();
  std::cout << "test the default data execution" << std::endl;
  // test call the server
  if (fmr->m_blockManager == NULL)
  {
    throw std::runtime_error("the pointer to the m_blockManager is null");
  }
  // do sth and put data into the blockManager if it is necessary
  // do some random operation, and give some load to the task
  // add the timer
  struct timespec start, end;
  double diff;
  clock_gettime(CLOCK_REALTIME, &start);

  double a = 0;
  for (int i = 0; i < 30 * 30 * 30; i++)
  {
    a = a + i * 0.1;
  }

  clock_gettime(CLOCK_REALTIME, &end);
  double timespan =
    (end.tv_sec - start.tv_sec) * 1.0 + (end.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
  std::cout << bs.m_dataType << " execute time span: " << timespan << std::endl;
  return "";
}

std::string valueRange(FunctionManagerRaw* fmr, const BlockSummary& bs, void* inputData,
  const std::vector<std::string>& parameters)
{

  if (parameters.size() < 1)
  {
    throw std::runtime_error("the parameter of value range should larger than 3");
  }

  std::string type = parameters[0];

  /*
  for (int i=0;i<parameters.size();i++){
      std::cout << "paras " << i << ": " << parameters[i] << std::endl;
  }
  */
  // range all the elements
  size_t elemNum = bs.getArrayElemNum(bs.m_blockid);
  size_t elemSize = bs.getArrayElemSize(bs.m_blockid);
  for (int i = 0; i < elemNum; i++)
  {
    if (elemSize == 8)
    {
      double tempData = *((double*)inputData + i);
      double threshA, threshB;

      // std::cout << "check data " << tempData << std::endl;

      if (type.compare("G") == 0)
      {
        threshA = atof(parameters[1].c_str());

        if (tempData >= threshA)
        {
          return "1";
        }
      }
      else if (type.compare("L") == 0)
      {
        threshA = atof(parameters[1].c_str());
        if (tempData <= threshA)
        {
          return "1";
        }
      }
      else if (type.compare("B") == 0)
      {
        if (parameters.size() != 3)
        {
          throw std::runtime_error("the length of parameters should be 3 when the type is B");
        }

        threshA = atof(parameters[1].c_str());
        threshB = atof(parameters[2].c_str());

        if (tempData >= threshA && tempData <= threshB)
        {
          return "1";
        }
      }
      else
      {
        throw std::runtime_error("unsuported type " + type);
      }
    }
    else
    {
      throw std::runtime_error("unsuported data length " + std::to_string(elemSize));
    }
  }

  return "0";
}

// this function assume that the input data is the cartisian grid
std::string testvtk(FunctionManagerRaw* fmr, const BlockSummary& bs, void* inputData,
  const std::vector<std::string>& parameters)
{
  // load to vtk data and execute write option

  std::cout << "debug testvtk\n";

  std::cout << bs.m_indexlb[0] << "," << bs.m_indexlb[1] << "," << bs.m_indexlb[2] << std::endl;
  std::cout << bs.m_indexub[0] << "," << bs.m_indexub[1] << "," << bs.m_indexub[2] << std::endl;

  for (int i = 0; i < 1000; i++)
  {
    double value = *((double*)(inputData) + i);
    std::cout << "index " << i << " value " << value << std::endl;
  }

  auto importer = vtkSmartPointer<vtkImageImport>::New();
  importer->SetDataSpacing(1, 1, 1);

  importer->SetDataOrigin(1.0 * bs.m_indexlb[2], 1.0 * bs.m_indexlb[1], 1.0 * bs.m_indexlb[0]);

  importer->SetWholeExtent(0, bs.m_indexub[2], 0, bs.m_indexub[1], 0, bs.m_indexub[0]);

  importer->SetDataExtentToWholeExtent();
  importer->SetDataScalarTypeToDouble();
  importer->SetNumberOfScalarComponents(1);
  importer->SetImportVoidPointer((double*)(inputData));
  importer->Update();

  // Write the file by vtkXMLDataSetWriter
  vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();

  std::string fileName = "testvtk.vti";
  writer->SetFileName(fileName.data());

  // get the specific polydata and check the results
  writer->SetInputConnection(importer->GetOutputPort());
  // writer->SetInputData(importer->GetOutputPort());
  // Optional - set the mode. The default is binary.
  writer->SetDataModeToBinary();
  // writer->SetDataModeToAscii();
  writer->Write();

  return "";
}

// there are some issues for using adios. maybe it is the problem of the multithread using of adios
// https://github.com/ornladios/ADIOS2/issues/2076
std::string adiosWrite(FunctionManagerRaw* fmr, const BlockSummary& bs, void* inputData,
  const std::vector<std::string>& parameters)
{
  // write the data into the adios bp file
  // the first parameter represent the status
  if (parameters.size() != 2)
  {
    throw std::runtime_error("the parameter len for adiosWrite should be 2");
  }

  if (fmr == NULL)
  {
    throw std::runtime_error("the function manager should not be null");
    return "FAIL";
  }

  char str[256];

  // simulate the writting process

  // int writeTime = 3.8;
  // usleep(writeTime * 1000000);

  // record time

  // there is still unsolved bug here

  size_t shapex = bs.m_indexub[0] - bs.m_indexlb[0] + 1;
  size_t shapey = bs.m_indexub[1] - bs.m_indexlb[1] + 1;
  size_t shapez = bs.m_indexub[2] - bs.m_indexlb[2] + 1;

  sprintf(str, "adios write step %s varName %s start %d %d %d count %d %d %d\n",
    parameters[0].c_str(), parameters[1].c_str(), bs.m_indexlb[0], bs.m_indexlb[1], bs.m_indexlb[2],
    shapex, shapey, shapez);
  std::cout << str << std::endl;
  if (fmr->m_statefulConfig == NULL)
  {
    throw std::runtime_error("pointer to statefulConfig should not be null");
  }
  std::cout << "--- debug engine name " << fmr->m_statefulConfig->m_engine.Name() << std::endl;
  std::cout << "--- debug type " << fmr->m_statefulConfig->m_engine.Type() << std::endl;
  std::cout << "--- debug io name " << fmr->m_statefulConfig->m_io.Name() << std::endl;

  // the lock is used to avoid the race condition between write of different thread
  fmr->m_statefulConfig->m_adiosLock.lock();

  adios2::Dims start = { (size_t)bs.m_indexlb[0], (size_t)bs.m_indexlb[1],
    (size_t)bs.m_indexlb[2] };
  adios2::Dims count = { shapex, shapey, shapez };

  // use set selection to modify the variable selection into the current partition
  const std::string variableName = "data";
  adios2::Variable<double> variableData =
    fmr->m_statefulConfig->m_io.InquireVariable<double>(variableName);
  variableData.SetSelection({ start, count });
  // adios2::Variable<int> variableStep = fmr->m_statefulConfig->m_io.InquireVariable<int>("step");

  int step = atoi(parameters[0].c_str());

  std::cout << "check step " << step << std::endl;
  double* temp = (double*)inputData;
  for (int i = 0; i < 10; i++)
  {
    std::cout << "check data " << *temp << std::endl;
    temp++;
  }

  // fmr->m_statefulConfig->m_engine.Put<int>(variableStep, &step);
  // fmr->m_statefulConfig->m_engine.PerformPuts();

  fmr->m_statefulConfig->m_engine.Put<double>(variableData, (double*)inputData);
  fmr->m_statefulConfig->m_engine.PerformPuts();

  fmr->m_statefulConfig->m_adiosLock.unlock();

  // write the current block into the adios
  // the partition of the staging server is useully less than the partition of the writer

  return "OK";
}

}