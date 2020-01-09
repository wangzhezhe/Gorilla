#include "functionManager.h"
#include <spdlog/spdlog.h>


void FunctionManagerRaw::execute(std::string fiunctionName, const BlockSummary &bs, void *inputData)
{

    if (this->m_functionMap.find(fiunctionName) == this->m_functionMap.end())
    {
        spdlog::info("the function {} is not registered into the map", fiunctionName);
        return;
    }

    rawdatafunctionPointer fp = this->m_functionMap[fiunctionName];
    fp(bs, inputData);
    return;
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