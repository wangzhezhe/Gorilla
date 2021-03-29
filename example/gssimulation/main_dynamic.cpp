#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <mpi.h>
#include <thread>

#include "InSitu.hpp"
#include "gray-scott.h"
#include "timer.hpp"

#include <chrono>
#include <stdio.h>
#include <thallium.hpp>
#include <time.h>
#include <unistd.h>

#define BILLION 1000000000L

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
using namespace std::chrono_literals;

const std::string masterConfigFile = "unimos_server.conf";
const std::string serverCred = "Gorila_cred_conf";

void print_settings(const Settings& s)
{
  std::cout << "grid:             " << s.L << "x" << s.L << "x" << s.L << std::endl;
  std::cout << "steps:            " << s.steps << std::endl;
  std::cout << "plotgap:          " << s.plotgap << std::endl;
  std::cout << "F:                " << s.F << std::endl;
  std::cout << "k:                " << s.k << std::endl;
  std::cout << "dt:               " << s.dt << std::endl;
  std::cout << "Du:               " << s.Du << std::endl;
  std::cout << "Dv:               " << s.Dv << std::endl;
  std::cout << "noise:            " << s.noise << std::endl;
  std::cout << "output:           " << s.output << std::endl;
  std::cout << "adios_config:     " << s.adios_config << std::endl;
}

void print_simulator_settings(const GrayScott& s)
{
  std::cout << "process layout:   " << s.npx << "x" << s.npy << "x" << s.npz << std::endl;
  std::cout << "local grid size:  " << s.size_x << "x" << s.size_y << "x" << s.size_z << std::endl;
}

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

  MPI_Init(&argc, &argv);
  int rank, procs, wrank;

  MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

  const unsigned int color = 1;
  MPI_Comm comm;
  MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &comm);

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &procs);

  if (argc < 4)
  {
    if (rank == 0)
    {
      std::cerr << "Too few arguments" << std::endl;
      std::cerr << "Usage: gray-scott settings.json protocol pattern" << std::endl;
    }
    MPI_Abort(MPI_COMM_WORLD, -1);
  }

  Settings settings = Settings::from_json(argv[1]);
  std::string protocol = argv[2];

  // variable related with dynamic thing
  std::string pattern = argv[3];
  double totalsavedTime = 0;
  bool ifTCAna = false;
  bool ifWriteToStage = false;
  bool ifdynamic = false;
  if (pattern == "baseline")
  {
    ifTCAna = true;
    ifWriteToStage = true;
  }
  else if (pattern == "alltightly")
  {
    ifTCAna = true;
  }
  else if (pattern == "allloosely")
  {
    ifWriteToStage = true;
  }
  else if (pattern.find("dynamic") != std::string::npos)
  {
    // we have the dynamicNaive and dynamicEstimation
    ifdynamic = true;
  }
  else
  {
    throw std::runtime_error("wrong pattern value");
  }

  GrayScott sim(settings, comm);
  sim.init();

  // Init the engine according to the protocol
  if (rank == 0)
  {
    std::cout << "--use protocol: " << protocol << std::endl;
  }
// assum that the serverCred is initilized by the data server service
#ifdef USE_GNI
  // get the drc id from the shared file
  std::ifstream infile(serverCred);
  std::string cred_id;
  std::getline(infile, cred_id);
  if (rank == 0)
  {
    std::cout << "load cred_id: " << cred_id << std::endl;
  }

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

  // margo_instance_id mid;
  // mid = margo_init_opt("gni", MARGO_CLIENT_MODE, &hii, 0, 1);
  // tl::engine globalclientEngine(mid);
  tl::engine globalclientEngine("ofi+gni", THALLIUM_CLIENT_MODE, false, 1, &hii);
#else

  tl::engine globalclientEngine(protocol, THALLIUM_CLIENT_MODE);
