
#ifndef __UNIMOS_SERVER__H__
#define __UNIMOS_SERVER__H__

#include "addrManager.h"
#include "DHTManager/dhtmanager.h"
#include "../client/unimosclient.h"
#include "RawdataManager/blockManager.h"
#include "MetadataManager/metadataManager.h"
#include "FunctionManager/functionManager.h"
#include "TriggerManager/triggerManager.h"
#include <map>

struct UniServer
{
    UniServer(){};
    //the addr manager
    AddrManager *m_addrManager = nullptr;

    //the global manager for raw data
    BlockManager *m_blockManager = nullptr;

    //the meta data service
    MetaDataManager *m_metaManager = nullptr;

    //the dht manager
    DHTManager *m_dhtManager = nullptr;

    //the trigger manager
    FunctionManagerMeta *m_fmetamanager = nullptr;
    FunctionManagerRaw *m_frawmanager = nullptr;
    DynamicTriggerManager *m_dtmanager = nullptr;

    void initManager(int globalProc, int metaServerNum, UniClient *client, bool ifDistributed)
    {
        //init global managers
        m_addrManager = new AddrManager();

        //config the address manager
        m_addrManager->m_serverNum = globalProc;
        m_addrManager->m_metaServerNum = metaServerNum;

        //the global manager for raw data
        m_blockManager = new BlockManager();

        //the meta data service
        m_metaManager = new MetaDataManager();

        //the dht manager is global which should be inited before server
        m_dhtManager = new DHTManager();
        //dynamic trigger manager
        if (client == NULL && ifDistributed == true)
        {
            throw std::runtime_error("the client should not be NULL for init func");
        }

        m_fmetamanager = new FunctionManagerMeta();
        m_dtmanager = new DynamicTriggerManager(m_fmetamanager, 16, client);
        m_frawmanager = new FunctionManagerRaw();
    };

    //init the bulk at the server end
    tl::mutex m_bulkMapmutex;
    std::map<int, tl::bulk> m_bulkMap;
    std::map<int, void *> m_dataContainerMap;

    ~UniServer()
    {
        //delete the m_dataContainerList
    };
};

#endif