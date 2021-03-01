
#ifndef __AMREXINSITU_H__
#define __AMREXINSITU_H__

#include <metricManager/metricManager.hpp>

class InSitu
{
public:
  InSitu(){};
  ~InSitu(){};

  //the metric manager that store particular metric information
  MetricManager m_metricManager(50);

  //the polycyengine that determin the task placement strategy
  //it should access the metricManager
};

#endif
