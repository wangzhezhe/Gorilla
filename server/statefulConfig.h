
#ifndef __STATEFUL_CONFIG__H__
#define __STATEFUL_CONFIG__H__

//#include "mpi.h"

#include <adios2.h>
#include <stdio.h>
#include <thallium.hpp>
#include <time.h>
#include <unistd.h>
#include <unordered_map>

#define BILLION 1000000000L

namespace tl = thallium;

struct statefulConfig
{
  statefulConfig(){
    // this->initADIOS();
  };
  void initADIOS()
  { // TODO, update this part
    // the adios object is deleted after this function
    if (this->Adios == nullptr)
    {
      this->Adios.reset(new adios2::ADIOS);
    }
    this->m_io = this->Adios->DeclareIO("gorilla_gs");
    // this->m_io.SetEngine("BP4");

    // define varaible, it doesn't matter about the start and count since it will be udpated by
    // setselection
    const std::string variableName = "data";
    this->m_io.DefineVariable<double>(
      variableName, adios2::Dims{ 512, 512, 512 }, adios2::Dims(), adios2::Dims());
    this->m_io.DefineVariable<int>("step");

    this->m_engine = m_io.Open("gorilla_gs.bp", adios2::Mode::Write);
    // the close engine should be called when every thing (file writing) finish

    // std::cout << "---debug adios io name in init: " <<  this->m_io.Name() << std::endl;
    // std::cout << "---debug adios engine type in init: " <<  this->m_engine.Type() << std::endl;
  }

  void startTimer(std::string timerName)
  {

    std::lock_guard<tl::mutex> lck(this->m_timerLock);

    if (this->m_timer_map.count(timerName) != 0)
    {
      throw std::runtime_error("the timer exist with name: " + timerName);
    }

    struct timespec start;
    clock_gettime(CLOCK_REALTIME, &start);
    this->m_timer_map[timerName] = start;
  }

  void endTimer(std::string timerName)
  {
    std::lock_guard<tl::mutex> lck(this->m_timerLock);

    if (this->m_timer_map.count(timerName) == 0)
    {
      throw std::runtime_error("the timer is not initilized with name: " + timerName);
    }

    struct timespec end;
    clock_gettime(CLOCK_REALTIME, &end);
    double timespan = (end.tv_sec - this->m_timer_map[timerName].tv_sec) * 1.0 +
      (end.tv_nsec - this->m_timer_map[timerName].tv_nsec) * 1.0 / BILLION;
    std::cout << timerName << " time span: " << timespan << std::endl;

    // delete the timer
    this->m_timer_map.erase(timerName);
    return;
  }

  ~statefulConfig(){};

  // adios info
  // refer to the implementation for this
  // https://gitlab.kitware.com/vtk/adis/-/blob/master/adis/DataSource.h
  std::unique_ptr<adios2::ADIOS> Adios = nullptr;
  adios2::IO m_io;
  adios2::Engine m_engine;
  // the variable is supposed to defined once per process
  std::unique_ptr<adios2::Variable<double> > var_u = nullptr;
  adios2::Variable<int> var_step;

  // this lock is used to avoid the race condition for data write between different thread in one
  // process refer to https://github.com/ornladios/ADIOS2/issues/2076#issuecomment-617925292 for
  // detials, adios is not fully supported by the multi thread version
  tl::mutex m_adiosLock;

  // timer info
  tl::mutex m_timerLock;
  std::unordered_map<std::string, struct timespec> m_timer_map;
};

#endif