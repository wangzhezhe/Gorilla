#ifndef EXECUTIONENGINE_H
#define EXECUTIONENGINE_H

#include "../../commondata/metadata.h"
#include "../../utils/ArgothreadPool.h"
#include "defaultFunctions/defaultfuncraw.h"
#include "defaultFunctions/defaultfuncmeta.h"

#include <thallium.hpp>
#include <map>
#include <vector>

//the function pointer that execute at the metadata server

typedef std::string (*checkPtr)(std::vector<std::string> parameters);

typedef bool (*comparisonPtr)(std::string checkResults, std::vector<std::string> parameters);

typedef void (*actionPtr)(std::vector<std::string> parameters);

// one profile is corresponding to a constraint manager
// the execute method can be implemented by different drivers
// the execution engine runs at the meta server
struct ExecutionEngineMeta
{

    ExecutionEngineMeta(size_t poolSize)
    {
        this->m_threadPool = new ArgoThreadPool(poolSize);
        m_metafunctionMapMutex.lock();
        m_checkPtrMap["defaultCheck"] = &defaultCheck;
        m_comparisonPtrMap["defaultComparison"] = &defaultComparison;
        m_actionPtrMap["defaultAction"] = &defaultAction;
        m_metafunctionMapMutex.unlock();
    };

    bool registerCheckFunc(std::string str, checkPtr checkfunc)
    {
        m_metafunctionMapMutex.lock();
        m_checkPtrMap[str] = checkfunc;
        m_metafunctionMapMutex.unlock();
    };

    bool registerComparisonFunc(std::string str, comparisonPtr comparisonfunc)
    {
        m_metafunctionMapMutex.lock();
        m_comparisonPtrMap[str] = comparisonfunc;
        m_metafunctionMapMutex.unlock();
    };

    bool registerActionFunc(std::string str, actionPtr actionfunc)
    {
        m_metafunctionMapMutex.lock();
        m_actionPtrMap[str] = actionfunc;
        m_metafunctionMapMutex.unlock();
    };

    checkPtr getCheckFunc(std::string str)
    {
        if (m_checkPtrMap.find(str) == m_checkPtrMap.end())
        {
            return NULL;
        }
        return m_checkPtrMap[str];
    }

    comparisonPtr getComparisonFunc(std::string str)
    {
        if (m_comparisonPtrMap.find(str) == m_comparisonPtrMap.end())
        {
            return NULL;
        }
        return m_comparisonPtrMap[str];
    }

    actionPtr getActionPtr(std::string str)
    {
        if (m_actionPtrMap.find(str) == m_actionPtrMap.end())
        {
            return NULL;
        }
        return m_actionPtrMap[str];
    }

    //generate the new data
    //the blocksummary may changed after the execution
    //this is executed by asynchrounous way by thread pool

    ~ExecutionEngineMeta()
    {
        std::cout << "destroy ExecutionEngineMeta\n";
        if (this->m_threadPool != NULL)
        {
            delete this->m_threadPool;
        }
    };

    ArgoThreadPool *m_threadPool = NULL;

    //function map that maps the name of the function into the pointer of the function
    tl::mutex m_metafunctionMapMutex;
    std::map<std::string, checkPtr> m_checkPtrMap;
    std::map<std::string, comparisonPtr> m_comparisonPtrMap;
    std::map<std::string, actionPtr> m_actionPtrMap;
};

//the function pointer that execute at the raw data server

typedef void (*rawdatafunctionPointer)(const BlockSummary &bs, void *inputData);

// the execution engine runs at the raw data server
struct ExecutionEngineRaw
{

    ExecutionEngineRaw()
    {
        //register the default function
        this->m_functionMap["test"] = &test;
        this->m_functionMap["testvtk"] = &testvtk;
    };

    bool registerFunction(std::string functionName, rawdatafunctionPointer fp);

    void execute(std::string fiunctionName, const BlockSummary &bs, void *inputData);

    ~ExecutionEngineRaw()
    {
        std::cout << "destroy ExecutionEngineRaw\n";
    };

    //function map that maps the name of the function into the pointer of the function
    tl::mutex m_functionMapMutex;
    std::map<std::string, rawdatafunctionPointer> m_functionMap;
};

#endif