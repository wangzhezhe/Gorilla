#include <stdio.h>
#include <thallium.hpp>
#include <time.h>
#include <unistd.h>
#include <unordered_map>

#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkMarchingCubes.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkXMLImageDataReader.h>

#define BILLION 1000000000L

namespace tl = thallium;

struct TimerMap
{
  TimerMap(){};

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
    char str[256];

    sprintf(str, "%s time span: %lf existing task %d", timerName.data(), timespan,
      this->m_timer_map.size());
    // if (timerName.back() == '0')
    // if (timerName.substr(timerName.length() - 3) == "000")
    //{
    std::cout << std::string(str) << std::endl;
    //}

    // delete the timer
    this->m_timer_map.erase(timerName);
    return;
  }

  ~TimerMap(){};

  tl::mutex m_timerLock;
  std::unordered_map<std::string, struct timespec> m_timer_map;
};

// on cori, when the workload is 200 it takes around 0.5s
void taskDummy(int workLoad)
{
  // extra work
  for (int j = 0; j < workLoad; j++)
  {
    for (int j = 0; j < workLoad; j++)
    {
      for (int j = 0; j < workLoad; j++)
      {
        double temp = (j + 1) * 0.01 * 0.02 * 0.03 * 0.04 * 0.05;
        temp = (j + 2) * 0.01 * 0.02 * 0.03 * 0.04 * 0.05;
        temp = (j + 3) * 0.01 * 0.02 * 0.03 * 0.04 * 0.05;
        temp = (j + 4) * 0.01 * 0.02 * 0.03 * 0.04 * 0.05;
        temp = (j + 5) * 0.01 * 0.02 * 0.03 * 0.04 * 0.05;
        temp = (j + 6) * 0.01 * 0.02 * 0.03 * 0.04 * 0.05;
        temp = (j + 7) * 0.01 * 0.02 * 0.03 * 0.04 * 0.05;
        temp = (j + 8) * 0.01 * 0.02 * 0.03 * 0.04 * 0.05;
        temp = (j + 9) * 0.01 * 0.02 * 0.03 * 0.04 * 0.05;
        temp = (j + 10) * 0.01 * 0.02 * 0.03 * 0.04 * 0.05;
      }
    }
  }
}

void taskDummy2(double runtime)
{
  // record time
  struct timespec start, end;
  clock_gettime(CLOCK_REALTIME, &start);

  while (true)
  {
    double temp = 0.01 * 0.02 * 0.03 * 0.04 * 0.05 / 0.06;
    clock_gettime(CLOCK_REALTIME, &end);

    double timespan =
      (end.tv_sec - start.tv_sec) * 1.0 + (end.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
    if (timespan > runtime)
    {
      break;
    }
  }
}

void processImageData(vtkSmartPointer<vtkImageData> imageData, int workload)
{
  // do the marching cube
  // Run the marching cubes algorithm
  for (int j = 0; j < workload; j++)
  {
    auto mcubes = vtkSmartPointer<vtkMarchingCubes>::New();
    mcubes->SetInputData(imageData);
    mcubes->ComputeNormalsOn();
    mcubes->SetValue(0, 0.5);
    mcubes->Update();

    // caculate the number of polygonals
    vtkSmartPointer<vtkPolyData> polyData = mcubes->GetOutput();
  }
}

int main(int argc, char** argv)
{

  tl::abt scope;
  tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true, 8);

  tl::engine* myEnginePtr = &myEngine;
  // start the timer and check the schedule time
  TimerMap* tmptr = new TimerMap;

  if (argc != 3)
  {
    std::cout << "binary <taskNum> <workLoad>";
    return 0;
  }

  int taskNum = std::stoi(argv[1]);
  int workLoad = std::stoi(argv[2]);

  std::cout << "taskNum " << taskNum << " workLoad " << workLoad << std::endl;

  // load the image data for processing test
  std::string filename = "writeImageData/gsimage_0.vti";

  // Read all the data from the file
  vtkSmartPointer<vtkXMLImageDataReader> reader = vtkSmartPointer<vtkXMLImageDataReader>::New();
  reader->SetFileName(filename.c_str());
  reader->Update();

  // get the specific unstructureGridData and check the results
  vtkSmartPointer<vtkImageData> imageData = reader->GetOutput();

  // start multiple lambda functions to execute things
  for (int i = 0; i < taskNum; i++)
  {
    std::string timerNameSchedule = "schedule_" + std::to_string(i);
    std::string timerNameExec = "exec_" + std::to_string(i);

    tmptr->startTimer(timerNameSchedule);

    myEngine.get_handler_pool().make_thread(
      [myEnginePtr, tmptr, workLoad, timerNameSchedule, timerNameExec, imageData]() {
        // time it
        // taskDummy(workLoad);
        tmptr->endTimer(timerNameSchedule);
        tmptr->startTimer(timerNameExec);

        // processImageData(imageData, workLoad);
        taskDummy(workLoad);
        // taskDummy2(workLoad*1.0);
        tmptr->endTimer(timerNameExec);
      },
      tl::anonymous());

    // sleep this time and then put another task
    // tl::thread::sleep(myEngine, 100);
  }
}