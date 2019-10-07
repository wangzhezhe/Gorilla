
//the mini data space server that provide the get and put api

#include <iostream>
#include <thread>
#include <chrono>
#include <typeinfo>
#include <string>
#include <iostream>
#include <vector>
#include <thallium.hpp>
//import this to parse the template
#include "memcache.h"

namespace tl = thallium;

//TODO put this in separate class
//it is better initilize the myengine at the main
//this shoule be initilized after the initilization of the argobot

tl::engine *globalEnginePointer = nullptr;

//init memory cache
MemCache *mcache = new MemCache();

//the rpc for testing
void hello(const tl::request &req, const std::string &name)
{
    std::cout << "Hello " << name << std::endl;
}

void putDoubleVector(const tl::request &req, const std::vector<double> &inputVector)
{
    std::cout << "check vector at the server:" << std::endl;
    int size = inputVector.size();
    for (int i = 0; i < size; i++)
    {
        std::cout << "index " << i << " value " << inputVector[i] << std::endl;
    }
    return;
}

void putMetaData(const tl::request &req, DataMeta &datameta)
{
    std::cout << "check putMetaData at the server:" << std::endl;
    datameta.printMeta();
    return;
}

//return the error code
//be careful with the parameters, they should match with the type used at the client end
//otherwise, there are some template issues
void dsput(const tl::request &req, DataMeta &datameta, size_t &blockID, tl::bulk &dataBulk)
{
    std::cout << "execute dataspace put:" << std::endl;
    datameta.printMeta();
    std::cout << "blockID is " << blockID << std::endl;

    //get the bulk value
    tl::endpoint ep = req.get_endpoint();

    //assign the memory
    size_t mallocSize = datameta.getDataMallocSize();

    std::cout << "malloc size is " << mallocSize << std::endl;

    try
    {
        void *localContainer = (void *)malloc(mallocSize);

        if (localContainer == NULL)
        {
            req.respond(-1);
        }

        //it is mandatory to use this expression
        std::vector<std::pair<void *, std::size_t>> segments(1);
        segments[0].first = localContainer;
        segments[0].second = mallocSize;

        //create the container for the data
        tl::bulk local = globalEnginePointer->expose(segments, tl::bulk_mode::write_only);
        dataBulk.on(ep) >> local;

        std::cout << "Server received bulk, check the contents: " << std::endl;

        //check the bulk
        /*
    double *rawdata = (double *)localContainer;
    for (int i = 0; i < 10; i++)
    {
        std::cout << "index " << i << " value " << *rawdata << std::endl;
        rawdata++;
    }
    */

        //TODO get the pointer wisely to decrease the data copy
        //generate the empty container firstly, then get the data pointer
        int status = mcache->putIntoCache(datameta, blockID, localContainer);

        //TODO update the return value
        req.respond(status);

        std::cout << "ok to put the data" << std::endl;
    }
    catch(...)
    {
        std::cout << "exception for data put" << std::endl;
        req.respond(-1);
    }

    return;
}

//is it ok to set the return value as bulk
//bad strategy to send the bulk back to server
/*
void dsget(const tl::request &req, std::string &varName, int &ts, size_t &blockID)
{
    //get variable type from the data
    void *data = nullptr;

    std::cout << "dsget by varName: " << varName << " ts: " << ts << " blockID: " << blockID << std::endl;

    DataMeta *datameta = mcache->getFromCache(varName, ts, blockID, data);

    //get the data value
    if (datameta == NULL)
    {
        std::cout << "failed to get the data at the server end" << std::endl;
        //return empty bulk info if it is failed to get data
        //how to adjust it is empty at the client end?
        req.respond(tl::bulk());
        return;
    }

    
    double *rawdata = (double *)data;
    std::cout << "check the get data at the server end" << std::endl;

    for (int i = 0; i < 10; i++)
    {
        std::cout << "index " << i << " value " << *rawdata << std::endl;
        rawdata++;
    }
    

    std::vector<std::pair<void *, std::size_t>> segments(1);
    segments[0].first = (void *)(data);
    segments[0].second = datameta->getDataMallocSize();

    tl::bulk returnBulk = globalEnginePointer->expose(segments, tl::bulk_mode::read_only);
    //it is dangerous to return the bulk since the memory at the server end might be released for some reasons
    req.respond(returnBulk);
}
*/

