#include "../server/DHTManager/dhtmanager.h"
#include "../utils/ArgothreadPool.h"
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#define BILLION 1000000000L

void testInit1d() {
  std::cout << "------init 1d------" << std::endl;
  DHTManager *dhtm = new DHTManager();
  Bound *a = new Bound(0, 15);
  BBX *ra1d = new BBX(1);
  ra1d->BoundList.push_back(a);
  dhtm->initDHTBySFC(1, 4, ra1d);
  dhtm->printDTMInfo();

  if (dhtm->metaServerIDToBBX[0]->BoundList[0]->m_lb != 0 ||
      dhtm->metaServerIDToBBX[0]->BoundList[0]->m_ub != 3) {
    throw std::runtime_error("failed for testInit1d\n");
  }
  if (dhtm->metaServerIDToBBX[1]->BoundList[0]->m_lb != 4 ||
      dhtm->metaServerIDToBBX[1]->BoundList[0]->m_ub != 7) {
    throw std::runtime_error("failed for testInit1d\n");
  }
  if (dhtm->metaServerIDToBBX[2]->BoundList[0]->m_lb != 8 ||
      dhtm->metaServerIDToBBX[2]->BoundList[0]->m_ub != 11) {
    throw std::runtime_error("failed for testInit1d\n");
  }
  if (dhtm->metaServerIDToBBX[3]->BoundList[0]->m_lb != 12 ||
      dhtm->metaServerIDToBBX[3]->BoundList[0]->m_ub != 15) {
    throw std::runtime_error("failed for testInit1d\n");
  }
}

void testInit2d() {
  std::cout << "------init 2d------" << std::endl;
  DHTManager *dhtm = new DHTManager();
  // the number of the dimention is supposed to be 2^n
  // there are problems for other number
  // TODO provide partition method that is not limit to the 2^n
  Bound *a = new Bound(0, 511);
  BBX *ra2d = new BBX(2);
  ra2d->BoundList.push_back(a);
  ra2d->BoundList.push_back(a);
  dhtm->initDHTBySFC(2, 4, ra2d);
  dhtm->printDTMInfo();

  if (dhtm->metaServerIDToBBX[0]->BoundList[0]->m_lb != 0 ||
      dhtm->metaServerIDToBBX[0]->BoundList[0]->m_ub != 255) {
    throw std::runtime_error("failed for testInit2d\n");
  }
  if (dhtm->metaServerIDToBBX[1]->BoundList[0]->m_lb != 0 ||
      dhtm->metaServerIDToBBX[1]->BoundList[0]->m_ub != 255) {
    throw std::runtime_error("failed for testInit2d\n");
  }
  if (dhtm->metaServerIDToBBX[2]->BoundList[0]->m_lb != 256 ||
      dhtm->metaServerIDToBBX[2]->BoundList[0]->m_ub != 511) {
    throw std::runtime_error("failed for testInit2d\n");
  }
  if (dhtm->metaServerIDToBBX[3]->BoundList[0]->m_lb != 256 ||
      dhtm->metaServerIDToBBX[3]->BoundList[0]->m_ub != 511) {
    throw std::runtime_error("failed for testInit2d\n");
  }
}

void testInit2dEgecase() {
  std::cout << "------init 2d edge cases------" << std::endl;
  DHTManager *dhtm = new DHTManager();

  // if the dim is not the 2^n
  Bound *a = new Bound(0, 9);
  BBX *ra2d = new BBX(2);
  ra2d->BoundList.push_back(a);
  ra2d->BoundList.push_back(a);
  dhtm->initDHTBySFC(2, 4, ra2d);
  dhtm->printDTMInfo();
}

void testInit3dLarge(){
  std::cout << "------init 3d large------" << std::endl;
  DHTManager *dhtm = new DHTManager();
  // the number of the dimention is supposed to be 2^n
  // there are problems for other number
  // TODO provide partition method that is not limit to the 2^n
  Bound *a = new Bound(0, 511);
  BBX *ra3d = new BBX(3);
  ra3d->BoundList.push_back(a);
  ra3d->BoundList.push_back(a);
  ra3d->BoundList.push_back(a);
  dhtm->initDHTBySFC(3, 4, ra3d);
  dhtm->printDTMInfo();
}

void testInit3d() {
  std::cout << "------init 3d------" << std::endl;
  DHTManager *dhtm = new DHTManager();
  // the number of the dimention is supposed to be 2^n
  // there are problems for other number
  // TODO provide partition method that is not limit to the 2^n
  Bound *a = new Bound(0, 7);
  BBX *ra3d = new BBX(3);
  ra3d->BoundList.push_back(a);
  ra3d->BoundList.push_back(a);
  ra3d->BoundList.push_back(a);
  dhtm->initDHTBySFC(3, 4, ra3d);
  dhtm->printDTMInfo();
}

