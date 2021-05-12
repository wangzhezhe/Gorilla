#include <insitu/InSituTransferCommon.hpp>
#include <memory>
#include <string>
#include <thallium.hpp>
#include <vector>
#include <mpi.h>
// we still need the runtest.scripts to do this test since there still exists issues
// to run gni communication between different programs
#ifdef USE_GNI
extern "C"
{
#include <rdmacred.h>
}
#include <margo.h>
#include <mercury.h>
#define DIE_IF(cond_expr, err_fmt, ...)                                                            \
  do                                                                                               \
  {                                                                                                \
    if (cond_expr)                                                                                 \
    {                                                                                              \
      fprintf(stderr, "ERROR at %s:%d (" #cond_expr "): " err_fmt "\n", __FILE__, __LINE__,        \
        ##__VA_ARGS__);                                                                            \
      exit(1);                                                                                     \
    }                                                                                              \
  } while (0)
#endif

const std::string credFileName = "Gorila_cred_conf";

using namespace GORILLA;
namespace tl = thallium;

std::string loadMasterAddr(std::string masterConfigFile)
{

  std::ifstream infile(masterConfigFile);
  std::string content = "";
  std::getline(infile, content);
  // spdlog::debug("load master server conf {}, content -{}-", masterConfigFile,content);
  if (content.compare("") == 0)
  {
    std::getline(infile, content);
    if (content.compare("") == 0)
    {
      throw std::runtime_error("failed to load the master server\n");
    }
  }
  return content;
}

int main(int argc, char** argv)
{
  std::cout << "start to create the InSituTransferCommon" << std::endl;
  MPI_Init(&argc, &argv);
  int ret;
#ifdef USE_GNI
  drc_info_handle_t drc_credential_info;
  uint32_t drc_cookie;
  char drc_key_str[256] = { 0 };
  struct hg_init_info hii;
  memset(&hii, 0, sizeof(hii));
  uint32_t drc_credential_id;

  std::ifstream infile(credFileName);
  std::string cred_id;
  std::getline(infile, cred_id);
  std::cout << "load cred_id: " << cred_id << std::endl;
  drc_credential_id = (uint32_t)atoi(cred_id.c_str());

  /*
  int ret = ssg_group_id_load(g_ssg_file.c_str(), &num_addrs, &g_id);
  DIE_IF(ret != SSG_SUCCESS, "ssg_group_id_load");
  if (rank == 0)
  {
    std::cout << "get num_addrs: " << num_addrs << std::endl;
  }

  int64_t ssg_cred = ssg_group_id_get_cred(g_id);
  DIE_IF(ssg_cred == -1, "ssg_group_id_get_cred");
  */

  /* access credential and covert to string for use by mercury */
  ret = drc_access(drc_credential_id, 0, &drc_credential_info);
  DIE_IF(ret != DRC_SUCCESS, "drc_access %u %ld", drc_credential_id);
  drc_cookie = drc_get_first_cookie(drc_credential_info);
  sprintf(drc_key_str, "%u", drc_cookie);
  hii.na_init_info.auth_key = drc_key_str;

  margo_instance_id mid = margo_init_opt("gni", MARGO_CLIENT_MODE, &hii, true, 2);
  tl::engine engine(mid);

#else
  // Initialize the thallium server
  tl::engine engine("tcp", THALLIUM_CLIENT_MODE);

#endif

  // get the server addr by configfile
  // it is critical to ge the root addr server befor initing any other clients
  std::string addrServer = loadMasterAddr("unimos_server.conf");

  std::cout << "start to create the transferCommon" << std::endl;

  // this will call the server api to check the addr
  InSituTransferCommon transferCommon(&engine, addrServer, 0, 1);

  std::cout << "ok to create the InSituTransferCommon, finish test" << std::endl;
}