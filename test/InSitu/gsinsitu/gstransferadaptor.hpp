#ifndef __GSTRANSFERADAPTOR_H__
#define __GSTRANSFERADAPTOR_H__

#include <mpi.h>

#include "gray-scott.h"
#include "settings.h"
#include <memory>

#include <chrono>
#include <thread>
#include <vector>
#include <vtkImageData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <insitu/InSituTransferCommon.hpp>


namespace tl = thallium;
using namespace GORILLA;
using namespace std::chrono_literals;

// this adaptor transfer the gs data into the standard vtk format
// then it uses methods in transfer common to transfer the standard objects(only vtk objects
// currently) into the staging area it inherits the common transfer method based on vtk
// the adaptor should be out of the scope of the core src dir, since this is extended part based on
// core project
class GSTransferAdaptor : public InSituTransferCommon
{
public:
  GSTransferAdaptor(tl::engine* clientEnginePtr, std::string addrServer, int rank, int totalStep)
    : InSituTransferCommon(clientEnginePtr, addrServer, rank, totalStep){};

  vtkSmartPointer<vtkImageData> getImageData(const GrayScott& sim);

  vtkSmartPointer<vtkPolyData> getPolyData(const GrayScott& sim, double iso, int rank);

  // getImage and then write image
  void stageGsData(
    const GrayScott& sim, std::string varName, size_t step, int rank, std::string recordInfo = "");

  ~GSTransferAdaptor() {}
};

#endif
