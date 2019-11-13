
#include "ipmanagement.h"
#include "endpointManagement.h"
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