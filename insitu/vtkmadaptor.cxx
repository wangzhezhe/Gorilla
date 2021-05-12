// for vtkm

#include "vtkmadaptor.hpp"
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/filter/ClipWithField.h>
#include <vtkm/io/VTKDataSetWriter.h>
namespace GORILLA
{
void VTKMAdaptor::testclip()
{
  //int argc;
  //char* argv[] = { NULL };
  //vtkm::cont::Initialize(argc, argv, vtkm::cont::InitializeOptions::Strict);

  vtkm::cont::DataSet input = vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSetCowNose();

  vtkm::filter::ClipWithField clipFilter;
  clipFilter.SetActiveField("pointvar");
  clipFilter.SetClipValue(20.0);
  vtkm::cont::DataSet output = clipFilter.Execute(input);

  vtkm::io::VTKDataSetWriter writer("out_data.vtk");
  writer.WriteDataSet(output);
}
}