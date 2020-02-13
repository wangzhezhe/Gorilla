
// https://stackoverflow.com/questions/50482490/multiple-definition-of-namespace-function

#ifndef MATRIXTOOL_H
#define MATRIXTOOL_H

#include "bbxtool.h"
#include <array>
#include <climits>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#define BILLION 1000000000L

using namespace BBXTOOL;

namespace MATRIXTOOL
{

// TODO the dimention can be larger than 3
// the start position of the global domain is (0,0,0)
// the sequence of the bound is x,y,z
// assume the elem number of the matrix match with the globalUb
inline void *getSubMatrix(size_t elemSize, std::array<size_t, DEFAULT_MAX_DIM> subLb,
                          std::array<size_t, DEFAULT_MAX_DIM> subUb, std::array<size_t, DEFAULT_MAX_DIM> gloablUb,
                          void *globalMatrix)
{

  // check subdomain is valid
  if (subUb[2] < subLb[2] || subUb[1] < subLb[1] || subUb[0] < subLb[0])
  {
    throw std::runtime_error("invalid subdomain bounding box");
    return NULL;
  }

  // check the subdomain is in global domain
  if (subUb[2] > gloablUb[2] || subUb[1] > gloablUb[1] ||
      subUb[0] > gloablUb[0])
  {
    throw std::runtime_error(
        "subdomain should in the bounding box of the global domain");
    return NULL;
  }

  std::array<size_t, DEFAULT_MAX_DIM> subLen;
  std::array<size_t, DEFAULT_MAX_DIM> globalLen;
  size_t globalelemNum = 1;
  size_t subelemNum = 1;

  for (int i = 0; i < DEFAULT_MAX_DIM; i++)
  {
    subLen[i] = subUb[i] - subLb[i] + 1;
    globalLen[i] = gloablUb[i] - 0 + 1;
    subelemNum = subelemNum * subLen[i];
    globalelemNum = globalelemNum * globalLen[i];
    //std::cout << "debug dim " << i << std::endl;
    //std::cout << subLen[i] << " " << globalLen[i] << " " << subelemNum << " "
    //          << globalelemNum << std::endl;
  }

  void *subMrx = (void *)malloc(elemSize * subelemNum);

  if (subMrx == NULL)
  {
    throw std::runtime_error("failed to allocate the memory for sub mstrix");
  }

  // if there is full overlap, return direactly
  if (subLb[2] == gloablUb[2] && subLb[1] == gloablUb[1] &&
      subLb[0] == gloablUb[0])
  {
    std::memcpy(subMrx, (char *)globalMatrix, elemSize * globalelemNum);
  }

  size_t globalIndex = 0;
  size_t subIndex = 0;
  for (size_t z = 0; z <= gloablUb[2]; z++)
  {
    // avoid unnessasery check
    if (z >= subLb[2] && z <= subUb[2])
    {
      for (size_t y = 0; y <= gloablUb[1]; y++)
      {
        if (y >= subLb[1] && y <= subUb[1])
        {
          for (size_t x = 0; x <= gloablUb[0]; x++)
          {
            if (x >= subLb[0] && x <= subUb[0])
            {
              globalIndex =
                  z * globalLen[1] * globalLen[0] + y * globalLen[0] + x;
              // be careful to minus the boundry for when caculate the index for
              // sub domain
              subIndex = (z - subLb[2]) * subLen[1] * subLen[0] +
                         (y - subLb[1]) * subLen[0] + x - subLb[0];

              memcpy((char *)subMrx + subIndex * elemSize,
                     (char *)globalMatrix + globalIndex * elemSize, elemSize);
            }
          }
        }
      }
    }
  }

  return subMrx;
}

// assume that the bounding box match with the data size
struct MatrixView
{
  MatrixView(BBX *bbx, void *data) : m_bbx(bbx), m_data(data){};
  BBX *m_bbx = NULL;
  void *m_data = NULL;
  ~MatrixView(){
      //This is just a view, it will not release the data
  };
};

// assemble separate matrixView into the intact matrixView
// assume the offset is decreased from the matrix
inline MatrixView matrixAssemble(size_t elemSize, std::vector<MatrixView> &matrixViewList,
                                 BBX *intactBBX)
{

  //struct timespec start, end1,end2;
  //double diff1,diff2;
  //clock_gettime(CLOCK_REALTIME, &start);
  
  
  size_t intactElemNum = intactBBX->getElemNum();

  /* do the check when it is necessary
  //the flag to detect if there is value for each position and
  //if there is overlap between different matrix
  std::vector<bool> flag(intactElemNum, false);
  for (auto it = matrixViewList.begin(); it != matrixViewList.end(); ++it)
  {

    MatrixView mv = *it;
    size_t mvDim = mv.m_bbx->m_dims;
    if (mvDim != intactBBX->m_dims)
    {
      throw std::runtime_error("the dims in matrixViewList not match with the query bbx");
    }

    std::array<int, DEFAULT_MAX_DIM> sublb = mv.m_bbx->getIndexlb();
    std::array<int, DEFAULT_MAX_DIM> subub = mv.m_bbx->getIndexub();

    //for every bbx, go through every elements
    //the part can be accelerated by using multiple thread
    // this is time consuming operation add this when it is necessary
    for (int z = sublb[2]; z <= subub[2]; z++)
    {
      for (int y = sublb[1]; y <= subub[1]; y++)
      {
        for (int x = sublb[0]; x <= subub[0]; x++)
        {
          //std::cout << "coordinates " << x << "," << y << "," << z << std::endl;
          //caculate intactIndex
          //caculate the subdomainIndex
          //copy from the subdomain position into the position with the intact position
          //mv is the view of the subdomain and the intactView is the view of the whole domain
          size_t globalIndex = intactBBX->getPhysicalIndex(mvDim, {{x, y, z}});
          if (flag[globalIndex] == false)
          {
            flag[globalIndex] = true;
          }
          else
          {
            throw std::runtime_error("there is overlaping between sub matrix");
          }
        }
      }
    }
  }
  

  for (int i = 0; i < intactElemNum; i++)
  {
    if (flag[i] == false)
    {
      throw std::runtime_error("the element in intact matrix is not covered");
    }
  }
  */

  //this part is used to check if there is overlapping between each nodes
  //which can be deleted for performance testing

  //clock_gettime(CLOCK_REALTIME, &end1);
  //diff1 = (end1.tv_sec - start.tv_sec) * 1.0 + (end1.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
  //std::cout << "debug assemble stage 1 " << diff1 << std::endl;
  //std::cout << "ok to check the data cover for matrix assembly" << std::endl;

  //start to copy the element from the subdomain into the intact domain
  void *intactDataPtr = (void *)malloc(elemSize * intactElemNum);

  //allocate the space for intecatView
  MatrixView intactView(intactBBX, intactDataPtr);

  // copy the elements from every piece into the intact domain
  // check if every piece is included in the matrixViewList
  // check if there is overlaping between two matrix
  // go through every bound, check if position in the bound is covered

  for (auto it = matrixViewList.begin(); it != matrixViewList.end(); ++it)
  {
    MatrixView mv = *it;
    size_t dim = mv.m_bbx->m_dims;

    std::array<int, DEFAULT_MAX_DIM> sublb = mv.m_bbx->getIndexlb();
    std::array<int, DEFAULT_MAX_DIM> subub = mv.m_bbx->getIndexub();

    //for every bbx, go through every elements
    //the part can be accelerated by using multiple thread
    for (int z = sublb[2]; z <= subub[2]; z++)
    {
      for (int y = sublb[1]; y <= subub[1]; y++)
      {

        //copy one continuous line at one time
        int xlb = sublb[0];
        int xub = subub[0];

        //std::cout << "coordinates " << x << "," << y << "," << z << std::endl;
        //caculate intactIndex
        //caculate the subdomainIndex
        //copy from the subdomain position into the position with the intact position
        //mv is the view of the subdomain and the intactView is the view of the whole domain
        size_t subIndex = mv.m_bbx->getPhysicalIndex(dim, {{xlb, y, z}});
        size_t globalIndex = intactView.m_bbx->getPhysicalIndex(dim, {{xlb, y, z}});

        //std::cout << "subIndex " << subIndex << " globalIndex " << globalIndex << std::endl;

        //data one data for one time
        memcpy((char *)intactView.m_data + globalIndex * elemSize,
               (char *)mv.m_data + subIndex * elemSize, elemSize * (xub - xlb + 1));
      }
    }
  }

  //clock_gettime(CLOCK_REALTIME, &end2);
  //diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
  //std::cout << "debug assemble stage 2 " << diff2 << std::endl;

  return intactView;
}

} // namespace MATRIXTOOL

#endif
