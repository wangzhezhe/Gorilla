#ifndef FUNCTIONRAW_H
#define FUNCTIONRAW_H

#include <commondata/metadata.h>
#include "../statefulConfig.h"
//#include "../unimosserver.hpp"

#include <thallium.hpp>
#include <map>
#include <vector>
#include <blockManager/blockManager.h>

namespace tl = thallium;

namespace GORILLA
{

struct FunctionManagerRaw;
//TODO use separate .h file???
typedef std::string (*rawdatafunctionPointer)(
    FunctionManagerRaw *fmw,
    const BlockSummary &bs,
    void *inputData,
    const std::vector<std::string> &parameters);

std::string test(
    FunctionManagerRaw *fmr,
    const BlockSummary &bs,
    void *inputData,
    const std::vector<std::string> &parameters);

std::string testvtk(
    FunctionManagerRaw *fmr,
    const BlockSummary &bs,
    void *inputData,
    const std::vector<std::string> &parameters);

std::string valueRange(
    FunctionManagerRaw *fmr,
    const BlockSummary &bs,
    void *inputData,
    const std::vector<std::string> &parameters);

std::string adiosWrite(
    FunctionManagerRaw *fmr,
    const BlockSummary &bs,
    void *inputData,
    const std::vector<std::string> &parameters);


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
        FunctionManagerRaw *fmr,
        const BlockSummary &bs,
        void *inputData,
        std::string fiunctionName,
        const std::vector<std::string> &parameters);

    ~FunctionManagerRaw()
    {
        std::cout << "destroy FunctionManagerRaw\n";
    };

    statefulConfig *m_statefulConfig = NULL;

    //function map that maps the name of the function into the pointer of the function
    tl::mutex m_functionMapMutex;
    std::map<std::string, rawdatafunctionPointer> m_functionMap;

    //hold a pointer to the block manager
    BlockManager *m_blockManager = NULL;

};

}
#endif