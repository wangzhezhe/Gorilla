#include "Mandelbulb_dynamic.hpp"

#include <mpi.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <chrono>
#include <future>
#include <iostream>
#include <thread>

#include "InSituAdaptor.hpp"
#define BILLION 1000000000L

#include "writer.hpp"

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



std::string diskdataDir = "./diskdata";
std::string rawvtkDir = "./rawvtkdata";
const std::string serverCred = "Gorila_cred_conf";

void fixMPICommSize(
  std::string scriptname, int total_block_number, int totalstep, Writer& gorillaWriter)
{
  int rank, nprocs;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  InSitu::MPIInitialize(scriptname);

  unsigned reminder = 0;
  if (total_block_number % nprocs != 0 && rank == (nprocs - 1))
  {
    // the last process will process the reminder
    reminder = (total_block_number) % unsigned(nprocs);
  }
  // this value will vary when there is process join/leave
  const unsigned nblocks_per_proc = reminder + total_block_number / nprocs;

  int blockid_base = rank * nblocks_per_proc;
  std::vector<Mandelbulb> MandelbulbList;
  for (int i = 0; i < nblocks_per_proc; i++)
  {
    int blockid = blockid_base + i;
    int block_offset = blockid * DEPTH;
    MandelbulbList.push_back(
      Mandelbulb(WIDTH, HEIGHT, DEPTH, block_offset, 1.2, total_block_number));
  }

  for (int i = 0; i < totalstep; i++)
  {
    double t_start, t_end;
    {
      double order = 4.0 + ((double)i) * 8.0 / 100.0;
      MPI_Barrier(MPI_COMM_WORLD);
      t_start = MPI_Wtime();
      for (auto& mandelbulb : MandelbulbList)
      {
        mandelbulb.compute(order);
      }
      MPI_Barrier(MPI_COMM_WORLD);
      t_end = MPI_Wtime();
    }

    if (rank == 0)
    {
      std::cout << "Computation " << i << " completed in " << (t_end - t_start) << " seconds."
                << std::endl;
    }

    {
      MPI_Barrier(MPI_COMM_WORLD);
      t_start = MPI_Wtime();

      gorillaWriter.writetoStaging(i, MandelbulbList, blockid_base, total_block_number);

      MPI_Barrier(MPI_COMM_WORLD);
      t_end = MPI_Wtime();

      if (rank == 0)
      {
        std::cout << "InSitu " << i << " data transfer completed in " << (t_end - t_start)
                  << " seconds." << std::endl;
      }
    }
  }

  InSitu::Finalize();
}

void recordMetric(std::future<void> futureObj)
{
  char command[512];
  sprintf(command,
    "/bin/bash "
    "/global/homes/z/zw241/cworkspace/src/mochi-vtk/example/"
    "MandelbulbCatalystExample/scripts/"
    "performance.sh %ld",
    Globalpid);
  while (futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
  {
    // std::cout << "command: " << std::string(command) << std::endl;
    std::system(command);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
}

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  int globalrank;
  MPI_Comm_rank(MPI_COMM_WORLD, &globalrank);
  Globalpid = getpid();
  std::cout << " globalrank " << globalrank << " with Globalpid " << Globalpid << std::endl;

  if (argc != 5)
  {
    std::cerr << "Usage: " << argv[0]
              << " <script.py> <total_blocks_num> "
                 "<blockLength> <totalstep> "
              << std::endl;
    exit(0);
  }

  std::string scriptname = argv[1];
  int total_block_number = std::stoi(argv[2]);
  int blockLength = std::stoi(argv[3]);
  int totalstep = std::stoi(argv[4]);

  WIDTH = blockLength;
  HEIGHT = blockLength;
  DEPTH = blockLength;

#ifdef USE_GNI
  // get the drc id from the shared file
  std::ifstream infile(serverCred);
  std::string cred_id;
  std::getline(infile, cred_id);
  if (globalrank == 0)
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

  Writer dataWriter(&globalclientEngine, globalrank);

  fixMPICommSize(scriptname, total_block_number, totalstep, dataWriter);

  MPI_Finalize();

  return 0;
}
