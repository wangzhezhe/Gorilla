#ifndef __SYN_SETTINGS_H__
#define __SYN_SETTINGS_H__

#include <string>
#include <vector>

struct SingleSettings {
    std::vector<int> lenArray;
    int steps;
    int plotgap;
    double calcutime;
    std::string output;
    std::vector<int> processLayout;
    std::vector<int> anaLayout;

    SingleSettings();
    static SingleSettings from_json(const std::string &fname);
};

#endif
