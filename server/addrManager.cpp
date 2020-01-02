
#include "addrManager.h"
#include "../utils/hash.h"

std::string AddrManager::getByVarTs(std::string varName, int ts)
{

    size_t hashId = HASHFUNC::getIdByVarTs(varName, ts);

    if (this->m_endPointsLists.size() == 0)
    {
        throw std::runtime_error("the m_endPointsLists is 0 at server end");
    }

    int serverId = hashId % (m_endPointsLists.size());

    //std::cout << " debug hashId " << hashId <<" serverID " << serverId << std::endl;

    return m_endPointsLists[serverId];
}

std::string AddrManager::getByVarTsBlockID(std::string varName, int ts, size_t blockID)
{

    size_t hashId = HASHFUNC::getIdByVarTsBlockID(varName, ts, blockID);

    if (this->m_endPointsLists.size() == 0)
    {
        throw std::runtime_error("the m_endPointsLists is 0 at server end");
    }

    int serverId = hashId % (m_endPointsLists.size());

    //std::cout << " debug hashId " << hashId <<" serverID " << serverId << std::endl;

    return m_endPointsLists[serverId];
}

std::string AddrManager::getByRRobin()
{

    std::string serverAddr = m_endPointsLists[this->m_rrobinValue];

    this->m_rrbinMutex.lock();
    this->m_rrobinValue = (this->m_rrobinValue + 1) % (this->m_endPointsLists.size());
    this->m_rrbinMutex.unlock();

    return serverAddr;
}

void AddrManager::broadcastMetaServer(UniClient *globalClient)
{
    //send the meta server to all the compute node (except the master server itsself)

    int metaServerNum = this->m_metaServerNum;
    std::vector<MetaAddrWrapper> metaAddrWrapperList;

    for (int i = 0; i < metaServerNum; i++)
    {
        MetaAddrWrapper mdw(i, this->m_endPointsLists[i]);
        metaAddrWrapperList.push_back(mdw);
    }

    for (auto it = m_endPointsLists.begin(); it != m_endPointsLists.end(); it++)
    {
        std::string serverAddr = *it;
        int status = globalClient->updateDHT(serverAddr, metaAddrWrapperList);
        if (status != 0)
        {
            throw std::runtime_error("failed to update DHT for server: " + serverAddr);
        }
    }
    return;
}