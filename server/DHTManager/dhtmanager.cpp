#include "dhtmanager.h"
#include "../utils/hilbert/hilbert.h"
#include <algorithm>
#include <string>
#include <cmath>

// n is the number of the elements
int computeBits(int n)
{
  int nr_bits = 0;

  while (n)
  {
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

  while (n != 0)
  {
    n >>= 1;
    count += 1;
  }

  return 1 << count;
}

// init the metaServerBBOXList according to the metaServerNum(partitionNum) and
// the bbox of the global domain The hilbert DHT is only suitable for the cubic
void DHTManager::initDHTBySFC(size_t ndim, size_t metaServerNum, BBX *globalBBX)
{

  if (globalBBX->BoundList.size() != ndim)
  {
    throw std::runtime_error("dim num should same with the number of bound in globalBBX");
  }
  // get max dim value
  int maxDimLen = INT_MIN;
  for (int i = 0; i < ndim; i++)
  {
    // this is the number of the value, so plus one
    maxDimLen = std::max(maxDimLen, globalBBX->BoundList[i].m_ub + 1);
  }

  //the len of every dim in virtual box is virtualMaxDimLen
  //the real value is the maxDimLen
  int virtualMaxDimLen = nextPowerOf2(maxDimLen);

  // caculate the total number of the elements
  // the value should be the 2^n that cover the current bbx
  int totalElems = virtualMaxDimLen;
  for (int i = 0; i < ndim; i++)
  {
    if (i > 0)
    {
      totalElems = totalElems * virtualMaxDimLen;
    }
  }

  int serverNumberForEachNode = 0;
  serverNumberForEachNode = totalElems / metaServerNum;

  //this is used to trim for when search the boundry
  //only the points around the trim position can be the boundry
  int span = virtualMaxDimLen / metaServerNum;
  if (span == 0 || span == 1)
  {
    throw std::runtime_error("the span value should not be zero or one when init dht");
  }

  int nBits = computeBits(virtualMaxDimLen);

  std::cout << "debug totalElems " << totalElems << " elemtNumberForEachNode "
            << serverNumberForEachNode << " virtualMaxDimLen " << virtualMaxDimLen << " nbits " << nBits << std::endl;

  if (ndim == 1)
  {
    for (int i = 0; i < maxDimLen; i++)
    {

      int sfc_coord[1] = {i};
      bitmask_t index = hilbert_c2i(ndim, nBits, sfc_coord);
      int serverID = index / serverNumberForEachNode;
      if ((index + 1) > serverNumberForEachNode * metaServerNum)
      {
        // put all the reminder into the last server
        serverID = metaServerNum - 1;
      }

      //std::cout << "debug " << i << "index " << index << " serverID "
      //          << serverID << std::endl;

      auto iter = this->metaServerIDToBBX.find(serverID);
      if (iter == metaServerIDToBBX.end())
      {

        BBX *bbx = new BBX(1);
        bbx->BoundList.push_back(Bound());
        this->metaServerIDToBBX[serverID] = bbx;
      }

      // update the lowerbound and the upper bound for specific partition
      this->metaServerIDToBBX[serverID]->BoundList[0].m_lb =
          std::min(this->metaServerIDToBBX[serverID]->BoundList[0].m_lb, i);

      this->metaServerIDToBBX[serverID]->BoundList[0].m_ub =
          std::max(this->metaServerIDToBBX[serverID]->BoundList[0].m_ub, i);
      // modify the ub if it is larger than real value
    }
  }
  else if (ndim == 2)
  {
    for (int i = 0; i < maxDimLen; i++)
    {
      for (int j = 0; j < maxDimLen; j++)
      {
        int sfc_coord[2] = {i, j};
        bitmask_t index = hilbert_c2i(ndim, nBits, sfc_coord);
        int serverID = index / serverNumberForEachNode;
        if ((index + 1) > serverNumberForEachNode * metaServerNum)
        {
          // put all the reminder into the last server
          serverID = metaServerNum - 1;
        }

        //std::cout << "debug " << i << "," << j << "index " << index
        //          << " serverID " << serverID << std::endl;

        auto iter = this->metaServerIDToBBX.find(serverID);
        if (iter == metaServerIDToBBX.end())
        {

          Bound b1 ;
          Bound b2 ;
          BBX *bbx = new BBX(2);
          bbx->BoundList.push_back(b1);
          bbx->BoundList.push_back(b2);
          this->metaServerIDToBBX[serverID] = bbx;
        }

        // update the lowerbound and the upper bound for specific partition
        this->metaServerIDToBBX[serverID]->BoundList[0].m_lb =
            std::min(this->metaServerIDToBBX[serverID]->BoundList[0].m_lb, i);
        this->metaServerIDToBBX[serverID]->BoundList[1].m_lb =
            std::min(this->metaServerIDToBBX[serverID]->BoundList[1].m_lb, j);

        this->metaServerIDToBBX[serverID]->BoundList[0].m_ub =
            std::max(this->metaServerIDToBBX[serverID]->BoundList[0].m_ub, i);
        this->metaServerIDToBBX[serverID]->BoundList[1].m_ub =
            std::max(this->metaServerIDToBBX[serverID]->BoundList[1].m_ub, j);
      }
    }
  }
  else if (ndim == 3)
  {
    for (int i = 0; i < maxDimLen;)
    {
      for (int j = 0; j < maxDimLen;)
      {
        for (int k = 0; k < maxDimLen;)
        {

          int sfc_coord[3] = {i, j, k};
          bitmask_t index = hilbert_c2i(ndim, nBits, sfc_coord);
          int serverID = index / serverNumberForEachNode;
          if ((index + 1) > serverNumberForEachNode * metaServerNum)
          {
            // put all the reminder into the last server
            serverID = metaServerNum - 1;
          }

          //std::cout << "debug " << i << "index " << index << " serverID "
          //          << serverID << std::endl;

          auto iter = this->metaServerIDToBBX.find(serverID);
          if (iter == metaServerIDToBBX.end())
          {

            Bound b1 ;
            Bound b2 ;
            Bound b3 ;
            BBX *bbx = new BBX(3);
            bbx->BoundList.push_back(b1);
            bbx->BoundList.push_back(b2);
            bbx->BoundList.push_back(b3);
            this->metaServerIDToBBX[serverID] = bbx;
          }

          // update the lowerbound and the upper bound for specific partition
          this->metaServerIDToBBX[serverID]->BoundList[0].m_lb = std::min(
              this->metaServerIDToBBX[serverID]->BoundList[0].m_lb, i);
          this->metaServerIDToBBX[serverID]->BoundList[1].m_lb = std::min(
              this->metaServerIDToBBX[serverID]->BoundList[1].m_lb, j);
          this->metaServerIDToBBX[serverID]->BoundList[2].m_lb = std::min(
              this->metaServerIDToBBX[serverID]->BoundList[2].m_lb, k);

          this->metaServerIDToBBX[serverID]->BoundList[0].m_ub = std::max(
              this->metaServerIDToBBX[serverID]->BoundList[0].m_ub, i);
          this->metaServerIDToBBX[serverID]->BoundList[1].m_ub = std::max(
              this->metaServerIDToBBX[serverID]->BoundList[1].m_ub, j);
          this->metaServerIDToBBX[serverID]->BoundList[2].m_ub = std::max(
              this->metaServerIDToBBX[serverID]->BoundList[2].m_ub, k);

          if (k % 2 == 0)
          {
            k = k + span - 1;
          }
          else
          {
            k = k + 1;
          }
        }
        if (j % 2 == 0)
        {
          j = j + span - 1;
        }
        else
        {
          j = j + 1;
        }
      }
      if (i % 2 == 0)
      {
        i = i + span - 1;
      }
      else
      {
        i = i + 1;
      }
    }
  }
  else
  {
    throw std::runtime_error("unsupported ndim value " + std::to_string(ndim));
  }

  return;
}

void DHTManager::initDHTManually(std::vector<int> &lenArray, std::vector<int> &partitionLayout)
{

  //detecting dimention
  if (lenArray.size() != partitionLayout.size())
  {
    throw std::runtime_error("the dimention of the lenArray should match with the partitionLayout");
  }

  int partitionLayout3d[3] = {1, 1, 1};
  int dims = partitionLayout.size();

  int xSpan = 0;
  int ySpan = 0;
  int zSpan = 0;

  if (dims >= 1)
  {
    xSpan = (int)std::ceil(lenArray[0] * 1.0 / partitionLayout[0]);
  }
  if (dims >= 2)
  {
    ySpan = (int)std::ceil(lenArray[1] * 1.0 / partitionLayout[1]);
  }
  if (dims >= 3)
  {
    zSpan = (int)std::ceil(lenArray[2] * 1.0 / partitionLayout[2]);
  }

  for (int i = 0; i < dims; i++)
  {
    partitionLayout3d[i] = partitionLayout[i];
  }

  for (int z = 0; z < partitionLayout3d[2]; z++)
  {
    for (int y = 0; y < partitionLayout3d[1]; y++)
    {
      for (int x = 0; x < partitionLayout3d[0]; x++)
      {
        int partitionID = z * partitionLayout3d[1] * partitionLayout3d[0] + y * partitionLayout3d[0] + x;
        BBX *bbx = new BBX(dims);
        this->metaServerIDToBBX[partitionID] = bbx;

        if (dims >= 1)
        {
          Bound b;
          bbx->BoundList.push_back(b);
          this->metaServerIDToBBX[partitionID]->BoundList[0].m_lb = x * xSpan;
          this->metaServerIDToBBX[partitionID]->BoundList[0].m_ub = std::min((x + 1) * xSpan - 1, lenArray[0] - 1);

          if (dims >= 2)
          {
            Bound b;
            bbx->BoundList.push_back(b);
            this->metaServerIDToBBX[partitionID]->BoundList[1].m_lb = y * ySpan;
            this->metaServerIDToBBX[partitionID]->BoundList[1].m_ub = std::min((y + 1) * ySpan - 1, lenArray[1] - 1);

            if (dims >= 3)
            {
              Bound b;
              bbx->BoundList.push_back(b);
              this->metaServerIDToBBX[partitionID]->BoundList[2].m_lb = z * zSpan;
              this->metaServerIDToBBX[partitionID]->BoundList[2].m_ub = std::min((z + 1) * zSpan - 1, lenArray[2] - 1);
            }
          }
        }
      }
    }
  }

  return;
}

// get the corresponding metaserver according to the input bbox

std::vector<ResponsibleMetaServer> DHTManager::getMetaServerID(BBX& BBXQuery)
{
  // go through the metaServerIDToBBX
  // save the corresponding BBX
  std::vector<ResponsibleMetaServer> rmList;
  for (auto it = this->metaServerIDToBBX.begin();
       it != this->metaServerIDToBBX.end(); ++it)
  {
    // compare the BBXQuery with every item and store the overlapping part into list
    BBX *overlapBBX = getOverlapBBX(BBXQuery, *(it->second));
    if (overlapBBX != NULL)
    {
      ResponsibleMetaServer rbm(it->first, overlapBBX);
      rmList.push_back(rbm);
    }
  }
  return rmList;
}
