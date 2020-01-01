

#include "../../commondata/metadata.h"
#include "../../server/RawdataManager/blockManager.h"

void test_rawdatamanager_basic()
{
  tl::abt scope;
  BlockManager bm;
  BlockSummary bs;
  bm.putBlock("123", bs, NULL);
}

void test_rawdatamanager_putget()
{
  //put a data and check the results
  //assume the real data match with the BlockSummary
  tl::abt scope;
  BlockManager bm;

  

  std::array<size_t, 3> indexlb = {{0, 0, 0}};
  std::array<size_t, 3> indexub = {{99, 0, 0}};
  std::vector<double> rawdata;

  for (int i = 0; i <= 99; i++)
  {
    rawdata.push_back(i * 0.1);
  }

  /*
  BlockSummary(std::string typeName, size_t elemSize, size_t elemNum,
               std::string driverType, std::array<size_t, 3> indexlb,
               std::array<size_t, 3> indexub)
  */
  BlockSummary bs(sizeof(double), 100, DRIVERTYPE_RAWMEM, 1, indexlb, indexub);

  std::string blockid = "123";
  bm.putBlock(blockid, bs, rawdata.data());

  //test get data

  //get summary according to the id
  std::cout << "get summary" << std::endl;
  BlockSummary bsget = bm.getBlockSummary(blockid);
  bsget.printSummary();
  void *getContainer = (void *)malloc(bsget.m_elemNum * bsget.m_elemSize);

  bm.getBlock(blockid, getContainer);
  if (getContainer != NULL)
  {
    //check the results
    for (int i = 0; i <= 99; i++)
    {
      double value = *((double *)(getContainer) + i);
      //std::cout << "index " << i << " value " << value << std::endl;

      if (value != i * 0.1)
      {
        std::cout << "index " << i << " value " << value << std::endl;
        throw std::runtime_error("return value is wrong for index");
      }
    }
  }

  //allocate the space

  //get data according to the id

  return;
}

void test_rawdatamanager_putgetsubregion()
{
  return;
}

int main()
{
  test_rawdatamanager_basic();
  test_rawdatamanager_putget();
  test_rawdatamanager_putgetsubregion();
}