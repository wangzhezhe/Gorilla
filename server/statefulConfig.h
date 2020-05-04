
#ifndef __STATEFUL_CONFIG__H__
#define __STATEFUL_CONFIG__H__


//#include "mpi.h"

#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <adios2.h>
#include <thallium.hpp>

#define BILLION 1000000000L

namespace tl = thallium;

struct statefulConfig
{
        statefulConfig()
        {
                this->initADIOS();
        };
        void initADIOS()
        {       //TODO, update this part
                //the adios object is deleted after this function
                if(this->Adios==nullptr){
                    this->Adios.reset(new adios2::ADIOS);
                }
                this->m_io = this->Adios->DeclareIO("gorilla_gs");
                //this->m_io.SetEngine("BP4");


                //define varaible, it doesn't matter about the start and count since it will be udpated by setselection
                const std::string variableName = "data";
                this->m_io.DefineVariable<double>(variableName,adios2::Dims{512,512,512},adios2::Dims(), adios2::Dims());
                this->m_io.DefineVariable<int>("step");

                this->m_engine = m_io.Open("gorilla_gs.bp", adios2::Mode::Write);
                //the close engine should be called when every thing (file writing) finish
                
                std::cout << "---debug adios io name in init: " <<  this->m_io.Name() << std::endl;
                std::cout << "---debug adios engine type in init: " <<  this->m_engine.Type() << std::endl;
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
        //refer to the implementation for this https://gitlab.kitware.com/vtk/adis/-/blob/master/adis/DataSource.h
        std::unique_ptr<adios2::ADIOS> Adios = nullptr;
        adios2::IO m_io;
        adios2::Engine m_engine;
        //the variable is supposed to defined once per process
        std::unique_ptr<adios2::Variable<double>> var_u = nullptr;
        adios2::Variable<int> var_step;

        //this lock is used to avoid the race condition for data write between different thread in one process
        //refer to https://github.com/ornladios/ADIOS2/issues/2076#issuecomment-617925292 for detials
        tl::mutex m_adiosLock;


        //timer info
        bool ifTimerInit = false;
        struct timespec m_global_start;
};

#endif