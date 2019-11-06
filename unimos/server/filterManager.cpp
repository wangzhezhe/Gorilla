
#include "filterManager.h"

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

        throw std::runtime_error("failed to subscribe filter profile, the cm " + fp.m_profileName + " exist");
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