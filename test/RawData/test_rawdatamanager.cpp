

#include "../../commondata/metadata.h"
#include "../../server/RawdataManager/blockManager.h"

void test_rawdatamanager() {
  tl::abt scope;
  BlockManager bm(4);
  BlockSummary bs;
  bm.putBlock(DRIVERTYPE_RAWMEM, 123, bs, NULL);
}

int main() { test_rawdatamanager(); }