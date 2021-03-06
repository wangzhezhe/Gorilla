
#include "../../client/ClientForSim.hpp"
#include "../../commondata/metadata.h"
#include <thallium.hpp>
#include <vector>

namespace tl = thallium;
using namespace GORILLA;

std::string loadMasterAddr(std::string masterConfigFile)
{

  std::ifstream infile(masterConfigFile);
  std::string content = "";
  std::getline(infile, content);
  // spdlog::debug("load master server conf {}, content -{}-", masterConfigFile,content);
  if (content.compare("") == 0)
  {
    std::getline(infile, content);
    if (content.compare("") == 0)
    {
      throw std::runtime_error("failed to load the master server\n");
    }
  }
  return content;
}

// assume that the server is alreasy started normally
void test_put_1d()
{

  // client engine
  tl::engine clientEngine("tcp", THALLIUM_CLIENT_MODE);
  std::string addrServer = loadMasterAddr("./unimos_server.conf");
  ClientForSim* uniclient = new ClientForSim(&clientEngine, addrServer, 0);

  // there is data screw if the length of the data is not the 2^n
  // there is data screw if the input data is not in the shape of the cubic
  int elemSize = 128;
  size_t elemNum = sizeof(double);
  std::string dataType = DATATYPE_CARGRID;
  size_t dims = 1;
  std::array<int, 3> indexlb = { { 0, 0, 0 } };
  std::array<int, 3> indexub = { { elemSize - 1, 0, 0 } };
  std::string varName = "testVar";

  // allocate space
  std::vector<double> rawdata;
  for (int i = 0; i < elemSize - 1; i++)
  {
    rawdata.push_back(i * 0.1);
  }

  // generate raw data summary block
  ArraySummary as("12345", (size_t)elemSize, elemNum);
  std::vector<ArraySummary> aslist;
  aslist.push_back(as);

  BlockSummary bs(aslist, dataType, "12345", dims, indexlb, indexub);

  // generate raw data
  for (size_t step = 0; step < 9; step++)
  {
    int status = uniclient->putrawdata(step, varName, bs, rawdata.data());
    if (status != 0)
    {
      throw std::runtime_error("failed to put data for step " + std::to_string(step));
    }
  }

  // check the output at the server end manually
}

void test_put_2d()
{

  // client engine
  tl::engine clientEngine("tcp", THALLIUM_CLIENT_MODE);
  std::string addrServer = loadMasterAddr("./unimos_server.conf");
  ClientForSim* uniclient = new ClientForSim(&clientEngine, addrServer, 0);
  // there is data screw if the length of the data is not the 2^n
  // there is data screw if the input data is not in the shape of the cubic
  size_t elemInOneDim = (99 - 10 + 1);
  size_t elemSize = elemInOneDim * elemInOneDim;
  size_t elemNum = sizeof(double);
  std::string dataType = DATATYPE_CARGRID;
  size_t dims = 2;
  std::array<int, 3> indexlb = { { 10, 10, 0 } };
  std::array<int, 3> indexub = { { 99, 99, 0 } };
  std::string varName = "testVar";

  // allocate space
  std::vector<double> rawdata;
  for (int i = 0; i < elemSize - 1; i++)
  {

    rawdata.push_back(i * 0.1);
  }

  // generate raw data summary block
  ArraySummary as("12345", elemSize, elemNum);
  std::vector<ArraySummary> aslist;
  aslist.push_back(as);

  BlockSummary bs(aslist, dataType, "12345", dims, indexlb, indexub);

  // generate raw data
  for (size_t step = 0; step < 9; step++)
  {
    int status = uniclient->putrawdata(step, varName, bs, rawdata.data());
    if (status != 0)
    {
      throw std::runtime_error("failed to put data for step " + std::to_string(step));
    }
  }

  // check the output at the server end manually
}

void test_put_3d()
{

  // client engine
  tl::engine clientEngine("tcp", THALLIUM_CLIENT_MODE);
  std::string addrServer = loadMasterAddr("./unimos_server.conf");
  ClientForSim* uniclient = new ClientForSim(&clientEngine, addrServer, 0);
  
  // there is data screw if the length of the data is not the 2^n
  // there is data screw if the input data is not in the shape of the cubic
  size_t elemInOneDim = (512);
  size_t elemSize = elemInOneDim * elemInOneDim * elemInOneDim;
  size_t elemNum = sizeof(double);
  std::string dataType = DATATYPE_CARGRID;
  size_t dims = 3;
  std::array<int, 3> indexlb = { { 0, 0, 0 } };
  std::array<int, 3> indexub = { { 511, 511, 511 } };
  std::string varName = "testVar";

  // allocate space
  std::vector<double> rawdata;
  for (int i = 0; i < elemSize - 1; i++)
  {
    rawdata.push_back(i * 0.1);
  }

  // generate raw data summary block
  ArraySummary as("12345", elemSize, elemNum);
  std::vector<ArraySummary> aslist;
  aslist.push_back(as);
  BlockSummary bs(aslist, dataType, "12345", dims, indexlb, indexub);

  // generate raw data
  for (size_t step = 0; step < 9; step++)
  {
    int status = uniclient->putrawdata(step, varName, bs, rawdata.data());
    if (status != 0)
    {
      throw std::runtime_error("failed to put data for step " + std::to_string(step));
    }
  }

  // check the output at the server end manually
}

int main()
{
  // adjust the configuration file before doing different tests
  // the maxDimValue is fixed as the 127
  // the dims should be modified according to different test

  // test_put_1d();

  // test_put_2d();

  test_put_3d();
}