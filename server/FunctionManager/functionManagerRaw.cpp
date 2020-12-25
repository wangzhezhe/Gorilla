#include "functionManagerRaw.h"
#include <spdlog/spdlog.h>

namespace GORILLA
{
// if the execute aims to generate the new version of the data
// the return value is the id of original function,
// then at the trigger, replace the original id and use the same index (the data is on the same
// node) we do not need to update the metadata when there is multiple version of the dataduring the
// workflow execution

std::string FunctionManagerRaw::execute(FunctionManagerRaw* fmr, const BlockSummary& bs,
  void* inputData, std::string fiunctionName, const std::vector<std::string>& parameters)
{

  if (this->m_functionMap.find(fiunctionName) == this->m_functionMap.end())
  {
    spdlog::info("the function {} is not registered into the map", fiunctionName);
    return "";
  }
  // since it calles functionPointer
  // this functionPointer is beyond the scope of the class
  // it is better to make it as a static function???
  rawdatafunctionPointer fp = this->m_functionMap[fiunctionName];
  std::string results = fp(this, bs, inputData, parameters);
  return results;
}

bool FunctionManagerRaw::registerFunction(std::string functionName, rawdatafunctionPointer fp)
{

  m_functionMapMutex.lock();
  if (this->m_functionMap.find(functionName) != this->m_functionMap.end())
  {
    return false;
  }

  m_functionMap[functionName] = fp;
  m_functionMapMutex.unlock();

  return true;
}

std::string FunctionManagerRaw::aggregateProcess(UniClient* uniclient, std::string blockIDSuffix,
  std::string fiunctionName, const std::vector<std::string>& parameters)
{
  // aggregate poly from different processes
  // blocksuppary contains the var and step info
  std::cout << "debug start to process with suffix " << blockIDSuffix << std::endl;

  // get the block BlockSummary list from all stage servers with current blockIDSuffix
  std::vector<BlockSummary> bslist = uniclient->getBlockSummaryListBySuffix(blockIDSuffix);

  // get the data back based on the size and the name

  // assemble them into one poly data

  // process the poly data

  // if the last step, call the timer
  if (blockIDSuffix.find("20") != std::string::npos)
  {
    // end the timer, for the master node
    uniclient->endTimer();
  }

  return "";
}

}