
#ifndef FILTERMANAGER_H
#define FILTERMANAGER_H

// store the registered triggers

#include <map>
#include <set>
#include <string>
#include <thallium.hpp>
#include <vector>
#include "../ExecutionEngine/executionengine.h"

namespace tl = thallium;

struct FuncDescriptor
{
    FuncDescriptor(std::string funcName, std::vector<std::string> parameters) : m_funcName(funcName), m_parameters(parameters) {}
    std::string m_funcName;
    std::vector<std::string> m_parameters;
    ~FuncDescriptor(){};
};

struct DynamicTrigger
{

    DynamicTrigger(FuncDescriptor check,
                   FuncDescriptor comparison,
                   FuncDescriptor action) : m_check(check), m_comparison(comparison), m_action(action) {}

    FuncDescriptor m_check;
    FuncDescriptor m_comparison;
    FuncDescriptor m_action;

    ~DynamicTrigger() {}

    void start(ExecutionEngineMeta *exengine);
};

#endif