//is it ok to set the return value as bulk
void dsget(const tl::request &req, std::string &varName, int &ts, size_t &blockID, tl::bulk &clientBulk)
{
    //get variable type from the data
    void *data = nullptr;

    std::cout << "dsget by varName: " << varName << " ts: " << ts << " blockID: " << blockID << std::endl;

    try
    {
        DataMeta *datameta = mcache->getFromCache(varName, ts, blockID, data);
        //get the data value
        if (datameta == NULL)
        {
            std::cout << "failed to get the data at the server end" << std::endl;
            //return empty bulk info if it is failed to get data
            //how to adjust it is empty at the client end?
            req.respond(-1);
            return;
        }
        std::vector<std::pair<void *, std::size_t>> segments(1);
        segments[0].first = (void *)(data);
        segments[0].second = datameta->getDataMallocSize();

        tl::bulk returnBulk = globalEnginePointer->expose(segments, tl::bulk_mode::read_only);

        tl::endpoint ep = req.get_endpoint();
        clientBulk.on(ep) << returnBulk;

        req.respond(0);
    }
    catch(...)
    {
        std::cout << "exception for get" << std::endl;
        req.respond(-1);
    }

    /*
    double *rawdata = (double *)data;
    std::cout << "check the get data at the server end" << std::endl;

    for (int i = 0; i < 10; i++)
    {
        std::cout << "index " << i << " value " << *rawdata << std::endl;
        rawdata++;
    }
    */
}

void runRerver()
{

    //tl::engine myEngine("na+sm", THALLIUM_SERVER_MODE);
    //tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);
    tl::engine myEngine("verbs", THALLIUM_SERVER_MODE);
    std::cout << "addr " << myEngine.self() << std::endl;
    globalEnginePointer = &myEngine;
    //globalEnginePointer->define("hello", hello).disable_response();
    //globalEnginePointer->define("putDoubleVector", putDoubleVector).disable_response();
    globalEnginePointer->define("putMetaData", putMetaData).disable_response();
    globalEnginePointer->define("dsput", dsput);
    globalEnginePointer->define("dsget", dsget);

    std::cout << "Start the mini dataspace server..." << std::endl;

    return;
}

void dspaces_server_init()
{

    return;
}

void dspaces_server_fini()
{
    return;
}

void dpsaces_server_put()
{
    return;
}

void dspaces_server_get()
{
    return;
}

void dspaces_server_lock()
{
    return;
}

void dspaces_server_unlock()
{
    return;
}

//TODO, put this at the separate test
void testput()
{
    //generate the data and put it in memcache
    std::vector<double> array;

    for (int i = 0; i < 10; i++)
    {
        array.push_back(i * 1.1);
    }

    std::string varName = "testName";
    int ts = 0;
    //TODO distinguish local info and the global info
    //Block id could be calculated by the simulation
    //the minidataspace only know the data block id but it don't know how user partition the data domain
    //this two parameters is for the data variable in specific time step
    std::array<size_t, 3> lowbound = {0, 0, 0};
    std::array<size_t, 3> shape = {10, 0, 0};
    size_t blockID = 0;

    //TODO, update the method for type string here
    DataMeta datameta = DataMeta(varName, ts, 1, typeid(double).name(), sizeof(double), lowbound, shape);
    mcache->putIntoCache<double>(datameta, blockID, array);

    blockID = 1;
    mcache->putIntoCache(datameta, blockID, (void *)(array.data()));
}

//this is supposed to be called by the rpc
void testget(size_t blockID)
{
    //get variable type from the data
    void *data = nullptr;

    std::string varName = "testName";
    int ts = 0;

    DataMeta *datameta = mcache->getFromCache(varName, ts, blockID, data);

    //check the datameta
    if (datameta != NULL)
    {
        datameta->printMeta();
    }
    else
    {
        std::cout << "failed to get data with blockID " << blockID << std::endl;
        return;
    }

    std::string typeString = typeid(double).name();

    if (datameta->m_typeName.compare(typeString) == 0)
    {
        //There is bug if use static_cast<int*> here, not sure why
        if (data != nullptr)
        {
            double *dataValue = (double *)(data);
            std::cout << "check output..." << std::endl;
            //assume there is only one dimention
            //TODO add function to get index from shape
            for (int i = 0; i < datameta->m_shape[0]; i++)
            {
                std::cout << "index " << i << " value " << *dataValue << std::endl;
                dataValue++;
            }
        }
        else
        {
            std::cout << "data pointer is nullptr" << std::endl;
        }
    }

    //transfer the raw data according to meta info

    //check the data value

    //print

    return;
}

void test()
{
    testput();

    std::cout << "ok to put..." << std::endl;

    testget(0);

    std::cout << "ok to get data with block 0..." << std::endl;

    testget(1);

    std::cout << "ok to get data with block 1..." << std::endl;

    while (true)
    {

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return;
}

int main()
{
    //start the mini dataspace server
    runRerver();
}
