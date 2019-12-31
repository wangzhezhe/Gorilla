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
        {"masterAddr", s.masterAddr},
        {"datachecking", s.datachecking},
        {"loglevel", s.loglevel},
    };
}

void from_json(const nlohmann::json &j, Settings &s)
{
    j.at("dims").get_to(s.dims);
    j.at("metaserverNum").get_to(s.metaserverNum);
    j.at("maxDimValue").get_to(s.maxDimValue);
    j.at("protocol").get_to(s.protocol);
    j.at("masterAddr").get_to(s.masterAddr);
    j.at("datachecking").get_to(s.datachecking);
    j.at("loglevel").get_to(s.loglevel);
}

Settings::Settings()
{
    size_t dims = 3;
    size_t metaserverNum = 2;
    size_t maxDimValue = 8;
    std::string protocol = "tcp";
    std::string masterAddr = "./unimos_server.conf";
    bool datachecking = false;
    int loglevel = 0;
}

Settings Settings::from_json(const std::string &fname)
{
    std::ifstream ifs(fname);
    nlohmann::json j;

    ifs >> j;

    return j.get<Settings>();
}
