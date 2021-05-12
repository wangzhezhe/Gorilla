#ifndef __INSITUTRANSFERCOMMON_H__
#define __INSITUTRANSFERCOMMON_H__

#include <client/ClientForSim.hpp>
#include <vtkImageData.h>
#include <vtkPolyData.h>

namespace tl = thallium;

namespace GORILLA
{

// this class is in charge of the data transfer things based on vtk object
// it contains the core transfer method that transferes standard vtk object into the staging area
// this is mainly used at simulation part
class InSituTransferCommon
{

public:
  InSituTransferCommon(tl::engine* clientEnginePtr, std::string addrServer, int rank, int totalStep)
    : m_totalStep(totalStep)
    , m_rank(rank)
  {
    // the lifecycle of client is hold by InSituTransferCommon, we use the unique ptr here
    m_uniclient = std::make_unique<ClientForSim>(clientEnginePtr, addrServer, rank);
  };

  // we do not allow the default constructor here
  InSituTransferCommon() = delete;

  void writeImageDataFile(vtkSmartPointer<vtkImageData> imageData, std::string fileName);

  void writePolyDataFile(vtkSmartPointer<vtkPolyData> polyData, std::string fileName);

  void stagePolyData(
    vtkSmartPointer<vtkPolyData> polydata, std::string varName, int step, int rank);
  
  // TODO we may have multiple ways to transfer the image data
  // how to optimize this data transfer need to be considered further
  void stageImageData(vtkSmartPointer<vtkImageData> imageData, std::string varName, size_t step,
    int rank, std::string recordInfo);

  void registerRtrigger(int num);

  void startwftimer()
  {
    m_uniclient->startTimer(m_uniclient->m_addrServer);
    std::cout << "start the timer\n";
  };

  void endwftimer()
  {
    m_uniclient->endTimer(m_uniclient->m_addrServer);
    std::cout << "end the timer\n";
  };

  ~InSituTransferCommon(){};

  std::unique_ptr<ClientForSim> m_uniclient;
  // own the client
  int m_totalStep = 0;
  int m_rank = 0;
};

}
#endif