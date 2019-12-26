#include "dhtmanager.h"
#include "../utils/hilbert/hilbert.h"
#include <algorithm>
#include <string>

// n is the number of the elements
int computeBits(int n) {
  int nr_bits = 0;

  while (n) {
    n = n >> 1;
    nr_bits++;
  }

  return nr_bits;
}

int nextPowerOf2(int n)  
{  
    unsigned count = 0;  
      
    // First n in the below condition  
    // is for the case where n is 0  
    if (n && !(n & (n - 1)))  
        return n;  
      
    while( n != 0)  
    {  
        n >>= 1;  
        count += 1;  
    }  
      
    return 1 << count;  
}

Bound *DHTManager::getOverlapBound(Bound *a, Bound *b) {

  if ((a->m_ub < b->m_lb) || (a->m_lb > b->m_ub)) {
    return NULL;
  }

  Bound *overlap = new Bound();
  overlap->m_lb = std::max(a->m_lb, b->m_lb);
  overlap->m_ub = std::min(a->m_ub, b->m_ub);

  return overlap;
}

BBX *DHTManager::getOverlapBBX(BBX *a, BBX *b) {

  int aDim = a->BoundList.size();
  int bDim = b->BoundList.size();

  if (aDim != bDim) {
    throw std::runtime_error(
        "the dimention of two bounding box is not equal aDim is " +
        std::to_string(aDim) + " bDim is " + std::to_string(bDim));
    return NULL;
  }

  if (aDim == 0 || bDim > 3) {
    throw std::runtime_error("the dimention of bounding box can not be 0 or "
                             "value larger than 3, current value is: " +
                             std::to_string(aDim));
    return NULL;
  }

  BBX *overlapBBX = new BBX();
  for (int i = 0; i < aDim; i++) {
    Bound *bound = getOverlapBound(a->BoundList[i], b->BoundList[i]);
    if (bound == NULL) {
      // if there is not matching for any dimention, return NULL
      return NULL;
    } else {
      overlapBBX->BoundList.push_back(bound);
    }
  }

  return overlapBBX;
}

