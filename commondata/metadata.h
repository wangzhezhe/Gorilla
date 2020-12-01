
#ifndef DATAMETA_H
#define DATAMETA_H

#include <array>
#include <cstring>
#include <iostream>
#include <string>
#include <thallium/serialization/stl/array.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <tuple>
#include <typeinfo>
#include <vector>

// use namespace to avoid the conflicts for type defination
// comapred with system library
namespace GORILLA
{

// experimental
enum MetaStatus
{
  ERROR,
  BEFOREPROCESS,
  INPROCESS,
  AFTERPROCESS,
  UNDELETABLE,
};

enum BACKEND
{
  MEM,
  FILE,
  MEMVTKPTR,
};

// This backend is stored at the metadata server
// to index multiple blocks with the same type
// and they can be customized
// therefore we use string instead of enum

// cartisian grid
static std::string const DATATYPE_CARGRID = "CARGRID";
static std::string const DATATYPE_VTKPTR = "VTKPTR";

// how to put the data, maybe there is also the lcoal put
// for the local put, the data object and summary is managed by rawdataManager in local way
static std::string RDMAPUT = "RDMAPUT";
static std::string FILEPUT = "FILEPUT";

/*
struct FunctionInfo
{
  FunctionInfo(){};
  FunctionInfo(std::string functionName,
  std::vector<std::string> parameters):m_functionName(functionName),
  m_parameters(parameters){};

  std::string m_functionName;
  std::vector<std::string> m_parameters;

  ~FunctionInfo(){};



  template <typename A>
  void serialize(A &ar)
  {
    ar &m_functionName;
    ar &m_parameters;
  }
};
*/

struct FilterProfile
{
  FilterProfile(){};

  FilterProfile(std::string profileName, std::string stepFilterName, std::string blockIDFilterName,
    std::string contentFilterName, std::string subscriberAddr)
    : m_profileName(profileName)
    , m_stepFilterName(stepFilterName)
    , m_blockIDFilterName(blockIDFilterName)
    , m_contentFilterName(contentFilterName)
    , m_subscriberAddr(subscriberAddr){};

  std::string m_profileName = "default";
  std::string m_stepFilterName = "default";
  std::string m_blockIDFilterName = "default";
  std::string m_contentFilterName = "default";
  // this string will be populated by the server
  std::string m_subscriberAddr;

  ~FilterProfile(){};

  template <typename A>
  void serialize(A& ar)
  {
    ar& m_profileName;
    ar& m_stepFilterName;
    ar& m_blockIDFilterName;
    ar& m_contentFilterName;
    ar& m_subscriberAddr;
  }
};

// the meta data to index the raw data and block id
struct MetaAddrWrapper
{
  MetaAddrWrapper(){};
  MetaAddrWrapper(int index, std::string addr)
    : m_index(index)
    , m_addr(addr)
  {
  }
  int m_index;
  std::string m_addr;
  ~MetaAddrWrapper(){};

  template <typename A>
  void serialize(A& ar)
  {
    ar& m_index;
    ar& m_addr;
  }
};

const size_t STRLENLONG = 256;
const size_t STRLENSHORT = 64;

typedef char DATAYPE[STRLENSHORT];
typedef char BlOCKID[STRLENLONG];
typedef char EXTRAINFO[STRLENSHORT];

// the Block Summary for every data block, this info is stored at the raw data
// server
struct BlockSummary
{
  // for empty meta data, the initial value is 0
  // std::string m_typeName = "";

  size_t m_elemSize = 0;
  size_t m_elemNum = 0;

  // it is necessary to make the size of the blockSumamry fixed
  // since we may need to get data from the file, in that case
  // the metadata size should be a fixed size
  DATAYPE m_dataType;
  BlOCKID m_blockid;

  int m_backend = BACKEND::MEM;

  size_t m_dims = 3;

  // TODO, consider to add the real bbox, since the real data dimention might be
  // smaller than the index dimention the bounding box can be negative number,
  // so use the int here
  std::array<int, 3> m_indexlb{ { 0, 0, 0 } };
  // the origin can be caculated by offset
  std::array<int, 3> m_indexub{ { 0, 0, 0 } };
  // this is use to find the associated rdma endpoint at the server in order to
  // improve the performance
  int m_clientID;
  // some extra information need to be sent from the data writer
  EXTRAINFO m_extraInfo;

  BlockSummary()
  {
    // init the string member
    strcpy(m_dataType, "");
    strcpy(m_blockid, "");
    strcpy(m_extraInfo, "");
  };
  BlockSummary(size_t elemSize, size_t elemNum, std::string dataType, std::string blockid,
    size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub, std::string extraInfo = "")
    : m_elemSize(elemSize)
    , m_elemNum(elemNum)
    , m_dims(dims)
    , m_indexlb(indexlb)
    , m_indexub(indexub)
  {

    if (strlen(dataType.data()) >= STRLENSHORT)
    {
      throw std::runtime_error("length of data type should less than 256: " + dataType);
    }

    if (strlen(blockid.data()) >= STRLENLONG)
    {
      throw std::runtime_error("length of blockid should less than 512: " + blockid);
    }

    if (strlen(extraInfo.data()) >= STRLENSHORT)
    {
      throw std::runtime_error("length of extraInfo should less than 512: " + extraInfo);
    }

    strcpy(m_dataType, dataType.data());
    strcpy(m_blockid, blockid.data());
    strcpy(m_extraInfo, extraInfo.data());
  };

