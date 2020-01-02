
#ifndef DATAMETA_H
#define DATAMETA_H

#include <array>
#include <iostream>
#include <string>
#include <thallium/serialization/stl/array.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <tuple>
#include <typeinfo>
//Thaliium may not good at transfer enum type
//enum DRIVERTYPE { RAWMEM, VTK };

static std::string const DRIVERTYPE_RAWMEM = "RAWMEM";

struct FilterProfile
{

  FilterProfile(){};

  FilterProfile(std::string profileName, std::string stepFilterName,
                std::string blockIDFilterName, std::string contentFilterName,
                std::string subscriberAddr)
      : m_profileName(profileName), m_stepFilterName(stepFilterName),
        m_blockIDFilterName(blockIDFilterName),
        m_contentFilterName(contentFilterName),
        m_subscriberAddr(subscriberAddr){};

  std::string m_profileName = "default";
  std::string m_stepFilterName = "default";
  std::string m_blockIDFilterName = "default";
  std::string m_contentFilterName = "default";
  // this string will be populated by the server
  std::string m_subscriberAddr;

  ~FilterProfile(){};

  template <typename A>
  void serialize(A &ar)
  {
    ar &m_profileName;
    ar &m_stepFilterName;
    ar &m_blockIDFilterName;
    ar &m_contentFilterName;
    ar &m_subscriberAddr;
  }
};

// the meta data to index the raw data and block id
struct MetaAddrWrapper
{
  MetaAddrWrapper(){};
  MetaAddrWrapper(int index, std::string addr) : m_index(index), m_addr(addr) {}
  int m_index;
  std::string m_addr;
  ~MetaAddrWrapper(){};

  template <typename A>
  void serialize(A &ar)
  {
    ar &m_index;
    ar &m_addr;
  }
};

// the Block Summary for every data block, this info is stored at the raw data
// server
struct BlockSummary
{
  // for empty meta data, the initial value is 0
  //std::string m_typeName = "";

  size_t m_elemSize = 0;
  size_t m_elemNum = 0;
  std::string m_drivertype = DRIVERTYPE_RAWMEM;

  size_t m_dims = 3;

  //TODO, consider to add the real bbox, since the real data dimention might be smaller than the index dimention
  //the bounding box can be negative number, so use the int here
  std::array<int, 3> m_indexlb{{0, 0, 0}};
  //the origin can be caculated by offset
  std::array<int, 3> m_indexub{{0, 0, 0}};

  BlockSummary(){};
  BlockSummary(size_t elemSize, size_t elemNum,
               std::string driverType,
               size_t dims,
               std::array<int, 3> indexlb,
               std::array<int, 3> indexub)
      : m_elemSize(elemSize), m_elemNum(elemNum),
        m_drivertype(driverType), m_dims(dims), m_indexlb(indexlb), m_indexub(indexub){};

  std::array<size_t, 3> getShape()
  {
    std::array<size_t, 3> shape;
    for (int i = 0; i < 3; i++)
    {
      shape[i] = m_indexub[i] - m_indexlb[i] + 1;
    }
  };

  size_t getTotalSize(){
      if(m_drivertype.compare(DRIVERTYPE_RAWMEM)==0) {
        return m_elemNum*m_elemSize;
      }else{
        throw std::runtime_error("unsuportted getsize for " + m_drivertype);
      }
  }

  /*
  size_t getValidDimention() {
    size_t d = 0;
    for (int i = 0; i < 3; i++) {
      if (m_shape[i] != 0) {
        d++;
      }
    }
    return d;
  }

  size_t getBlockMallocSize() {
    if (m_shape[0] == 0 && m_shape[1] == 0 && m_shape[2] == 0) {
      return 0;
    }
    if (m_elemSize == 0) {
      std::cout << "failed to get malloc size, elem size is 0" << std::endl;
    }

    size_t elemNum = 1;
    for (int i = 0; i < 3; i++) {
      if (m_shape[i] != 0) {
        elemNum = elemNum * m_shape[i];
      }
    }

    return m_elemSize * elemNum;
  }
  */