// init the metaServerBBOXList according to the metaServerNum(partitionNum) and
// the bbox of the global domain The hilbert DHT is only suitable for the cubic
void DHTManager::initDHT(int ndim, int metaServerNum, BBX *globalBBX) {
  
  if(globalBBX->BoundList.size()!=ndim){
      throw std::runtime_error("dim num should same with the number of bound in globalBBX");
  }
  // get max dim value
  int maxDimLen = INT_MIN;
  for (int i = 0; i < ndim; i++) {
    // this is the number of the value, so plus one
    maxDimLen = std::max(maxDimLen, globalBBX->BoundList[i]->m_ub + 1);
  }
  
  //the len of every dim in virtual box is virtualMaxDimLen
  //the real value is the maxDimLen
  int virtualMaxDimLen = nextPowerOf2(maxDimLen);

  // caculate the total number of the elements
  // the value should be the 2^n that cover the current bbx
  int totalElems = virtualMaxDimLen;
  for (int i = 0; i < ndim; i++) {
    if (i > 0) {
      totalElems = totalElems * virtualMaxDimLen;
    }
  }

  int serverNumberForEachNode = 0;
  serverNumberForEachNode = totalElems / metaServerNum;

  int nBits = computeBits(virtualMaxDimLen);
  std::cout << "debug totalElems " << totalElems << " serverNumberForEachNode "
            << serverNumberForEachNode << " virtualMaxDimLen " << virtualMaxDimLen << " nbits " << nBits << std::endl;
  if (ndim == 1) {
    for (int i = 0; i < maxDimLen; i++) {

      int sfc_coord[1] = {i};
      bitmask_t index = hilbert_c2i(ndim, nBits, sfc_coord);
      int serverID = index / serverNumberForEachNode;
      if ((index + 1) > serverNumberForEachNode * metaServerNum) {
        // put all the reminder into the last server
        serverID = metaServerNum - 1;
      }

      //std::cout << "debug " << i << "index " << index << " serverID "
      //          << serverID << std::endl;

      auto iter = this->metaServerIDToBBX.find(serverID);
      if (iter == metaServerIDToBBX.end()) {

        Bound *b = new Bound();
        BBX *bbx = new BBX();
        bbx->BoundList.push_back(b);
        this->metaServerIDToBBX[serverID] = bbx;
      }

      // update the lowerbound and the upper bound for specific partition
      this->metaServerIDToBBX[serverID]->BoundList[0]->m_lb =
          std::min(this->metaServerIDToBBX[serverID]->BoundList[0]->m_lb, i);

      this->metaServerIDToBBX[serverID]->BoundList[0]->m_ub =
          std::max(this->metaServerIDToBBX[serverID]->BoundList[0]->m_ub, i);
      // modify the ub if it is larger than real value
    }
  } else if (ndim == 2) {
    for (int i = 0; i < maxDimLen; i++) {
      for (int j = 0; j < maxDimLen; j++) {
        int sfc_coord[2] = {i, j};
        bitmask_t index = hilbert_c2i(ndim, nBits, sfc_coord);
        int serverID = index / serverNumberForEachNode;
        if ((index + 1) > serverNumberForEachNode * metaServerNum) {
          // put all the reminder into the last server
          serverID = metaServerNum - 1;
        }

        //std::cout << "debug " << i << "," << j << "index " << index
        //          << " serverID " << serverID << std::endl;

        auto iter = this->metaServerIDToBBX.find(serverID);
        if (iter == metaServerIDToBBX.end()) {

          Bound *b1 = new Bound();
          Bound *b2 = new Bound();
          BBX *bbx = new BBX();
          bbx->BoundList.push_back(b1);
          bbx->BoundList.push_back(b2);
          this->metaServerIDToBBX[serverID] = bbx;
        }

        // update the lowerbound and the upper bound for specific partition
        this->metaServerIDToBBX[serverID]->BoundList[0]->m_lb =
            std::min(this->metaServerIDToBBX[serverID]->BoundList[0]->m_lb, i);
        this->metaServerIDToBBX[serverID]->BoundList[1]->m_lb =
            std::min(this->metaServerIDToBBX[serverID]->BoundList[1]->m_lb, j);

        this->metaServerIDToBBX[serverID]->BoundList[0]->m_ub =
            std::max(this->metaServerIDToBBX[serverID]->BoundList[0]->m_ub, i);
        this->metaServerIDToBBX[serverID]->BoundList[1]->m_ub =
            std::max(this->metaServerIDToBBX[serverID]->BoundList[1]->m_ub, j);
      }
    }
  } else if (ndim == 3) {
    for (int i = 0; i < maxDimLen; i++) {
      for (int j = 0; j < maxDimLen; j++) {
        for (int k = 0; k < maxDimLen; k++) {
          int sfc_coord[3] = {i, j, k};
          bitmask_t index = hilbert_c2i(ndim, nBits, sfc_coord);
          int serverID = index / serverNumberForEachNode;
          if ((index + 1) > serverNumberForEachNode * metaServerNum) {
            // put all the reminder into the last server
            serverID = metaServerNum - 1;
          }

          //std::cout << "debug " << i << "index " << index << " serverID "
          //          << serverID << std::endl;

          auto iter = this->metaServerIDToBBX.find(serverID);
          if (iter == metaServerIDToBBX.end()) {

            Bound *b1 = new Bound();
            Bound *b2 = new Bound();
            Bound *b3 = new Bound();
            BBX *bbx = new BBX();
            bbx->BoundList.push_back(b1);
            bbx->BoundList.push_back(b2);
            bbx->BoundList.push_back(b3);
            this->metaServerIDToBBX[serverID] = bbx;
          }

          // update the lowerbound and the upper bound for specific partition
          this->metaServerIDToBBX[serverID]->BoundList[0]->m_lb = std::min(
              this->metaServerIDToBBX[serverID]->BoundList[0]->m_lb, i);
          this->metaServerIDToBBX[serverID]->BoundList[1]->m_lb = std::min(
              this->metaServerIDToBBX[serverID]->BoundList[1]->m_lb, j);
          this->metaServerIDToBBX[serverID]->BoundList[2]->m_lb = std::min(
              this->metaServerIDToBBX[serverID]->BoundList[2]->m_lb, k);

          this->metaServerIDToBBX[serverID]->BoundList[0]->m_ub = std::max(
              this->metaServerIDToBBX[serverID]->BoundList[0]->m_ub, i);
          this->metaServerIDToBBX[serverID]->BoundList[1]->m_ub = std::max(
              this->metaServerIDToBBX[serverID]->BoundList[1]->m_ub, j);
          this->metaServerIDToBBX[serverID]->BoundList[2]->m_ub = std::max(
              this->metaServerIDToBBX[serverID]->BoundList[2]->m_ub, k);
        }
      }
    }
  } else {
    throw std::runtime_error("unsupported ndim value " + std::to_string(ndim));
  }

  return;
}

// get the corresponding metaserver according to the input bbox

std::vector<ResponsibleMetaServer> DHTManager::getMetaServerID(BBX *BBXQuery) {
  // go through the metaServerIDToBBX
  // save the corresponding BBX
  std::vector<ResponsibleMetaServer> rmList;
  for (auto it = this->metaServerIDToBBX.begin();
       it != this->metaServerIDToBBX.end(); ++it) {
    // compare the BBXQuery with every item and store the overlapping part into
    BBX *overlapBBX = getOverlapBBX(BBXQuery, it->second);
    if (overlapBBX != NULL) {
      ResponsibleMetaServer rbm(it->first, overlapBBX);
      rmList.push_back(rbm);
    }
  }
  return rmList;
}
