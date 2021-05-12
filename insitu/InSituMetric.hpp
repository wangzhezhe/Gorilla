#ifndef __INSITUMETRICPIPELINE_H__
#define __INSITUMETRICPIPELINE_H__

#include "metricManager/metricManager.hpp"

namespace GORILLA
{

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

// data ana and controller based on the metric data
class InSituMetric
{
public:
  InSituMetric(){};

  // metric monitor
  MetricManager m_metricManager;

  // for deciding the task placement based on metric data
  MetricsSet estimationGet(std::string lastDecision, int currStep, double burden);

  MetricsSet naiveGet();

  void adjustment(int totalProcs, int step, MetricsSet& mset, bool& ifTCAna, bool& ifWriteToStage,
    std::string& lastDecision, double burden, bool lastStep);

  double m_Tmin = std::numeric_limits<double>::max();
  double m_pavg = 0;
  double m_savg = 0;
  double m_sim = 0;

  ~InSituMetric(){};
};

}

#endif