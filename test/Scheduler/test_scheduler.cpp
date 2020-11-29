
#include "../server/ScheduleManager/scheduleManager.h"
using namespace GORILLA;

void test_scheduler_basic()
{
  tl::abt scope;
  ScheduleManager smanager("0.01G");
  size_t dataSize = 1024*1024*1024;
  bool oktoput = smanager.oktoPutMem(dataSize);
  if (oktoput == true)
  {
    throw std::runtime_error("failed test 1");
  }
  smanager.releaseMem(dataSize);
  oktoput = smanager.oktoPutMem(dataSize);
  if (oktoput == false)
  {
    throw std::runtime_error("failed test 2");
  }
}

int main()
{
  test_scheduler_basic();
}