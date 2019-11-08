
#include "filterManager.h"
#include <spdlog/spdlog.h>
#include "../client/unimosclient.h"

int FilterManager::profileSubscribe(std::string varName, FilterProfile &fp)
{

    enum FMANAGERSTATUS cmStatus = getStatus(varName, fp.m_profileName);
    switch (cmStatus)
    {
    case FMANAGERSTATUS::FMANAGERNOTEXIST:
        //same as the CMNOTEXIST
    case FMANAGERSTATUS::CMNOTEXIST:
    {

        constraintManager *cm = new constraintManager(fp.m_stepFilterName, fp.m_blockIDFilterName, fp.m_contentFilterName);
        cm->addSubscriber(fp.m_subscriberAddr);
        std::map<std::string, constraintManager *> temp;
        temp[fp.m_profileName] = cm;
        constraintManagerMap[varName] = temp;
        break;
    }
    case FMANAGERSTATUS::CMEXIST:
    {

        spdlog::info("failed to subscribe filter profile, the cm {} exist " , fp.m_profileName);
        //use error code here
    }
    }

    return 0;
}

enum FMANAGERSTATUS FilterManager::getStatus(std::string varName, std::string cmName)
{
    if (this->constraintManagerMap.find(varName) != constraintManagerMap.end())
    {
        if (this->constraintManagerMap[varName].find(cmName) != this->constraintManagerMap[varName].end())
        {
            return FMANAGERSTATUS::CMEXIST;
        }
        else
        {
            return FMANAGERSTATUS::CMNOTEXIST;
        }
    }
    else
    {

        return FMANAGERSTATUS::FMANAGERNOTEXIST;
    }
}


void FilterManager::notify(size_t step, size_t blockID, std::set<std::string> &subscriberAddrSet)
{

    //init enginge
    //TODO, get engine type from the subscribed addr
    //go through the list of the subscriber
    for (auto elem : subscriberAddrSet)
    {
        spdlog::debug("notify subscriber addr {}", elem);
        int status = dsnotify_subscriber(*(this->m_Engine), elem, step, blockID);
        spdlog::debug("notify status for {} is {}", elem, status);
    }
    return;
}