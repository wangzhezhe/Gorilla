#ifndef __WRITER_H__
#define __WRITER_H__

#include <mpi.h>

#include "../../commondata/metadata.h"
#include "../client/ClientForSim.hpp"
#include "gray-scott.h"
#include "settings.h"
#include <vector>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

namespace tl = thallium;
using namespace GORILLA;

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



class Writer
{

public:
  Writer(){};
  Writer(tl::engine* clientEnginePtr, std::string addrServer, int rank)
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

  void write(const GrayScott& sim, size_t step, int rank, std::string recordInfo = "");

  std::string extractAndwrite(
    const GrayScott& sim, size_t step, int rank, std::string recordInfo = "");

  void triggerRemoteAsync(int step, std::string blockidSuffix);

  vtkSmartPointer<vtkPolyData> getPoly(const GrayScott& sim, double iso);

  void polyProcess(vtkSmartPointer<vtkPolyData> polyData, int step);

  ClientForSim* m_uniclient = NULL;

  void registerRtrigger(int num)
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

    DynamicTriggerInfo initTgInfo("defaultCheckGetStep", initCheckParameters,
      "defaultComparisonStep", initComparisonParameters, "defaultActionSartDt",
      initActionParameters);
    m_uniclient->registerTrigger(3, indexlb, indexub, triggerNameInit, initTgInfo);
  }

  ~Writer()
  {
    if (m_uniclient != NULL)
    {
      delete m_uniclient;
    }
  }
};

#endif