  void printSummary()
  {
    std::cout << "m_elemSize " << m_elemSize
              << " m_elemNum " << m_elemNum << " m_drivertype " << m_drivertype << " m_dims " << m_dims
              << ", m_indexlb " << m_indexlb[0] << " " << m_indexlb[1] << " "
              << m_indexlb[2] << ", m_indexub " << m_indexub[0] << " "
              << m_indexub[1] << " " << m_indexub[2] << std::endl;
    return;
  }

  ~BlockSummary(){};

  template <typename A>
  void serialize(A &ar)
  {
    ar &m_elemSize;
    ar &m_elemNum;
    ar &m_drivertype;
    ar &m_dims;
    ar &m_indexlb;
    ar &m_indexub;
  }
};


struct RawDataEndpoint {
  RawDataEndpoint(){};
  //the raw data end point info is part of the Block Summary
  RawDataEndpoint(std::string rawDataServerAddr, std::string rawDataID,
                  size_t dims, std::array<int, 3> indexlb,
                  std::array<int, 3> indexub)
      : m_rawDataServerAddr(rawDataServerAddr), m_rawDataID(rawDataID),
        m_dims(dims), m_indexlb(indexlb), m_indexub(indexub){};
  std::string m_rawDataServerAddr;
  std::string m_rawDataID;
  size_t m_dims = 0;
  std::array<int, 3> m_indexlb{{0, 0, 0}};
  std::array<int, 3> m_indexub{{0, 0, 0}};

  void printInfo() {
    std::cout << "server addr " << m_rawDataServerAddr << " dataID "
              << m_rawDataID << "dims " << m_dims << " lb " << m_indexlb[0] << ","
              << m_indexlb[1] << "," << m_indexlb[2] << " ub " << m_indexub[0]
              << "," << m_indexub[1] << "," << m_indexub[2] << std::endl;
  }

  ~RawDataEndpoint(){};

  template <typename A>
  void serialize(A &ar)
  {
    ar &m_rawDataServerAddr;
    ar &m_rawDataID;
    ar &m_dims;
    ar &m_indexlb;
    ar &m_indexub;
  }

};



/*
// TODO change name to the metadata
// the meta is for all the blocks for specific timestep
// this info is use for put the data, the server only store the BlockMeta for
// every data block return the BlockMeta for dsget interface Although the
// blockMeta is part of the DataMeta split them for easy serialize / deserialize
struct DataMeta {

  // varname and ts is suitable for all the block data
  std::string m_varName = "";
  size_t m_steps = 0;
  size_t m_elemSize = 0;
  // this can be detected by testing m_shape
  // size_t m_dimension;
  std::string m_typeName = "";
  std::array<size_t, 3> m_shape = {{0, 0, 0}};
  // the offset can be the origin of the new grid
  std::array<size_t, 3> m_offset = {{0, 0, 0}};
  DataMeta(){};
  DataMeta(std::string varName, size_t steps, std::string typeName,
           size_t elemSize, std::array<size_t, 3> shape,
           std::array<size_t, 3> offset = {{0, 0, 0}})
      : m_varName(varName), m_steps(steps), m_typeName(typeName),
        m_elemSize(elemSize), m_shape(shape), m_offset(offset){};

  void printMeta() {
    std::cout << "varName " << m_varName << ", steps " << m_steps
              << ", typeName " << m_typeName << ", elemSize " << m_elemSize
              << ", shape " << m_shape[0] << " " << m_shape[1] << " "
              << m_shape[2] << ", offset " << m_offset[0] << " " << m_offset[1]
              << " " << m_offset[2] << std::endl;
    return;
  }

  BlockMeta extractBlockMeta() {
    return BlockMeta(this->m_typeName, this->m_elemSize, this->m_shape,
                     this->m_offset);
  }

  template <typename A> void serialize(A &ar) {
    ar &m_varName;
    ar &m_steps;
    ar &m_elemSize;
    ar &m_typeName;
    ar &m_shape;
    ar &m_offset;
  }

  ~DataMeta(){};
};
*/

#endif