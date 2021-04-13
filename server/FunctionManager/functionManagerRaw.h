#ifndef FUNCTIONRAW_H
#define FUNCTIONRAW_H

#include "../statefulConfig.h"
#include <commondata/metadata.h>
//#include "../unimosserver.hpp"

#include <blockManager/blockManager.h>
#include <client/ClientForStaging.hpp>
#include <map>
#include <thallium.hpp>
#include <vector>

namespace tl = thallium;

namespace GORILLA
{

struct FunctionManagerRaw;
// TODO use separate .h file???
typedef std::string (*rawdatafunctionPointer)(FunctionManagerRaw* fmw, const BlockSummary& bs,
  void* inputData, const std::vector<std::string>& parameters);

std::string test(FunctionManagerRaw* fmr, const BlockSummary& bs, void* inputData,
  const std::vector<std::string>& parameters);

std::string testvtk(FunctionManagerRaw* fmr, const BlockSummary& bs, void* inputData,
  const std::vector<std::string>& parameters);

std::string valueRange(FunctionManagerRaw* fmr, const BlockSummary& bs, void* inputData,
  const std::vector<std::string>& parameters);

struct FunctionManagerRaw
{
  FunctionManagerRaw()
  {
    // register the default function
    this->m_functionMap["test"] = &test;
    this->m_functionMap["testvtk"] = &testvtk;
    this->m_functionMap["valueRange"] = &valueRange;

    // the io need to be started
  };

  bool registerFunction(std::string functionName, rawdatafunctionPointer fp);

  // put the execution logic together with the storage part
  std::string execute(FunctionManagerRaw* fmr, const BlockSummary& bs, void* inputData,
    std::string fiunctionName, const std::vector<std::string>& parameters);

  std::string aggregateProcess(ClientForStaging* uniclient, std::string blockIDSuffix,
    std::string fiunctionName, const std::vector<std::string>& parameters);

  // TODO maybe try to use macro to register things here
  void testisoExec(std::string blockCompleteName, const std::vector<std::string>& parameters);

  void dummyAna(int step, int dataID, int totalStep, std::string anatype);

  ~FunctionManagerRaw() { std::cout << "destroy FunctionManagerRaw\n"; };

  statefulConfig* m_statefulConfig = NULL;

  // function map that maps the name of the function into the pointer of the function
  tl::mutex m_functionMapMutex;
  std::map<std::string, rawdatafunctionPointer> m_functionMap;

  // hold a pointer to the block manager
  BlockManager* m_blockManager = NULL;

  tl::engine* m_globalServerEnginePtr = nullptr;
};

}
#endif