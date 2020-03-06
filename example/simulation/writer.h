#ifndef __WRITER_H__
#define __WRITER_H__

#include <mpi.h>

#include "gray-scott.h"
#include "settings.h"
#include <vector>
#include "../client/unimosclient.h"
#include "../../commondata/metadata.h"

namespace tl = thallium;

class Writer
{
public:
    Writer(tl::engine *clientEnginePtr, int rank)
    {
        m_uniclient = new UniClient(clientEnginePtr, "unimos_server.conf", rank);
        UniClientCache *uniCache = new UniClientCache();
        //the cache should be attached with the client firstly
        m_uniclient->m_uniCache = uniCache;
        m_uniclient->getAllServerAddr();
        m_uniclient->m_totalServerNum = uniCache->m_serverIDToAddr.size();

        //start the timer for the master server
        if (rank == 0)
        {
            registerRtrigger();
            m_uniclient->startTimer();
            std::cout << "ok to register the trigger and timer\n";

        }
    };

    void endwftimer(){
            registerRtrigger();
            m_uniclient->endTimer();
            std::cout << "end the  timer\n";
    };

    void writeImageData(const GrayScott &sim, std::string fileName);

    void write(const GrayScott &sim, size_t &step, std::string recordInfo = "");

    UniClient *m_uniclient = NULL;

    void registerRtrigger()
    {
        //add the init trigger
        std::string triggerNameInit = "InitTrigger";
        std::string triggerNameExp = "InsituTriggerExp";

        //declare the function and the parameters
        std::vector<std::string> initCheckParameters;
        std::vector<std::string> initComparisonParameters;
        std::vector<std::string> initActionParameters;

        initComparisonParameters.push_back("0");
        //start the trigger when the condition is satisfied
        initActionParameters.push_back(triggerNameExp);

        DynamicTriggerInfo initTgInfo(
            "defaultCheckGetStep", initCheckParameters,
            "defaultComparisonStep", initComparisonParameters,
            "defaultActionSartDt", initActionParameters);

        //register the trigger
        std::array<int, 3> indexlb = {{0, 0, 0}};
        std::array<int, 3> indexub = {{1287, 1287, 1287}};
        m_uniclient->registerTrigger(3, indexlb, indexub, triggerNameInit, initTgInfo);
        
        //how many seconds
        int anaTimeint = 0.25*1000000;
        std::string anaTime = std::to_string(anaTimeint);
        //declare the function and the parameters
        std::vector<std::string> checkParameters;
        std::vector<std::string> comparisonParameters;
        std::vector<std::string> actionParameters;

        checkParameters.push_back(anaTime);
        comparisonParameters.push_back("2");
        actionParameters.push_back("adiosWrite");

        //start the trigger when the condition is satisfied
        //InsituExpCheck is used to get step and execute the ana, input parameter
        //is the time for ana execution
        //InsituExpCompare is used to check the ana results
        //InsituExpAction is used to write the data by ADIOS
        DynamicTriggerInfo tgInfo(
            "InsituExpCheck", checkParameters,
            "InsituExpCompare", comparisonParameters,
            "InsituExpAction", actionParameters);

        //register the trigger
        //std::array<int, 3> indexlb = {{0, 0, 0}};
        //std::array<int, 3> indexub = {{1023, 1023, 1023}};
        m_uniclient->registerTrigger(3, indexlb, indexub, triggerNameExp, tgInfo);
    }

    ~Writer()
    {
        if (m_uniclient != NULL)
        {
            delete m_uniclient;
        }
    }
};

#endif
