#include "metricManager.hpp"
#include <fstream>
#include <iostream>

namespace GORILLA
{

void MetricManager::putMetric(std::string metricName, double metricValue)
{
  // if the metric name is null
  // create the array, insert the value and put it into the map
  CircularDoubleArray* arrayPtr;
  {
    std::lock_guard<tl::mutex> g(this->m_metricsMapMutex);
    if (this->m_metricsMap.find(metricName) == this->m_metricsMap.end())
    {
      // not found
      arrayPtr = new CircularDoubleArray(this->m_slot);
      this->m_metricsMap[metricName] = arrayPtr;
    }
    else
    {
      // found
      arrayPtr = this->m_metricsMap[metricName];
    }
  }
  arrayPtr->addElement(metricValue);

  return;
};

std::vector<double> MetricManager::getLastNmetrics(std::string metricName, uint32_t number)
{
  CircularDoubleArray* arrayPtr;
  {
    std::lock_guard<tl::mutex> g(this->m_metricsMapMutex);
    if (this->m_metricsMap.find(metricName) == this->m_metricsMap.end())
    {
      // not found
      throw std::runtime_error("metric name not exist");
    }
    else
    {
      // found
      arrayPtr = this->m_metricsMap[metricName];
    }
  }
  // extract the data from the array
  return arrayPtr->getLastN(number);
};

bool MetricManager::metricExist(std::string metricName)
{
  std::lock_guard<tl::mutex> g(this->m_metricsMapMutex);
  int count = this->m_metricsMap.count(metricName);
  if (count != 0)
  {
    return true;
  }
  return false;
}

void MetricManager::dumpall(int rank)
{
  // go through the map and dump value into a file
  // set the file
  std::string fname = "metricDump_" + std::to_string(rank) + ".csv";
  std::ofstream out(fname, std::ofstream::out | std::ofstream::trunc);

  for (auto& iter : this->m_metricsMap)
  {
    std::string metricName = iter.first;
    std::vector<double> values = iter.second->getAll();
    out << metricName;
    for (int i = 0; i < values.size(); i++)
    {
      out << "," << values[i];
    }
    out << '\n';
  }
}

int MetricManager::getBufferLen(std::string metricName)
{

  std::lock_guard<tl::mutex> g(this->m_metricsMapMutex);
  int count = this->m_metricsMap[metricName]->bufferLen();
  return count;
}

}