
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

using namespace BBXTOOL;

namespace MATRIXTOOL {

// TODO the dimention can be larger than 3
// the start position of the global domain is (0,0,0)
// the sequence of the bound is x,y,z
// assume the elem number of the matrix match with the globalUb
void *getSubMatrix(size_t elemSize, std::array<size_t, 3> subLb,
                   std::array<size_t, 3> subUb, std::array<size_t, 3> gloablUb,
                   void *globalMatrix) {

  // check subdomain is valid
  if (subUb[2] < subLb[2] || subUb[1] < subLb[1] || subUb[0] < subLb[0]) {
    throw std::runtime_error("invalid subdomain bounding box");
    return NULL;
  }

  // check the subdomain is in global domain
  if (subUb[2] > gloablUb[2] || subUb[1] > gloablUb[1] ||
      subUb[0] > gloablUb[0]) {
    throw std::runtime_error(
        "subdomain should in the bounding box of the global domain");
    return NULL;
  }

  std::array<size_t, 3> subLen;
  std::array<size_t, 3> globalLen;
  size_t globalelemNum = 1;
  size_t subelemNum = 1;

  for (int i = 0; i < 3; i++) {
    subLen[i] = subUb[i] - subLb[i] + 1;
    globalLen[i] = gloablUb[i] - 0 + 1;
    subelemNum = subelemNum * subLen[i];
    globalelemNum = globalelemNum * globalLen[i];
    std::cout << "debug dim " << i << std::endl;
    std::cout << subLen[i] << " " << globalLen[i] << " " << subelemNum << " "
              << globalelemNum << std::endl;
  }

  void *subMrx = (void *)malloc(elemSize * subelemNum);

  if (subMrx == NULL) {
    throw std::runtime_error("failed to allocate the memory for sub mstrix");
  }

  // if there is full overlap, return direactly
  if (subLb[2] == gloablUb[2] && subLb[1] == gloablUb[1] &&
      subLb[0] == gloablUb[0]) {
    std::memcpy(subMrx, (char *)globalMatrix, elemSize * globalelemNum);
  }

  size_t globalIndex = 0;
  size_t subIndex = 0;
  for (size_t z = 0; z <= gloablUb[2]; z++) {
    // avoid unnessasery check
    if (z >= subLb[2] && z <= subUb[2]) {
      for (size_t y = 0; y <= gloablUb[1]; y++) {
        if (y >= subLb[1] && y <= subUb[1]) {
          for (size_t x = 0; x <= gloablUb[0]; x++) {
            if (x >= subLb[0] && x <= subUb[0]) {
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
struct MatrixView {
  MatrixView(BBX *bbx, void *data) : m_bbx(bbx), m_data(data){};
  BBX *m_bbx = NULL;
  void *m_data = NULL;
  ~MatrixView(){};
};

// assemble separate matrixView into the intact matrixView
MatrixView matrixAssemble(std::vector<MatrixView> matrixViewList,
                          BBX *intactBBX) {

  // generate bit string for every bound
  // refer to https://owlcation.com/stem/C-bitset-with-example for bit set
  // use vector since the length of bitset need to be fixed from the beginning
  std::vector<std::vector<bool>> flagTable;
  for (int i = 0; i < intactBBX->m_dims; i++) {
    int elemNumInDim = intactBBX->BoundList[i]->m_ub - intactBBX->BoundList[i]->m_lb + 1;
    std::vector<bool> flag(elemNumInDim, false);
    flagTable.push_back(flag);
  }

  // check if dimention match with the intactBBX
  for (auto it = matrixViewList.begin(); it != matrixViewList.end(); ++it) {
    MatrixView mv = *it;
    if (mv.m_bbx->m_dims != intactBBX->m_dims) {
      throw std::runtime_error(
          "the dims in matrixViewList not match with the query bbx");
    }

    // go through every dimention and update the flag table
    for (int i = 0; i < mv.m_bbx->m_dims; i++) {
      // for dimention i
      size_t templb = mv.m_bbx->BoundList[i]->m_lb;
      size_t tempub = mv.m_bbx->BoundList[i]->m_ub;
      for (int j = templb; j <= tempub; j++) {
        int elemSize = flagTable[i].size();
        // the upperbound should less than the boundy
        if (j < elemSize) {
          // if the flag is already been true, there is overlap
          if (flagTable[i][j] == true) {
            throw std::runtime_error(
                "there is overlaping between boundies in list");
          } else {
            flagTable[i][j] = true;
          }

        } else {
          throw std::runtime_error(
              "the boundry in list beyond the intact boundy");
        }
      }
    }
  }

  // go through every dimention and check if there is element that is not
  // covered
  for (int i = 0; i < intactBBX->m_dims; i++) {
    int elemSize = flagTable[i].size();
    for (int j = 0; j < elemSize; j++) {
      if (flagTable[i][j] == false) {
        char str[200];
        sprintf(str, "the elements at dim %d position %d is not covered", i, j);
        throw std::runtime_error(std::string(str));
      }
    }
  }

  std::cout << "ok to check the data cover for matrix assemble" << std::endl;
  
  MatrixView intectView(NULL,NULL);
  // copy the elements from every piece into the intact domain

  // check if every piece is included in the matrixViewList
  // check if there is overlaping between two matrix
  // go through every bound, check if position in the bound is covered

  // assign the space for the intactBBX

  // copy every pieces into the intactBBX
  

  return intectView;
}

} // namespace MATRIXTOOL

#endif
