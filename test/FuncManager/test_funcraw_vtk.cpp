
#include "../../server/FunctionManager/defaultFunctions/defaultfuncraw.h"
#include "../../server/FunctionManager/functionManagerRaw.h"
#include <thallium.hpp>
#include <unistd.h>
using namespace GORILLA;

void test_exengineraw()
{
  tl::abt scope;
  FunctionManagerRaw* exengine = new FunctionManagerRaw();
  BlockSummary bs;
  std::string funcName = "test";
  std::vector<std::string> parameters;
  exengine->execute(NULL, bs, NULL, funcName, parameters);
  // make sure the delete operation is called
  // then the destructor will be called;
  delete exengine;
}

void test_executeVTK_raw()
{
  tl::abt scope;
  FunctionManagerRaw* exengine = new FunctionManagerRaw();

  std::vector<double> rawdata;

  int len = 10;

  for (int i = 0; i < len * len * len; i++)
  {
    rawdata.push_back(i * 0.1);
  }

  ArraySummary as("12345", sizeof(double), len * len * len);
  std::vector<ArraySummary> aslist;
  aslist.push_back(as);

  BlockSummary bs(
    aslist, DATATYPE_CARGRID, "12345", 3, { { 0, 0, 0 } }, { { 9, 9, 9 } });
  std::string funcName = "testvtk";
  std::vector<std::string> parameters;
  exengine->execute(NULL, bs, rawdata.data(), funcName, parameters);
  // make sure the delete operation is called
  // then the destructor will be called;
  delete exengine;
}

int main()
{

  test_exengineraw();
  test_executeVTK_raw();
}