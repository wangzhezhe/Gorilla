#include <fstream>

#include "json.hpp"
#include "settings.h"

void to_json(nlohmann::json &j, const Settings &s)
{
    j = nlohmann::json{

        {"dims", s.dims},
        {"metaserverNum", s.metaserverNum},
        {"maxDimValue", s.maxDimValue},
        {"protocol", s.protocol},
        {"masterInfo", s.masterInfo},
        {"addTrigger", s.addTrigger},
        {"logLevel", s.logLevel},
    };
}

void from_json(const nlohmann::json &j, Settings &s)
{
    j.at("dims").get_to(s.dims);
    j.at("metaserverNum").get_to(s.metaserverNum);
    j.at("maxDimValue").get_to(s.maxDimValue);
    j.at("protocol").get_to(s.protocol);
    j.at("masterInfo").get_to(s.masterInfo);
    j.at("addTrigger").get_to(s.addTrigger);
    j.at("logLevel").get_to(s.logLevel);
}

Settings::Settings()
{
    size_t dims = 3;
    size_t metaserverNum = 2;
    size_t maxDimValue = 8;
    std::string protocol = "tcp";
    std::string masterInfo = "./unimos_server.conf";
    bool addTrigger = false;
    int logLevel = 0;
}

Settings Settings::from_json(const std::string &fname)
{
    std::ifstream ifs(fname);
    nlohmann::json j;

    ifs >> j;

    return j.get<Settings>();
}
