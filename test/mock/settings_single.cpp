#include <fstream>

#include "json.hpp"
#include "settings_single.h"

/*
    std::cout << "grid:             " << s.L << "x" << s.L << "x" << s.L << std::endl;
    std::cout << "steps:            " << s.steps << std::endl;
    std::cout << "plotgap:          " << s.plotgap << std::endl;
    std::cout << "cacutime:         " << s.dt << std::endl;
    std::cout << "output:           " << s.output << std::endl;
*/

void to_json(nlohmann::json &j, const SingleSettings &s)
{
    j = nlohmann::json{{"lenArray", s.lenArray},
                       {"steps", s.steps},
                       {"plotgap", s.plotgap},
                       {"calcutime", s.calcutime},
                       {"output", s.output},
                       {"processLayout", s.processLayout},
                       {"anaLayout", s.anaLayout}};
}

void from_json(const nlohmann::json &j, SingleSettings &s)
{
    j.at("lenArray").get_to(s.lenArray);
    j.at("steps").get_to(s.steps);
    j.at("plotgap").get_to(s.plotgap);
    j.at("calcutime").get_to(s.calcutime);
    j.at("output").get_to(s.output);
    j.at("processLayout").get_to(s.processLayout);
    j.at("anaLayout").get_to(s.anaLayout);
}

SingleSettings::SingleSettings()
{
    steps = 20000;
    plotgap = 200;
    calcutime = 1.0;
    output = "foo.bp";
}

SingleSettings SingleSettings::from_json(const std::string &fname)
{
    std::ifstream ifs(fname);
    nlohmann::json j;

    ifs >> j;

    return j.get<SingleSettings>();
}
