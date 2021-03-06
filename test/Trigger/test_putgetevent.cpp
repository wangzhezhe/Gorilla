#include <commondata/metadata.h>
#include <server/FunctionManager/functionManagerMeta.h>
#include <server/TriggerManager/triggerManager.h>
#include <unistd.h>

using namespace GORILLA;

void test_eventWrapper()
{
  std::cout << "---test_eventWrapper---" << std::endl;

  GORILLA::EventWrapper event;
  if (event.m_dims != 0)
  {
    throw std::runtime_error("the empty value is supposed to be zero");
  }

  GORILLA::EventWrapper event2(
    EVENT_DATA_PUT, "testVar", 1, 3, { { 0, 0, 0 } }, { { 512, 512, 512 } });
  event = event2;
  if (event.m_dims == 0)
  {
    throw std::runtime_error("failed for testing m_dims assignment");
  }
  return;
}

void test_putevent()
{
  std::cout << "---test_putevent---" << std::endl;
  GORILLA::EventWrapper event1(
    EVENT_DATA_PUT, "testVar", 1, 3, { { 0, 0, 0 } }, { { 512, 512, 512 } });
  GORILLA::EventWrapper event2(
    EVENT_DATA_PUT, "testVar", 1, 3, { { 0, 0, 0 } }, { { 512, 512, 512 } });
  GORILLA::EventWrapper event3(
    EVENT_DATA_PUT, "testVar", 1, 3, { { 0, 0, 0 } }, { { 512, 512, 512 } });

  GORILLA::DynamicTriggerManager dtm(2, NULL, NULL);
  dtm.putEvent("testVar", event1);
  dtm.putEvent("testVar", event2);
  dtm.putEvent("testVar", event3);
  dtm.putEvent("testVar1", event1);

  return;
}

void test_getevent()
{
  std::cout << "---test_getevent---" << std::endl;

  GORILLA::DynamicTriggerManager dtm(2, NULL, NULL);
  EventWrapper event = dtm.getEvent("testVarabc");
  if (event.m_dims != 0)
  {
    throw std::runtime_error("event is supposed to be the empty");
  }
  event.printInfo();

  GORILLA::EventWrapper event1(
    EVENT_DATA_PUT, "testVar", 1, 3, { { 0, 0, 0 } }, { { 512, 512, 512 } });
  dtm.putEvent("testVar", event1);
  event = dtm.getEvent("testVar");
  if (event.m_dims != 3)
  {
    throw std::runtime_error("event is supposed to be not the empty");
  }
  event.printInfo();

  event = dtm.getEvent("testVar");
  if (event.m_dims != 0)
  {
    throw std::runtime_error("event is supposed to be not the empty again");
  }
  event.printInfo();

  return;
}

int main()
{
  tl::abt scope;

  test_eventWrapper();

  test_putevent();

  test_getevent();
}