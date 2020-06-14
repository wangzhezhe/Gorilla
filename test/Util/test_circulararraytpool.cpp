#include <unistd.h>

#include "../utils/circularArrayTPool.h"

void testCoverExist() {
  try {
    CircularArrayTP carray(10);
    for (int i = 1; i < 12; i++) {
      carray.addES();
      std::cout << "ok to add es " << i << std::endl;
    }
  } catch (std::exception &e) {
    std::cout << "catch exception: " << std::string(e.what()) << std::endl;
    return;
  }
  throw std::runtime_error("there should be a exception here");
}

int main() {
  tl::abt scope;
  testCoverExist();
  return 0;
}