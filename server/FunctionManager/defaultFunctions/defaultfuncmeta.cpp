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

//get block summary

bool defaultComparisonStep(std::string checkResults, std::vector<std::string> parameters)
{
    //std::cout << "defaultComparisonStep check results " << checkResults << std::endl;
    std::stringstream sstream(checkResults);
    size_t currentstep;
    sstream >> currentstep;

    //the first parameter is lower bound
    if (parameters.size() > 0)
    {
        std::string stepStrLb = parameters[0];
        std::stringstream sstreamcheck(stepStrLb);
        size_t steplb;

        sstreamcheck >> steplb;

        //std::cout << "step lb " << steplb << " current step " << currentstep << std::endl;
        //only return true for specific step
        if (currentstep >= steplb)
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

    //go through it
    for (int i = 0; i < parameters.size(); i++)
    {
        std::string triggerName = parameters[i];
        //std::cout << "start new trigger: " << triggerName << std::endl;
        dtm->commonstart(triggerName, step, varName, rde);
    }

    return;
}

//get metadata
std::string defaultCheck(size_t step, std::string varName, RawDataEndpoint &rde, std::vector<std::string> parameters)
{

    //std::cout << "execute defaultCheck" << std::endl;

    for (int i = 0; i < parameters.size(); i++)
    {
        std::cout << "parameters " << i << " value " << parameters[i] << std::endl;
    }
    return "defaultCheck results";
}

bool defaultComparison(std::string checkResults, std::vector<std::string> parameters)
{
    //std::cout << "execute defaultComparison" << std::endl;
    for (int i = 0; i < parameters.size(); i++)
    {
        std::cout << "parameters " << i << " value " << parameters[i] << std::endl;
    }
    return true;
}

void defaultAction(size_t step, std::string varName, UniClient *uniclient, RawDataEndpoint &rde, std::vector<std::string> parameters)
{
    //std::cout << "execute defaultAction" << std::endl;
    for (int i = 0; i < parameters.size(); i++)
    {
        std::cout << "parameters " << i << " value " << parameters[i] << std::endl;
    }
    return;
}

//in-situ function for exp
std::string InsituExpCheck(size_t step, std::string varName, RawDataEndpoint &rde, std::vector<std::string> parameters)
{
    if (parameters.size() <= 1)
    {
        throw std::runtime_error("the parameters of InsituExpCheck should longer than 1");
    }
    //this is the second
    int anaTime = std::stoi(parameters[0]);
    //std::cout << "usleep: " << anaTime <<std::endl;
    //assume data checking heppens here
    usleep(anaTime);
    return std::to_string(step);
}

bool InsituExpCompare(std::string checkResults, std::vector<std::string> parameters)
{
    //std::cout << "execute InsituExpCompare parameters " << parameters[0] << std::endl;
    if (parameters.size() != 1)
    {
        throw std::runtime_error("the parameters of InsituExpCompare should longer than 1");
    }

    int step = std::stoi(checkResults);

    if (parameters[0].compare("2") == 0)
    {
        if (step % 5 == 0)
        {
            return true;
        }
    }
    else if (parameters[0].compare("8") == 0)
    {
        if (step % 5 != 0)
        {
            return true;
        }
    }
    else if (parameters[0].compare("0") == 0)
    {
        return true;
    }
    else
    {
        throw std::runtime_error("the parameters of InsituExpCompare should be 2 or 8");
    }
    return false;
}

void InsituExpAction(size_t step, std::string varName, UniClient *uniclient, RawDataEndpoint &rde, std::vector<std::string> parameters)
{
    //write the data into the adios
    //send request to the raw data server and let it execut the adios data writting
    if (uniclient == NULL)
    {
        throw std::runtime_error("the client at the action function should not be null");
        return;
    }
    if (parameters.size() != 1)
    {
        throw std::runtime_error("the parameters of InsituExpAction should eauqls to 1");
        return;
    }

    std::string functionName = parameters[0];

    //transfer the varname and timestep
    std::vector<std::string> funcParameters;
    funcParameters.push_back(std::to_string(step));
    funcParameters.push_back(varName);

    if (rde.m_rawDataServerAddr.compare(uniclient->m_masterAddr) == 0)
    {
        //TODO if the destaddr is not equal to the current one
        return;
    }

    try
    {
        char str[256];
        sprintf(str, "adios write step %s varName %s lb %d %d %d ub %d %d %d\n",
                funcParameters[0].c_str(), funcParameters[1].c_str(),
                rde.m_indexlb[0], rde.m_indexlb[1], rde.m_indexlb[2],
                rde.m_indexub[0], rde.m_indexub[1], rde.m_indexub[2]);
        std::cout << str << std::endl;

        //simulate the writting process
        //do this for in-situ testing that contains the IO
        //int writeTime = 3.8;
        //usleep(writeTime * 1000000);
        //std::string result = uniclient->executeRawFunc(
        //    rde.m_rawDataServerAddr, rde.m_rawDataID, functionName, funcParameters);
    }
    catch (const std::exception &e)
    {
        std::cout << std::string(e.what()) << std::endl;
        std::cout << "exception for executeRawFunc " << rde.m_rawDataServerAddr << std::endl;
    }

    //std::cout << "execute results of InsituExpAction: " << result << std::endl;

    return;
}