#include <iostream>
#include <sstream>
#include "defaultfuncmeta.h"
#include "../../TriggerManager/triggerManager.h"

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
void defaultActionSartDt(FunctionManagerMeta *fmm, size_t step, std::string varName, RawDataEndpoint &rde, std::vector<std::string> parameters)
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
        if (fmm->m_dtm == NULL)
        {
            throw std::runtime_error("the pointer to the dynamic trigger should not be null");
        }
        fmm->m_dtm->commonstart(triggerName, step, varName, rde);
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

void defaultAction(FunctionManagerMeta *fmm, size_t step, std::string varName, std::string triggerMaster, UniClient *uniclient, RawDataEndpoint &rde, std::vector<std::string> parameters)
{
    //std::cout << "execute defaultAction" << std::endl;
    for (int i = 0; i < parameters.size(); i++)
    {
        std::cout << "parameters " << i << " value " << parameters[i] << std::endl;
    }
    return;
}

//TODO, if it is master, then notify to the watcher, if it is not master, then sent the information to the master
void defaultNotifyAction(FunctionManagerMeta *fmm, size_t step, std::string varName, std::string triggerMaster, UniClient *uniclient, RawDataEndpoint &rde, std::vector<std::string> parameters)
{

    //notify the watcher
    if (fmm->m_dtm == NULL)
    {
        throw std::runtime_error("the pointer to the dynamic trigger should not be null for defaultNotifyAction");
    }

    //range the watcher set and send block info to master
    //the master will send the notification back to the watcher
    for (auto it = fmm->m_dtm->m_registeredWatcherSet.begin(); it != fmm->m_dtm->m_registeredWatcherSet.end(); ++it)
    {
        std::cout << "send notification to master " << *it << std::endl;
        //TODO use specialized data structure to send the event information
        BlockSummary bs;
        bs.m_indexlb = rde.m_indexlb;
        bs.m_indexub = rde.m_indexub;

        //let the client to do the reduce operations
        uniclient->notifyBack(*it, bs);
    }

    //todo, ask raw data server to get the metadata then send the block summary back to the watcher
    return;
}

//put information of the current step into the group master
void defaultPutEvent(FunctionManagerMeta *fmm, size_t step, std::string varName, std::string triggerMaster, UniClient *uniclient, RawDataEndpoint &rde, std::vector<std::string> parameters)
{
    if (fmm->m_dtm == NULL)
    {
        throw std::runtime_error("the pointer to the trigger should not be null for defaultPutEvent");
        return;
    }

    if (triggerMaster == "")
    {
        throw std::runtime_error("the groupMaster should not be empty string");
        return;
    }

    //how to make sure the format of the event (varname, step, lb , ub)
    EventWrapper event(EVENT_DATA_PUT, varName, step, rde.m_dims, rde.m_indexlb, rde.m_indexub);
    //todo add triggerName as another parameter
    std::string triggerName = "testTrigger1";
    //std::cout << "put event to triggerMaster " << std::endl;
    // TODO, use more delegate way here, add condition, only one write
    // decrease unnecessary event number
    if (uniclient->m_position == 0)
    {
        uniclient->putEventIntoQueue(triggerMaster, triggerName, event);
    }
    return;
}

//in-situ function for exp
std::string InsituExpCheck(size_t step, std::string varName, RawDataEndpoint &rde, std::vector<std::string> parameters)
{
    if (parameters.size() == 0)
    {
        throw std::runtime_error("the parameters of InsituExpCheck should longer than 1");
    }
    //this is the second
    int anaTime = std::stoi(parameters[0]);
    //std::cout << "usleep: " << anaTime <<std::endl;
    //assume data checking heppens here
    //Attention! the usleep will hang all the process, use the 
    usleep(anaTime);
    //the value here is the ms, the engine parameter is needed
    //tl::thread::sleep(anaTime);
    return std::to_string(step);
}

bool InsituExpCompare(std::string checkResults, std::vector<std::string> parameters)
{
    //std::cout << "execute InsituExpCompare parameters " << parameters[0] << std::endl;
    if (parameters.size() == 0)
    {
        throw std::runtime_error("the parameters of InsituExpCompare should longer than 1");
    }

    int step = std::stoi(checkResults);

    if (parameters[0].compare("2") == 0)
    {
        if (step % 5 == 1)
        {
            return true;
        }
    }
    else if (parameters[0].compare("4") == 0)
    {
        if (step % 5 == 1 || step % 5 == 2)
        {
            return true;
        }
    }
    else if (parameters[0].compare("6") == 0)
    {
        if (step % 5 == 1 || step % 5 == 2 || step % 5 == 3)
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

void InsituExpAction(FunctionManagerMeta *fmm, size_t step, std::string varName, std::string triggerMaster, UniClient *uniclient, RawDataEndpoint &rde, std::vector<std::string> parameters)
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
        //char str[256];
        //sprintf(str, "adios write step %s varName %s lb %d %d %d ub %d %d %d\n",
        //        funcParameters[0].c_str(), funcParameters[1].c_str(),
        //        rde.m_indexlb[0], rde.m_indexlb[1], rde.m_indexlb[2],
        //        rde.m_indexub[0], rde.m_indexub[1], rde.m_indexub[2]);
        //std::cout << str << std::endl;

        //simulate the writting process
        //do this for in-situ testing that contains the IO
        //int writeTime = 3.8;
        //usleep(writeTime * 1000000);
        std::string result = uniclient->executeRawFunc(
            rde.m_rawDataServerAddr, rde.m_rawDataID, functionName, funcParameters);
    }
    catch (const std::exception &e)
    {
        std::cout << std::string(e.what()) << std::endl;
        std::cout << "exception for executeRawFunc " << rde.m_rawDataServerAddr << std::endl;
    }

    //std::cout << "execute results of InsituExpAction: " << result << std::endl;

    return;
}
