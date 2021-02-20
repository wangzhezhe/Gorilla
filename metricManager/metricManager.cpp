#include "metricManager.hpp"
#include <iostream>

namespace GORILLA
{

void MetricManager::putMetric(std::string metricName, double metricValue)
{
  // if the metric name is null
  // create the array, insert the value and put it into the map
  CircularDoubleArray* arrayPtr;
  this->m_metricsMapMutex.lock();
  int exist = this->m_metricsMap.count(metricName);
  if (exist == 0)
  {
    arrayPtr = new CircularDoubleArray(this->m_slot);
    this->m_metricsMap[metricName] = arrayPtr;
  }
  else
  {
    arrayPtr = this->m_metricsMap[metricName];
  }
  this->m_metricsMapMutex.unlock();

  // insert value into array ptr
  // different ptr use different lock
  arrayPtr->addElement(metricValue);
};

std::vector<double> MetricManager::getLastNmetrics(std::string metricName, uint32_t number)
{
  CircularDoubleArray* arrayPtr;
  this->m_metricsMapMutex.lock();
  int exist = this->m_metricsMap.count(metricName);
  if (exist == 0)
  {
    this->m_metricsMapMutex.unlock();
    throw std::runtime_error("metric name not exist");
  }
  else
  {
    arrayPtr = this->m_metricsMap[metricName];
  }

  this->m_metricsMapMutex.unlock();

  // extract the data from the array
  return arrayPtr->getLastN(number);
};

}