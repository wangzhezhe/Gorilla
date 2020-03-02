#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>

#include <mpi.h>
#include <thread>

#include "timer.hpp"
#include "gray-scott.h"
#include "writer.h"

#include <time.h>
#include <stdio.h>
#include <unistd.h>

#define BILLION 1000000000L
//#include "../putgetMeta/metaclient.h"

void print_settings(const Settings &s)
{
    std::cout << "grid:             " << s.L << "x" << s.L << "x" << s.L
              << std::endl;
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

void print_simulator_settings(const GrayScott &s)
{
    std::cout << "process layout:   " << s.npx << "x" << s.npy << "x" << s.npz
              << std::endl;
    std::cout << "local grid size:  " << s.size_x << "x" << s.size_y << "x"
              << s.size_z << std::endl;
}

int main(int argc, char **argv)
{

    MPI_Init(&argc, &argv);
    int rank, procs, wrank;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    const unsigned int color = 1;
    MPI_Comm comm;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &comm);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &procs);

    if (argc < 2)
    {
        if (rank == 0)
        {
            std::cerr << "Too few arguments" << std::endl;
            std::cerr << "Usage: gray-scott settings.json" << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

#ifdef ENABLE_TIMERS
    MPI_Barrier(comm);
    struct timespec wfstart, wfend;
    clock_gettime(CLOCK_REALTIME, &wfstart); /* mark start time */
#endif

    Settings settings = Settings::from_json(argv[1]);
    GrayScott sim(settings, comm);
    sim.init();


    adios2::ADIOS adios(settings.adios_config, comm, adios2::DebugON);
    adios2::IO io_main = adios.DeclareIO("SimulationOutput");
    Writer writer_main(settings, sim, io_main);
    writer_main.open(settings.output);

    //anatime in ms
    int anaTime = 0.028 * 1000;

    if (rank == 0)
    {
        print_settings(settings);
        print_simulator_settings(sim);
        std::cout << "========================================" << std::endl;
    }

    for (int i = 0; i < settings.steps; i++)
    {

#ifdef ENABLE_TIMERS
        MPI_Barrier(comm);
        struct timespec simstart, simend;
        double simtime;
        clock_gettime(CLOCK_REALTIME, &simstart); /* mark start time */
#endif

        sim.iterate();

#ifdef ENABLE_TIMERS
        MPI_Barrier(comm);
        clock_gettime(CLOCK_REALTIME, &simend); /* mark start time */
        simtime = (simend.tv_sec - simstart.tv_sec) * 1.0 + (simend.tv_nsec - simstart.tv_nsec) * 1.0 / BILLION;
        double time_sum_sim;
        MPI_Reduce(&simtime, &time_sum_sim, 1, MPI_DOUBLE, MPI_SUM, 0, comm);

        if (rank == 0)
        {
            std::cout << "step " << i << " avg sim time: " << time_sum_sim / procs << std::endl;
        }
#endif

        if (rank == 0)
        {
            std::cout << "Simulation at step " << i
                      << " writing output step     " << i / settings.plotgap
                      << std::endl;
        }

        int step = i;

        //run ana
        std::this_thread::sleep_for(std::chrono::milliseconds(anaTime));

        bool ifQualified = false;

        //10 steps in total
        //if 20% data is interesting

        if (step % 5 == 0)
        {
            ifQualified = true;
        }

        //if 80% data is interesting
        //if (step % 5 > 0)
        //{
        //    ifQualified = true;
        //}

        if (ifQualified)
        {
           writer_main.write(step,sim);
        }
    }

#ifdef ENABLE_TIMERS
    MPI_Barrier(comm);
    clock_gettime(CLOCK_REALTIME, &wfend); /* mark start time */
    double wftime = (wfend.tv_sec - wfstart.tv_sec) * 1.0 + (wfend.tv_nsec - wfstart.tv_nsec) * 1.0 / BILLION;
    //caculate the avg wf time
    double time_sum_wf;
    MPI_Reduce(&wftime, &time_sum_wf, 1, MPI_DOUBLE, MPI_SUM, 0, comm);

    if (rank == 0)
    {
        std::cout << "whole wf time: " << time_sum_wf / procs << std::endl;
    }
#endif

    
    MPI_Finalize();
    return 0;
}