  std::array<size_t, 3> getShape()
  {
    std::array<size_t, 3> shape;
    for (int i = 0; i < 3; i++)
    {
      shape[i] = m_indexub[i] - m_indexlb[i] + 1;
    }
    return shape;
  };

  size_t getTotalSize()
  {
    if (strcmp(m_dataType, DATATYPE_CARGRID.data()) == 0 ||
      strcmp(m_dataType, DATATYPE_VTKPTR.data()) == 0)
    {
      return m_elemNum * m_elemSize;
    }
    else
    {
      throw std::runtime_error("unsuportted getsize for " + std::string(m_dataType));
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
    std::cout << "m_elemSize " << m_elemSize << " m_elemNum " << m_elemNum << " m_dataType "
              << m_dataType << " m_blockid " << m_blockid << " m_dims " << m_dims << ", m_indexlb "
              << m_indexlb[0] << " " << m_indexlb[1] << " " << m_indexlb[2] << ", m_indexub "
              << m_indexub[0] << " " << m_indexub[1] << " " << m_indexub[2]
              << "extra info: " << m_extraInfo << std::endl;

    return;
  }

  bool equals(BlockSummary& other)
  {
    if (this->m_elemSize != other.m_elemSize)
    {
      return false;
    }
    if (this->m_elemNum != other.m_elemNum)
    {
      return false;
    }
    if (strcmp(this->m_dataType, other.m_dataType) != 0)
    {
      return false;
    }
    if (strcmp(this->m_blockid, other.m_blockid) != 0)
    {
      return false;
    }
    if (this->m_dims != other.m_dims)
    {
      return false;
    }
    if (this->m_indexlb[0] != other.m_indexlb[0] || this->m_indexlb[1] != other.m_indexlb[1] ||
      this->m_indexlb[2] != other.m_indexlb[2])
    {
      return false;
    }
    if (this->m_indexub[0] != other.m_indexub[0] || this->m_indexub[1] != other.m_indexub[1] ||
      this->m_indexub[2] != other.m_indexub[2])
    {
      return false;
    }
    if (this->m_clientID != other.m_clientID)
    {
      return false;
    }

    return true;
  }

  ~BlockSummary(){};

  template <typename A>
  void serialize(A& ar)
  {
    ar& m_elemSize;
    ar& m_elemNum;
    ar& m_dataType;
    ar& m_blockid;
    ar& m_backend;
    ar& m_dims;
    ar& m_indexlb;
    ar& m_indexub;
    ar& m_extraInfo;
  }
};

// the block descriptor is stored at the index service
// this is used to find the block data (meta and actual data)
// for client get operation
struct BlockDescriptor
{
  BlockDescriptor(){};
  // the raw data end point info is part of the Block Summary
  BlockDescriptor(std::string rawDataServerAddr, std::string rawDataID, std::string dataType,
    size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub)
    : m_rawDataServerAddr(rawDataServerAddr)
    , m_rawDataID(rawDataID)
    , m_dataType(dataType)
    , m_dims(dims)
    , m_indexlb(indexlb)
    , m_indexub(indexub){};
  // the data is stored at which server
  // it can be null if it is stored by file
  std::string m_rawDataServerAddr;
  // the id of the raw data
  std::string m_rawDataID;
  // experiment, label the lifecycle of the data
  int m_metaStatus = MetaStatus::BEFOREPROCESS;
  std::string m_dataType = DATATYPE_CARGRID;
  int m_backend = BACKEND::MEM;

  size_t m_dims = 0;
  std::array<int, 3> m_indexlb{ { 0, 0, 0 } };
  std::array<int, 3> m_indexub{ { 0, 0, 0 } };

  void printInfo()
  {
    std::cout << "server addr " << m_rawDataServerAddr << " dataID " << m_rawDataID
              << " m_metaStatus " << m_metaStatus << " m_dataType " << m_dataType << " m_backend "
              << m_backend << " dims " << m_dims << " lb " << m_indexlb[0] << "," << m_indexlb[1]
              << "," << m_indexlb[2] << " ub " << m_indexub[0] << "," << m_indexub[1] << ","
              << m_indexub[2] << std::endl;
  }

  ~BlockDescriptor(){};

  template <typename A>
  void serialize(A& ar)
  {
    ar& m_rawDataServerAddr;
    ar& m_rawDataID;
    ar& m_metaStatus;
    ar& m_dataType;
    ar& m_backend;
    ar& m_dims;
    ar& m_indexlb;
    ar& m_indexub;
  }
};

struct InfoForPut
{
  InfoForPut(){};
  InfoForPut(std::string putMethod, std::vector<std::string> metaServerList)
    : m_putMethod(putMethod)
    , m_metaServerList(metaServerList){};
  std::string m_putMethod = "";
  std::vector<std::string> m_metaServerList;

