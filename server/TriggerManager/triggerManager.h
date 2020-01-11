
#ifndef TRIGGERMANAGER_H
#define TRIGGERMANAGER_H

// store the registered triggers

#include <map>
#include <set>
#include <string>
#include <thallium.hpp>
#include <vector>
#include "../../commondata/metadata.h"
#include "../../utils/ArgothreadPool.h"


namespace tl = thallium;

struct FunctionManagerMeta;

struct DynamicTriggerManager
{
    DynamicTriggerManager(FunctionManagerMeta* funcmanagerMeta, size_t poolSize) : m_funcmanagerMeta(funcmanagerMeta)
    {
        this->m_threadPool = new ArgoThreadPool(poolSize);
    };

    void updateTrigger(std::string triggerName, DynamicTriggerInfo triggerInfo){
        m_dynamicTrigger[triggerName] = triggerInfo;
    };

    void initstart(std::string triggerName, size_t step, std::string varName, RawDataEndpoint &rde);

    void commonstart(std::string triggerName, size_t step, std::string varName, RawDataEndpoint &rde);


    //void removeTrigger(std::string triggerName);

    //put the trigger into the manager
    //from the trigger name to the trigger instance
    std::map<std::string, DynamicTriggerInfo> m_dynamicTrigger;

    //the place that store the function
    FunctionManagerMeta* m_funcmanagerMeta = NULL;

    ArgoThreadPool* m_threadPool = NULL;

    ~DynamicTriggerManager()
    {
        if (this->m_threadPool != NULL)
        {
            delete this->m_threadPool;
        }
    };
};



#endif