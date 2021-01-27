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
  try
  {
    for (int i = 0; i < 20; i++)
    {
      metricmanager.putMetric(testMetric, i * 0.1);
    }
  }
  catch (std::exception& e)
  {
    std::cout << "catch exception: " << std::string(e.what()) << std::endl;
    return;
  }
  throw std::runtime_error("there should be a exception here");
};

void test_get()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;
  MetricManager metricmanager(10);
  std::string testMetric = "testmetric";
  for (int i = 0; i < 5; i++)
  {
    metricmanager.putMetric(testMetric, i * 0.1);
  }

  std::vector<double> resutls = metricmanager.getNmetrics(testMetric, 3);

  if (resutls.size() != 3)
  {
    throw std::runtime_error("wrong results length " + std::to_string(resutls.size()));
  }

  for (int i = 0; i < resutls.size(); i++)
  {
    if (resutls[i] != i * 0.1)
    {
      throw std::runtime_error("failed return value");
    }
  }
};

void test_get_more()
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
    std::vector<double> resutls = metricmanager.getNmetrics(testMetric, 10);

    for (int i = 0; i < resutls.size(); i++)
    {
      if (resutls[i] != i * 0.1)
      {
        throw std::runtime_error("failed return value");
      }
    }
  }

  catch (std::exception& e)
  {
    std::cout << "catch exception: " << std::string(e.what()) << std::endl;
    return;
  }
  throw std::runtime_error("there should be a exception here");
};

int main()
{
  tl::abt scope;

  test_put_basic();
  test_put_more();
  test_get();
  test_get_more();
}