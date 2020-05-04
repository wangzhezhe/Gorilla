#ifndef FUNCTIONMETA_H
#define FUNCTIONMETA_H

#include "../../commondata/metadata.h"

#include "../statefulConfig.h"
//#include "../unimosserver.hpp"
#include "./defaultFunctions/defaultfuncmeta.h"
#include <thallium.hpp>
#include <map>
#include <vector>

namespace tl = thallium;

struct FunctionManagerMeta;
struct DynamicTriggerManager;
//the function pointer that execute at the metadata server

typedef std::string (*initCheckPtr)(
    size_t step,
    std::string varName,
    RawDataEndpoint &rde);

typedef std::string (*checkPtr)(
    size_t step,
    std::string varName,
    RawDataEndpoint &rde,
    std::vector<std::string> parameters);

typedef bool (*comparisonPtr)(std::string checkResults, std::vector<std::string> parameters);

typedef void (*initActionPtr)(
    FunctionManagerMeta *fmm,
    size_t step,
    std::string varName,
    RawDataEndpoint &rde,
    std::vector<std::string> parameters);

typedef void (*actionPtr)(
    FunctionManagerMeta *fmm,
    size_t step,
    std::string varName,
    std::string triggerMaster,
    UniClient *uniclient,
    RawDataEndpoint &rde,
    std::vector<std::string> parameters);


//this should hold a pointer to the dynamic trigger
struct FunctionManagerMeta
{
    FunctionManagerMeta()
    {
        registerFunc();
    };

    void registerFunc()
    {

        registerInitCheckFunc("defaultCheckGetStep", &defaultCheckGetStep);
        registerComparisonFunc("defaultComparisonStep", &defaultComparisonStep);
        registerInitActionFunc("defaultActionSartDt", &defaultActionSartDt);

        registerCheckFunc("defaultCheck", &defaultCheck);
        registerComparisonFunc("defaultComparison", &defaultComparison);
        registerActionFunc("defaultAction", &defaultAction);

        registerCheckFunc("InsituExpCheck", &InsituExpCheck);
        registerComparisonFunc("InsituExpCompare", &InsituExpCompare);
        registerActionFunc("InsituExpAction", &InsituExpAction);

        registerActionFunc("defaultNotifyAction", &defaultNotifyAction);
        registerActionFunc("defaultPutEvent", &defaultPutEvent);


        return;
    }

    void registerInitCheckFunc(std::string str, initCheckPtr checkfunc)
    {
        m_metafunctionMapMutex.lock();
        m_initCheckPtrMap[str] = checkfunc;
        m_metafunctionMapMutex.unlock();
    }

    void registerCheckFunc(std::string str, checkPtr checkfunc)
    {
        m_metafunctionMapMutex.lock();
        m_checkPtrMap[str] = checkfunc;
        m_metafunctionMapMutex.unlock();
    };

    void registerComparisonFunc(std::string str, comparisonPtr comparisonfunc)
    {
        m_metafunctionMapMutex.lock();
        m_comparisonPtrMap[str] = comparisonfunc;
        m_metafunctionMapMutex.unlock();
    };

    void registerInitActionFunc(std::string str, initActionPtr actionfunc)
    {
        m_metafunctionMapMutex.lock();
        m_initActionPtrMap[str] = actionfunc;
        m_metafunctionMapMutex.unlock();
    };

    void registerActionFunc(std::string str, actionPtr actionfunc)
    {
        m_metafunctionMapMutex.lock();
        m_actionPtrMap[str] = actionfunc;
        m_metafunctionMapMutex.unlock();
    };

    initCheckPtr getInitCheckFunc(std::string str)
    {
        if (m_initCheckPtrMap.find(str) == m_initCheckPtrMap.end())
        {
            return NULL;
        }
        return m_initCheckPtrMap[str];
    };

    checkPtr getCheckFunc(std::string str)
    {
        if (m_checkPtrMap.find(str) == m_checkPtrMap.end())
        {
            return NULL;
        }
        return m_checkPtrMap[str];
    };

    comparisonPtr getComparisonFunc(std::string str)
    {
        if (m_comparisonPtrMap.find(str) == m_comparisonPtrMap.end())
        {
            return NULL;
        }
        return m_comparisonPtrMap[str];
    };

    initActionPtr getInitActionPtr(std::string str)
    {
        if (m_initActionPtrMap.find(str) == m_initActionPtrMap.end())
        {
            return NULL;
        }
        return m_initActionPtrMap[str];
    };

    actionPtr getActionPtr(std::string str)
    {
        if (m_actionPtrMap.find(str) == m_actionPtrMap.end())
        {
            return NULL;
        }
        return m_actionPtrMap[str];
    };

    //generate the new data
    //the blocksummary may changed after the execution
    //this is executed by asynchrounous way by thread pool

    ~FunctionManagerMeta(){};

    //function map that maps the name of the function into the pointer of the function
    tl::mutex m_metafunctionMapMutex;

    std::map<std::string, initCheckPtr> m_initCheckPtrMap;
    std::map<std::string, checkPtr> m_checkPtrMap;
    std::map<std::string, comparisonPtr> m_comparisonPtrMap;
    std::map<std::string, initActionPtr> m_initActionPtrMap;
    std::map<std::string, actionPtr> m_actionPtrMap;

    //hold a pointer to the trigger manager
    DynamicTriggerManager* m_dtm = NULL;
};


#endif