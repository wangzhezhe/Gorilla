#include <fstream>

#include "json.hpp"
#include "settings.h"

void to_json(nlohmann::json &j, const Settings &s)
{
    j = nlohmann::json{
        {"lenArray", s.lenArray},
        {"partitionMethod", s.partitionMethod},
        {"partitionLayout", s.partitionLayout},
        {"metaserverNum", s.metaserverNum},
        {"protocol", s.protocol},
        {"masterInfo", s.masterInfo},
        {"addTrigger", s.addTrigger},
        {"memLimit",s.memLimit},
        {"logLevel", s.logLevel},
    };
}

void from_json(const nlohmann::json &j, Settings &s)
{
    j.at("lenArray").get_to(s.lenArray);
    j.at("partitionMethod").get_to(s.partitionMethod);
    j.at("partitionLayout").get_to(s.partitionLayout);
    j.at("metaserverNum").get_to(s.metaserverNum);
    j.at("protocol").get_to(s.protocol);
    j.at("masterInfo").get_to(s.masterInfo);
    j.at("addTrigger").get_to(s.addTrigger);
    j.at("memLimit").get_to(s.memLimit);
    j.at("logLevel").get_to(s.logLevel);
}

Settings::Settings()
{
    std::string partitionMethod = "manual";
    size_t metaserverNum = 2;
    std::string protocol = "tcp";
    std::string masterInfo = "./unimos_server.conf";
    bool addTrigger = false;
    std::string memLimit = "1G";
    int logLevel = 0;
}

Settings Settings::from_json(const std::string &fname)
{
    std::ifstream ifs(fname);
    nlohmann::json j;

    ifs >> j;
    return j.get<Settings>();
}
