#include <iostream>
#include <sstream>
#include "defaultfuncmeta.h"

struct DynamicTriggerManager;

//get metadata
std::string defaultCheckGetStep(size_t step, std::string varName, RawDataEndpoint &rde)
{
    std::string stepStr = std::to_string(step);
    return stepStr;
}

bool defaultComparisonStep(std::string checkResults, std::vector<std::string> parameters)
{
    std::cout << "defaultComparisonStep check results " << checkResults << std::endl;
    std::stringstream sstream(checkResults);
    size_t currentstep;
    sstream >> currentstep;

    //the first parameter is lb
    if (parameters.size() > 0)
    {
        std::string stepStrLb = parameters[0];
        std::stringstream sstreamcheck(stepStrLb);
        size_t steplb;

        sstreamcheck >> steplb;

        std::cout << "step lb " << steplb << " current step " << currentstep << std::endl;
        //only return true for specific step
        if (currentstep == steplb)
        {
            return true;
        }
    }

    return false;
}

//start a new dynamic trigger
void defaultActionSartDt(DynamicTriggerManager *dtm, size_t step, std::string varName, RawDataEndpoint &rde, std::vector<std::string> parameters)
{
    //start another dynamic trigger
    //the first element id the name of the dynamic trigger
    if (parameters.size() == 0)
    {
        throw std::runtime_error("the parameters should larger than 1 for defaultActionSartDt");
    }
    std::string triggerName = parameters[0];

    std::cout << "start new trigger: " << triggerName << std::endl;

    dtm->commonstart(triggerName, step, varName, rde);
    return;
}

//get metadata
std::string defaultCheck(size_t step, std::string varName, RawDataEndpoint &rde, std::vector<std::string> parameters)
{

    std::cout << "execute defaultCheck" << std::endl;
    
    for (int i = 0; i < parameters.size(); i++)
    {
        std::cout << "parameters " << i << " value " << parameters[i] << std::endl;
    }
    return "defaultCheck results";
}


bool defaultComparison(std::string checkResults, std::vector<std::string> parameters)
{
        std::cout << "execute defaultComparison" << std::endl;
    for (int i = 0; i < parameters.size(); i++)
    {
        std::cout << "parameters " << i << " value " << parameters[i] << std::endl;
    }
    return true;
}

void defaultAction(size_t step, std::string varName, RawDataEndpoint &rde, std::vector<std::string> parameters)
{
    std::cout << "execute defaultAction" << std::endl;
    for (int i = 0; i < parameters.size(); i++)
    {
        std::cout << "parameters " << i << " value " << parameters[i] << std::endl;
    }
    return;
}