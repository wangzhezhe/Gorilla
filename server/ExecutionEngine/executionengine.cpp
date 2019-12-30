#include "executionengine.h"
#include <spdlog/spdlog.h>

void ExecutionEngineMeta::execute(std::string fiunctionName, const BlockSummary &bs, void *inputData)
{

    if (this->m_functionMap.find(fiunctionName) == this->m_functionMap.end())
    {
        spdlog::info("the function {} is not registered into the map", fiunctionName);
        return;
    }

    functionPointer fp = this->m_functionMap[fiunctionName];
    //put the checking service at the thread pool
    //only when the filter manager is loaded
    int threadid = this->m_threadPool->getEssId();
    tl::managed<tl::thread> th = this->m_threadPool->m_ess[threadid]->make_thread(
        [fp, bs, inputData] {
            fp(bs, inputData);
            return;
        });
    this->m_threadPool->m_userThreadList.push_back(std::move(th));
    return;
}

bool ExecutionEngineMeta::registerFunction(std::string functionName, functionPointer fp)
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

void ExecutionEngineRaw::execute(std::string fiunctionName, const BlockSummary &bs, void *inputData)
{

    if (this->m_functionMap.find(fiunctionName) == this->m_functionMap.end())
    {
        spdlog::info("the function {} is not registered into the map", fiunctionName);
        return;
    }

    functionPointer fp = this->m_functionMap[fiunctionName];
    fp(bs, inputData);
    return;
}

bool ExecutionEngineRaw::registerFunction(std::string functionName, functionPointer fp)
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