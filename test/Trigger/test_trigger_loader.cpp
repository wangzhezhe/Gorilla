
#include <fstream>
#include "json.hpp"
#include <iostream>
#include "../../commondata/metadata.h"

DynamicTriggerInfo *parseTrigger(nlohmann::json &j)
{
    std::string funcNameCheck = j.at("funcName_check");
    std::string funcNameCompare = j.at("funcName_compare");
    std::string funcNameAction = j.at("funcName_action");
                
    std::vector<std::string> ParametersCheck;
    std::vector<std::string> ParametersCompare;
    std::vector<std::string> ParametersAction;


    j.at("parameters_check").get_to(ParametersCheck);
    j.at("parameters_compare").get_to(ParametersCompare);
    j.at("parameters_action").get_to(ParametersAction);


    DynamicTriggerInfo* tgInfo = new DynamicTriggerInfo(funcNameCheck, ParametersCheck,
                               funcNameCompare, ParametersCompare,
                               funcNameAction, ParametersAction);
                               
    
    return tgInfo;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stdout, "<binary> <path of the json file>");
        exit(-1);
    }
    std::string fileName(argv[1]);
    std::ifstream ifs(fileName);
    nlohmann::json j;
    ifs >> j;

    //j.at("lenArray").get_to(s.lenArray);
    //iterate the array
    int i = 0;
    for (nlohmann::json::iterator it = j.begin(); it != j.end(); ++it)
    {
        std::cout << "array id " << i << std::endl;
        //std::cout << *it << '\n';
        i++;
        //transfer the content into the structure of the trigger
        DynamicTriggerInfo * dti = parseTrigger(*it);

        std::string triggerName = (*it).at("name_trigger");
        std::vector<int> lb;
        std::vector<int> ub;
        (*it).at("lb").get_to(lb);
        (*it).at("ub").get_to(ub);

        std::cout << "register trigger name: " << triggerName << std::endl;
        for(int i=0;i<lb.size();i++){
            std::cout << "dim " << i << std::endl;
            std::cout << "lb " << lb[i] << " ub " << ub[i] << std::endl;
        }

        dti->printInfo();

        //register the trigger to the server, the init trigger should be registered automatically
    }

    return 0;
}