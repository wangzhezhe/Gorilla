#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <mpi.h>
#include <thread>

#include "gray-scott.h"
#include "timer.hpp"
#include "writer.h"

#include <stdio.h>
#include <thallium.hpp>
#include <time.h>
#include <unistd.h>

#define BILLION 1000000000L
//#include "../putgetMeta/metaclient.h"

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

  margo_instance_id mid;
  mid = margo_init_opt("gni", MARGO_CLIENT_MODE, &hii, 0, 1);
  tl::engine globalclientEngine(mid);
#else

  tl::engine globalclientEngine(protocol, THALLIUM_CLIENT_MODE);
#endif

  // TODO broadcast
  /*
  char tempAddr[200];
  if (rank == 0)
  {
          std::string gorillaAddr;
      std::ifstream infile(masterConfigFile);
      std::getline(infile, gorillaAddr);
      std::cout << "--load the master server of the staging service:"
                << gorillaAddr << std::endl;

      memcpy(tempAddr, gorillaAddr.c_str(), strlen(gorillaAddr.c_str()));
  }
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Bcast(
      tempAddr,
      200,
      MPI_CHAR,
      0,
      MPI_COMM_WORLD)
  */

  Writer dataWriter(&globalclientEngine, rank);

  // writer_main.open(settings.output);

  if (rank == 0)
  {
    print_settings(settings);
    print_simulator_settings(sim);
    std::cout << "========================================" << std::endl;
  }

#ifdef ENABLE_TIMERS
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

#endif

   if (rank == 0)
  {
  // start the timer explicitly
    dataWriter.startwftimer();
  }

  for (int i = 0; i < settings.steps;)
  {

    for (int j = 0; j < settings.plotgap; j++)
    {
#ifdef ENABLE_TIMERS
      MPI_Barrier(comm);
      struct timespec iterstart, iterend;
      double iterdiff;
      clock_gettime(CLOCK_REALTIME, &iterstart); /* mark start time */
#endif
      sim.iterate();
      i++;

#ifdef ENABLE_TIMERS

      MPI_Barrier(comm);
      clock_gettime(CLOCK_REALTIME, &iterend); /* mark end time */
      iterdiff = (iterend.tv_sec - iterstart.tv_sec) * 1.0 +
        (iterend.tv_nsec - iterstart.tv_nsec) * 1.0 / BILLION;

      // char tempstr[200];
      // sprintf(tempstr,"step %d rank %d put %f\n",i,rank,diff);

      // std::cout << tempstr << std::endl;
      std::cout << "step " << i << " rank " << rank << " detailediter " << iterdiff << std::endl;

      // caculate the avg
      double sumiterdiff;
      MPI_Reduce(&iterdiff, &sumiterdiff, 1, MPI_DOUBLE, MPI_SUM, 0, comm);

      if (rank == 0)
      {
        std::cout << "step " << i << " avg iter " << sumiterdiff / procs << std::endl;
      }

#endif
    }

#ifdef ENABLE_TIMERS
    MPI_Barrier(comm);
    struct timespec start, end;
    double diff;
    clock_gettime(CLOCK_REALTIME, &start); /* mark start time */
#endif

    if (rank == 0)
    {
      std::cout << "Simulation at step " << i << " writing output step     " << i / settings.plotgap
                << std::endl;
    }

    size_t step = i;

    // send record to the metadata server
    // if (rank == 0)
    //{
    // start meta server when use this
    // MetaClient *metaclient = new MetaClient(&globalclientEngine);
    // std::string recordKey = "Trigger_" + std::to_string(step);
    // metaclient->Recordtime(recordKey);
    // dataWriter.write(sim, step, recordKey);
    //    dataWriter.write(sim, step);
    //}
    // else
    //{

    // bool ifStage = true;
    // int detectionTime = 0.5 * 1000;

    // percentage of the in-staging execution
    // if (step % 5 == 1)
    //{
    // bool ifStage = true;
    //}
    // else
    //{
    // execute the analytics
    //    std::this_thread::sleep_for(std::chrono::milliseconds(detectionTime));
    //}

    // if test the in-staging checking, all step is written into the staging service
    // if (ifStage)
    //{
    // write to the stage server
    //std::string blockSuffix = dataWriter.extractAndwrite(sim, step, rank);
    // assume all put operation finish
    //MPI_Barrier(comm);

    //if (rank == 0)
    //{
    //  dataWriter.triggerRemoteAsync(step, blockSuffix);
    //}
    //}

    //}
    // char countstr[50];
    // sprintf(countstr, "%03d_%04d", step, rank);
    // std::string fname = "./gsdataraw/vtkiso_" + std::string(countstr) + ".vti";
    // dataWriter.writeImageData(sim,fname);

#ifdef ENABLE_TIMERS

    MPI_Barrier(comm);
    clock_gettime(CLOCK_REALTIME, &end); /* mark end time */
    diff = (end.tv_sec - start.tv_sec) * 1.0 + (end.tv_nsec - start.tv_nsec) * 1.0 / BILLION;

    // char tempstr[200];
    // sprintf(tempstr,"step %d rank %d put %f\n",i,rank,diff);

    // std::cout << tempstr << std::endl;

    // caculate the avg
    double time_sum_write;
    MPI_Reduce(&diff, &time_sum_write, 1, MPI_DOUBLE, MPI_SUM, 0, comm);

    if (rank == 0)
    {
      std::cout << "step " << i << " avg write " << time_sum_write / procs << std::endl;
    }

#endif

    // if the inline engine is used, read data and generate the vtkm data here
    // the adis needed to be installed before using
  }

  clock_gettime(CLOCK_REALTIME, &wfend); /* mark end time */
  wfdiff =
    (wfend.tv_sec - wfstart.tv_sec) * 1.0 + (wfend.tv_nsec - wfstart.tv_nsec) * 1.0 / BILLION;

  if (rank == 0)
  {
    std::cout << "sim executiontime " << wfdiff << std::endl;
    // both ana and sim set tick, compare the maximum one
    // dataWriter.endwftimer();
  }
  MPI_Finalize();
}
