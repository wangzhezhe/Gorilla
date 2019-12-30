#ifndef EXECUTIONENGINE_H
#define EXECUTIONENGINE_H

#include "../../commondata/metadata.h"
#include "../../utils/ArgothreadPool.h"
#include "defaultFunctions/defaultfunc.h"

#include <thallium.hpp>
#include <map>

typedef void (*functionPointer)(const BlockSummary &bs, void *inputData);

// one profile is corresponding to a constraint manager
// the execute method can be implemented by different drivers
// the execution engine runs at the meta server
struct ExecutionEngineMeta
{

    ExecutionEngineMeta(size_t poolSize)
    {
        this->m_threadPool = new ArgoThreadPool(poolSize);
        //register the default function
        this->m_functionMap["test"] = &test;
                this->m_functionMap["testvtk"] = &testvtk;

    };

    bool registerFunction(std::string functionName, functionPointer fp);

    //generate the new data
    //the blocksummary may changed after the execution
    //this is executed by asynchrounous way by thread pool
    void execute(std::string fiunctionName, const BlockSummary &bs, void *inputData);

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
    tl::mutex m_functionMapMutex;
    std::map<std::string, functionPointer> m_functionMap;
};

// the execution engine runs at the raw data server
struct ExecutionEngineRaw
{

    ExecutionEngineRaw()
    {
        //register the default function
        this->m_functionMap["test"] = &test;
        this->m_functionMap["testvtk"] = &testvtk;
    };

    bool registerFunction(std::string functionName, functionPointer fp);


    void execute(std::string fiunctionName, const BlockSummary &bs, void *inputData);

    ~ExecutionEngineRaw()
    {
        std::cout << "destroy ExecutionEngineRaw\n";
    };


    //function map that maps the name of the function into the pointer of the function
    tl::mutex m_functionMapMutex;
    std::map<std::string, functionPointer> m_functionMap;
};

#endif