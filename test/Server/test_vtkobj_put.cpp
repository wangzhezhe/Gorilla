
#include "../../client/unimosclient.h"
#include "../../commondata/metadata.h"
#include <thallium.hpp>
#include <vector>

// for sphere
#include <vtkCharArray.h>
#include <vtkCommunicator.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkXMLPolyDataWriter.h>

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

namespace tl = thallium;
using namespace GORILLA;

// assume that the server is alreasy started normally
void test_put_vtk(int pointNum)
{
  const std::string serverCred = "Gorila_cred_conf";
#ifdef USE_GNI
  // get the drc id from the shared file
  std::ifstream infile(serverCred);
  std::string cred_id;
  std::getline(infile, cred_id);

  std::cout << "load cred_id: " << cred_id << std::endl;

  struct hg_init_info hii;
  memset(&hii, 0, sizeof(hii));
  char drc_key_str[256] = { 0 };
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
  // printf("use the drc_key_str %s\n", drc_key_str);

  margo_instance_id mid;
  mid = margo_init_opt("gni", MARGO_CLIENT_MODE, &hii, 0, 1);
  tl::engine globalclientEngine(mid);
#else

  tl::engine globalclientEngine("tcp", THALLIUM_CLIENT_MODE);
#endif

  size_t elemSize = 1;
  size_t elemNum = 1;
  std::string dataType = DATATYPE_VTKPTR;
  size_t dims = 3;
  std::array<int, 3> indexlb = { { 0, 0, 0 } };
  std::array<int, 3> indexub = { { 10, 10, 10 } };
  std::string varName = "testVTK" + std::to_string(pointNum);
  std::string blockid = "block_0+"+ std::to_string(pointNum);

  // allocate vtk data
  vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->SetThetaResolution(pointNum);
  sphereSource->SetPhiResolution(pointNum);
  sphereSource->SetStartTheta(0.0);
  sphereSource->SetEndTheta(360.0);
  sphereSource->SetStartPhi(0.0);
  sphereSource->SetEndPhi(180.0);
  sphereSource->LatLongTessellationOff();

  sphereSource->Update();
  vtkSmartPointer<vtkPolyData> polyData = sphereSource->GetOutput();

  // generate raw data summary block
  BlockSummary bs(elemSize, elemNum, dataType, blockid, dims, indexlb, indexub);

  // generate raw data
  UniClient* uniclient = new UniClient(&globalclientEngine, "./unimos_server.conf", 0);
  // The server may do things in different order
  // We call getAllServerAddr separately
  uniclient->getAllServerAddr();
  uniclient->m_totalServerNum = uniclient->m_serverIDToAddr.size();

  int status = uniclient->putrawdata(0, varName, bs, polyData.GetPointer());
  if (status != 0)
  {
    throw std::runtime_error("failed to put data for step " + std::to_string(0));
  }
}

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    throw std::runtime_error("the point number should be added");
  }
  int pointnum = std::stoi(argv[1]);
  test_put_vtk(pointnum);
}