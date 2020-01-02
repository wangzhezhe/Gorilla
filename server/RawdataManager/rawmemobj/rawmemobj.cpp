#include "rawmemobj.h"
#include "../../../utils/matrixtool.h"

int RawMemObj::putData(void *dataSourcePtr)
{
  int memSize =
      this->m_blockSummary.m_elemSize * this->m_blockSummary.m_elemNum;

  this->m_rawMemPtr = (void *)malloc(memSize);
  if (m_rawMemPtr == NULL)
  {
    throw std::runtime_error("failed to allocate the memroy for RawMemObj");
  }
  std::memcpy(m_rawMemPtr, dataSourcePtr, memSize);
  return 0;
}

BlockSummary RawMemObj::getData(void *&dataContainer)
{
  if (this->m_rawMemPtr == NULL)
  {
    throw std::runtime_error("failed to getData for RawMemObj");
  }
  dataContainer = (void *)(this->m_rawMemPtr);
  return this->m_blockSummary;
}

//this is a kind of data filter ？
BlockSummary RawMemObj::getDataSubregion(
    size_t dims,
    std::array<int, 3> subregionlb,
    std::array<int, 3> subregionub,
    void *&dataContainer)
{

  /*
  void *getSubMatrix(size_t elemSize, std::array<size_t, 3> subLb,
                     std::array<size_t, 3> subUb, std::array<size_t, 3>
  gloablUb, void *globalMatrix) {
  */
  if (dims != this->m_blockSummary.m_dims)
  {
    throw std::runtime_error("the dims is not equal to the value in blockSummary");
  }

  //if the query match with the bbx in storage, retuen how data direactly
  bool ifAllMatch = true;
  for (int i = 0; i < dims; i++)
  {
    if (subregionlb[i] != this->m_blockSummary.m_indexlb[i] || subregionub[i] != this->m_blockSummary.m_indexub[i])
    {
      ifAllMatch = false;
      break;
    }
  }
  if (ifAllMatch)
  {
    dataContainer = (void *)(this->m_rawMemPtr);
    return this->m_blockSummary;
  }

  size_t elemSize = (size_t)this->m_blockSummary.m_elemSize;
  // decrease the offset
  std::array<int, 3> offset = this->m_blockSummary.m_indexlb;
  std::array<size_t, 3> subregionLbNonoffset;
  std::array<size_t, 3> subregionUbNonoffset;
  std::array<size_t, 3> globalUbNonoffset;

  //the value whithout offset should larger or equal to 0
  for (int i = 0; i < 3; i++)
  {
    subregionLbNonoffset[i] = (size_t)(subregionlb[i] - offset[i]);
    subregionUbNonoffset[i] = (size_t)(subregionub[i] - offset[i]);
    globalUbNonoffset[i] = (size_t)(this->m_blockSummary.m_indexub[i] - offset[i]);
  }

  // check if the data elem number match with the required one
  size_t currentElemNum = 1;
  for (int i = 0; i < 3; i++)
  {
    if (globalUbNonoffset[i] != 0)
    {
      currentElemNum = currentElemNum * globalUbNonoffset[i];
    }
  }

  if (this->m_blockSummary.m_elemNum < currentElemNum)
  {
    m_blockSummary.printSummary();
    std::cout << "m_blockSummary.m_elemNum " << m_blockSummary.m_elemNum << " currentElemNum " << currentElemNum << std::endl;
    throw std::runtime_error("failed to getDataSubregion, current elem number is larger than the value in Blocksummary");
  }

  void *result = MATRIXTOOL::getSubMatrix(elemSize, subregionLbNonoffset, subregionUbNonoffset,
                                          globalUbNonoffset, this->m_rawMemPtr);

  if (result == NULL)
  {
    throw std::runtime_error("failed to getDataSubregion");
  }

  dataContainer = (void *)(result);
  BlockSummary bs = this->m_blockSummary;

  //update the information in the subregion
  bs.m_indexlb = subregionlb;
  bs.m_indexub = subregionub;

  return bs;
}
