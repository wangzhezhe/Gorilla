
#include "../../blockManager/blockManager.h"
#include "../../commondata/metadata.h"

void test_filebackend_basic()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;

  BlockManager bm;
  BlockSummary bs;
  strcpy(bs.m_dataType, DATATYPE_RAWFILE.data());
  strcpy(bs.m_blockid, "testblock");
  bs.m_elemSize = sizeof(int);
  int a = 123;
  bm.putBlock(bs, BACKENDFILE, &a);
}

void test_filebackend_putget()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;

  BlockManager bm;

  std::array<int, 3> indexlb = { { 0, 0, 0 } };
  std::array<int, 3> indexub = { { 99, 0, 0 } };
  std::vector<double> rawdata;

  for (int i = 0; i <= 99; i++)
  {
    rawdata.push_back(i * 0.1);
  }

  std::string blockid = "12345";
  BlockSummary bs(sizeof(double), 100, DATATYPE_RAWFILE, blockid, 1, indexlb, indexub);

  bm.putBlock(bs, BACKENDFILE, rawdata.data());

  // test get data

  // get summary according to the id
  std::cout << "get summary" << std::endl;
  BlockSummary bsget = bm.getBlockSummary(blockid);
  bsget.printSummary();

  if (bsget.equals(bs) == false)
  {
    throw std::runtime_error("failed to get the correct block summary");
  }


  void* getContainer = (void*)calloc(bsget.m_elemNum, bsget.m_elemSize);
  for (int i = 0; i < 5; i++)
  {
    bm.getBlock(blockid, BACKENDFILE, getContainer);
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
  }
}

void test_filebackend_direactget()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;

  // this is created by previous test
  // we try to create the new manager and
  // get id direacly without call put first

  std::string blockid = "12345";
  BlockManager bmanager;
  void* getContainer = nullptr;
  BlockSummary bsget = bmanager.getBlock(blockid, BACKENDFILE, getContainer);
  bsget.printSummary();

  // assign space in the getBlock function if it is nullptr
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
  return;
}

void test_filebackend_putgetsubregion()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;

  return;
}

void test_filebackend_erase()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;

  BlockManager bm;

  std::array<int, 3> indexlb = { { 0, 0, 0 } };
  std::array<int, 3> indexub = { { 99, 0, 0 } };
  std::vector<double> rawdata;

  for (int i = 0; i <= 99; i++)
  {
    rawdata.push_back(i * 0.1);
  }

  std::string blockid = "123456";
  BlockSummary bs(sizeof(double), 100, DATATYPE_RAWFILE, blockid, 1, indexlb, indexub);

  bm.putBlock(bs, BACKENDFILE, rawdata.data());

  int status = bm.eraseBlock(blockid, BACKENDFILE);

  if (status != 0)
  {
    throw std::runtime_error("failed to erase file object");
  }

  return;
}

int main()
{
  tl::abt scope;

  // clear test dir
  system("rm ./filedataobj/*");

  // test_rawdatamanager_basic();
  test_filebackend_putget();
  test_filebackend_direactget();

  test_filebackend_putgetsubregion();
  test_filebackend_erase();
}