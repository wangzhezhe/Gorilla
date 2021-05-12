
#include <iostream>
#include <utils/uuid.h>

#include "InSituAnaCommon.hpp"

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

#include <chrono>

#include <thread>

using namespace std::chrono_literals;

namespace GORILLA
{

void InSituAnaCommon::polyProcess2(vtkSmartPointer<vtkPolyData> polyData, std::string blockIDSuffix)
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

void InSituAnaCommon::polyProcess(vtkSmartPointer<vtkPolyData> polyData)
{
  // std::cout << "debug polyProcess start" << std::endl;

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

void vinconsistency(int step, int dataID, int totalStep)
{
  int workLoadhigh = 10000;
  int workLoadlow = 50;
  int workLoad = 0;
  double rate = 0.1;
  int num = 500;
  std::vector<double> v(num, 0);
  double results = 0;

  if (dataID % 2 == 0)
  {
    workLoad = workLoadlow;
  }
  else
  {
    if (step <= totalStep / 3)
    {
      workLoad = workLoadlow;
    }
    else
    {
      // increase gradually
      workLoad = workLoadhigh - workLoadhigh * (1.0 / (1.0 + rate * (step - totalStep / 3)));
    }
  }
  // unit of the work
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
}

void InSituAnaCommon::dummyAna(int step, int dataID, int totalStep, std::string anatype)
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
  else if (anatype == "V_INCONSISTENCY")
  {
    vinconsistency(step, dataID, totalStep);
  }
  else
  {
    std::cout << "unsupported cases for tightly coupled in-situ" << std::endl;
  }
  return;
}
#ifdef USE_GPU
void InSituAnaCommon::testVTKm()
{

  this->m_vtkmwrapper.testclip();
}
#endif
}