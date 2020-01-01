
// https://stackoverflow.com/questions/50482490/multiple-definition-of-namespace-function

#ifndef BBXTOOL_H
#define BBXTOOL_H

#include <array>
#include <climits>
#include <iostream>
#include <vector>

//if this number is modified, the for loop part also need to be modified
static const size_t DEFAULT_MAX_DIM = 3;

namespace BBXTOOL
{

// the bound value for specific dimention
struct Bound
{
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
struct BBX
{
  // the bound for every dimention

  BBX(size_t dims) : m_dims(dims){};
  // the value of the m_dims represent the valid dimentions for this bounding
  // box
  size_t m_dims = DEFAULT_MAX_DIM;
  /*
  BBX(std::array<int, 2> indexlb, std::array<int, 2> indexub) {
    for (int i = 0; i < 2; i++) {
      Bound *b = new Bound(indexlb[i], indexub[i]);
      this->BoundList.push_back(b);
    }
  };
  */
  // TODO, the bounding box here can larger than 3 in theory, we only implement
  // the 3 for get subregion

    BBX(size_t dimNum, std::array<size_t, DEFAULT_MAX_DIM> indexlb, std::array<size_t, DEFAULT_MAX_DIM> indexub){
          m_dims = dimNum;
    // if there is only one dim, the second and third value will be the 0
    for (int i = 0; i < m_dims; i++)
    {
      Bound *b = new Bound((int)indexlb[i], (int)indexub[i]);
      BoundList.push_back(b);
    }
    };

  BBX(size_t dimNum, std::array<int, DEFAULT_MAX_DIM> indexlb, std::array<int, DEFAULT_MAX_DIM> indexub)
  {
    m_dims = dimNum;
    // if there is only one dim, the second and third value will be the 0
    for (int i = 0; i < m_dims; i++)
    {
      Bound *b = new Bound(indexlb[i], indexub[i]);
      BoundList.push_back(b);
    }
  };

  // the default sequence is x-y-z
  std::vector<Bound *> BoundList;

  ~BBX();

  std::array<int, DEFAULT_MAX_DIM> getIndexlb()
  {
    std::array<int, DEFAULT_MAX_DIM> indexlb = {{0, 0, 0}};
    for (int i = 0; i < m_dims; i++)
    {
      indexlb[i] = BoundList[i]->m_lb;
    }
    return indexlb;
  }

  std::array<int, DEFAULT_MAX_DIM> getIndexub()
  {
    std::array<int, DEFAULT_MAX_DIM> indexub = {{0, 0, 0}};
    for (int i = 0; i < m_dims; i++)
    {
      indexub[i] = BoundList[i]->m_ub;
    }
    return indexub;
  }

  size_t getElemNum()
  {
    size_t elemNum = 1;
    for (int i = 0; i < m_dims; i++)
    {
      elemNum = elemNum * (BoundList[i]->m_ub - BoundList[i]->m_lb + 1);
    }
    return elemNum;
  }

  void printBBXinfo()
  {
    int dim = BoundList.size();
    for (int i = 0; i < dim; i++)
    {
      std::cout << "dim id:" << i << ",lb:" << BoundList[i]->m_lb
                << ",ub:" << BoundList[i]->m_ub << std::endl;
    }
  }

  // calculate the physical position in BBX , for example, the lb is (5,0,0) and
  // the ub is (10,0,0) if the coordinates is the (6,0,0) the index is the 1
  // the sequency in memroy is x-y-z
  size_t getPhysicalIndex(size_t coordim, std::array<int, DEFAULT_MAX_DIM> coordinates)
  {
    if (coordim != m_dims)
    {
      throw std::runtime_error("the dimension of the coordinates should same with the BBX");
    }
    std::array<int, DEFAULT_MAX_DIM> coordinatesNonoffset = {{0, 0, 0}};
    std::array<int, DEFAULT_MAX_DIM> shape = {{0, 0, 0}};
    for (int i = 0; i < m_dims; i++)
    {
      if (coordinates[i] > BoundList[i]->m_ub || coordinates[i] < BoundList[i]->m_lb)
      {
        throw std::runtime_error("the coordinates beyond the boundry of the BBX");
      }
      coordinatesNonoffset[i] = coordinates[i] - BoundList[i]->m_lb;
      shape[i] = BoundList[i]->m_ub - BoundList[i]->m_lb + 1;
    }

    size_t index = coordinatesNonoffset[2] * shape[1] * shape[0] +
                   coordinatesNonoffset[1] * shape[0] + coordinatesNonoffset[0];

    return index;
  }
};

inline Bound *getOverlapBound(Bound *a, Bound *b)
{
  if ((a->m_ub < b->m_lb) || (a->m_lb > b->m_ub))
  {
    return NULL;
  }

  Bound *overlap = new Bound();
  overlap->m_lb = std::max(a->m_lb, b->m_lb);
  overlap->m_ub = std::min(a->m_ub, b->m_ub);

  return overlap;
};

inline BBX *getOverlapBBX(BBX *a, BBX *b)
{
  int aDim = a->m_dims;
  int bDim = b->m_dims;

  if (aDim != bDim)
  {
    throw std::runtime_error(
        "the dimention of two bounding box is not equal aDim is " +
        std::to_string(aDim) + " bDim is " + std::to_string(bDim));
    return NULL;
  }

  if (aDim == 0 || bDim > DEFAULT_MAX_DIM)
  {
    throw std::runtime_error("the dimention of bounding box can not be 0 or "
                             "value larger than 3, current value is: " +
                             std::to_string(aDim));
    return NULL;
  }

  BBX *overlapBBX = new BBX(aDim);
  for (int i = 0; i < aDim; i++)
  {
    Bound *bound = getOverlapBound(a->BoundList[i], b->BoundList[i]);
    if (bound == NULL)
    {
      // if there is not matching for any dimention, return NULL
      return NULL;
    }
    else
    {
      overlapBBX->BoundList.push_back(bound);
    }
  }

  return overlapBBX;
};

inline BBX *trimOffset(BBX *a, std::array<int, DEFAULT_MAX_DIM> offset)
{

  std::array<int, DEFAULT_MAX_DIM> indexlb = a->getIndexlb();
  std::array<int, DEFAULT_MAX_DIM> indexub = a->getIndexub();

  for (int i = 0; i < a->m_dims; i++)
  {
    indexlb[i] = indexlb[i] - offset[i];
    indexub[i] = indexub[i] - offset[i];
  }

  BBX *trimbbx = new BBX(a->m_dims, indexlb, indexub);

  return trimbbx;
}

} // namespace BBXTOOL

#endif
