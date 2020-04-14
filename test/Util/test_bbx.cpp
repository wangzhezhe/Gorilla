

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

void test_bbxequal()
{

  std::array<size_t, DEFAULT_MAX_DIM> indexlb = {{3, 3, 0}};
  std::array<size_t, DEFAULT_MAX_DIM> indexub = {{8, 8, 0}};
  std::array<size_t, DEFAULT_MAX_DIM> indexub2 = {{8, 9, 0}};
  BBX *BBX1 = new BBX(3, indexlb, indexub);
  BBX *BBX2 = new BBX(3, indexlb, indexub);
  BBX *BBX3 = new BBX(3, indexlb, indexub2);

  if (BBX1->equal(*BBX2) == false)
  {
    std::cerr << "supposed to be equal" << std::endl;
  }

  if (BBX1->equal(*BBX3) == true)
  {
    std::cerr << "supposed to be not equal" << std::endl;
  }
  return;
}

void test_splitBound()
{
  std::cout << "test_splitBound" << std::endl;
  Bound b0(10, 100);
  Bound qb1(1, 5);
  std::vector<Bound> blist = b0.splitBound(qb1);
  if (blist.size() != 0)
  {
    throw std::runtime_error("failed for qb1\n");
  }

  Bound qb2(101, 102);
  std::vector<Bound> blist2 = b0.splitBound(qb2);
  if (blist.size() != 0)
  {
    throw std::runtime_error("failed for qb1");
  }

  Bound qb3(9, 11);
  std::vector<Bound> blist3 = b0.splitBound(qb3);
  if (blist3.size() != 2)
  {
    throw std::runtime_error("failed for qb3");
  }
  if (blist3[0].m_lb != 10 || blist3[0].m_ub != 11 || blist3[1].m_lb != 12 || blist3[1].m_ub != 100)
  {
    for (auto v : blist3)
    {
      std::cout << "lb " << v.m_lb << " ub " << v.m_ub << std::endl;
    }
    throw std::runtime_error("failed for blist3 bound check");
  }

  Bound qb4(15, 90);
  std::vector<Bound> blist4 = b0.splitBound(qb4);
  if (blist4.size() != 3)
  {

    throw std::runtime_error("failed fot qb4");
  }
  if (blist4[0].m_lb != 10 || blist4[0].m_ub != 14 || blist4[1].m_lb != 15 || blist4[1].m_ub != 90 || blist4[2].m_lb != 91 || blist4[2].m_ub != 100)
  {
    for (auto v : blist4)
    {
      std::cout << "lb " << v.m_lb << " ub " << v.m_ub << std::endl;
    }
    throw std::runtime_error("failed for blist4 bound check");
  }

  Bound qb5(90, 110);
  std::vector<Bound> blist5 = b0.splitBound(qb5);
  if (blist5.size() != 2)
  {
    throw std::runtime_error("failed fot qb5");
  }
  if (blist5[0].m_lb != 10 || blist5[0].m_ub != 89 || blist5[1].m_lb != 90 || blist5[1].m_ub != 100)
  {
    for (auto v : blist5)
    {
      std::cout << "lb " << v.m_lb << " ub " << v.m_ub << std::endl;
    }
    throw std::runtime_error("failed for blist5 bound check");
  }

  Bound qb6(10, 100);
  std::vector<Bound> blist6 = b0.splitBound(qb6);
  if (blist6.size() != 1)
  {
    throw std::runtime_error("failed fot qb6");
  }
  if (blist6[0].m_lb != 10 || blist6[0].m_ub != 100)
  {
    for (auto v : blist6)
    {
      std::cout << "lb " << v.m_lb << " ub " << v.m_ub << std::endl;
    }
    throw std::runtime_error("failed for blist6 bound check");
  }

  Bound qb7(10, 50);
  std::vector<Bound> blist7 = b0.splitBound(qb7);
  if (blist7.size() != 2)
  {
    throw std::runtime_error("failed fot qb7");
  }
  if (blist7[0].m_lb != 10 || blist7[0].m_ub != 50 || blist7[1].m_lb != 51 || blist7[1].m_ub != 100)
  {
    for (auto v : blist7)
    {
      std::cout << "lb " << v.m_lb << " ub " << v.m_ub << std::endl;
    }
    throw std::runtime_error("failed for blist7 bound check");
  }

  Bound qb8(50, 100);
  std::vector<Bound> blist8 = b0.splitBound(qb8);
  if (blist8.size() != 2)
  {
    throw std::runtime_error("failed fot qb8");
  }
  if (blist8[0].m_lb != 10 || blist8[0].m_ub != 49 || blist8[1].m_lb != 50 || blist8[1].m_ub != 100)
  {
    for (auto v : blist8)
    {
      std::cout << "lb " << v.m_lb << " ub " << v.m_ub << std::endl;
    }
    throw std::runtime_error("failed for qb8 bound check");
  }

  return;
}

//assume there is overlap
void test_splitBBX()
{
  std::cout << "---test_splitBBX---" << std::endl;
  std::array<size_t, DEFAULT_MAX_DIM> a({10, 10, 0});
  std::array<size_t, DEFAULT_MAX_DIM> b({20, 20, 0});
  std::array<size_t, DEFAULT_MAX_DIM> c({5, 15, 0});
  std::array<size_t, DEFAULT_MAX_DIM> d({15, 25, 0});
  std::array<size_t, DEFAULT_MAX_DIM> e({15, 15, 0});
  std::array<size_t, DEFAULT_MAX_DIM> f({16, 16, 0});

  BBX largeDomain(2, a, b);
  BBX sdomain1(2, c, d);
  std::vector<BBX> bbxlist = splitReduceBBX(largeDomain, sdomain1);
  if (bbxlist.size() != 3)
  {
    throw std::runtime_error("error for splitReduceBBX case1\n");
  }
  for (auto bbx : bbxlist)
  {
    bbx.printBBXinfo();
  }

  std::cout << "case2" << std::endl;
  BBX sdomain2(2, e, f);
  std::vector<BBX> bbxlist2 = splitReduceBBX(largeDomain, sdomain2);
  if (bbxlist2.size() != 8)
  {
    throw std::runtime_error("error for splitReduceBBX bbxlist2\n");
  }
  for (auto bbx : bbxlist2)
  {
    bbx.printBBXinfo();
  }

  std::cout << "case3" << std::endl;
  std::array<size_t, DEFAULT_MAX_DIM> a3d({10, 10, 10});
  std::array<size_t, DEFAULT_MAX_DIM> b3d({20, 20, 20});
  std::array<size_t, DEFAULT_MAX_DIM> e3d({15, 15, 15});
  std::array<size_t, DEFAULT_MAX_DIM> f3d({16, 16, 16});
  BBX largeDomain3d(3, a3d, b3d);
  BBX sdomain3d(3, e3d, f3d);
  std::vector<BBX> bbxlist3 = splitReduceBBX(largeDomain3d, sdomain3d);
  if (bbxlist3.size() != 26)
  {
    throw std::runtime_error("error for splitReduceBBX bbxlist3\n");
  }
  for (auto bbx : bbxlist2)
  {
    bbx.printBBXinfo();
  }

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

  test_bbxequal();

  test_splitBound();

  test_splitBBX();

  test_getPhysicalIndex();
}