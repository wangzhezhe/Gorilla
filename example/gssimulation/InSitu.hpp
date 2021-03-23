#ifndef __GSINSITU_H__
#define __GSINSITU_H__

#include <mpi.h>

#include "../../commondata/metadata.h"
#include "../client/ClientForSim.hpp"
#include "../metricManager/metricManager.hpp"
#include "gray-scott.h"
#include "settings.h"

#include <vector>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <chrono>
#include <thread>

namespace tl = thallium;
using namespace GORILLA;
using namespace std::chrono_literals;

/*
inline std::string loadMasterAddr(std::string masterConfigFile)
{

  std::ifstream infile(masterConfigFile);
  std::string content = "";
  std::getline(infile, content);
  // spdlog::debug("load master server conf {}, content -{}-", masterConfigFile,content);
  if (content.compare("") == 0)
  {
    std::getline(infile, content);
    if (content.compare("") == 0)
    {
      throw std::runtime_error("failed to load the master server\n");
    }
  }
  return content;
}
*/

struct MetricsSet
{
  MetricsSet() = default;
  double At = 0;
  double S = 0;
  double T = 0;
  double Al = 0;
  double W = 0;
  ~MetricsSet() = default;
};

class InSitu
{

public:
  // init the metric in the constructor
  InSitu(int metribuffer = 80)
    : m_metricManager(metribuffer){};
  InSitu(tl::engine* clientEnginePtr, std::string addrServer, int rank, int totalStep,
    int metribuffer = 80)
    : m_metricManager(metribuffer)
    , m_totalStep(totalStep)
  {
    m_uniclient = new ClientForSim(clientEnginePtr, addrServer, rank);
  };

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

  void writePolyDataFile(vtkSmartPointer<vtkPolyData> polyData, std::string fileName);

  void writeImageData(const GrayScott& sim, std::string fileName);

  void isosurfacePolyNum(const GrayScott& sim, int rank, double iso, int step);

  void write(
    const GrayScott& sim, std::string varName, size_t step, int rank, std::string recordInfo = "");

  std::string extractAndwrite(
    const GrayScott& sim, size_t step, int rank, std::string recordInfo = "");

  vtkSmartPointer<vtkPolyData> getPoly(const GrayScott& sim, double iso);

  void stagePolyData(
    vtkSmartPointer<vtkPolyData> polydata, std::string varName, int step, int rank);

  void polyProcess(vtkSmartPointer<vtkPolyData> polyData, int step);

  void registerRtrigger(int num);

  void dummyAna(int step, int totalStep, std::string anatype);

  ClientForSim* m_uniclient = NULL;

  void decideTaskPlacement(int step, std::string strategy, bool& ifTCAna, bool& ifWriteToStage);

  MetricsSet estimationGet(std::string lastDecision, int currStep);

  MetricsSet naiveGet();

  // metric monitor
  MetricManager m_metricManager;

  int m_totalStep = 0;

  double m_Tmin = std::numeric_limits<double>::max();
  double m_pavg = 0;
  double m_savg = 0;
  double m_sim = 0;

  ~InSitu()
  {
    if (m_uniclient != NULL)
    {
      delete m_uniclient;
    }
  }
};

#endif
