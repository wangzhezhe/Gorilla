

#include "../../utils/bbxtool.h"

using namespace BBXTOOL;

void test_bbx() {
  BBX *queryBBX = new BBX(3, {{3, 3, 0}}, {{8, 8, 0}});
  queryBBX->printBBXinfo();
  return;
}

int main() { test_bbx(); }