#ifndef DHTMANAGER_H
#define DHTMANAGER_H


#include <iostream>
#include <map>
#include <vector>
#include "../utils/bbxtool.h"

using namespace BBXTOOL;

namespace GORILLA
{

struct ResponsibleMetaServer {
  ResponsibleMetaServer(int metaServerID, BBX *bbx)
      : m_metaServerID(metaServerID), m_bbx(bbx){};
  int m_metaServerID;
  BBX *m_bbx;
  ~ResponsibleMetaServer(){};
};

struct DHTManager {

  DHTManager(){};
  ~DHTManager(){};
  // init the metaServerBBOXList according to the partitionNum and the bbox of
  // the global domain
  void initDHTBySFC(size_t ndim, size_t metaServerNum, BBX* globalBBX);
  
  //init dht manually, the number of the partition at each dimention is specified by partitionLayout array
  void initDHTManually(std::vector<int> &lenArray, std::vector<int> &partitionLayout);
  
  // get the corresponding metaserver according to the input bbox
  std::vector<ResponsibleMetaServer> getMetaServerID(BBX& BBXQuery);

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
}
#endif