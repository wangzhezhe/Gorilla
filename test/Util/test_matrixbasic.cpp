#include "../../utils/matrixtool.h"
#include <iostream>
#include <vector>

using namespace MATRIXTOOL;

void test_matrixcheck() {

  std::vector<int> a;

  for (int i = 0; i < 9; i++) {
    a.push_back(i);
  }

  size_t elemSize = sizeof(int);
  std::array<size_t, 3> gloablUb = {{3, 3, 3}};

  std::array<size_t, 3> subLb = {{0, 0, 0}};
  std::array<size_t, 3> subUb = {{5, 5, 5}};

  try {
    void *results = getSubMatrix(elemSize, subLb, subUb, gloablUb, a.data());
  } catch (std::exception &e) {
    std::string excstring = std::string(e.what());
    std::cout << "get exp:" << excstring << std::endl;
    size_t find = excstring.find("subdomain should in the bounding");
    if (find == std::string::npos) {
      throw std::runtime_error("failed for test_matrixcheck");
    }
  }

  try {
    void *results = getSubMatrix(elemSize, subUb, subLb, gloablUb, a.data());
  } catch (std::exception &e) {
    std::string excstring = std::string(e.what());
    std::cout << "get exp:" << excstring << std::endl;
    size_t find = excstring.find("invalid subdomain bounding");
    if (find == std::string::npos) {
      throw std::runtime_error("failed for test_matrixcheck");
    }
  }

  return;
}

void test_matrix1d() {

  std::cout << "---test_matrix1d---" << std::endl;

  // be careful for the type here!!!
  std::vector<double> a;
  for (int i = 0; i < 9; i++) {
    a.push_back(i * 0.1);
  }
  size_t elemSize = sizeof(double);
  std::array<size_t, 3> gloablUb = {{9, 0, 0}};

  std::array<size_t, 3> subLb = {{1, 0, 0}};
  std::array<size_t, 3> subUb = {{3, 0, 0}};

  void *results = getSubMatrix(elemSize, subLb, subUb, gloablUb, a.data());

  double *temp = (double *)results;
  for (int i = 0; i < 3; i++) {
    double value = *(temp + i);
    std::cout << "value " << value << std::endl;

    if (value != (i + 1) * 0.1) {
      throw std::runtime_error("failed for test_matrix1d");
    }
  }
}

void test_matrix2d() {

  std::cout << "---test_matrix2d---" << std::endl;

  size_t dim = 10;
  size_t elemSize = sizeof(double);
  std::array<size_t, 3> gloablUb = {{dim - 1, dim - 1, 0}};

  std::vector<double> a;
  for (int i = 0; i < dim * dim; i++) {
    a.push_back((i + 1) * 0.1);
  }

  std::array<size_t, 3> subLb = {{3, 3, 0}};
  std::array<size_t, 3> subUb = {{8, 8, 0}};

  void *results = getSubMatrix(elemSize, subLb, subUb, gloablUb, a.data());

  double *temp = (double *)results;
  for (int i = 0; i < 36; i++) {
    double value = *(temp + i);
    std::cout << "value " << value << std::endl;
  }
}

void test_matrix3d() {

  std::cout << "---test_matrix3d---" << std::endl;

  size_t dim = 10;
  size_t elemSize = sizeof(double);
  std::array<size_t, 3> gloablUb = {{dim - 1, dim - 1, dim - 1}};

  std::vector<double> a;
  for (int i = 0; i < dim * dim * dim; i++) {
    a.push_back(i * 0.1);
  }

  std::array<size_t, 3> subLb = {{3, 3, 3}};
  std::array<size_t, 3> subUb = {{5, 5, 5}};

  void *results = getSubMatrix(elemSize, subLb, subUb, gloablUb, a.data());

  double *temp = (double *)results;
  for (int i = 0; i < 27; i++) {
    double value = *(temp + i);
    std::cout << "value " << value << std::endl;
  }

}

void test_matrix3dto2d() {
  std::cout << "---test_matrix3dto2d---" << std::endl;

  size_t dim = 10;
  size_t elemSize = sizeof(double);
  std::array<size_t, 3> gloablUb = {{dim - 1, dim - 1, dim - 1}};

  std::vector<double> a;
  for (int i = 0; i < dim * dim * dim; i++) {
    a.push_back(i * 0.1);
  }

  std::array<size_t, 3> subLb = {{3, 3, 0}};
  std::array<size_t, 3> subUb = {{5, 5, 0}};

  void *results = getSubMatrix(elemSize, subLb, subUb, gloablUb, a.data());

  double *temp = (double *)results;
  for (int i = 0; i < 9; i++) {
    double value = *(temp + i);
    std::cout << "value " << value << std::endl;
  }
}

int main() {

  test_matrixcheck();

  test_matrix1d();

  test_matrix2d();

  test_matrix3d();

  test_matrix3dto2d();
}