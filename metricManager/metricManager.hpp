
#ifndef METRICMANAGER_H
#define METRICMANAGER_H

#include "circularDoubleArray.hpp"
#include <cstdlib>
#include <stdint.h>
#include <string>
#include <thallium.hpp>
#include <unordered_map>
#include <vector>

namespace tl = thallium;

namespace GORILLA
{
/**
 * @brief the manager for metric data
 *
 */
class MetricManager
{
public:
  // the slot labels the number of elements stored by the manager
  MetricManager(size_t slot)
    : m_slot(slot){};

  ~MetricManager()
  {
    // go through the map and delete all
    // the value is allocated by new
    for (auto it = this->m_metricsMap.begin(); it != this->m_metricsMap.end(); ++it)
    {
      if (it->second != nullptr)
      {
        // remove the inner array
        delete it->second;
      }
    }
  };

  // copy constructor
  MetricManager(const MetricManager& other) = delete;

  // move constructor
  MetricManager(MetricManager&& other) = delete;

  // assignment operator
  MetricManager& operator=(const MetricManager& t) = delete;

  // move assignment operator
  MetricManager& operator=(MetricManager&& other) = delete;

  bool metricExist(std::string metricName);

  int getBufferLen(std::string metricName);

  // capabilities
  void putMetric(std::string metricName, double metricValue);
  // get the latest N matrix
  // return all values if there are less then N values
  std::vector<double> getLastNmetrics(std::string metricName, uint32_t number);

  void dumpall(int rank);

private:
  size_t m_slot;
  tl::mutex m_metricsMapMutex;
  // use layered data structure to avoid using a lock in large scope
  std::unordered_map<std::string, CircularDoubleArray*> m_metricsMap;
};

}

#endif