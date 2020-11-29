
#include <fstream>
#include "json.hpp"
#include <iostream>
#include "../../commondata/metadata.h"
#include "../../client/unimosclient.h"
#include <thallium.hpp>

#ifdef USE_GNI
extern "C"
{
#include <rdmacred.h>
}
#include <mercury.h>
#include <margo.h>
#define DIE_IF(cond_expr, err_fmt, ...)                                                                           \
    do                                                                                                            \
    {                                                                                                             \
        if (cond_expr)                                                                                            \
        {                                                                                                         \
            fprintf(stderr, "ERROR at %s:%d (" #cond_expr "): " err_fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
            exit(1);                                                                                              \
        }                                                                                                         \
    } while (0)

const std::string serverCred = "Gorila_cred_conf";
#endif

namespace tl = thallium;
using namespace GORILLA;

 GORILLA::DynamicTriggerInfo parseTrigger(nlohmann::json &j)
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

     GORILLA::DynamicTriggerInfo tgInfo(funcNameCheck, ParametersCheck,
                              funcNameCompare, ParametersCompare,
                              funcNameAction, ParametersAction);

    return tgInfo;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stdout, "<binary> <protocol> <path of the json file>");
        exit(-1);
    }

    std::string protocol(argv[1]);

#ifdef USE_GNI
    if (protocol.compare("gni") != 0)
    {
        throw std::runtime_error("set the USE_GNI as on for gni protocol");
    }
    //get the drc id from the shared file
    std::ifstream infile(serverCred);
    std::string cred_id;
    std::getline(infile, cred_id);

    std::cout << "load cred_id: " << cred_id << std::endl;

    struct hg_init_info hii;
    memset(&hii, 0, sizeof(hii));
    char drc_key_str[256] = {0};
    uint32_t drc_cookie;
    uint32_t drc_credential_id;
    drc_info_handle_t drc_credential_info;
    int ret;
    drc_credential_id = (uint32_t)atoi(cred_id.c_str());

    ret = drc_access(drc_credential_id, 0, &drc_credential_info);
    DIE_IF(ret != DRC_SUCCESS, "drc_access %u", drc_credential_id);
    drc_cookie = drc_get_first_cookie(drc_credential_info);

    sprintf(drc_key_str, "%u", drc_cookie);
    hii.na_init_info.auth_key = drc_key_str;
    //printf("use the drc_key_str %s\n", drc_key_str);

    margo_instance_id mid;
    mid = margo_init_opt("gni", MARGO_CLIENT_MODE, &hii, 0, -1);
    tl::engine globalclientEngine(mid);
#else
    tl::engine globalclientEngine(protocol, THALLIUM_CLIENT_MODE);
#endif

    //register the trigger to the server, the init trigger should be registered automatically

     GORILLA::UniClient *m_uniclient = new UniClient(&globalclientEngine, "unimos_server.conf", 0);
    m_uniclient->getAllServerAddr();

    std::string fileName(argv[2]);
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
         GORILLA::DynamicTriggerInfo dti = parseTrigger(*it);

        std::string triggerName = (*it).at("name_trigger");
        std::vector<int> lb;
        std::vector<int> ub;
        (*it).at("lb").get_to(lb);
        (*it).at("ub").get_to(ub);

        std::array<int, 3> indexlb;
        std::array<int, 3> indexub;

        std::cout << "register trigger name: " << triggerName << std::endl;
        for (int i = 0; i < lb.size(); i++)
        {
            std::cout << "dim " << i << std::endl;
            std::cout << "lb " << lb[i] << " ub " << ub[i] << std::endl;
            indexlb[i] = lb[i];
            indexub[i] = ub[i];
        }

        dti.printInfo();

        int dims = lb.size();
        //only the master server is necessary for the trigger
        std::string triggerMasterAddr = m_uniclient->registerTrigger(dims, indexlb, indexub, triggerName, dti);
        std::cout << "trigger master addr is: " << triggerMasterAddr << std::endl;
    }

    delete m_uniclient;

    return 0;
}