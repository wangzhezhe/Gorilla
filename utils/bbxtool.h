
// https://stackoverflow.com/questions/50482490/multiple-definition-of-namespace-function

#ifndef BBXTOOL_H
#define BBXTOOL_H

#include <climits>
#include <iostream>
#include <vector>
#include <array>

namespace BBXTOOL {

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

  BBX(size_t dims): m_dims(dims){};
  //the value of the m_dims represent the valid dimentions for this bounding box
  size_t m_dims=3;
  /*
  BBX(std::array<int, 2> indexlb, std::array<int, 2> indexub) {
    for (int i = 0; i < 2; i++) {
      Bound *b = new Bound(indexlb[i], indexub[i]);
      this->BoundList.push_back(b);
    }
  };
  */
  //TODO, the bounding box here can larger than 3 in theory, we only implement the 3 for get subregion
  BBX(size_t dimNum, std::array<int, 3> indexlb, std::array<int, 3> indexub) {
    m_dims = dimNum;
    //if there is only one dim, the second and third value will be the 0
    for (int i = 0; i < 3; i++) {
      Bound *b = new Bound(indexlb[i], indexub[i]);
      BoundList.push_back(b);
    }
  };
  // the default sequence is x-y-z
  std::vector<Bound *> BoundList;
  ~BBX();

  std::array<int, 3> getIndexlb() {
    std::array<int, 3> indexlb;
    int size = BoundList.size();
    for (int i = 0; i < size; i++) {
      indexlb[i] = BoundList[i]->m_lb;
    }
    return indexlb;
  }

  std::array<int, 3> getIndexub() {
    std::array<int, 3> indexub;
    int size = BoundList.size();
    for (int i = 0; i < size; i++) {
      indexub[i] = BoundList[i]->m_ub;
    }
    return indexub;
  }

  void printBBXinfo() {
    int dim = BoundList.size();
    for (int i = 0; i < dim; i++) {
      std::cout << "dim id:" << i << ",lb:" << BoundList[i]->m_lb
                << ",ub:" << BoundList[i]->m_ub << std::endl;
    }
  }
};

inline Bound *getOverlapBound(Bound *a, Bound *b) {
  if ((a->m_ub < b->m_lb) || (a->m_lb > b->m_ub)) {
    return NULL;
  }

  Bound *overlap = new Bound();
  overlap->m_lb = std::max(a->m_lb, b->m_lb);
  overlap->m_ub = std::min(a->m_ub, b->m_ub);

  return overlap;
};

inline BBX *getOverlapBBX(BBX *a, BBX *b) {
  int aDim = a->m_dims;
  int bDim = b->m_dims;

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

  BBX *overlapBBX = new BBX(aDim);
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
};

} // namespace BBXTOOL

#endif
