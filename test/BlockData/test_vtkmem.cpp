#include "../../blockManager/blockManager.h"
#include "../../commondata/metadata.h"
#include <cstdlib>

// for sphere
#include <vtkCharArray.h>
#include <vtkCommunicator.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkXMLPolyDataWriter.h>

// refer to paraview/VTK/Parallel/Core/MarshalData

using namespace GORILLA;

void test_marshal_unmarshal()
{
  std::cout << "---test " << __FUNCTION__ << std::endl;
  // create sphere source
  vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->SetThetaResolution(8);
  sphereSource->SetPhiResolution(8);
  sphereSource->SetStartTheta(0.0);
  sphereSource->SetEndTheta(360.0);
  sphereSource->SetStartPhi(0.0);
  sphereSource->SetEndPhi(180.0);
  sphereSource->LatLongTessellationOff();

  sphereSource->Update();
  vtkSmartPointer<vtkPolyData> polyData = sphereSource->GetOutput();

  // marshal the vtk data into the string
  vtkSmartPointer<vtkCharArray> buffer = vtkSmartPointer<vtkCharArray>::New();
  bool oktoMarshal = vtkCommunicator::MarshalDataObject(polyData, buffer);

  if (oktoMarshal == false)
  {
    throw std::runtime_error("failed to marshal vtk data");
  }

  buffer->PrintSelf(std::cout, vtkIndent(5));

  // array is grouped into smaller subarrays with NumberOfComponents components.
  // These subarrays are called tuples, the array size is tuple size times the components size

  std::cout << "data type: " << buffer->GetDataType() << std::endl;

  // try to send the marshared vtkArray into the remote server
  // get the block summary, type, size, etc
  vtkIdType numTuples = buffer->GetNumberOfTuples();
  int numComponents = buffer->GetNumberOfComponents();

  size_t dataSize = numTuples * numComponents;

  std::cout << "numTuples " << numTuples << " numComponents " << numComponents << std::endl;

  // try to recv the data
  vtkSmartPointer<vtkCharArray> recvbuffer = vtkSmartPointer<vtkCharArray>::New();

  // set key properties, type, numTuples, numComponents, name,
  recvbuffer->SetNumberOfComponents(numComponents);
  recvbuffer->SetNumberOfTuples(numTuples);

  size_t recvSize = dataSize;

  // try to simulate the data recv process
  // when the memory of the vtkchar array is allocated???
  memcpy(recvbuffer->GetPointer(0), buffer->GetPointer(0), recvSize);

  std::cout << "------check recvbuffer content: ------" << std::endl;
  for (int i = 0; i < recvSize; i++)
  {
    std::cout << recvbuffer->GetValue(i);
  }
  std::cout << "------" << std::endl;

  // unmarshal
  vtkSmartPointer<vtkDataObject> recvbj = vtkCommunicator::UnMarshalDataObject(recvbuffer);

  std::cout << "---unmarshal object:" << std::endl;
  recvbj->PrintSelf(std::cout, vtkIndent(5));
}

void test_put(BlockManager& bm)
{
  std::cout << "---test " << __FUNCTION__ << std::endl;
  // create sphere source
  // Attention! do not use the smart pointer here to avoid the data is deleted by smart pointer
  vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
  // vtkSphereSource* sphereSource = vtkSphereSource::New();
  sphereSource->SetThetaResolution(8);
  sphereSource->SetPhiResolution(8);
  sphereSource->SetStartTheta(0.0);
  sphereSource->SetEndTheta(360.0);
  sphereSource->SetStartPhi(0.0);
  sphereSource->SetEndPhi(180.0);
  sphereSource->LatLongTessellationOff();

  sphereSource->Update();
  // vtkPolyData* polyData = sphereSource->GetOutput();
  vtkSmartPointer<vtkPolyData> polyData = sphereSource->GetOutput();
  std::string blockid = "vtktest";

  // give it to the smart pointer
  std::array<int, 3> indexlb = { { 0, 0, 0 } };
  std::array<int, 3> indexub = { { 10, 10, 10 } };

  // the both elem size and elem number is 1 useless for vtk data
  BlockSummary bs(1, 1, DATATYPE_VTKPTR, blockid, 3, indexlb, indexub);
  // the actual data is supposed to be created outof the block manager
  // and the block manager only keep the pointer to the object in this case
  bm.putBlock(bs, BACKEND::MEMVTKPTR, &polyData);
  return;
}

void test_get(BlockManager& bm)
{
  std::cout << "---test " << __FUNCTION__ << std::endl;

  std::string blockid = "vtktest";

  void* rawdataptr = NULL;

  bm.getBlock(blockid, BACKEND::MEMVTKPTR, rawdataptr);

  if (rawdataptr == NULL)
  {
    throw std::runtime_error("failed to get data");
  }
  vtkPolyData* polyData = (vtkPolyData*)rawdataptr;

  polyData->PrintSelf(std::cout, vtkIndent(5));
  return;
}

void test_erase(BlockManager& bm)
{

  std::cout << "---test " << __FUNCTION__ << std::endl;

  std::string blockid = "vtktest";

  bm.eraseBlock(blockid, BACKEND::MEMVTKPTR);

  // check that the destructor is called properly
}

int main()
{

  tl::abt scope;
  test_marshal_unmarshal();
  // test, when the object leave the scope it still works
  BlockManager bm;
  test_put(bm);
  test_get(bm);
  test_erase(bm);

  return 0;
}