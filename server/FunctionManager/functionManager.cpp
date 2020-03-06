#include "functionManager.h"
#include <spdlog/spdlog.h>



std::string FunctionManagerRaw::execute(
FunctionManagerRaw* fmr,
const BlockSummary &bs, 
void *inputData,
std::string fiunctionName,
const std::vector<std::string>& parameters)
{

    if (this->m_functionMap.find(fiunctionName) == this->m_functionMap.end())
    {
        spdlog::info("the function {} is not registered into the map", fiunctionName);
        return "";
    }
    //since it calles functionPointer
    //this functionPointer is beyond the scope of the class
    //it is better to make it as a static function???
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