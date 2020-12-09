#include "../../commondata/metadata.h"
#include <unordered_map>
using namespace GORILLA;

void test_basic()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;
  ArraySummary as1;
  ArraySummary as2("testarrayName", 1, 1);
  as1.printSummary();
  as2.printSummary();
  return;
}

void test_arraymap()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;
  std::unordered_map<ArraySummary, std::string, ArraySummaryHash> summaryMap;
  ArraySummary as2("testarrayName", 1, 1);
  summaryMap[as2] = "test";
  if (summaryMap.count(as2) != 1)
  {
    throw std::runtime_error("failed for execute count");
  }
  return;
}

void test_blockwitharray()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;
  ArraySummary as1;
  ArraySummary as2("testarrayName", 1, 1);
  std::vector<ArraySummary> aslist;
  aslist.push_back(as1);
  aslist.push_back(as2);
  std::array<int, 3> indexlb = { { 0, 0, 0 } };
  std::array<int, 3> indexub = { { 99, 0, 0 } };
  BlockSummary bs(aslist, DATATYPE_CARGRID, "testblockid", 1, indexlb, indexub);
  bs.printSummary();
  return;
}

int main()
{
  test_basic();
  test_arraymap();
  test_blockwitharray();
}