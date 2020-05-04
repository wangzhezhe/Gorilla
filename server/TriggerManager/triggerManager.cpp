#include "triggerManager.h"

void DynamicTriggerManager::initstart(std::string triggerName, size_t step, std::string varName, RawDataEndpoint rde)
{
    if (this->m_dynamicTrigger.find(triggerName) == this->m_dynamicTrigger.end())
    {
        throw std::runtime_error("failed to get the trigger with name " + triggerName);
    }

    DynamicTriggerInfo *dti = &(this->m_dynamicTrigger[triggerName]);

    initCheckPtr initCp = this->m_funcmanagerMeta->getInitCheckFunc(dti->m_checkFunc);
    if (initCp == NULL)
    {
        std::cout << "failed to get check function " << dti->m_checkFunc << std::endl;
        throw std::runtime_error("failed to get check function");
    }

    comparisonPtr comp = this->m_funcmanagerMeta->getComparisonFunc(dti->m_comparisonFunc);
    if (comp == NULL)
    {
        std::cout << "failed to get comparison function " << dti->m_comparisonFunc << std::endl;
        throw std::runtime_error("failed to get comparison function");
    }

    initActionPtr initAp = this->m_funcmanagerMeta->getInitActionPtr(dti->m_actionFunc);
    if (initAp == NULL)
    {
        std::cout << "failed to get action function " << dti->m_actionFunc << std::endl;
        throw std::runtime_error("failed to get action function");
    }

    //get data checking function pointer
    //execute checking
    //size_t step, std::string varName, RawDataEndpoint& rde

    std::string checkResults = initCp(step, varName, rde);

    //it doesn't matter if the thread pool should be used here
    //since the checking at the metadata part is always lightweight
    bool indicator = comp(checkResults, dti->m_comparisonFuncPara);
    //get action function pointer
    //action
    if (indicator)
    {
        //execute the action
        //ap(this->m_dtm, step, varName, rde, this->m_fdAction.m_parameters);
        initAp(this->m_funcmanagerMeta, step, varName, rde, dti->m_actionFuncPara);
    }
    return;
}

//common start
void DynamicTriggerManager::commonstart(std::string triggerName, size_t step, std::string varName, RawDataEndpoint rde)
{
    if (this->m_dynamicTrigger.find(triggerName) == this->m_dynamicTrigger.end())
    {
        throw std::runtime_error("failed to get the trigger with name " + triggerName);
    }

    DynamicTriggerInfo *dti = &(this->m_dynamicTrigger[triggerName]);

    checkPtr cp = this->m_funcmanagerMeta->getCheckFunc(dti->m_checkFunc);
    if (cp == NULL)
    {
        std::cout << "failed to get check function " << dti->m_checkFunc << std::endl;
        throw std::runtime_error("failed to get check function");
    }

    comparisonPtr comp = this->m_funcmanagerMeta->getComparisonFunc(dti->m_comparisonFunc);
    if (comp == NULL)
    {
        std::cout << "failed to get comparison function " << dti->m_comparisonFunc << std::endl;
        throw std::runtime_error("failed to get comparison function");
    }

    actionPtr ap = this->m_funcmanagerMeta->getActionPtr(dti->m_actionFunc);
    if (ap == NULL)
    {
        std::cout << "failed to get action function " << dti->m_actionFunc << std::endl;
        throw std::runtime_error("failed to get action function");
    }

    //get data checking function pointer
    //execute checking
    //size_t step, std::string varName, RawDataEndpoint& rde

    std::string checkResults = cp(step, varName, rde, dti->m_checkFuncPara);

    //it doesn't matter if the thread pool should be used here
    //since the checking at the metadata part is always lightweight
    bool indicator = comp(checkResults, dti->m_comparisonFuncPara);
    //get action function pointer
    //action
    if (indicator)
    {
        //execute the action
        //ap(this->m_dtm, step, varName, rde, this->m_fdAction.m_parameters);
        std::string triggerMaster = dti->m_masterAddr;
        ap(this->m_funcmanagerMeta, step, varName, triggerMaster, this->m_uniclient, rde, dti->m_actionFuncPara);
    }
    return;
}

void DynamicTriggerManager::putEvent(std::string varName, EventWrapper &event)
{
    //if not exist
    this->m_eventMapQueueMutex.lock();
    this->eventMapQueue[varName].push(event);
    std::cout << "put event " <<std::endl;
    event.printInfo();
    this->m_eventMapQueueMutex.unlock();
    return;
}

EventWrapper DynamicTriggerManager::getEvent(std::string varName)
{
    EventWrapper event;
    //if var name not exist
    if (this->eventMapQueue.count(varName) == 0)
    {
        return event;
    }


    if (this->eventMapQueue[varName].size() == 0)
    {
        return event;
    }
    
    //if var name exist
    this->m_eventMapQueueMutex.lock();
    event = this->eventMapQueue[varName].front();
    this->eventMapQueue[varName].pop();
    this->m_eventMapQueueMutex.unlock();
    return event;

}