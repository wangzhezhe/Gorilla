
#ifndef __UNIMOS_SERVER__H__
#define __UNIMOS_SERVER__H__

#include "addrManager.h"
#include "DHTManager/dhtmanager.h"
#include "../client/unimosclient.h"
#include "RawdataManager/blockManager.h"
#include "MetadataManager/metadataManager.h"
#include "FunctionManager/functionManagerRaw.h"
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
    FunctionManagerRaw *m_frawmanager = nullptr;
    FunctionManagerMeta *m_fmetamanager = nullptr;
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

        //TODO, control the thread number based on if the trigger is enabled
        //the number for the in-situ thread pool should be small for scale

        m_dtmanager = new DynamicTriggerManager(8, client);
        
        m_fmetamanager = new FunctionManagerMeta();
        m_fmetamanager->m_dtm = m_dtmanager;
        m_dtmanager->m_funcmanagerMeta = m_fmetamanager;

        m_frawmanager = new FunctionManagerRaw();
        m_frawmanager->m_blockManager=m_blockManager;
    };

    //init the bulk at the server end
    tl::mutex m_bulkMapmutex;
    std::map<int, tl::bulk> m_bulkMap;
    std::map<int, void *> m_dataContainerMap;

    ~UniServer(){
        //delete the m_dataContainerList
        //TODO there is double free issue here
        delete m_addrManager;
        delete m_blockManager;
        // TODO this coause the double free issue
        //delete m_metaManager;
        //delete m_fmetamanager;
        //delete m_dtmanager;
        //delete m_frawmanager;
    };
};

#endif