

#include "../../utils/bbxtool.h"

using namespace BBXTOOL;

void test_bbx()
{
  std::array<size_t, DEFAULT_MAX_DIM> indexlb = {{3, 3, 0}};
  std::array<size_t, DEFAULT_MAX_DIM> indexub = {{8, 8, 0}};

  BBX *queryBBX = new BBX(3, indexlb, indexub);
  queryBBX->printBBXinfo();
  return;
}

void test_getPhysicalIndex()
{

  std::array<size_t, DEFAULT_MAX_DIM> indexlb = {{1}};
  std::array<size_t, DEFAULT_MAX_DIM> indexub = {{8}};

  BBX *bbx = new BBX(1, indexlb, indexub);
  size_t index = bbx->getPhysicalIndex(1, {{2, 0, 0}});
  if (index != 1)
  {
    throw std::runtime_error("failed to check the physical index for test 1");
  }

  try
  {
    index = bbx->getPhysicalIndex(1, {{0, 0, 0}});
  }
  catch (std::exception &e)
  {
    std::string excstring = std::string(e.what());
    std::cout << "get exp:" << excstring << std::endl;
    size_t find = excstring.find("beyond the boundry of the BBX");
    if (find == std::string::npos)
    {
      throw std::runtime_error("failed to check the physical index for test 2");
    }
  }

  try
  {
    index = bbx->getPhysicalIndex(2, {{0, 0, 0}});
  }
  catch (std::exception &e)
  {
    std::string excstring = std::string(e.what());
    std::cout << "get exp:" << excstring << std::endl;
    size_t find = excstring.find("dimension of the coordinates");
    if (find == std::string::npos)
    {
      throw std::runtime_error("failed to check the physical index for test 3");
    }
  }
   indexlb = {{1, 1}};
  indexub = {{8, 8}};

  BBX *bbx2 = new BBX(2, indexlb, indexub);
  index = bbx2->getPhysicalIndex(2, {{2, 2, 0}});
  std::cout << "index " << index << std::endl;
  if (index != 9)
  {
    throw std::runtime_error("failed to check the physical index for test 4");
  }

   indexlb = {{1, 1, 1}};
   indexub = {{8, 8, 8}};
  BBX *bbx3 = new BBX(3, indexlb, indexub);
  index = bbx3->getPhysicalIndex(3, {{2, 2, 2}});
  std::cout << "index " << index << std::endl;
  if (index != (64 + 9))
  {
    throw std::runtime_error("failed to check the physical index for test 5");
  }

  indexlb = {{1, 1, 1}};
  indexub = {{8, 8, 8}};

  BBX *bbx3_2 = new BBX(3, indexlb, indexub);
  index = bbx3_2->getPhysicalIndex(3, {{8, 8, 8}});
  std::cout << "index " << index << std::endl;
  if (index != ((8 * 8 * 8) - 1))
  {
    throw std::runtime_error("failed to check the physical index for test 6");
  }
}

int main()
{

  test_bbx();

  test_getPhysicalIndex();
}