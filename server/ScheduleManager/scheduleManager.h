#ifndef SCHEDULEMANAGER_H
#define SCHEDULEMANAGER_H

#include <string>
#include <thallium.hpp>
#include <unordered_map>
#include <vector>
#include <math.h>

namespace tl = thallium;

namespace GORILLA
{

// store all kinds of metrics
// and the scheduler method
struct ScheduleManager
{

  ScheduleManager(){};

  ScheduleManager(std::string memLimit)
  {
    char unit = memLimit.back();
    if (unit != 'G')
    {
      throw std::runtime_error("the unit should be G for the memlimit");
    }

    std::string subStr = memLimit.substr(0, memLimit.size() - 1);

    double memGb = std::stod(subStr);

    this->m_avaliableMem = ceil(memGb * 1024 * 1024 * 1024);

    std::cout << "memLimit(Byte) for one server is " << this->m_avaliableMem << std::endl;
  };

  bool oktoPutMem(size_t objSize);

  void releaseMem(size_t memSize);

  void assignMem(size_t memSize);

  tl::mutex m_memMutex;
  // the unit is Byte
  size_t m_avaliableMem = 0;

  ~ScheduleManager(){};
};

}
#endif