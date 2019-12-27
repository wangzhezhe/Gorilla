#include "rawmemobj.h"
#include "../../../utils/matrixtool.h"


int RawMemObj::putData(void *dataSourcePtr) {
  int memSize =
      this->m_blockSummary.m_elemSize * this->m_blockSummary.m_elemNum;

  this->m_rawMemPtr = (void *)malloc(memSize);
  if (m_rawMemPtr == NULL) {
    throw std::runtime_error("failed to allocate the memroy for RawMemObj");
  }
  std::memcpy(m_rawMemPtr, dataSourcePtr, memSize);
  return 0;
}

BlockSummary RawMemObj::getData(void *&dataContainer) {
  if (this->m_rawMemPtr == NULL) {
    throw std::runtime_error("failed to getData for RawMemObj");
  }
  dataContainer = (void *)(this->m_rawMemPtr);
  return this->m_blockSummary;
}


//this is a kind of data filter???
BlockSummary RawMemObj::getDataSubregion(std::array<size_t, 3> subregionlb,
                                         std::array<size_t, 3> subregionub,
                                         void *&dataContainer) {

  /*
  void *getSubMatrix(size_t elemSize, std::array<size_t, 3> subLb,
                     std::array<size_t, 3> subUb, std::array<size_t, 3>
  gloablUb, void *globalMatrix) {
  */

  size_t elemSize = (size_t)this->m_blockSummary.m_elemSize;
  // decrease the offset
  std::array<size_t, 3> offset = this->m_blockSummary.m_indexlb;
  std::array<size_t, 3> subregionLbNonoffset = subregionlb;
  std::array<size_t, 3> subregionUbNonoffset = subregionub;
  std::array<size_t, 3> globalUbNonoffset = this->m_blockSummary.m_indexub;

  for (int i = 0; i < 3; i++) {
    subregionLbNonoffset[i] = subregionLbNonoffset[i] - offset[i];
    subregionUbNonoffset[i] = subregionUbNonoffset[i] - offset[i];
    globalUbNonoffset[i] = globalUbNonoffset[i] - offset[i];
  }

  // check if the data elem number match with the required one
  size_t currentElemNum = 1;
  for (int i = 0; i < 3; i++) {
    if (globalUbNonoffset[i] != 0) {
      currentElemNum = currentElemNum * globalUbNonoffset[i];
    }
  }

  if (this->m_blockSummary.m_elemNum != currentElemNum) {
    throw std::runtime_error("failed to getDataSubregion, current elem number "
                             "not match with the value in Blocksummary");
  }

  void *result = MATRIXTOOL::getSubMatrix(elemSize, subregionLbNonoffset, subregionUbNonoffset,
                   globalUbNonoffset, this->m_rawMemPtr);

  if (result == NULL) {
    throw std::runtime_error("failed to getDataSubregion");
  }

  dataContainer = (void *)(result);
  BlockSummary bs = this->m_blockSummary;

  //update the information in the subregion
  bs.m_indexlb = subregionlb;
  bs.m_indexub = subregionub;

  return bs;
}