void testgetMetaServerID() {
  std::cout << "------init 3d testgetMetaServerID------" << std::endl;
  DHTManager *dhtm = new DHTManager();

  // if the dim is not the 2^n
  Bound *a = new Bound(0, 15);
  BBX *globalDomain = new BBX(2);
  globalDomain->BoundList.push_back(a);
  globalDomain->BoundList.push_back(a);

  dhtm->initDHTBySFC(2, 4, globalDomain);

  Bound *b = new Bound(4, 10);
  BBX *queryDomain1 = new BBX(2);
  queryDomain1->BoundList.push_back(b);
  queryDomain1->BoundList.push_back(b);

  Bound *c = new Bound(4, 6);
  BBX *queryDomain2 = new BBX(2);
  queryDomain2->BoundList.push_back(c);
  queryDomain2->BoundList.push_back(c);

  std::vector<ResponsibleMetaServer> rbs = dhtm->getMetaServerID(queryDomain1);

  std::cout << "---query domain 1---" << std::endl;
  for (int i = 0; i < rbs.size(); i++) {
    std::cout << "metaserverid " << rbs[i].m_metaServerID << std::endl;
    rbs[i].m_bbx->printBBXinfo();
  }

  std::cout << "---query domain 2---" << std::endl;
  std::vector<ResponsibleMetaServer> rbs2 = dhtm->getMetaServerID(queryDomain2);
  for (int i = 0; i < rbs2.size(); i++) {
    std::cout << "metaserverid " << rbs2[i].m_metaServerID << std::endl;
    rbs2[i].m_bbx->printBBXinfo();
  }
}

void testBBX() {
  DHTManager *dhtm = new DHTManager();
  Bound *a = new Bound(0, 2);
  Bound *b = new Bound(3, 5);
  Bound *c = new Bound(1, 4);
  Bound *d = new Bound(0, 1);
  Bound *e = new Bound(1, 2);

  BBX *ra = new BBX(2);
  ra->BoundList.push_back(a);
  ra->BoundList.push_back(a);

  BBX *rb = new BBX(2);
  rb->BoundList.push_back(b);
  rb->BoundList.push_back(b);

  BBX *rc = new BBX(2);
  rc->BoundList.push_back(c);
  rc->BoundList.push_back(c);

  BBX *rd = new BBX(2);
  rd->BoundList.push_back(d);
  rd->BoundList.push_back(e);

  BBX *ra3 = new BBX(3);
  ra3->BoundList.push_back(a);
  ra3->BoundList.push_back(a);
  ra3->BoundList.push_back(a);

  BBX *rb3 = new BBX(3);
  ra3->BoundList.push_back(a);
  ra3->BoundList.push_back(a);
  ra3->BoundList.push_back(b);

  // BBX *rab = dhtm->getOverlapBBX(ra, rb);
  // if (rab != NULL) {
  //  throw std::runtime_error("failed for testing the overlap of ra,rb\n");
  //}

  BBX *rac = getOverlapBBX(ra, rc);
  if (rac == NULL) {
    throw std::runtime_error(
        "failed for testing the overlap of ra,rc, it should not be null\n");
  }
  if (rac->BoundList.size() != 2 || rac->BoundList[0]->m_lb != 1 ||
      rac->BoundList[0]->m_ub != 2 || rac->BoundList[1]->m_lb != 1 ||
      rac->BoundList[1]->m_ub != 2) {
    rac->printBBXinfo();
    throw std::runtime_error("failed for testing the overlap of ra,rc\n");
  }

  BBX *rad = getOverlapBBX(ra, rd);

  if (rad == NULL) {
    throw std::runtime_error(
        "failed for testing the overlap of ra,rd, it should not be null\n");
  }
  if (rad->BoundList.size() != 2 || rad->BoundList[0]->m_lb != 0 ||
      rad->BoundList[0]->m_ub != 1 || rad->BoundList[1]->m_lb != 1 ||
      rad->BoundList[1]->m_ub != 2) {
    rad->printBBXinfo();
    throw std::runtime_error("failed for testing the overlap of ra,rd\n");
  }
}

void testBound() {

  DHTManager *dhtm = new DHTManager();

  Bound *a = new Bound(10, 20);
  Bound *b = new Bound(30, 50);
  Bound *c = new Bound(15, 40);

  Bound *d =  getOverlapBound(a, b);
  if (d != NULL) {
    throw std::runtime_error(
        "failed for test bound for overlap of a,b it should be null\n");
  }

  Bound *e =  getOverlapBound(a, c);
  if (e == NULL) {
    throw std::runtime_error(
        "failed for test bound, overlap of a,c should be not null\n");
  }
  if (e->m_lb != 15 || e->m_ub != 20) {
    std::cout << e->m_lb << "," << e->m_ub;
    throw std::runtime_error("failed for caculating the overlap of a,c\n");
  }

  Bound *f =  getOverlapBound(c, b);
  if (f == NULL) {
    throw std::runtime_error(
        "failed for test bound, overlap of c,b should be not null\n");
  }
  if (f->m_lb != 30 || f->m_ub != 40) {
    std::cout << f->m_lb << "," << f->m_ub << std::endl;
    throw std::runtime_error("failed for caculating the overlap of c,b\n");
  }
}

int main() {
  // test bound
  testBound();

  // test the overlap of the bbx
  testBBX();

  // test init dht and getMetaServerID
  testInit1d();

  testInit2d();

  testInit2dEgecase();

  testInit3d();

  testgetMetaServerID();
  
   struct timespec start, end;
   double diff;
   clock_gettime(CLOCK_REALTIME, &start); /* mark start time */
   testInit3dLarge();
   //some functions here
   clock_gettime(CLOCK_REALTIME, &end); /* mark end time */
   diff = (end.tv_sec - start.tv_sec) * 1.0 + (end.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
   std::cout << "time is " << diff << "seconds\n";
   return 0; 
}