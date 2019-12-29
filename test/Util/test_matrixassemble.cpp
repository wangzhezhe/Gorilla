#include "../../utils/matrixtool.h"
#include <iostream>
#include <vector>

using namespace MATRIXTOOL;

void test_assemble_not_ok()
{

  // 1d
  // MatrixView matrixAssemble(std::vector<MatrixView> matrixViewList,
  //                       BBX *intactBBX) {

  std::cout << "---test_assemble_not_ok 1d---" << std::endl;
  std::vector<MatrixView> matrixViewList1d;

  BBX *bbx1d1 = new BBX(1, {{0}}, {{3}});
  BBX *bbx1d2 = new BBX(1, {{4}}, {{6}});
  BBX *bbx1d3 = new BBX(2, {{7}}, {{11}});

  BBX *bbxquery = new BBX(1, {{0}}, {{9}});

  MatrixView mv1d1(bbx1d1, NULL);
  MatrixView mv1d2(bbx1d2, NULL);
  MatrixView mv1d3(bbx1d3, NULL);

  matrixViewList1d.push_back(mv1d1);
  matrixViewList1d.push_back(mv1d2);
  matrixViewList1d.push_back(mv1d3);

  try
  {
    MatrixView mvwhole = matrixAssemble(sizeof(double), matrixViewList1d, bbxquery);
  }
  catch (std::exception &e)
  {
    std::string excstring = std::string(e.what());
    std::cout << "get exp:" << excstring << std::endl;
    size_t find = excstring.find("the dims in matrixViewList");
    if (find == std::string::npos)
    {
      throw std::runtime_error("failed for test_assemble_not_ok 1d ");
    }
  }

  // 2d
  std::cout << "---test_assemble_not_ok 2d---" << std::endl;

  std::vector<MatrixView> matrixViewList2d;

  BBX *bbx2d1 = new BBX(2, {{0, 0}}, {{3, 3}});
  BBX *bbx2d2 = new BBX(2, {{1, 1}}, {{6, 6}});
  BBX *bbx2d3 = new BBX(2, {{7, 7}}, {{9, 9}});

  BBX *bbxquery2 = new BBX(2, {{0, 0}}, {{9, 9}});

  MatrixView mv2d1(bbx2d1, NULL);
  MatrixView mv2d2(bbx2d2, NULL);
  MatrixView mv2d3(bbx2d3, NULL);

  matrixViewList2d.push_back(mv2d1);
  matrixViewList2d.push_back(mv2d2);
  matrixViewList2d.push_back(mv2d3);

  try
  {
    MatrixView mvwhole2 = matrixAssemble(sizeof(double), matrixViewList2d, bbxquery2);
  }
  catch (std::exception &e)
  {
    std::string excstring = std::string(e.what());
    std::cout << "get exp:" << excstring << std::endl;
    size_t find = excstring.find("there is overlaping");
    if (find == std::string::npos)
    {
      throw std::runtime_error("failed for test_assemble_not_ok 2d ");
    }
  }

  // 3d
  std::cout << "---test_assemble_not_ok 3d---" << std::endl;

  std::vector<MatrixView> matrixViewList3d;
  BBX *bbx3d1 = new BBX(3, {{0, 0, 0}}, {{3, 3, 3}});
  BBX *bbx3d2 = new BBX(3, {{4, 4, 5}}, {{6, 6, 6}});
  BBX *bbx3d3 = new BBX(3, {{7, 7, 7}}, {{9, 9, 9}});

  BBX *bbxquery3 = new BBX(3, {{0, 0, 0}}, {{9, 9, 9}});

  MatrixView mv3d1(bbx3d1, NULL);
  MatrixView mv3d2(bbx3d2, NULL);
  MatrixView mv3d3(bbx3d3, NULL);

  matrixViewList3d.push_back(mv3d1);
  matrixViewList3d.push_back(mv3d2);
  matrixViewList3d.push_back(mv3d3);

  try
  {
    MatrixView mvwhole2 = matrixAssemble(sizeof(double), matrixViewList3d, bbxquery3);
  }
  catch (std::exception &e)
  {
    std::string excstring = std::string(e.what());
    std::cout << "get exp:" << excstring << std::endl;
    size_t find = excstring.find("the element in intact matrix");
    if (find == std::string::npos)
    {
      throw std::runtime_error("failed for test_assemble_not_ok 3d ");
    }
  }
}