#endif

  std::string addrServer = loadMasterAddr(masterConfigFile);
  InSitu gsinsitu(&globalclientEngine, addrServer, rank, settings.steps);

  // writer_main.open(settings.output);

  if (rank == 0)
  {
    print_settings(settings);
    print_simulator_settings(sim);
    std::cout << "========================================" << std::endl;
  }

  Timer timer_total;
  Timer timer_compute;
  Timer timer_write;

  MPI_Barrier(comm);
  double wfStart = tl::timer::wtime();

  // std::ostringstream log_fname;
  // log_fname << "gray_scott_pe_" << rank << ".log";

  // std::ofstream log(log_fname.str());
  // log << "step\ttotal_gs\tcompute_gs\twrite_gs" << std::endl;

  bool useactualAna = true;
  std::string anatype = "";

  if (useactualAna == false)
  {
    anatype = getenv("ANATYPE");
    if (anatype == "")
    {
      throw std::runtime_error("ANATYPE should not be empty");
    }
  }

  if (rank == 0)
  {
    // start the timer explicitly
    gsinsitu.startwftimer();
  }

  for (int step = 0; step < settings.steps;)
  {
    MPI_Barrier(comm);
    double iterStart = tl::timer::wtime();

    for (int j = 0; j < settings.plotgap; j++)
    {

      MPI_Barrier(comm);
      double simStart = tl::timer::wtime();

      sim.iterate();

      // add synthetic execute time
      // std::this_thread::sleep_for(600ms);

      MPI_Barrier(comm);
      double simEnd = tl::timer::wtime();

      double simDiff = simEnd - simStart;
      std::string metricName = "S";
      gsinsitu.m_metricManager.putMetric(metricName, simDiff);

      if (rank == 0)
      {
        std::cout << "step " << step << " avg sim " << simDiff << std::endl;
      }
    }

    // Do the policy decision
    MPI_Barrier(comm);
    double dynamicStart = tl::timer::wtime();

    // TODO, do not get this data for all the client, which will slow down the server
    // on client get one and send to the sub comm
    // collect information from the staging service by calling getStageStatus
    // and put them into the current metric store
    std::vector<double> stageStatus =
      gsinsitu.m_uniclient->getStageStatus(gsinsitu.m_uniclient->m_associatedDataServer);

    // wait time (schedule time)
    std::string metricName = "W";
    gsinsitu.m_metricManager.putMetric(metricName, stageStatus[0]);

    // loosely coupled execution time
    metricName = "Al";
    gsinsitu.m_metricManager.putMetric(metricName, stageStatus[1]);

    if (ifdynamic)
    {
      gsinsitu.decideTaskPlacement(step, pattern, ifTCAna, ifWriteToStage);
    }

    MPI_Barrier(comm);
    double dynamicEnd = tl::timer::wtime();

    double decisionTime = dynamicEnd - dynamicStart;

    // if (rank == 0)
    //{
    // if we want to know all decisions for every process
    std::cout << "rank " << rank << " step " << step << " ifTCAna " << ifTCAna << " ifWriteToStage "
              << ifWriteToStage << " decision time " << decisionTime << std::endl;
    //}

    // TODO set time out mechanims here?
    // use async, when it longer then specific time, then let it go
    // if (decisionTime > 1.0)
    //{
    //  ifTCAna = true;
    //  ifWriteToStage = false;
    //}

    /*
    process the data by tightly coupled in-situ
    */
    if (ifTCAna)
    {
      // do not use comm since not all sim may decide the ifcAna
      double anaStart = tl::timer::wtime();
      if (useactualAna == true)
      {
        // try to do tightly coupled in-situ processing
        // iso surface extraction
        // it may takes long time for some processes more than 40 seconds
        // but only for particular step, it is weird
        //double anaStep0 = tl::timer::wtime();
        auto polydata = gsinsitu.getPoly(sim, 0.5, rank);
        //double anaStep1 = tl::timer::wtime();

        // caculate the largest region size
        gsinsitu.polyProcess(polydata, step);
        //double anaStep2 = tl::timer::wtime();

        //std::cout << "ana substep 1 " << anaStep1 - anaStep0 << " substep 2 " << anaStep2 - anaStep1
        //          << " iteration " << step << std::endl;
      }
      else
      {
        gsinsitu.dummyAna(step, settings.steps, anatype);
      }

      // clock_gettime(CLOCK_REALTIME, &anaend1);
      // double anadiff = (anaend1.tv_sec - anastart.tv_sec) * 1.0 +
      //  (anaend1.tv_nsec - anastart.tv_nsec) * 1.0 / BILLION;
      double anaEnd = tl::timer::wtime();
      // tightly coupleds
      std::string metricName = "At";
      double anaSpan = anaEnd - anaStart;
      gsinsitu.m_metricManager.putMetric(metricName, anaSpan);

      if (rank == 0)
      {
        // some process takes more then 40 seconds for first step, not sure the reason
        // jump out the first step
        std::cout << "step " << step << " rank " << rank << " anaTime: " << anaSpan << std::endl;
      }
    }

    /*
    write data to the staging service
    */
    if (ifWriteToStage)
    {
      // not use barrier, since not all process have the same decision
      // MPI_Barrier(comm);
      // struct timespec writestart, writeend;
      // double writediff;
      // clock_gettime(CLOCK_REALTIME, &writestart);
      double putStart = tl::timer::wtime();

      std::string VarNameU = "grascottu";
      gsinsitu.write(sim, VarNameU, step, rank);

      // when all write ok, trigger the staging process
      // call this when there is depedency between the in-situ task execution
      // MPI_Barrier(comm);
      // call the in-staging execution to trigger functions
      std::vector<std::string> funcPara;
      std::string funcName;

      if (useactualAna == true)
      {
        funcName = "testisoExec";
      }
      else
      {
        funcName = "dummyAna";
        funcPara.push_back(std::to_string(settings.steps));
        funcPara.push_back(anatype);
      }

      // make sure the parameter match with the functions specified at the server
      // the blockid equals to the rank in this case
      bool iflast = false;
      if (step >= (settings.steps - 3))
      {
        iflast = true;
      }

      gsinsitu.m_uniclient->executeAsyncExp(step, VarNameU, rank, funcName, funcPara, iflast);

      // clock_gettime(CLOCK_REALTIME, &writeend);
      // writediff = (writeend.tv_sec - writestart.tv_sec) * 1.0 +
      //  (writeend.tv_nsec - writestart.tv_nsec) * 1.0 / BILLION;
      //   MPI_Barrier(comm);
      double putEnd = tl::timer::wtime();
      double putDiff = putEnd - putStart;
      if (rank == 0)
      {
        std::cout << "step " << step << " put data: " << putDiff << std::endl;
      }
      std::string metricName = "T";
      gsinsitu.m_metricManager.putMetric(metricName, putDiff);
    }

    MPI_Barrier(comm);
    double iterEnd = tl::timer::wtime();

    if (rank == 0)
    {
      std::cout << "step " << step << " iter " << iterEnd - iterStart << std::endl;
    }
    // let he step records start from 0
    step++;
  }

  MPI_Barrier(comm);
  double wfEnd = tl::timer::wtime();
  if (rank == 0)
  {
    std::cout << "sim executiontime " << wfEnd - wfStart << std::endl;
    // both ana and sim set tick, compare the maximum one
    // gsinsitu.endwftimer();
    // TODO try to dump out the data in the metric monitor
  }

  // std::cout << "taotal saved time " << totalsavedTime << std::endl;

  gsinsitu.m_metricManager.dumpall(rank);

  // self terminate the client engine
  // just call the finalize
  // the shut down server is not desinged for self termination
  // TODO, tell the server that client is shut down if the client expose API
  // the server will not propagate info back
  // delete the current address from the associated server
  // globalclientEngine.finalize();

  MPI_Finalize();
}
