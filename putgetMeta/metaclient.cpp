#include "metaclient.h"

std::string MetaClient::loadMasterAddr(std::string masterConfigFile)
{

    std::ifstream infile(masterConfigFile);
    std::string content;
    std::getline(infile, content);
    std::cout << "load master server: " << content << std::endl;

    return content;
}

void MetaClient::hello()
{
    tl::remote_procedure remoteHello = this->m_clientEnginePtr->define("hello").disable_response();
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(this->m_masterAddr);
    remoteHello.on(serverEndpoint)();
    return;
}



void MetaClient::Recordtime(std::string recordkey)
{
    tl::remote_procedure remoteRecordtime = this->m_clientEnginePtr->define("recordtime");
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(this->m_masterAddr);
    //the parameters here should be consistent with the defination at the server end
    Input input(recordkey);
    int status = remoteRecordtime.on(serverEndpoint)(input);
    if(status!=0){
        throw std::runtime_error("failed for Recordtime");
    }
    return;

}

void MetaClient::Recordtimestart(std::string recordkey)
{
    tl::remote_procedure remoteRecordtimestart = this->m_clientEnginePtr->define("recordtimestart");
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(this->m_masterAddr);
    //the parameters here should be consistent with the defination at the server end
    Input input(recordkey);
    int status = remoteRecordtimestart.on(serverEndpoint)(recordkey);
    if(status!=0){
        throw std::runtime_error("failed for Recordtimestart");
    }
    return;

}

void MetaClient::Recordtimetick(std::string recordkey)
{
    tl::remote_procedure remoteRecordtimetick = this->m_clientEnginePtr->define("recordtimetick");
    tl::endpoint serverEndpoint = this->m_clientEnginePtr->lookup(this->m_masterAddr);
    //the parameters here should be consistent with the defination at the server end
    Input input(recordkey);
    int status = remoteRecordtimetick.on(serverEndpoint)(recordkey);
    if(status!=0){
        throw std::runtime_error("failed for Recordtimetick");
    }
    return;

}
