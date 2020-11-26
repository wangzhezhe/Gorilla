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

unsigned WIDTH = 30;
unsigned HEIGHT = 30;
unsigned DEPTH = 30;
unsigned Globalpid = 0;

std::string diskdataDir = "./diskdata";
std::string rawvtkDir = "./rawvtkdata";

void fixMPICommSize(std::string scriptname, int total_block_number,
                    int totalstep) {
  int rank, nprocs;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  InSitu::MPIInitialize(scriptname);

  unsigned reminder = 0;
  if (total_block_number % nprocs != 0 && rank == (nprocs - 1)) {
    // the last process will process the reminder
    reminder = (total_block_number) % unsigned(nprocs);
  }
  // this value will vary when there is process join/leave
  const unsigned nblocks_per_proc = reminder + total_block_number / nprocs;

  int blockid_base = rank * nblocks_per_proc;
  std::vector<Mandelbulb> MandelbulbList;
  for (int i = 0; i < nblocks_per_proc; i++) {
    int blockid = blockid_base + i;
    int block_offset = blockid * DEPTH;
    MandelbulbList.push_back(Mandelbulb(WIDTH, HEIGHT, DEPTH, block_offset, 1.2,
                                        total_block_number));
  }

  for (int i = 0; i < totalstep; i++) {
    double t_start, t_end;
    {
      double order = 4.0 + ((double)i) * 8.0 / 100.0;
      MPI_Barrier(MPI_COMM_WORLD);
      t_start = MPI_Wtime();
      for (auto& mandelbulb : MandelbulbList) {
        mandelbulb.compute(order);
      }
      MPI_Barrier(MPI_COMM_WORLD);
      t_end = MPI_Wtime();
    }

    if (rank == 0) {
      std::cout << "Computation " << i << " completed in " << (t_end - t_start)
                << " seconds." << std::endl;
    }

    {
      MPI_Barrier(MPI_COMM_WORLD);
      t_start = MPI_Wtime();
      InSitu::MPICoProcessDynamic(MPI_COMM_WORLD, MandelbulbList,
                                  total_block_number, i, i);

      MPI_Barrier(MPI_COMM_WORLD);
      t_end = MPI_Wtime();

      if (rank == 0) {
        std::cout << "InSitu catalyst gridwriter" << i
                  << " process completed in " << (t_end - t_start)
                  << " seconds." << std::endl;
      }

      // MPI_Barrier(MPI_COMM_WORLD);
      // t_start = MPI_Wtime();

      // add other in-situ to see how it works
      // InSitu::isoExtraction(MandelbulbList, total_block_number);
      // MPI_Barrier(MPI_COMM_WORLD);
      // t_end = MPI_Wtime();

      // if (rank == 0)
      //{
      //  std::cout << "InSitu iso extraction " << i << " completed in " <<
      //  (t_end - t_start)
      //            << " seconds." << std::endl;
      //}

      MPI_Barrier(MPI_COMM_WORLD);
      t_start = MPI_Wtime();

      // add other in-situ to see how it works
      InSitu::sampleExtraction(MandelbulbList, total_block_number);
      MPI_Barrier(MPI_COMM_WORLD);
      t_end = MPI_Wtime();

      if (rank == 0) {
        std::cout << "InSitu resample extraction " << i << " completed in "
                  << (t_end - t_start) << " seconds." << std::endl;
      }

      MPI_Barrier(MPI_COMM_WORLD);
      t_start = MPI_Wtime();

      // add other in-situ to see how it works
      InSitu::sampleisoExtraction(MandelbulbList, total_block_number);
      MPI_Barrier(MPI_COMM_WORLD);
      t_end = MPI_Wtime();

      if (rank == 0) {
        std::cout << "InSitu sampleiso extraction " << i << " completed in "
                  << (t_end - t_start) << " seconds." << std::endl;
      }

      MPI_Barrier(MPI_COMM_WORLD);
      t_start = MPI_Wtime();

      char countstr[50];
      sprintf(countstr, "%02d_%04d", rank, i);
      std::string filename = "data" + std::string(countstr);

      // check existance of the dir, if write to dir
      /*
      struct stat buffer;
      if (rank == 0) {
        if (stat(diskdataDir.data(), &buffer) != 0) {
          throw std::runtime_error("failed to check the dir");
          return;
        }
      }
      InSitu::StoreBlockDisk(MandelbulbList, diskdataDir, filename);
      */
      MPI_Barrier(MPI_COMM_WORLD);
      t_end = MPI_Wtime();

      if (rank == 0) {
        std::cout << "InSitu " << i << " data transfer completed in "
                  << (t_end - t_start) << " seconds." << std::endl;
      }

      // std::string fileName = rawvtkDir + "/data_" + std::to_string(rank) +
      // "_" + std::to_string(i); InSitu::writeVTKData(MandelbulbList,
      // total_block_number, fileName);
    }
  }

  InSitu::Finalize();
}

void recordMetric(std::future<void> futureObj) {
  char command[512];
  sprintf(command,
          "/bin/bash "
          "/global/homes/z/zw241/cworkspace/src/mochi-vtk/example/"
          "MandelbulbCatalystExample/scripts/"
          "performance.sh %ld",
          Globalpid);
  while (futureObj.wait_for(std::chrono::milliseconds(1)) ==
         std::future_status::timeout) {
    // std::cout << "command: " << std::string(command) << std::endl;
    std::system(command);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  int globalrank;
  MPI_Comm_rank(MPI_COMM_WORLD, &globalrank);
  Globalpid = getpid();
  std::cout << " globalrank " << globalrank << " with Globalpid " << Globalpid
            << std::endl;

  if (argc != 5) {
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

  fixMPICommSize(scriptname, total_block_number, totalstep);

  MPI_Finalize();

  return 0;
}
