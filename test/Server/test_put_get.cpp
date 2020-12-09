
#include "../../client/unimosclient.h"
#include "../../commondata/metadata.h"
#include "../../utils/bbxtool.h"
#include "../../utils/matrixtool.h"
#include <thallium.hpp>
#include <vector>

namespace tl = thallium;
using namespace GORILLA;

// use the two dimentions in the setting files for testing!!!!

void test_get_meta()
{

  // client engine
  tl::engine clientEngine("verbs", THALLIUM_CLIENT_MODE);
  UniClient* uniclient = new UniClient(&clientEngine, "./unimos_server.conf", 0);

  // although we use same config here, if the max length at the metaserver is different
  // the number of returned metaserver is different
  // for example, if we use the 511 at the setting.json, there is only 1 value in the vector
  // if we use the 127 at the setting.json, there is 4 value in the vector
  std::array<int, 3> indexlb = { { 10, 10, 0 } };
  std::array<int, 3> indexub = { { 99, 99, 0 } };

  std::vector<std::string> metaList = uniclient->getmetaServerList(2, indexlb, indexub);

  std::cout << "---tests1---" << std::endl;
  for (int i = 0; i < metaList.size(); i++)
  {
    std::cout << "server " << metaList[i] << std::endl;
  }

  indexlb = { { 10, 10, 0 } };
  indexub = { { 99, 12, 0 } };

  metaList = uniclient->getmetaServerList(2, indexlb, indexub);

  std::cout << "---tests2---" << std::endl;
  for (int i = 0; i < metaList.size(); i++)
  {
    std::cout << "server " << metaList[i] << std::endl;
  }
}

// assume that the server is alreasy started normally
void test_put()
{

  // client engine
  tl::engine clientEngine("verbs", THALLIUM_CLIENT_MODE);
  UniClient* uniclient = new UniClient(&clientEngine, "./unimos_server.conf", 0);

  // there is data screw if the length of the data is not the 2^n
  // there is data screw if the input data is not in the shape of the cubic
  size_t elemInOneDim = (99 - 10 + 1);
  size_t elemSize = sizeof(double);
  size_t elemNum = elemInOneDim * elemInOneDim;
  std::string dataType = DATATYPE_CARGRID;
  size_t dims = 2;
  std::array<int, 3> indexlb = { { 10, 10, 0 } };
  std::array<int, 3> indexub = { { 99, 99, 0 } };
  std::string varName = "testVar";

  // allocate space
  std::vector<double> rawdata;
  for (int i = 0; i < elemNum - 1; i++)
  {

    rawdata.push_back(i * 0.1);
  }

  // generate raw data summary block
  std::string blockid = "12345";
  ArraySummary as(blockid, elemSize, elemNum);
  std::vector<ArraySummary> aslist;
  aslist.push_back(as);
  BlockSummary bs(aslist, dataType, blockid, dims, indexlb, indexub);

  // generate raw data
  for (size_t step = 0; step < 5; step++)
  {
    int status = uniclient->putrawdata(step, varName, bs, rawdata.data());
    if (status != 0)
    {
      throw std::runtime_error("failed to put data for step " + std::to_string(step));
    }
  }

  // check the output at the server end manually
}
// use the dims=2  maxDimValue=127 as the config for server
void test_get_2drawDatList()
{

  // put some data

  test_put();

  // init client engine
  tl::engine clientEngine("verbs", THALLIUM_CLIENT_MODE);
  UniClient* uniclient = new UniClient(&clientEngine, "./unimos_server.conf", 0);

  // although we use same config here, if the max length at the metaserver is different
  // the number of returned metaserver is different
  // for example, if we use the 511 at the setting.json, there is only 1 value in the vector
  // if we use the 127 at the setting.json, there is 4 value in the vector
  std::array<int, 3> indexlb = { { 10, 10, 0 } };
  std::array<int, 3> indexub = { { 99, 99, 0 } };

  // get the data according to the arbitrary bounding box
  MATRIXTOOL::MatrixView mvassemble =
    uniclient->getArbitraryData(0, "testVar", sizeof(double), 2, indexlb, indexub);

  double* temp = (double*)(mvassemble.m_data);
  std::cout << "check whole region " << std::endl;
  for (int i = 0; i < (90 * 90) - 1; i++)
  {
    double value = *(temp + i);
    if (value != i * 0.1)
    {
      std::cout << "index " << i << " value " << value << std::endl;
      throw std::runtime_error("failed to check the assemble data value");
    }
  }

  // free matrixViewList manually according to different usecases
}

// use the dims=3 and maxDimValue=63 as the config for server
void test_get_3drawDatList() {}

// use srun oto tun this exp
int main()
{
  test_get_meta();

  // test_put();

  test_get_2drawDatList();

  // test_get_metaList();

  // test_get_3drawDatList();
}