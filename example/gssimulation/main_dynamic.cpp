#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <mpi.h>
#include <thread>

#include "InSitu.hpp"
#include "gray-scott.h"
#include "timer.hpp"

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

  if (argc < 3)
  {
    if (rank == 0)
    {
      std::cerr << "Too few arguments" << std::endl;
      std::cerr << "Usage: gray-scott settings.json protocol" << std::endl;
    }
    MPI_Abort(MPI_COMM_WORLD, -1);
  }

  Settings settings = Settings::from_json(argv[1]);
  std::string protocol = argv[2];

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
  InSitu gsinsitu(&globalclientEngine, addrServer, rank);

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
  struct timespec wfstart, wfend;
  double wfdiff;
  clock_gettime(CLOCK_REALTIME, &wfstart); /* mark start time */

  // std::ostringstream log_fname;
  // log_fname << "gray_scott_pe_" << rank << ".log";

  // std::ofstream log(log_fname.str());
  // log << "step\ttotal_gs\tcompute_gs\twrite_gs" << std::endl;

  if (rank == 0)
  {
    // start the timer explicitly
    gsinsitu.startwftimer();
  }

  for (int step = 0; step < settings.steps;)
  {

    for (int j = 0; j < settings.plotgap; j++)
    {

      MPI_Barrier(comm);
      struct timespec iterstart, iterend;
      double iterdiff;
      clock_gettime(CLOCK_REALTIME, &iterstart); /* mark start time */
      sim.iterate();
      step++;

      MPI_Barrier(comm);
      clock_gettime(CLOCK_REALTIME, &iterend); /* mark end time */
      iterdiff = (iterend.tv_sec - iterstart.tv_sec) * 1.0 +
        (iterend.tv_nsec - iterstart.tv_nsec) * 1.0 / BILLION;

      // char tempstr[200];
      // sprintf(tempstr,"step %d rank %d put %f\n",i,rank,diff);

      // std::cout << tempstr << std::endl;
      //std::cout << "step " << step << " rank " << rank << " detailed iter " << iterdiff
      //          << std::endl;
      // put data into the monitor
      // simulation
      std::string metricName = "S";
      gsinsitu.m_metricManager.putMetric(metricName, iterdiff);

      // caculate the avg
      double sumiterdiff;
      MPI_Reduce(&iterdiff, &sumiterdiff, 1, MPI_DOUBLE, MPI_SUM, 0, comm);

      if (rank == 0)
      {
        std::cout << "step " << step << " avg iter " << sumiterdiff / procs << std::endl;
      }
    }

    // TODO use the policy decision process to decide the following behaviours
    // For the baseline case, both the tightly coupled and loosely coupled is executed
    // TODO
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

    // Do the policy decision

    bool ifTCAna = false;
    bool ifWriteToStage = false;

    if (step <= 2)
    {
      ifTCAna = true;
    }
    else if (step == 3)
    {
      ifWriteToStage = true;
    }
    else
    {
      // we have all avalible data try to use policy
      double T = gsinsitu.m_metricManager.getLastNmetrics("T", 1)[0];
      double At = gsinsitu.m_metricManager.getLastNmetrics("At", 1)[0];
      double S = gsinsitu.m_metricManager.getLastNmetrics("S", 1)[0];
      double Al = gsinsitu.m_metricManager.getLastNmetrics("Al", 1)[0];
      double W = gsinsitu.m_metricManager.getLastNmetrics("W", 1)[0];
      if (S >= (W + Al))
      {
        if (At >= T)
        {
          ifWriteToStage = true;
        }
        else
        {
          ifTCAna = true;
        }
      }
      else
      {
        if (At + S >= (T + W + Al))
        {
          ifWriteToStage = true;
        }
        else
        {
          ifTCAna = true;
        }
      }
    }
    if (rank == 0)
    {
      std::cout << "step " << step << " ifTCAna " << ifTCAna << " ifWriteToStage " << ifWriteToStage
                << std::endl;
    }

    /*
    process the data by tightly coupled in-situ
    */
    if (ifTCAna)
    {
      MPI_Barrier(comm);
      // struct timespec anastart, anaend1, anaend2;
      // double anadiff1, anadiff2;

      // clock_gettime(CLOCK_REALTIME, &anastart);

      double anaStart = tl::timer::wtime();

      // try to do tightly coupled in-situ processing
      // iso surface extraction
      auto polydata = gsinsitu.getPoly(sim, 0.5);
      // caculate the largest region size
      gsinsitu.polyProcess(polydata, step);

      // clock_gettime(CLOCK_REALTIME, &anaend2);
      // anadiff2 = (anaend2.tv_sec - anastart.tv_sec) * 1.0 +
      //  (anaend2.tv_nsec - anastart.tv_nsec) * 1.0 / BILLION;
      double anaEnd = tl::timer::wtime();

      // tightly coupled
      std::string metricName = "At";
      double anaSpan = anaEnd - anaStart;
      gsinsitu.m_metricManager.putMetric(metricName, anaSpan);
      //std::cout << "debug ana diff : " << anaSpan << std::endl;
    }

    /*
    write data to the staging service
    */
    if (ifWriteToStage)
    {
      if (rank == 0)
      {
        std::cout << "Simulation at step " << step << " put data for step "
                  << step / settings.plotgap << std::endl;
      }

      MPI_Barrier(comm);
      struct timespec writestart, writeend;
      double writediff;
      clock_gettime(CLOCK_REALTIME, &writestart);

      std::string VarNameU = "grascott_u";
      gsinsitu.write(sim, VarNameU, step, rank);

      // when all write ok, trigger the staging process
      // call this when there is depedency between the in-situ task execution
      // MPI_Barrier(comm);
      // call the in-staging execution to trigger functions
      std::string funcName = "testisoExec";
      // make sure the parameter match with the functions specified at the server
      // the blockid equals to the rank in this case

      gsinsitu.m_uniclient->executeAsyncExp(step, VarNameU, rank, funcName);

      clock_gettime(CLOCK_REALTIME, &writeend);
      writediff = (writeend.tv_sec - writestart.tv_sec) * 1.0 +
        (writeend.tv_nsec - writestart.tv_nsec) * 1.0 / BILLION;
      std::string metricName = "T";
      gsinsitu.m_metricManager.putMetric(metricName, writediff);
    }
  }

  clock_gettime(CLOCK_REALTIME, &wfend); /* mark end time */
  wfdiff =
    (wfend.tv_sec - wfstart.tv_sec) * 1.0 + (wfend.tv_nsec - wfstart.tv_nsec) * 1.0 / BILLION;

  if (rank == 0)
  {
    std::cout << "sim executiontime " << wfdiff << std::endl;
    // both ana and sim set tick, compare the maximum one
    // gsinsitu.endwftimer();
    // TODO try to dump out the data in the metric monitor
  }

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
