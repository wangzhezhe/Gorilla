
#ifndef __UNIMOS_SERVER__H__
#define __UNIMOS_SERVER__H__

#include "DHTManager/dhtmanager.h"
#include "FunctionManager/functionManagerRaw.h"
#include "MetadataManager/metadataManager.h"
#include "ScheduleManager/scheduleManager.h"
#include "TriggerManager/triggerManager.h"
#include "addrManager.h"
#include <client/ClientForStaging.hpp>
#include <map>
#include <blockManager/blockManager.h>
#include <metricManager/metricManager.hpp>
namespace GORILLA
{

struct UniServer
{
  UniServer(){};
  // the addr manager
  AddrManager* m_addrManager = nullptr;

  // the global manager for raw data
  BlockManager* m_blockManager = nullptr;

  // the meta data service
  MetaDataManager* m_metaManager = nullptr;

  // the scheduler manager
  ScheduleManager* m_schedulerManager = nullptr;

  // the dht manager
  DHTManager* m_dhtManager = nullptr;

  // the trigger manager
  FunctionManagerRaw* m_frawmanager = nullptr;
  FunctionManagerMeta* m_fmetamanager = nullptr;
  DynamicTriggerManager* m_dtmanager = nullptr;

  // the metric manager
  MetricManager *m_metricManager = nullptr;


  void initManager(int globalProc, int globalrank, int metaServerNum, std::string memLimit,
    ClientForStaging* clientStage, tl::engine*globalEnginePtr, bool ifDistributed)
  {
    // init global managers
    m_addrManager = new AddrManager();

    // config the address manager
    m_addrManager->m_serverNum = globalProc;
    m_addrManager->m_metaServerNum = metaServerNum;

    // the global manager for raw data
    m_blockManager = new BlockManager(globalrank);

    m_schedulerManager = new ScheduleManager(memLimit);

    // the meta data service
    m_metaManager = new MetaDataManager();

    // the dht manager is global which should be inited before server
    m_dhtManager = new DHTManager();
    // dynamic trigger manager
    if (clientStage == NULL && ifDistributed == true)
    {
      throw std::runtime_error("the client should not be NULL for init func");
    }

    // TODO, control the thread number based on if the trigger is enabled
    // the number for the in-situ thread pool should be small for scale
    // 32 is some kind of maximum in this case, other wise, the write will too long
    m_dtmanager = new DynamicTriggerManager(64, m_metaManager, clientStage);

    // the dtmanager should associate with necessary managers
    m_fmetamanager = new FunctionManagerMeta();
    m_fmetamanager->m_dtm = m_dtmanager;
    m_dtmanager->m_funcmanagerMeta = m_fmetamanager;

    m_frawmanager = new FunctionManagerRaw();
    m_frawmanager->m_blockManager = m_blockManager;
    m_frawmanager->m_globalServerEnginePtr = globalEnginePtr;

    m_metricManager = new MetricManager(50);
  };

  // init the bulk at the server end
  tl::mutex m_bulkMapmutex;
  std::map<int, tl::bulk> m_bulkMap;
  //instead of keeping a map of the segment
  //it is convenient to keep a map of datacontainer
  //this map stores all memspace used for data transfering
  //this space is supposed to reused multiple times
  //tl::mutex m_dataContainerMapmutex;
  //make sure these two data structure have the same view, we use same lock here
  std::map<int, void*> m_dataContainerMap;
  

  ~UniServer()
  {
    // delete the m_dataContainerList
    // TODO there is double free issue here
    delete m_addrManager;
    delete m_blockManager;
    // TODO this coause the double free issue
    // delete m_metaManager;
    // delete m_fmetamanager;
    // delete m_dtmanager;
    // delete m_frawmanager;
  };
};
}
#endif