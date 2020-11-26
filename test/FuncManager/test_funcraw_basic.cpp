
#include "../../server/FunctionManager/defaultFunctions/defaultfuncraw.h"
#include "../../server/FunctionManager/functionManagerRaw.h"
#include <adios2.h>
#include <thallium.hpp>
#include <unistd.h>

void test_exengine_rangeG()
{
  std::cout << "---test_exengine_rangeG---" << std::endl;

  tl::abt scope;
  FunctionManagerRaw* exengine = new FunctionManagerRaw();

  std::vector<double> rawdata;

  int len = 10;

  for (int i = 0; i < len * len * len; i++)
  {
    rawdata.push_back(i * 0.1);
  }

  std::vector<std::string> parameters;

  parameters.push_back("G");
  parameters.push_back("0.05");

  BlockSummary bs(
    sizeof(double), len * len * len, DATATYPE_RAWMEM, "12345", 3, { { 0, 0, 0 } }, { { 9, 9, 9 } });
  std::string funcName = "valueRange";
  std::string results = exengine->execute(NULL, bs, rawdata.data(), funcName, parameters);
  if (results.compare("1") != 0)
  {
    throw std::runtime_error("failed for test_exengine_rangeG");
  }
  // make sure the delete operation is called
  // then the destructor will be called;
  delete exengine;
}
void test_exengine_rangeL()
{
  std::cout << "---test_exengine_rangeL---" << std::endl;
  tl::abt scope;
  FunctionManagerRaw* exengine = new FunctionManagerRaw();

  std::vector<double> rawdata;

  int len = 10;

  for (int i = 0; i < len * len * len; i++)
  {
    rawdata.push_back(i * 0.1);
  }

  std::vector<std::string> parameters;

  parameters.push_back("L");
  parameters.push_back("5");

  BlockSummary bs(
    sizeof(double), len * len * len, DATATYPE_RAWMEM, "12345", 3, { { 0, 0, 0 } }, { { 9, 9, 9 } });
  std::string funcName = "valueRange";
  std::string results = exengine->execute(NULL, bs, rawdata.data(), funcName, parameters);
  if (results.compare("1") != 0)
  {
    throw std::runtime_error("failed for test_exengine_rangeG");
  }
  // make sure the delete operation is called
  // then the destructor will be called;
  delete exengine;
}
void test_exengine_rangeB()
{
  std::cout << "---test_exengine_rangeB---" << std::endl;

  tl::abt scope;
  FunctionManagerRaw* exengine = new FunctionManagerRaw();

  std::vector<double> rawdata;

  int len = 10;

  for (int i = 0; i < len * len * len; i++)
  {
    rawdata.push_back(i * 0.1);
  }

  std::vector<std::string> parameters;

  parameters.push_back("B");
  parameters.push_back("0.001");
  parameters.push_back("0.002");

  BlockSummary bs(
    sizeof(double), len * len * len, DATATYPE_RAWMEM, "12345", 3, { { 0, 0, 0 } }, { { 9, 9, 9 } });
  std::string funcName = "valueRange";
  std::string results = exengine->execute(NULL, bs, rawdata.data(), funcName, parameters);
  if (results.compare("0") != 0)
  {
    throw std::runtime_error("failed for test_exengine_rangeG");
  }
  // make sure the delete operation is called
  // then the destructor will be called;
  delete exengine;
}
int main()
{
  test_exengine_rangeG();
  test_exengine_rangeL();
  test_exengine_rangeB();
}