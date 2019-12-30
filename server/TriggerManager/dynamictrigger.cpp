
#include "dynamictrigger.h"
#include <spdlog/spdlog.h>

void DynamicTrigger::start(ExecutionEngineMeta *exengine)
{

    checkPtr cp = exengine->getCheckFunc(this->m_check.m_funcName);
    if (cp == NULL)
    {
        std::cout << "failed to get check function " << this->m_check.m_funcName << std::endl;
        throw std::runtime_error("failed to get check function");
    }

    comparisonPtr comp = exengine->getComparisonFunc(this->m_comparison.m_funcName);
    if (comp == NULL)
    {
        std::cout << "failed to get comparison function " << this->m_comparison.m_funcName << std::endl;
        throw std::runtime_error("failed to get check function");
    }

    actionPtr ap = exengine->getActionPtr(this->m_action.m_funcName);
    if (ap == NULL)
    {
        std::cout << "failed to get action function " << this->m_action.m_funcName << std::endl;
        throw std::runtime_error("failed to get check function");
    }

    //get data checking function pointer
    //execute checking

    std::string checkResults = cp(this->m_check.m_parameters);

    //get execute function pointer
    //execute operation

    bool indicator = comp(checkResults, this->m_comparison.m_parameters);

    //get action function pointer
    //action

    if (indicator)
    {

        //execute the action
        ap(this->m_action.m_parameters);
    }

    return;
}