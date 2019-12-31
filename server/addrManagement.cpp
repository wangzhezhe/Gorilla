
#include "addrManagement.h"
#include "../utils/hash.h"

std::string endPointsManager::getByVarTs(std::string varName, int ts)
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

std::string endPointsManager::getByVarTsBlockID(std::string varName, int ts, size_t blockID)
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

void broadcastMetaServer()
{
    //send the meta server to all the compute node (except the master server itsself)

    int metaDatasize = m_metaserverLists.size();
    std::vector<MetaAddtWrapper> metaAddtWrapperList;

    for (int i = 0; i < metaDatasize; i++)
    {
        MetaAddtWrapper mdw(i, m_metaserverLists[i]);
        metaAddtWrapperList.push_back(mdw);
    }


    for (auto it = m_endPointsLists.begin(); it != m_endPointsLists.end(); it++)
    {
        std::string serverAddr = *it;
        if(serverAddr.compare(epManager->nodeAddr)!=0){
            UNICLIENT::updateDHT(serverAddr, metaAddtWrapperList);
        }
    }
    return;
}