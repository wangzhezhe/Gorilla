#ifndef DHTMANAGER_H
#define DHTMANAGER_H

#include <climits>
#include <iostream>
#include <map>
#include <vector>

// the bound value for specific dimention
struct Bound {
  Bound(){};
  Bound(int lb, int ub) : m_lb(lb), m_ub(ub){};
  int m_lb = INT_MAX;
  int m_ub = INT_MIN;
  ~Bound(){};
};

// the bbox for the application domain
// the lb/ub represents the lower bound and the upper bound of every dimention
// Attention, if there is 1 elements for every dimention, the lower bound is
// same with the upper bound
// TODO update, use the vector of the Bound
struct BBX {
  // the bound for every dimention
  BBX(){};
  // the default sequence is x-y-z
  std::vector<Bound *> BoundList;
  ~BBX();

  void printBBXinfo() {
    int dim = BoundList.size();
    for (int i = 0; i < dim; i++) {
      std::cout << "dim id:" << i << ",lb:" << BoundList[i]->m_lb << ",ub:" << BoundList[i]->m_ub<< std::endl;
    }
  }
};

struct ResponsibleMetaServer {
  ResponsibleMetaServer(int metaServerID, BBX *bbx)
      : m_metaServerID(metaServerID), m_bbx(bbx){};
  int m_metaServerID;
  BBX *m_bbx;
  ~ResponsibleMetaServer(){};
};

struct DHTManager {

  DHTManager(){};

  // init the metaServerBBOXList according to the partitionNum and the bbox of
  // the global domain
  void initDHT(int ndim, int metaServerNum, BBX* globalBBX);

  Bound *getOverlapBound(Bound *a, Bound *b);

  BBX *getOverlapBBX(BBX *a, BBX *b);

  // get the corresponding metaserver according to the input bbox
  std::vector<ResponsibleMetaServer> getMetaServerID(BBX *BBXQuery);

  std::map<int, BBX *> metaServerIDToBBX;

  //TODO:
  //map the metaserver ID into the address of the meta server
  //when the rank=0 server gathered all the address, then init this 
  //the rank=0 server will broadcast to all the rawdata nodes to init this map
  std::map<int, std::string> metaServerIDToAddr;

  void printDTMInfo() {
    auto it = metaServerIDToBBX.begin();
    while (it != metaServerIDToBBX.end()) {
      // Accessing KEY from element pointed by it.
      int id = it->first;
      std::cout << "id: " << id << std::endl;
      it->second->printBBXinfo();
      it++;
    }
  }

  // TODO, aggregate bounding box, input a list that contains several BBX and
  // then aggregate them into a large one
};

#endif