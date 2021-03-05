#include "../../metricManager/metricManager.hpp"

using namespace GORILLA;

void test_put_basic()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;
  MetricManager metricmanager(10);
  std::string testMetric = "testmetric";
  for (int i = 0; i < 8; i++)
  {
    metricmanager.putMetric(testMetric, i * 0.1);
  }
};

void test_put_more()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;
  MetricManager metricmanager(10);
  std::string testMetric = "testmetric";
  // try
  //{
  for (int i = 0; i < 20; i++)
  {
    metricmanager.putMetric(testMetric, i * 0.1);
  }
  //}
  // catch (std::exception& e)
  //{
  // std::cout << "catch exception: " << std::string(e.what()) << std::endl;
  //  return;
  //}
  // throw std::runtime_error("there should be a exception here");
};

void test_get1()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;
  MetricManager metricmanager(10);
  std::string testMetric = "testmetric";
  for (int i = 0; i < 5; i++)
  {
    metricmanager.putMetric(testMetric, i * 0.1);
  }

  std::vector<double> resutls = metricmanager.getLastNmetrics(testMetric, 3);

  if (resutls.size() != 3)
  {
    throw std::runtime_error("wrong results length " + std::to_string(resutls.size()));
  }

  for (int i = 0; i < resutls.size(); i++)
  {
    if (resutls[i] != (i + 2) * 0.1)
    {
      std::cout << "i " << i << " value " << resutls[i] << std::endl;
      throw std::runtime_error("failed return value");
    }
  }
};

void test_get2()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;
  MetricManager metricmanager(10);
  std::string testMetric = "testmetric";
  for (int i = 0; i < 5; i++)
  {
    metricmanager.putMetric(testMetric, i * 0.1);
  }
  try
  {
    // there should be an error
    std::vector<double> resutls = metricmanager.getLastNmetrics(testMetric, 10);
  }

  catch (std::exception& e)
  {
    std::cout << "catch exception: " << std::string(e.what()) << std::endl;
    return;
  }
  throw std::runtime_error("there should be a exception here");
};

void test_get3()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;
  MetricManager metricmanager(10);
  std::string testMetric = "testmetric";
  for (int i = 0; i < 26; i++)
  {
    metricmanager.putMetric(testMetric, i * 0.1);
  }

  std::vector<double> resutls = metricmanager.getLastNmetrics(testMetric, 3);

  if (resutls.size() != 3)
  {
    throw std::runtime_error("wrong results length " + std::to_string(resutls.size()));
  }

  for (int i = 0; i < resutls.size(); i++)
  {
    if (resutls[i] != (i + 26 - 3) * 0.1)
    {
      std::cout << "i " << i << " value " << resutls[i] << std::endl;
      throw std::runtime_error("failed return value");
    }
  }
}

void test_get4()
{
  // test the case that the store finish one cycle
  std::cout << "---test " << __FUNCTION__ << std::endl;
  MetricManager metricmanager(10);
  std::string testMetric = "testmetric";
  // put 9
  for (int i = 0; i < 9; i++)
  {
    metricmanager.putMetric(testMetric, i * 0.1);
  }
  int len = metricmanager.getBufferLen(testMetric);
  if (len != 9)
  {
    std::cout << "len is " << len << std::endl;
    throw std::runtime_error("failed to get len");
  }
  metricmanager.putMetric(testMetric, 0.001);
  len = metricmanager.getBufferLen(testMetric);
  // it becomes 1, we need to aovid the overlap between two pointers
  if (len != 9)
  {
    throw std::runtime_error("failed to get len when it is full");
  }
  double value = metricmanager.getLastNmetrics(testMetric, 1)[0];
  if (value != 0.001)
  {
    std::cout << "value " << value << std::endl;
    throw std::runtime_error("failed to fetch the value when the circular arry is full case 1");
  }
  metricmanager.putMetric(testMetric, 0.002);
  len = metricmanager.getBufferLen(testMetric);
  // it becomes 1, we need to aovid the overlap between two pointers
  if (len != 9)
  {
    throw std::runtime_error("failed to get len when it is full");
  }
  value = metricmanager.getLastNmetrics(testMetric, 1)[0];
  if (value != 0.002)
  {
    throw std::runtime_error("failed to fetch the value when the circular arry is full case 2");
  }

  for (int i = 0; i < 100; i++)
  {
    metricmanager.putMetric(testMetric, 0.001 * i);
    len = metricmanager.getBufferLen(testMetric);
    // it becomes 1, we need to aovid the overlap between two pointers
    if (len != 9)
    {
      throw std::runtime_error("failed to get len when it is full for multiple inserts");
    }
    value = metricmanager.getLastNmetrics(testMetric, 1)[0];
    if (value != 0.001 * i)
    {
      throw std::runtime_error(
        "failed to fetch the value when the circular arry for multiple inserts");
    }
  }

  return;
}

int main()
{
  tl::abt scope;

  test_put_basic();
  test_put_more();
  test_get1();
  test_get2();
  test_get3();
  test_get4();
}