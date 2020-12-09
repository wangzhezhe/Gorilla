

#include "../../blockManager/blockManager.h"
#include "../../commondata/metadata.h"
using namespace GORILLA;

void test_membackend_basic()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;

  tl::abt scope;
  BlockManager bm;
  BlockSummary bs;
  strcpy(bs.m_dataType, DATATYPE_CARGRID.data());
  strcpy(bs.m_blockid, "123");
  try{
    bm.putBlock(bs, BACKEND::MEM, NULL);
  }catch (std::exception &e) {
    std::string excstring = std::string(e.what());
    std::cout << "get exp: " << excstring << std::endl;
    size_t find = excstring.find("failed to find array name");
    if (find == std::string::npos)
    {
      throw std::runtime_error("failed for test_matrixcheck");
    }
  }
}

void test_membackend_putget()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;
  // put a data and check the results
  // assume the real data match with the BlockSummary
  tl::abt scope;
  BlockManager bm;

  std::array<int, 3> indexlb = { { 0, 0, 0 } };
  std::array<int, 3> indexub = { { 99, 0, 0 } };
  std::vector<double> rawdata;

  for (int i = 0; i <= 99; i++)
  {
    rawdata.push_back(i * 0.1);
  }

  std::string blockid = "12345";
  ArraySummary as(blockid, sizeof(double), 100);
  std::vector<ArraySummary> aslist;
  aslist.push_back(as);

  BlockSummary bs(aslist, DATATYPE_CARGRID, blockid, 1, indexlb, indexub);

  bm.putBlock(bs, BACKEND::MEM, rawdata.data());

  // test get data

  // get summary according to the id
  std::cout << "get summary" << std::endl;
  BlockSummary bsget = bm.getBlockSummary(blockid);
  bsget.printSummary();
  if (bsget.equals(bs) == false)
  {
    throw std::runtime_error("the returned data not equal");
  }

  // get the pointer stored in the raw data manager
  void* getContainer = NULL;
  bm.getBlock(blockid, BACKEND::MEM, getContainer);
  if (getContainer != NULL)
  {
    // check the results
    for (int i = 0; i <= 99; i++)
    {
      double value = *((double*)(getContainer) + i);
      // std::cout << "index " << i << " value " << value << std::endl;

      if (value != i * 0.1)
      {
        std::cout << "index " << i << " value " << value << std::endl;
        throw std::runtime_error("return value is wrong for index");
      }
    }
  }

  // allocate the space

  // get data according to the id

  return;
}

void test_membackend_putgetsubregion()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;

  return;
}

int main()
{
  test_membackend_basic();
  test_membackend_putget();
  test_membackend_putgetsubregion();
}