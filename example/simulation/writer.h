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
        //the cache should be attached with the client firstly
        m_uniclient->getAllServerAddr();
        m_uniclient->m_totalServerNum = m_uniclient->m_serverIDToAddr.size();

        //start the timer for the master server
        if (rank == 0)
        {
            registerRtrigger(1);
            m_uniclient->startTimer();
            std::cout << "ok to register the trigger and timer\n";
        }
    };

    void endwftimer()
    {
        m_uniclient->endTimer();
        std::cout << "end the timer\n";
    };

    void writeImageData(const GrayScott &sim, std::string fileName);

    void write(const GrayScott &sim, size_t &step, std::string recordInfo = "");

    UniClient *m_uniclient = NULL;

    void registerRtrigger(int num)
    {
        //add the init trigger
        std::string triggerNameInit = "InitTrigger";

        //declare the function and the parameters
        std::vector<std::string> initCheckParameters;
        std::vector<std::string> initComparisonParameters;
        std::vector<std::string> initActionParameters;

        initComparisonParameters.push_back("0");

        //how many seconds
        int anaTimeint = 0 * 1000000;
        std::string anaTime = std::to_string(anaTimeint);
        //declare the function and the parameters
        std::vector<std::string> checkParameters;
        std::vector<std::string> comparisonParameters;
        std::vector<std::string> actionParameters;

        checkParameters.push_back(anaTime);
        comparisonParameters.push_back("0");
        actionParameters.push_back("adiosWrite");


        //register the trigger
        std::array<int, 3> indexlb = {{0, 0, 0}};
        std::array<int, 3> indexub = {{1287, 1287, 1287}};
        
        //register multiple in-staging executions
        for (int i = 0; i < num; i++)
        {
            std::string triggerNameExp = "InsituTriggerExp_" + std::to_string(i);
            initActionParameters.push_back(triggerNameExp);
            DynamicTriggerInfo tgInfo(
                "InsituExpCheck", checkParameters,
                "InsituExpCompare", comparisonParameters,
                "InsituExpAction", actionParameters);

            m_uniclient->registerTrigger(3, indexlb, indexub, triggerNameExp, tgInfo);
        }

        DynamicTriggerInfo initTgInfo(
            "defaultCheckGetStep", initCheckParameters,
            "defaultComparisonStep", initComparisonParameters,
            "defaultActionSartDt", initActionParameters);
        m_uniclient->registerTrigger(3, indexlb, indexub, triggerNameInit, initTgInfo);
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
