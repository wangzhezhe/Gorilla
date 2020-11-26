#include "fileobj.h"
#include <fstream>
#include <sys/stat.h>
#include <utils/matrixtool.h>

// TODO, the block summary retured here is the one that is extracted from the file
BlockSummary FileObj::getData(void*& dataContainer)
{
  // the block id is supposed to be set by the get handle operation
  if (strcmp(this->m_blockid, "") == 0)
  {
    throw std::runtime_error("the block id is empty when get data");
  }
  // read from the file
  std::string fileName = FILEOBJ_PREXIX + "/" + std::string(this->m_blockid);

  // set the property when declare the stream
  std::ifstream rfile(fileName.c_str(), std::ios::in | std::ios::binary);

  if (!rfile.good())
  {
    throw std::runtime_error("failed to open the file, file not exist");
    // if file not exist
    return BlockSummary();
  }

  rfile.seekg(0, rfile.end);
  size_t fileSize = rfile.tellg();

  rfile.seekg(0);
  // extract block summary from the file
  size_t bsummarySize = sizeof(BlockSummary);

  // need to allocate memory instead of using exist structure
  rfile.read((char*)&(this->m_blockSummary), bsummarySize);

  size_t blockDataSize = this->m_blockSummary.m_elemSize * this->m_blockSummary.m_elemNum;

  if ((bsummarySize + blockDataSize) != fileSize)
  {
    throw std::runtime_error("size in block and summary size not match with actual file size");
  }

  rfile.seekg(bsummarySize);

  // if the dataContainer is null
  // we do not know the size of the actual data previously in this case
  // assign memory space here

  if (dataContainer == nullptr)
  {
    dataContainer = (char*)malloc(blockDataSize);
  }

  rfile.read((char*)dataContainer, blockDataSize);

  rfile.close();
  // return current block summary
  return this->m_blockSummary;
}

// check the block summay to make sure it is not empty
// the blockid should not be empty
// attention, the size should be both the block summary plus the actual data size
int FileObj::putData(void* dataSourcePtr)
{
  // write the block summary
  if (this->m_blockSummary.m_blockid == "")
  {
    throw std::runtime_error("failed to put data into file, the blockid should not be empty");
  }
  size_t dataSize = this->m_blockSummary.m_elemSize * this->m_blockSummary.m_elemNum;
  std::string blockid = this->m_blockSummary.m_blockid;

  // write from the file
  std::string fileName = FILEOBJ_PREXIX + "/" + blockid;

  // if file exist
  std::ifstream f(fileName.c_str());
  if (f.good())
  {
    throw std::runtime_error("file exist");
    return -1;
  }
  f.close();

  std::ofstream wfile(fileName.c_str(), std::ios::out | std::ios::binary);
  if (!wfile.good())
  {
    throw std::runtime_error("not good to write into the file");
    return -1;
  }

  size_t blockSummarySize = sizeof(BlockSummary);
  wfile.seekp(0);
  // write the block summary from the 0 to th summarySize-1
  wfile.write((char*)(&(this->m_blockSummary)), blockSummarySize);

  wfile.seekp(blockSummarySize);
  // write actual data from the summarySize to summarySize+dataSize-1
  wfile.write((char*)dataSourcePtr, dataSize);

  wfile.close();
  return 0;
}

int FileObj::eraseData()
{
  // the block id is supposed to be set by the get handle operation
  if (strcmp(this->m_blockid, "") == 0)
  {
    throw std::runtime_error("the block id is empty when eraseData data");
  }
  // erase the file on disk
  std::string blockid = this->m_blockid;
  // write from the file
  std::string fileName = FILEOBJ_PREXIX + "/" + blockid;
  std::ifstream rfile(fileName.c_str());
  if (!rfile.good())
  {
    // if file not exist
    std::cerr << "file not exist " << fileName << std::endl;
    return 0;
  }

  int status = std::remove(fileName.data());
  if (status != 0)
  {
    throw std::runtime_error("failed to remove file " + fileName);
  }
  return status;
}

