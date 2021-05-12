

#include <insitu/InSituAnaCommon.hpp>

using namespace GORILLA;

int main()
{
  InSituAnaCommon insituana;
  insituana.dummyAna(0, 0, 1, "S_LOW");
  std::cout << "ok to test the InSituAnaCommon" << std::endl;

  // test vtkm
#ifdef USE_GPU
  insituana.testVTKm();
#endif
  std::cout << "ok to test the testVTKm" << std::endl;
}