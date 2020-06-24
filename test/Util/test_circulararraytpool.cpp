#include <unistd.h>

#include "../utils/circularArrayTPool.h"

void testFull() {
  std::cout << "---testFull" << std::endl;
  try {
    CircularArrayTP carray(10);
    for (int i = 0; i < 12; i++) {
      carray.addES();
      std::cout << "ok to add es " << i << std::endl;
    }
  } catch (std::exception &e) {
    std::cout << "catch exception: " << std::string(e.what()) << std::endl;
    return;
  }
  throw std::runtime_error("there should be a exception here");
}

void testRemove() {
  std::cout << "---testRemove" << std::endl;

  CircularArrayTP carray(23);
  for (int i = 0; i < 20; i++) {
    carray.addES();
    carray.addES();
    carray.removeES();
    std::cout << "add two ess and remove one, tail is " << carray.m_tail
              << " header is " << carray.m_head << std::endl;
  }

  if (carray.esbufferLen() != 20) {
    throw std::runtime_error("the esbufferLen is incorrect");
  }
}

void testLen() {
  std::cout << "---testLen" << std::endl;

  CircularArrayTP carray(10);
  if (carray.esbufferLen() != 0) {
    throw std::runtime_error("the esbufferLen empty is incorrect");
  }
  carray.addES();
  carray.addES();
  if (carray.esbufferLen() != 2) {
    throw std::runtime_error("the esbufferLen 2 is incorrect");
  }

  for (int i = 0; i < 6; i++) {
    carray.addES();
    carray.addES();
    carray.removeES();
  }
  if (carray.esbufferLen() != 8) {
    throw std::runtime_error("the esbufferLen 8 is incorrect");
  }
}
int main() {
  tl::abt scope;
  testFull();
  testRemove();
  testLen();
  return 0;
}