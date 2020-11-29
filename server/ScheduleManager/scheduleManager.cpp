

#include "./scheduleManager.h"


namespace GORILLA
{
bool ScheduleManager::oktoPutMem(size_t objSize)
{
  if (objSize > this->m_avaliableMem)
  {
    return false;
  }
  return true;
}

void ScheduleManager::releaseMem(size_t memSize)
{
  this->m_memMutex.lock();
  m_avaliableMem = m_avaliableMem + memSize;
  this->m_memMutex.unlock();
  return;
}

void ScheduleManager::assignMem(size_t memSize)
{
  this->m_memMutex.lock();
  m_avaliableMem = m_avaliableMem - memSize;
  this->m_memMutex.unlock();
  return;
}

}