void test_check_ok_1d()
{

  std::cout << "---test_check_ok 1d ---" << std::endl;

  //1d
  std::vector<MatrixView> matrixViewList1d;

  BBX *bbx1d1 = new BBX(1, {{0}}, {{30}});
  std::vector<double> dataVector1;
  for (int i = 0; i <= 30; i++)
  {
    dataVector1.push_back(1.1);
  }
  BBX *bbx1d2 = new BBX(1, {{31}}, {{60}});
  std::vector<double> dataVector2;
  for (int i = 31; i <= 60; i++)
  {
    dataVector2.push_back(2.2);
  }
  BBX *bbx1d3 = new BBX(1, {{61}}, {{90}});
  std::vector<double> dataVector3;
  for (int i = 61; i <= 90; i++)
  {
    dataVector3.push_back(3.3);
  }

  BBX *intectRegion = new BBX(1, {{0}}, {{90}});

  MatrixView mv1d1(bbx1d1, dataVector1.data());
  MatrixView mv1d2(bbx1d2, dataVector2.data());
  MatrixView mv1d3(bbx1d3, dataVector3.data());

  matrixViewList1d.push_back(mv1d1);
  matrixViewList1d.push_back(mv1d2);
  matrixViewList1d.push_back(mv1d3);

  MatrixView mv = matrixAssemble(sizeof(double), matrixViewList1d, intectRegion);
  mv.m_bbx->printBBXinfo();
  if (mv.m_data != NULL)
  {
    for (int i = 0; i <= 90; i++)
    {
      double value = *((double *)(mv.m_data) + i);
      if (i >= 0 && i <= 30 && value != 1.1)
      {
        throw std::runtime_error("failed to check the value for 1.1");
      }
      if (i >= 31 && i <= 60 && value != 2.2)
      {
        throw std::runtime_error("failed to check the value for 2.2");
      }
      if (i >= 61 && i <= 90 && value != 3.3)
      {
        throw std::runtime_error("failed to check the value for 3.3");
      }
    }
  }
  else
  {
    throw std::runtime_error("failed to get the data value");
  }
}

void test_check_ok_2d()
{
  //2d
  std::cout << "---test_check_ok 2d ---" << std::endl;

  std::vector<MatrixView> matrixViewList2d;

  BBX *bbx2d1 = new BBX(2, {{0, 0}}, {{30, 30}});
  std::vector<double> dataVector1;
  size_t elemNum = bbx2d1->getElemNum();
  for (int i = 0; i <= elemNum; i++)
  {
    dataVector1.push_back(1.1);
  }
  BBX *bbx2d2 = new BBX(2, {{31, 0}}, {{60, 30}});
  elemNum = bbx2d2->getElemNum();
  std::vector<double> dataVector2;
  for (int i = 0; i <= elemNum; i++)
  {
    dataVector2.push_back(2.2);
  }
  BBX *bbx2d3 = new BBX(2, {{0, 31}}, {{30, 60}});
  elemNum = bbx2d3->getElemNum();
  std::vector<double> dataVector3;
  for (int i = 0; i <= elemNum; i++)
  {
    dataVector3.push_back(3.3);
  }

  BBX *bbx2d4 = new BBX(2, {{31, 31}}, {{60, 60}});
  elemNum = bbx2d4->getElemNum();
  std::vector<double> dataVector4;
  for (int i = 0; i <= elemNum; i++)
  {
    dataVector4.push_back(4.4);
  }

  BBX *intectRegion = new BBX(2, {{0, 0}}, {{60, 60}});

  MatrixView mv2d1(bbx2d1, dataVector1.data());
  MatrixView mv2d2(bbx2d2, dataVector2.data());
  MatrixView mv2d3(bbx2d3, dataVector3.data());
  MatrixView mv2d4(bbx2d4, dataVector4.data());

  matrixViewList2d.push_back(mv2d1);
  matrixViewList2d.push_back(mv2d2);
  matrixViewList2d.push_back(mv2d3);
  matrixViewList2d.push_back(mv2d4);

  MatrixView mv = matrixAssemble(sizeof(double), matrixViewList2d, intectRegion);
  mv.m_bbx->printBBXinfo();
  if (mv.m_data != NULL)
  {

    for (int y = 0; y <= 60; y++)
    {
      for (int x = 0; x <= 60; x++)
      {
        size_t index = y * 61 + x;
        double value = *((double *)(mv.m_data) + index);
        if (x >= 0 && x <= 30 && y >= 0 && y <= 30 && value != 1.1)
        {
          std::cout << "debug " << x << "," << y << " value " << value << std::endl;

          throw std::runtime_error("failed to check the value for 2.1");
        }
        if (x >= 31 && x <= 60 && y >= 0 && y <= 30 && value != 2.2)
        {
          std::cout << "debug " << x << "," << y << " value " << value << std::endl;

          throw std::runtime_error("failed to check the value for 2.2");
        }
        if (x >= 0 && x <= 30 && y >= 31 && y <= 60 && value != 3.3)
        {
          std::cout << "debug " << x << "," << y << " value " << value << std::endl;

          throw std::runtime_error("failed to check the value for 2.3");
        }
        if (x >= 31 && x <= 60 && y >= 31 && y <= 60 && value != 4.4)
        {
          std::cout << "debug " << x << "," << y << " value " << value << std::endl;
          throw std::runtime_error("failed to check the value for 2.4");
        }
      }
    }
  }
  else
  {
    throw std::runtime_error("failed to check the value test_check_ok_2d, the return value is null");
  }
}

