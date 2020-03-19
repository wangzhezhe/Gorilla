
#ifndef __STATEFUL_CONFIG__H__
#define __STATEFUL_CONFIG__H__


#include "mpi.h"

#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <adios2.h>
#define BILLION 1000000000L

struct statefulConfig
{
        statefulConfig()
        {
                //this->initADIOS();
        };
        void initADIOS()
        {
                adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
                this->m_io = adios.DeclareIO("gorilla_gs");
                this->m_io.SetEngine("BP4");
                this->m_writer = m_io.Open("gorilla_gs", adios2::Mode::Write);
        }

        void initTimer()
        {
                clock_gettime(CLOCK_REALTIME, &m_global_start);
                this->ifTimerInit=true;
        }

        void endTimer(){
                if (ifTimerInit == false)
                {
                        throw std::runtime_error("the timer is not initilized");
                }

                struct timespec timestick;
                clock_gettime(CLOCK_REALTIME, &timestick);
                double timespan =
                    (timestick.tv_sec - m_global_start.tv_sec) * 1.0 +
                    (timestick.tv_nsec - m_global_start.tv_nsec) * 1.0 / BILLION;
                std::cout << "wf time end: " << timespan << std::endl;
                return;
        }

        void timeit()
        {
                if (ifTimerInit == false)
                {
                        throw std::runtime_error("the timer is not initilized");
                }

                struct timespec timestick;
                clock_gettime(CLOCK_REALTIME, &timestick);
                double timespan =
                    (timestick.tv_sec - m_global_start.tv_sec) * 1.0 +
                    (timestick.tv_nsec - m_global_start.tv_nsec) * 1.0 / BILLION;
                std::cout << "time tick: " << timespan << std::endl;

                return;
        }

        ~statefulConfig(){};

        //adios info
        adios2::IO m_io;
        adios2::Engine m_writer;

        //timer info
        bool ifTimerInit = false;
        struct timespec m_global_start;
};

#endif