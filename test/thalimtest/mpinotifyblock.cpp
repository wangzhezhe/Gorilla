#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <thallium.hpp>

namespace tl = thallium;

static int tag = 123;

void notify(int i, int procNum)
{
    std::cout << "notify others for step " << i << std::endl;
    int number = 1;
    for (int dest = 0; dest < procNum; dest++)
    {
        MPI_Send(&number, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
    }
    return;
}

void mockwatcher(int p_num)
{
    int i = 0;
    while (i < 10)
    {
        notify(i, p_num);
        usleep(2000000);
        i++;
    }
}

main(int argc, char **argv)
{
    tl::abt scope;
    int p_rank;
    int p_num;
    MPI_Status status;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &p_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p_num);
    int i = 0;
    tl::managed<tl::xstream> es = tl::xstream::create();
    if (p_rank == 0)
    {

        es->make_thread([p_num]() {
            mockwatcher(p_num);
        });

        std::cout << "create thread for watching" << std::endl;
    }

    while (true)
    {
        int number;
        std::cout << "rank " << p_rank << " is waiting " << std::endl;

        MPI_Recv(&number, 1, MPI_INT, 0, tag, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        std::cout << "rank " << p_rank << " is doing sth" << std::endl;

        usleep(1000000);
    }

    //shut down
    MPI_Finalize();
}