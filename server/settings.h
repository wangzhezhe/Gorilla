#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <string>

struct Settings {
    std::string protocol;
    std::string masterAddr;
    bool datachecking;
    size_t dims;
    size_t metaserverNum;
    size_t maxDimValue;
    Settings();
    static Settings from_json(const std::string &fname);
};

#endif
