#ifndef FUNCTIONMNG_H
#define FUNCTIONMNG_H

#include "../../commondata/metadata.h"
#include "./defaultFunctions/defaultfuncmeta.h"

#include <thallium.hpp>
#include <map>
#include <vector>
#include <adios2.h>

#include "mpi.h"

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
    UniClient* uniclient,
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

        registerCheckFunc("InsituExpCheck", &InsituExpCheck);
        registerComparisonFunc("InsituExpCompare", &InsituExpCompare);
        registerActionFunc("InsituExpAction", &InsituExpAction);

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



// the execution engine runs at the raw data server
// TODO put this at the dynamic trigger

struct FunctionManagerRaw;
//TODO use separate .h file???
typedef std::string (*rawdatafunctionPointer)(
FunctionManagerRaw* fmw,
const BlockSummary &bs, 
void *inputData, 
const std::vector<std::string>& parameters);

std::string test(
FunctionManagerRaw* fmr,
const BlockSummary &bs, 
void *inputData,
const std::vector<std::string>& parameters);

std::string testvtk(
FunctionManagerRaw*fmr,
const BlockSummary &bs, 
void *inputData,
const std::vector<std::string>& parameters);

std::string valueRange(
FunctionManagerRaw*fmr,
const BlockSummary &bs, 
void *inputData, 
const std::vector<std::string>& parameters);

std::string adiosWrite(
FunctionManagerRaw*fmr,
const BlockSummary &bs, 
void *inputData, 
const std::vector<std::string>& parameters);

struct FunctionManagerRaw
{
    FunctionManagerRaw()
    {
        //register the default function
        this->m_functionMap["test"] = &test;
        this->m_functionMap["testvtk"] = &testvtk;
        this->m_functionMap["valueRange"] = &valueRange;
        this->m_functionMap["adiosWrite"] = &adiosWrite;
        
        //the io need to be started
        //initADIOS();

    };

    bool registerFunction(std::string functionName, rawdatafunctionPointer fp);
    
    //put the execution logic together with the storage part
    std::string execute(
    FunctionManagerRaw* fmr,
    const BlockSummary &bs, 
    void *inputData,
    std::string fiunctionName,
    const std::vector<std::string>& parameters);

    ~FunctionManagerRaw()
    {
        std::cout << "destroy FunctionManagerRaw\n";
    };
    
    void initADIOS(MPI_Comm comm){
        std::cout << "debug1 adios"<< std::endl;
        adios2::ADIOS adios(comm, adios2::DebugON);
                std::cout << "debug2 adios"<< std::endl;

        this->m_io = adios.DeclareIO("gorilla_gs");
                std::cout << "debug3 adios"<< std::endl;

        this->m_io.SetEngine("BP4");
        std::cout << "debug4 adios"<< std::endl;
        this->m_writer = m_io.Open("gorilla_gs", adios2::Mode::Write);
                std::cout << "debug5 adios"<< std::endl;

    }

    //TODO close IO

    //function map that maps the name of the function into the pointer of the function
    tl::mutex m_functionMapMutex;
    std::map<std::string, rawdatafunctionPointer> m_functionMap;

    //if the adios is needed
    adios2::IO m_io;
    adios2::Engine m_writer;
};


#endif