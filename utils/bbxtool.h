
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
    Bound(const Bound &b)
    {
      m_lb = b.m_lb;
      m_ub = b.m_ub;
    }
    Bound &operator=(const Bound &b)
    {
      m_lb = b.m_lb;
      m_ub = b.m_ub;
      return *this;
    }
    int m_lb = INT_MAX;
    int m_ub = INT_MIN;
    ~Bound(){};
    bool equal(Bound otherb)
    {
      if (this->m_lb == otherb.m_lb && this->m_ub == otherb.m_ub)
      {
        return true;
      }
      else
      {
        return false;
      }
    };
    std::vector<Bound> splitBound(Bound queryBound)
    {
      std::vector<Bound> splitBoundList;
      if (queryBound.m_ub < this->m_lb || queryBound.m_lb > this->m_ub)
      {
        return splitBoundList;
      }
      if (this->equal(queryBound))
      {
        splitBoundList.push_back(Bound(this->m_lb, this->m_ub));
        return splitBoundList;
      }
      if (queryBound.m_lb <= this->m_lb && queryBound.m_ub >= this->m_lb && queryBound.m_ub < this->m_ub)
      {
        splitBoundList.push_back(Bound(this->m_lb, queryBound.m_ub));
        splitBoundList.push_back(Bound(queryBound.m_ub + 1, this->m_ub));
      }
      else if (queryBound.m_lb > this->m_lb && queryBound.m_ub < this->m_ub)
      {
        splitBoundList.push_back(Bound(this->m_lb, queryBound.m_lb - 1));
        splitBoundList.push_back(Bound(queryBound.m_lb, queryBound.m_ub));
        splitBoundList.push_back(Bound(queryBound.m_ub + 1, this->m_ub));
      }
      else if (queryBound.m_lb <= this->m_ub && queryBound.m_ub >= this->m_ub)
      {
        splitBoundList.push_back(Bound(this->m_lb, queryBound.m_lb - 1));
        splitBoundList.push_back(Bound(queryBound.m_lb, this->m_ub));
      }
      else
      {
        throw std::runtime_error("unsuported case for split boundry\n");
      }
      return splitBoundList;
    };
  };

  // Attention!!! for the bbx that is larger than 3, it is alwasy easy to use lb and ub for every dim,
  // and store several bound in bbx
  // instead of using other presentations
  // the bbox for the application domain
  // the lb/ub represents the lower bound and the upper bound of every dimention
  // Attention, if there is 1 elements for every dimention, the lower bound is
  // same with the upper bound
  // TODO update, use the vector of the Bound
  struct BBX
  {
    BBX(){};

    BBX(size_t dims) : m_dims(dims){};
    // the value of the m_dims represent the valid dimentions for this bounding
    // box

    BBX(size_t dimNum, std::array<size_t, DEFAULT_MAX_DIM> indexlb, std::array<size_t, DEFAULT_MAX_DIM> indexub)
    {
      m_dims = dimNum;
      m_status = 0;
      // if there is only one dim, the second and third value will be the 0
      for (int i = 0; i < m_dims; i++)
      {
        BoundList.push_back(Bound((int)indexlb[i], (int)indexub[i]));
      }
    };

    BBX(size_t dimNum, std::array<int, DEFAULT_MAX_DIM> indexlb, std::array<int, DEFAULT_MAX_DIM> indexub)
    {
      m_dims = dimNum;
      m_status = 0;
      // if there is only one dim, the second and third value will be the 0
      for (int i = 0; i < m_dims; i++)
      {

        this->BoundList.push_back(Bound(indexlb[i], indexub[i]));
      }
    };

    BBX(size_t dimNum, Bound b)
    {
      if (dimNum != 1)
      {
        throw std::runtime_error("failed for create BBX 1d");
      }
      this->BoundList.push_back(b);
      m_dims = dimNum;
      m_status = 0;
    }

    BBX(size_t dimNum, Bound b1, Bound b2)
    {
      if (dimNum != 2)
      {
        throw std::runtime_error("failed for create BBX 1d");
      }
      m_dims = dimNum;
      m_status = 0;
      this->BoundList.push_back(b1);
      this->BoundList.push_back(b2);
    }

    BBX(size_t dimNum, Bound b1, Bound b2, Bound b3)
    {
      if (dimNum != 3)
      {
        throw std::runtime_error("failed for create BBX 1d");
      }
      m_dims = dimNum;
      m_status = 0;
      this->BoundList.push_back(b1);
      this->BoundList.push_back(b2);
      this->BoundList.push_back(b3);
    }

    BBX(const BBX &otherbbx)
    {
      this->m_dims = otherbbx.m_dims;
      this->m_status = otherbbx.m_status;
      this->BoundList = otherbbx.BoundList;
    }

    BBX &operator=(const BBX &bbx)
    {
      this->m_dims = bbx.m_dims;
      this->m_status = bbx.m_status;
      this->BoundList = bbx.BoundList;
      return *this;
    }

    ~BBX(){};

    size_t m_dims = DEFAULT_MAX_DIM;
    //the status is used by the metadata server when checking if the queried region is covered
    int m_status = 0;
    // when the data is in y-z plane, the element at the first position should be inited value, there is still in 3 dim space
    // it is more convenient to allocate the space if use the vector<Bound>
    // the default sequence is x-y-z
    std::vector<Bound> BoundList;
    std::array<int, DEFAULT_MAX_DIM> getIndexlb()
    {
      std::array<int, DEFAULT_MAX_DIM> indexlb = {{0, 0, 0}};
      for (int i = 0; i < m_dims; i++)
      {
        indexlb[i] = BoundList[i].m_lb;
      }
      return indexlb;
    }

    std::array<int, DEFAULT_MAX_DIM> getIndexub()
    {
      std::array<int, DEFAULT_MAX_DIM> indexub = {{0, 0, 0}};
      for (int i = 0; i < m_dims; i++)
      {
        indexub[i] = BoundList[i].m_ub;
      }
      return indexub;
    }

    size_t getElemNum()
    {
      size_t elemNum = 1;
      for (int i = 0; i < m_dims; i++)
      {
        elemNum = elemNum * (BoundList[i].m_ub - BoundList[i].m_lb + 1);
      }
      return elemNum;
    }

    void printBBXinfo()
    {
      int dim = BoundList.size();
      if (dim != this->m_dims)
      {
        throw std::runtime_error("the m_dim not equal to actual dims\n");
      }
      for (int i = 0; i < dim; i++)
      {
        std::cout << "dim id:" << i << ",lb:" << BoundList[i].m_lb
                  << ",ub:" << BoundList[i].m_ub << std::endl;
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
        if (coordinates[i] > BoundList[i].m_ub || coordinates[i] < BoundList[i].m_lb)
        {
          throw std::runtime_error("the coordinates beyond the boundry of the BBX");
        }
        coordinatesNonoffset[i] = coordinates[i] - BoundList[i].m_lb;
        shape[i] = BoundList[i].m_ub - BoundList[i].m_lb + 1;
      }

      size_t index = coordinatesNonoffset[2] * shape[1] * shape[0] +
                     coordinatesNonoffset[1] * shape[0] + coordinatesNonoffset[0];

      return index;
    }

    bool equal(BBX otherbbx)
    {
      if (this->m_dims != otherbbx.m_dims)
      {
        return false;
      }

      for (int i = 0; i < this->m_dims; i++)
      {
        if (!this->BoundList[i].equal(otherbbx.BoundList[i]))
        {
          return false;
        }
      }
      return true;
    }
  };

  inline Bound *getOverlapBound(Bound a, Bound b)
  {
    if ((a.m_ub < b.m_lb) || (a.m_lb > b.m_ub))
    {
      return NULL;
    }

    Bound *overlap = new Bound();
    overlap->m_lb = std::max(a.m_lb, b.m_lb);
    overlap->m_ub = std::min(a.m_ub, b.m_ub);

    return overlap;
  };

  inline BBX *getOverlapBBX(BBX &a, BBX &b)
  {
    int aDim = a.m_dims;
    int bDim = b.m_dims;

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
      Bound *bound = getOverlapBound(a.BoundList[i], b.BoundList[i]);
      if (bound == NULL)
      {
        // if there is not matching for any dimention, return NULL
        // or use -1 to represent it
        return NULL;
      }
      else
      {
        overlapBBX->BoundList.push_back(*bound);
        if (bound != NULL)
        {
          delete bound;
        }
      }
    }
    //this is free by caller
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
  };

  //there is opportunity to reduce the number of the returnning block
  inline std::vector<BBX> splitReduceBBX(BBX original, BBX querybbx)
  {
    std::vector<BBX> bbxList;
    if (querybbx.m_dims != original.m_dims)
    {
      return bbxList;
    }
    BBX *overlap = getOverlapBBX(original, querybbx);
    if (overlap == NULL)
    {
      throw std::runtime_error("overlap should not be null");
    }
    //reduce bbx from current one and split the remaining partition
    if (original.m_dims == 2)
    {
      std::vector<Bound> boundList0 = original.BoundList[0].splitBound(querybbx.BoundList[0]);
      std::vector<Bound> boundList1 = original.BoundList[1].splitBound(querybbx.BoundList[1]);
      for (auto v0 : boundList0)
      {
        for (auto v1 : boundList1)
        {
          if (v0.equal(overlap->BoundList[0]) && v1.equal(overlap->BoundList[1]))
          {
            continue;
          }
          bbxList.push_back(BBX(2, v0, v1));
        }
      }
    }

    if (original.m_dims == 3)
    {
      std::vector<Bound> boundList0 = original.BoundList[0].splitBound(querybbx.BoundList[0]);
      std::vector<Bound> boundList1 = original.BoundList[1].splitBound(querybbx.BoundList[1]);
      std::vector<Bound> boundList2 = original.BoundList[2].splitBound(querybbx.BoundList[2]);

      for (auto v0 : boundList0)
      {
        for (auto v1 : boundList1)
        {
          for (auto v2 : boundList2)
          {
            if (v0.equal(querybbx.BoundList[0]) && v1.equal(querybbx.BoundList[1]) && v2.equal(querybbx.BoundList[2]))
            {
              continue;
            }
            bbxList.push_back(BBX(3, v0, v1, v2));
          }
        }
      }
    }

    if (original.m_dims > 3)
    {
      throw std::runtime_error("unsuported mesh that is larger than 3");
    }

    if (overlap != NULL)
    {
      delete overlap;
    }

    return bbxList;
  };

  /*
enum BBXSTATUS {DIFFDIM, EQUAL, NOOVERLAP, AINB, BINA, OVERLAP};
inline BBXSTATUS getBBXStatus(BBX A, BBX B){
  if(A.m_dims!=B.m_dims){
    return BBXSTATUS::DIFFDIM;
  }



}
*/

  //call this function when make sure there is overlap between two bbx
  //input an large bbx and divede it into several small bbxs after deleteing the overlap part
  //the getOverLap have been called, we can make sure there is overlap
  //inline std::vector<BBX> splitBBX(BBX *largeBBX, BBX *overlapBBX){

  //for every bound, construct

  //use splitBound for every domain and then construct new domain and put it into bbx

  //organize it into suitable domain

  //};

}; // namespace BBXTOOL

#endif
