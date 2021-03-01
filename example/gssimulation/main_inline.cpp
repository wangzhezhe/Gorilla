#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <mpi.h>
#include <thread>
#include <vtkAppendFilter.h>
#include <vtkAppendPolyData.h>
#include <vtkCleanPolyData.h>
#include <vtkGeometryFilter.h>
#include <vtkMPI.h>
#include <vtkMPIController.h>
#include <vtkPolyData.h>
#include <vtkUnstructuredGrid.h>

#include "gray-scott.h"
#include "timer.hpp"
#include "InSitu.hpp"

#include <stdio.h>
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

  vtkMPIController* vtkcontroller = vtkMPIController::New();
  vtkcontroller->Initialize(&argc, &argv, 1);

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
  /*
  #ifdef USE_GNI
      //get the drc id from the shared file
      std::ifstream infile(serverCred);
      std::string cred_id;
      std::getline(infile, cred_id);
      if (rank == 0)
      {
          std::cout << "load cred_id: " << cred_id << std::endl;
      }

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
      mid = margo_init_opt("gni", MARGO_CLIENT_MODE, &hii, 0, 1);
      tl::engine globalclientEngine(mid);
  #else


      tl::engine globalclientEngine(protocol, THALLIUM_CLIENT_MODE);
  #endif
  */

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

  // InSitu gsinsitu(&globalclientEngine, rank);
  InSitu gsinsitu;
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
    // gsinsitu.write(sim, step, recordKey);
    //    gsinsitu.write(sim, step);
    //}
    // else
    //{

    // bool ifStage = true;

    // int anaTime = 5.0 * 1000;
    // int detectionTime = 3.0 * 1000;
    // bool ifAna = false;

    // std::this_thread::sleep_for(std::chrono::milliseconds(detectionTime));

    // if (step % 5 == 1 || step % 5 == 2 || step % 5 == 3)
    //{
    //    ifAna = true;
    //}

    // if (ifAna)
    //{
    //    if (rank == 0)
    //    {
    //        std::cout << "inline ana/vis for step " << step << std::endl;
    //    }
    // execute the analytics
    //    std::this_thread::sleep_for(std::chrono::milliseconds(anaTime));
    //}

    // if (ifStage)
    //{
    //    //write to the stage server
    //    gsinsitu.write(sim, step);
    //}
    // else
    //{
    // execute the analytics
    //    std::this_thread::sleep_for(std::chrono::milliseconds(anaTime));
    //}

    //}
    // gsinsitu.isosurfacePolyNum(sim, rank, 0.3, step);
    // gsinsitu.isosurfacePolyNum(sim, rank, 0.5, step);
    // gsinsitu.isosurfacePolyNum(sim, rank, 0.7, step);

    // get poly time
    MPI_Barrier(comm);
    struct timespec getpolys, getpolye;
    double polydiff;
    clock_gettime(CLOCK_REALTIME, &getpolys); /* mark start time */

    //auto polydata = gsinsitu.getPoly(sim, 0.5);
    // std::cout << "cell number before transfer: " << polydata->GetNumberOfPolys() << std::endl;

    MPI_Barrier(comm);
    clock_gettime(CLOCK_REALTIME, &getpolye); /* mark start time */
    polydiff = (getpolye.tv_sec - getpolys.tv_sec) * 1.0 +
      (getpolye.tv_nsec - getpolys.tv_nsec) * 1.0 / BILLION;

    double sumpolydiff;
    MPI_Reduce(&polydiff, &sumpolydiff, 1, MPI_DOUBLE, MPI_SUM, 0, comm);

    if (rank == 0)
    {
      std::cout << "step " << i << " avg extract poly " << sumpolydiff / procs << std::endl;
    }

    // aggregate
    if (i == 10)
    {
      // output data for testing
      std::string fName = "writeImageData/gsimage_" + std::to_string(rank) + ".vti";
      gsinsitu.writeImageData(sim, fName);
    }
    // int Gather(vtkDataObject* sendBuffer, std::vector<vtkSmartPointer<vtkDataObject>>&
    // recvBuffer, int destProcessId)
    // init the vtkcontroller

    // gather time
    /*
    std::vector<vtkSmartPointer<vtkDataObject> > recvlist;
    int status = vtkcontroller->Gather(polydata, recvlist, 0);
    if (!status)
    {
      throw std::runtime_error("failed to gather poly data");
    }

    
    if (rank == 0)
    {
      // std::cout << "size of recvlist: " << recvlist.size() << std::endl;
      // other process time
      // merge the vtk
      // Append the two meshes
      // vtkSmartPointer<vtkAppendFilter> appendFilter = vtkSmartPointer<vtkAppendFilter>::New();
      vtkSmartPointer<vtkAppendPolyData> appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();
      for (int i = 0; i < recvlist.size(); i++)
      {
        // refer to https://vtk.org/Wiki/VTK/Examples/Cxx/PolyData/GeometryFilter
        vtkSmartPointer<vtkGeometryFilter> tempgeometryFilter =
          vtkSmartPointer<vtkGeometryFilter>::New();

        // transfer unstructure grid to polydata
        // refer to https://vtk.org/Wiki/VTK/Examples/Cxx/PolyData/GeometryFilter
        tempgeometryFilter->SetInputData(recvlist[i]);
        tempgeometryFilter->Update();
        vtkSmartPointer<vtkPolyData> tempPoly = tempgeometryFilter->GetOutput();
        // std::cout << "recv cell number for: " << i << " is " << tempPoly->GetNumberOfPolys()
        //          << std::endl;

        // transfer to poly to check the results
        appendFilter->AddInputData(tempPoly);
      }
      // appendFilter->Update();
      // vtkSmartPointer<vtkPolyData> output = appendFilter->GetOutput();
      // std::cout << "appended cell " << output->GetNumberOfPolys() << std::endl;

      // Remove any duplicate points.
      vtkSmartPointer<vtkCleanPolyData> cleanFilter = vtkSmartPointer<vtkCleanPolyData>::New();
      cleanFilter->SetInputConnection(appendFilter->GetOutputPort());
      cleanFilter->Update();

      // generate the merged polydata
      vtkSmartPointer<vtkPolyData> mergedPolydata = cleanFilter->GetOutput();

      // other processes based on polyonal
      gsinsitu.polyProcess(mergedPolydata, step);

      // for testing, try to write out the data
      // to see the size of processed data after in-situ extraction
      std::string fName = "reduced_" + std::to_string(step)+".vtp";
      gsinsitu.writePolyDataFile(mergedPolydata, fName);
    }
    */

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
      std::cout << "step " << i << " avg insitu execution " << time_sum_write / procs << std::endl;
    }

    // char countstr[50];
    // sprintf(countstr, "%03d_%04d", step, rank);
    // std::string fname = "./gsdataraw/vtkiso_" + std::string(countstr) + ".vti";
    // gsinsitu.writeImageData(sim, fname);

#endif

    // if the inline engine is used, read data and generate the vtkm data here
    // the adis needed to be installed before using
  }

  clock_gettime(CLOCK_REALTIME, &wfend); /* mark end time */
  wfdiff =
    (wfend.tv_sec - wfstart.tv_sec) * 1.0 + (wfend.tv_nsec - wfstart.tv_nsec) * 1.0 / BILLION;
  if (rank < 10)
  {
    std::cout << "wf time " << wfdiff << std::endl;
    // gsinsitu.endwftimer();
  }
  MPI_Finalize();
}
