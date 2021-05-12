#ifndef __INSITUVTKMADAPTOR_H__
#define __INSITUVTKMADAPTOR_H__

namespace GORILLA
{

// data analytics for sim data
// the common data processing functions that can be used both by tightly and loosely coupled way
// the class should be isolated with the data transfer things
class VTKMAdaptor
{
public:
  VTKMAdaptor(){};

  // try to integrate an vtkm kernel to see how it works
  void testclip();

  ~VTKMAdaptor(){};
};

}

#endif