  ~InfoForPut(){};

  template <typename A>
  void serialize(A& ar)
  {
    ar& m_putMethod;
    ar& m_metaServerList;
  }
};

/*deprecated
struct MetaDataWrapper {
  MetaDataWrapper(){};
  MetaDataWrapper(std::string destAddr, size_t step, std::string varName,
                  BlockDescriptor rde)
      : m_destAddr(destAddr), m_step(step), m_varName(varName), m_rde(rde){};
  std::string m_destAddr = "";
  size_t m_step = 0;
  std::string m_varName = "";
  BlockDescriptor m_rde;

  ~MetaDataWrapper(){};

  void printInfo() {
    std::cout << "m_destAddr " << m_destAddr << " m_step " << m_step
              << " m_varName " << m_varName << std::endl;
    m_rde.printInfo();
  }

  template <typename A>
  void serialize(A &ar) {
    ar &m_destAddr;
    ar &m_step;
    ar &m_varName;
    ar &m_rde;
  }
};
*/

struct DynamicTriggerInfo
{
  DynamicTriggerInfo(){};

  DynamicTriggerInfo(std::string checkFunc, std::vector<std::string> checkFuncPara,
    std::string comparisonFunc, std::vector<std::string> comparisonFuncPara, std::string actionFunc,
    std::vector<std::string> actionFuncPara)
  {
    m_checkFunc = checkFunc;
    m_checkFuncPara = checkFuncPara;
    m_comparisonFunc = comparisonFunc;
    m_comparisonFuncPara = comparisonFuncPara;

    m_actionFunc = actionFunc;
    m_actionFuncPara = actionFuncPara;
  }

  void printInfo()
  {
    std::cout << "checkFunc " << m_checkFunc << std::endl;
    for (int i = 0; i < m_checkFuncPara.size(); i++)
    {
      std::cout << "parameter " << i << " " << m_checkFuncPara[i] << std::endl;
    }

    std::cout << "comparisonFunc " << m_comparisonFunc << std::endl;
    for (int i = 0; i < m_comparisonFuncPara.size(); i++)
    {
      std::cout << "parameter " << i << " " << m_comparisonFuncPara[i] << std::endl;
    }

    std::cout << "actionFunc " << m_actionFunc << std::endl;
    for (int i = 0; i < m_actionFuncPara.size(); i++)
    {
      std::cout << "parameter " << i << " " << m_actionFuncPara[i] << std::endl;
    }

    std::cout << "group master is " << m_masterAddr << std::endl;
  }

  std::string m_checkFunc = "default";
  std::vector<std::string> m_checkFuncPara;
  std::string m_comparisonFunc = "default";
  std::vector<std::string> m_comparisonFuncPara;
  std::string m_actionFunc = "default";
  std::vector<std::string> m_actionFuncPara;
  std::string m_masterAddr = "";

  template <typename A>
  void serialize(A& ar)
  {
    ar& m_checkFunc;
    ar& m_checkFuncPara;
    ar& m_comparisonFunc;
    ar& m_comparisonFuncPara;
    ar& m_actionFunc;
    ar& m_actionFuncPara;
    ar& m_masterAddr;
  }
};

const std::string EVENT_DATA_PUT = "DATA_PUT";
const std::string EVENT_FINISH = "FINISH_PUT";

// TODO add event type, such as data_put, data_put_finish, etc
struct EventWrapper
{
  EventWrapper(){};
  EventWrapper(std::string type, std::string varName, size_t step, int dims,
    std::array<int, 3> indexlb, std::array<int, 3> indexub)
    : m_type(type)
    , m_varName(varName)
    , m_step(step)
    , m_dims(dims)
    , m_indexlb(indexlb)
    , m_indexub(indexub){};

  std::string m_type = EVENT_DATA_PUT;
  std::string m_varName = "NULL";

  size_t m_step = 0;
  int m_dims = 0;
  std::array<int, 3> m_indexlb{ { 0, 0, 0 } };
  std::array<int, 3> m_indexub{ { 0, 0, 0 } };

  ~EventWrapper(){};

  void printInfo()
  {
    std::cout << "m_varName " << m_varName << " m_step " << m_step << " m_type " << m_type
              << " m_dims " << m_dims << ", m_indexlb " << m_indexlb[0] << " " << m_indexlb[1]
              << " " << m_indexlb[2] << ", m_indexub " << m_indexub[0] << " " << m_indexub[1] << " "
              << m_indexub[2] << std::endl;
  }

  template <typename A>
  void serialize(A& ar)
  {
    ar& m_type;
    ar& m_varName;
    ar& m_step;
    ar& m_dims;
    ar& m_indexlb;
    ar& m_indexub;
  }
};

}
#endif
