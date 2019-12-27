#include "../../utils/matrixtool.h"
#include <iostream>
#include <vector>

using namespace MATRIXTOOL;

void test_assemble_not_ok() {

  // 1d
  // MatrixView matrixAssemble(std::vector<MatrixView> matrixViewList,
  //                       BBX *intactBBX) {

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

  try {
    MatrixView mvwhole = matrixAssemble(matrixViewList1d, bbxquery);

  } catch (std::exception &e) {
    std::string excstring = std::string(e.what());
    std::cout << "get exp:" << excstring << std::endl;
    size_t find = excstring.find("the dims in matrixViewList");
    if (find == std::string::npos) {
      throw std::runtime_error("failed for test_assemble_not_ok 1d ");
    }
  }

  // 2d

  std::vector<MatrixView> matrixViewList2d;

  BBX *bbx2d1 = new BBX(2, {{0, 0}}, {{3, 3}});
  BBX *bbx2d2 = new BBX(2, {{2, 4}}, {{6, 6}});
  BBX *bbx2d3 = new BBX(2, {{7, 7}}, {{9, 9}});

  BBX *bbxquery2 = new BBX(2, {{0, 0}}, {{9, 9}});

  MatrixView mv2d1(bbx2d1, NULL);
  MatrixView mv2d2(bbx2d2, NULL);
  MatrixView mv2d3(bbx2d3, NULL);

  matrixViewList2d.push_back(mv2d1);
  matrixViewList2d.push_back(mv2d2);
  matrixViewList2d.push_back(mv2d3);

  try {
    MatrixView mvwhole2 = matrixAssemble(matrixViewList2d, bbxquery2);

  } catch (std::exception &e) {
    std::string excstring = std::string(e.what());
    std::cout << "get exp:" << excstring << std::endl;
    size_t find = excstring.find("there is overlaping");
    if (find == std::string::npos) {
      throw std::runtime_error("failed for test_assemble_not_ok 2d ");
    }
  }

  // 3d

   std::vector<MatrixView> matrixViewList3d;

  BBX *bbx3d1 = new BBX(3, {{0, 0, 0}}, {{3, 3, 3}});
  BBX *bbx3d2 = new BBX(3, {{4, 4, 5}}, {{6, 6, 6}});
  BBX *bbx3d3 = new BBX(3, {{7, 7, 7}}, {{9, 9, 9}});

  BBX *bbxquery3 = new BBX(3, {{0, 0, 0 }}, {{9, 9, 9}});

  MatrixView mv3d1(bbx3d1, NULL);
  MatrixView mv3d2(bbx3d2, NULL);
  MatrixView mv3d3(bbx3d3, NULL);

  matrixViewList3d.push_back(mv3d1);
  matrixViewList3d.push_back(mv3d2);
  matrixViewList3d.push_back(mv3d3);

  try {
    MatrixView mvwhole2 = matrixAssemble(matrixViewList3d, bbxquery3);

  } catch (std::exception &e) {
    std::string excstring = std::string(e.what());
    std::cout << "get exp:" << excstring << std::endl;
    size_t find = excstring.find("is not covered");
    if (find == std::string::npos) {
      throw std::runtime_error("failed for test_assemble_not_ok 3d ");
    }
  }




}

int main() {
  test_assemble_not_ok();
  //test_check_ok();
}