// the caller should delete the mem data when finish processing such as data transfering
// TODO, extract the block summary from the file here!!! current code is incorrect
// when caculate the file size, need to add the size of the block summary
// current code is incorrect!!!
BlockSummary FileObj::getDataSubregion(
  size_t dims, std::array<int, 3> subregionlb, std::array<int, 3> subregionub, void*& dataContainer)
{

  // the block id is supposed to be set by the get handle operation
  if (strcmp(this->m_blockid, "") == 0)
  {
    throw std::runtime_error("the block id is empty when getDataSubregion data");
  }

  // get the block summary
  // read from the file
  std::string fileName = FILEOBJ_PREXIX + "/" + std::string(this->m_blockid);

  // set the property when declare the stream
  std::ifstream rfile(fileName.c_str(), std::ios::in | std::ios::binary);

  if (!rfile.good())
  {
    throw std::runtime_error("failed to open the file, file not exist");
    // if file not exist
    return BlockSummary();
  }

  rfile.seekg(0, rfile.end);
  size_t fileSize = rfile.tellg();
  rfile.seekg(0);

  size_t bsummarySize = sizeof(BlockSummary);

  // extract block summary from the file
  rfile.read((char*)(&(this->m_blockSummary)), bsummarySize);

  if (strcmp(this->m_blockSummary.m_blockid, this->m_blockid) != 0)
  {
    throw std::runtime_error("the block id extracted from file not equal with the actual one");
  }

  if (dims != this->m_blockSummary.m_dims)
  {
    throw std::runtime_error("the dims is not equal to the value in blockSummary for fileobj");
  }

  // if the query match with the bbx in storage, retuen how data direactly
  bool ifAllMatch = true;
  for (int i = 0; i < dims; i++)
  {
    if (subregionlb[i] != this->m_blockSummary.m_indexlb[i] ||
      subregionub[i] != this->m_blockSummary.m_indexub[i])
    {
      ifAllMatch = false;
      break;
    }
  }

  // assign mem space and get data
  size_t memSpace = this->m_blockSummary.m_elemNum * this->m_blockSummary.m_elemSize;
  void* tempData = (void*)malloc(memSpace);

  rfile.seekg(bsummarySize);
  rfile.read((char*)tempData, memSpace);

  rfile.close();

  if (ifAllMatch)
  {
    // this data will be deleted when caller finish the transfer operation
    dataContainer = tempData;
    return this->m_blockSummary;
  }

  // if the size not match with each other
  // reorganize the data
  size_t elemSize = (size_t)this->m_blockSummary.m_elemSize;
  // decrease the offset
  std::array<int, 3> offset = this->m_blockSummary.m_indexlb;
  std::array<size_t, 3> subregionLbNonoffset;
  std::array<size_t, 3> subregionUbNonoffset;
  std::array<size_t, 3> globalUbNonoffset;

  // the value whithout offset should larger or equal to 0
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
    std::cout << "m_blockSummary.m_elemNum " << m_blockSummary.m_elemNum << " currentElemNum "
              << currentElemNum << std::endl;
    throw std::runtime_error(
      "failed to getDataSubregion, current elem number is larger than the value in Blocksummary");
  }

  void* result = MATRIXTOOL::getSubMatrix(
    elemSize, subregionLbNonoffset, subregionUbNonoffset, globalUbNonoffset, tempData);

  // the temp data is useless now, delete it
  free(tempData);

  if (result == NULL)
  {
    throw std::runtime_error("failed to getDataSubregion");
  }

  dataContainer = (void*)(result);
  BlockSummary bs = this->m_blockSummary;

  // update the information in the subregion
  // use the new one
  bs.m_indexlb = subregionlb;
  bs.m_indexub = subregionub;

  return bs;
}