void test_check_ok_3d()
{
  //3d
  std::cout << "---test_check_ok 3d ---" << std::endl;

  std::vector<MatrixView> matrixViewList3d;

  BBX *bbx3d1 = new BBX(3, {{0, 0, 0}}, {{30, 30, 0}});
  std::vector<double> dataVector1;
  size_t elemNum = bbx3d1->getElemNum();
  for (int i = 0; i <= elemNum; i++)
  {
    dataVector1.push_back(1.1);
  }
  BBX *bbx3d2 = new BBX(3, {{0, 0, 1}}, {{30, 30, 1}});
  elemNum = bbx3d2->getElemNum();
  std::vector<double> dataVector2;
  for (int i = 0; i <= elemNum; i++)
  {
    dataVector2.push_back(2.2);
  }

  BBX *intectRegion = new BBX(3, {{0, 0, 0}}, {{30, 30, 1}});

  MatrixView mv3d1(bbx3d1, dataVector1.data());
  MatrixView mv3d2(bbx3d2, dataVector2.data());

  matrixViewList3d.push_back(mv3d1);
  matrixViewList3d.push_back(mv3d2);

  MatrixView mv = matrixAssemble(sizeof(double), matrixViewList3d, intectRegion);
  mv.m_bbx->printBBXinfo();

  if (mv.m_data != NULL)
  {
    for (int z = 0; z <= 1; z++)
    {
      for (int y = 0; y <= 30; y++)
      {
        for (int x = 0; x <= 30; x++)
        {
          size_t index = z * 31 * 31 + y * 31 + x;
          double value = *((double *)(mv.m_data) + index);
          if (x >= 0 && x <= 30 && y >= 0 && y <= 30 && z >= 0 && z <= 0 && value != 1.1)
          {
            std::cout << "debug " << x << "," << y << "," << z << " value " << value << std::endl;

            throw std::runtime_error("failed to check the value for 3.1");
          }
          if (x >= 0 && x <= 30 && y >= 0 && y <= 30 && z >= 1 && z <= 1 && value != 2.2)
          {
            std::cout << "debug " << x << "," << y << "," << z << " value " << value << std::endl;

            throw std::runtime_error("failed to check the value for 3.2");
          }
        }
      }
    }
  }
  else
  {
    throw std::runtime_error("failed to check the value test_check_ok_3d, the return value is null");
  }
}

int main()
{
  test_assemble_not_ok();

  test_check_ok_1d();

  test_check_ok_2d();

  test_check_ok_3d();
}