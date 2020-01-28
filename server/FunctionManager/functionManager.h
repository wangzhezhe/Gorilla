#ifndef FUNCTIONMNG_H
#define FUNCTIONMNG_H

#include "../../commondata/metadata.h"
#include "defaultFunctions/defaultfuncraw.h"
#include "defaultFunctions/defaultfuncmeta.h"


#include <thallium.hpp>
#include <map>
#include <vector>

namespace tl = thallium;

//forward declaration
struct DynamicTriggerManager;

//the function pointer that execute at the metadata server

typedef std::string (*initCheckPtr)(
    size_t step, 
    std::string varName, 
    RawDataEndpoint& rde);

typedef std::string (*checkPtr)(
    size_t step, 
    std::string varName, 
    RawDataEndpoint& rde,
    std::vector<std::string> parameters);

typedef bool (*comparisonPtr)(std::string checkResults, std::vector<std::string> parameters);

typedef void (*initActionPtr)(
    DynamicTriggerManager *dtm, 
    size_t step, 
    std::string varName, 
    RawDataEndpoint& rde,
    std::vector<std::string> parameters);

typedef void (*actionPtr)(    
    size_t step, 
    std::string varName, 
    RawDataEndpoint& rde,
    std::vector<std::string> parameters);

// one profile is corresponding to a constraint manager
// the execute method can be implemented by different drivers
// the execution engine runs at the meta server
struct FunctionManagerMeta
{
    FunctionManagerMeta()
    {
        registerFunc();
    };

    void registerFunc(){

        registerInitCheckFunc("defaultCheckGetStep",&defaultCheckGetStep);
        registerComparisonFunc("defaultComparisonStep",&defaultComparisonStep);
        registerInitActionFunc("defaultActionSartDt",&defaultActionSartDt);

        registerCheckFunc("defaultCheck", &defaultCheck);
        registerComparisonFunc("defaultComparison", &defaultComparison);
        registerActionFunc("defaultAction", &defaultAction);

        return;
    }

    void registerInitCheckFunc(std::string str, initCheckPtr checkfunc){
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
};

//the function pointer that execute at the raw data server

typedef void (*rawdatafunctionPointer)(const BlockSummary &bs, void *inputData);

// the execution engine runs at the raw data server
// TODO put this at the dynamic trigger

struct FunctionManagerRaw
{
    FunctionManagerRaw()
    {
        //register the default function
        this->m_functionMap["test"] = &test;
        this->m_functionMap["testvtk"] = &testvtk;
    };

    bool registerFunction(std::string functionName, rawdatafunctionPointer fp);
    
    //put the execution logic together with the storage part
    void execute(std::string fiunctionName, const BlockSummary &bs, void *inputData);

    ~FunctionManagerRaw()
    {
        std::cout << "destroy FunctionManagerRaw\n";
    };

    //function map that maps the name of the function into the pointer of the function
    tl::mutex m_functionMapMutex;
    std::map<std::string, rawdatafunctionPointer> m_functionMap;
};


#endif