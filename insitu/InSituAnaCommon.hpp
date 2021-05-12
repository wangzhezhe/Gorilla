#ifndef __INSITUANACOMMON_H__
#define __INSITUANACOMMON_H__

#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#ifdef USE_GPU
#include "vtkmadaptor.hpp"
#endif

namespace GORILLA
{

// data analytics for sim data
// the common data processing functions that can be used both by tightly and loosely coupled way
// the class should be isolated with the data transfer things
class InSituAnaCommon
{
public:
  InSituAnaCommon(){};

  void polyProcess(vtkSmartPointer<vtkPolyData> polyData);
  void polyProcess2(vtkSmartPointer<vtkPolyData> polyData, std::string blockIDSuffix);

  void dummyAna(int step, int dataID, int totalStep, std::string anatype);

  ~InSituAnaCommon(){};
#ifdef USE_GPU
  // try to integrate an vtkm kernel to see how it works
  void testVTKm();
  VTKMAdaptor m_vtkmwrapper;
#endif
};

}

#endif