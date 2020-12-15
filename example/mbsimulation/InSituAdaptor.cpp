#include "InSituAdaptor.hpp"

#include <mpi.h>
#include <sys/stat.h>
#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkCPPythonScriptPipeline.h>
#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkCommunicator.h>
#include <vtkFloatArray.h>
#include <vtkIceTContext.h>
#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkImageReslice.h>
#include <vtkIntArray.h>
#include <vtkMPI.h>
#include <vtkMPICommunicator.h>
#include <vtkMPIController.h>
#include <vtkMarchingCubes.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiPieceDataSet.h>
#include <vtkMultiProcessController.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkXMLImageDataWriter.h>

#include <fstream>
#include <iostream>

namespace
{
vtkMultiProcessController* Controller = nullptr;
vtkCPProcessor* Processor = nullptr;
vtkMultiBlockDataSet* VTKGrid;


// one process generates one data object
void BuildVTKGrid(Mandelbulb& grid, int nprocs, int rank)
{
  int* extents = grid.GetExtents();
  vtkNew<vtkImageData> imageData;
  imageData->SetSpacing(1.0 / nprocs, 1, 1);
  imageData->SetExtent(extents);
  imageData->SetOrigin(grid.GetOrigin()); // Not necessary for (0,0,0)
  vtkNew<vtkMultiPieceDataSet> multiPiece;
  multiPiece->SetNumberOfPieces(nprocs);
  multiPiece->SetPiece(rank, imageData.GetPointer());
  VTKGrid->SetNumberOfBlocks(1);
  VTKGrid->SetBlock(0, multiPiece.GetPointer());
}

// one process generates multiple data objects
void BuildVTKGridList(std::vector<Mandelbulb>& gridList, int global_blocks)
{
  int local_piece_num = gridList.size();
  vtkNew<vtkMultiPieceDataSet> multiPiece;
  multiPiece->SetNumberOfPieces(local_piece_num);

  for (int i = 0; i < local_piece_num; i++)
  {
    int* extents = gridList[i].GetExtents();
    vtkNew<vtkImageData> imageData;
    imageData->SetSpacing(1.0 / global_blocks, 1, 1);
    imageData->SetExtent(extents);
    imageData->SetOrigin(gridList[i].GetOrigin());
    multiPiece->SetPiece(i, imageData.GetPointer());
  }

  // one block conains one multipiece, one multipiece contains multiple actual
  // data objects
  VTKGrid->SetNumberOfBlocks(1);
  VTKGrid->SetBlock(0, multiPiece.GetPointer());
}

void UpdateVTKAttributes(Mandelbulb& mandelbulb, int rank, vtkCPInputDataDescription* idd)
{
  vtkMultiPieceDataSet* multiPiece = vtkMultiPieceDataSet::SafeDownCast(VTKGrid->GetBlock(0));
  if (idd->IsFieldNeeded("mandelbulb", vtkDataObject::POINT))
  {
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(multiPiece->GetPiece(rank));
    if (dataSet->GetPointData()->GetNumberOfArrays() == 0)
    {
      // pressure array
      vtkNew<vtkIntArray> data;
      data->SetName("mandelbulb");
      data->SetNumberOfComponents(1);
      dataSet->GetPointData()->AddArray(data.GetPointer());
    }
    vtkIntArray* data = vtkIntArray::SafeDownCast(dataSet->GetPointData()->GetArray("mandelbulb"));
    // The pressure array is a scalar array so we can reuse
    // memory as long as we ordered the points properly.
    int* theData = mandelbulb.GetData();
    data->SetArray(theData, static_cast<vtkIdType>(mandelbulb.GetNumberOfLocalCells()), 1);
  }
}

void UpdateVTKAttributesList(
  std::vector<Mandelbulb>& mandelbulbList, vtkCPInputDataDescription* idd)
{
  int pieceNum = mandelbulbList.size();
  vtkMultiPieceDataSet* multiPiece = vtkMultiPieceDataSet::SafeDownCast(VTKGrid->GetBlock(0));
  if (idd->IsFieldNeeded("mandelbulb", vtkDataObject::POINT))
  {
    for (int i = 0; i < pieceNum; i++)
    {
      vtkDataSet* dataSet = vtkDataSet::SafeDownCast(multiPiece->GetPiece(i));
      if (dataSet->GetPointData()->GetNumberOfArrays() == 0)
      {
        // pressure array
        vtkNew<vtkIntArray> data;
        data->SetName("mandelbulb");
        data->SetNumberOfComponents(1);
        dataSet->GetPointData()->AddArray(data.GetPointer());
      }
      vtkIntArray* data =
        vtkIntArray::SafeDownCast(dataSet->GetPointData()->GetArray("mandelbulb"));
      // The pressure array is a scalar array so we can reuse
      // memory as long as we ordered the points properly.
      // std::cout << "set actual value for piece " << i << std::endl;
      int* theData = mandelbulbList[i].GetData();
      data->SetArray(theData, static_cast<vtkIdType>(mandelbulbList[i].GetNumberOfLocalCells()), 1);
    }
  }
}

void BuildVTKDataStructures(
  Mandelbulb& mandelbulb, int nprocs, int rank, vtkCPInputDataDescription* idd)
{
  if (VTKGrid == NULL)
  {
    // The grid structure isn't changing so we only build it
    // the first time it's needed. If we needed the memory
    // we could delete it and rebuild as necessary.
    VTKGrid = vtkMultiBlockDataSet::New();
    BuildVTKGrid(mandelbulb, nprocs, rank);
  }
  UpdateVTKAttributes(mandelbulb, rank, idd);
}

void BuildVTKDataStructuresList(
  std::vector<Mandelbulb>& mandelbulbList, int global_nblocks, vtkCPInputDataDescription* idd)
{
  // reset vtk grid for each call??
  // if there is memory leak here
  if (VTKGrid != NULL)
  {
    // The grid structure isn't changing so we only build it
    // the first time it's needed. If we needed the memory
    // we could delete it and rebuild as necessary.
    // delete VTKGrid;
    // refer to https://vtk.org/Wiki/VTK/Tutorials/SmartPointers
    VTKGrid->Delete();
  }

  // reset the grid each time, since the block number may change for different
  // steps, block offset may also change
  VTKGrid = vtkMultiBlockDataSet::New();
  BuildVTKGridList(mandelbulbList, global_nblocks);

  // fill in actual values
  UpdateVTKAttributesList(mandelbulbList, idd);
}
} // namespace

namespace InSitu
{
std::queue<std::unique_ptr<Block> > blockqueue;

void MPIInitialize(const std::string& script)
{
  return;
}

void MochiInitialize(const std::string& script)
{
  return;
}

void Finalize()
{
  return;
}

void MPICoProcessDynamic(MPI_Comm subcomm, std::vector<Mandelbulb>& mandelbulbList,
  int global_nblocks, double time, unsigned int timeStep)
{
  return;
}
// this paraview branch support the MPI communicator changing dynamically
// https://gitlab.kitware.com/mdorier/paraview/-/tree/dev-icet-integration
void MPICoProcess(Mandelbulb& mandelbulb, int nprocs, int rank, double time, unsigned int timeStep)
{
  return;
}

void MochiCoProcess(
  Mandelbulb& mandelbulb, int nprocs, int rank, double time, unsigned int timeStep)
{
  return;
}

void StoreBlocks(unsigned int timeStep, std::vector<Mandelbulb>& mandelbulbList)
{
  std::unique_ptr<Block> objPtr(new Block(timeStep, mandelbulbList));
  // std::cout << "new added blocks number " << objPtr->m_mandelbulbList.size()
  // << std::endl;
  blockqueue.push(std::move(objPtr));
}

void isoExtraction(std::vector<Mandelbulb>& mandelbulbList, int global_blocks)
{
  for (auto& mandelbulb : mandelbulbList)
  {
    int* extents = mandelbulb.GetExtents();
    auto importer = vtkSmartPointer<vtkImageImport>::New();
    importer->SetDataSpacing(1.0 / global_blocks, 1, 1);
    importer->SetDataOrigin(mandelbulb.GetOrigin());
    // from 0 to the shape -1 or from lb to the ub??
    importer->SetWholeExtent(extents);
    importer->SetDataExtentToWholeExtent();
    importer->SetDataScalarTypeToDouble();
    importer->SetNumberOfScalarComponents(1);
    importer->SetImportVoidPointer((int*)(mandelbulb.GetData()));
    importer->Update();

    // Run the marching cubes algorithm for isocounter
    auto mcubes = vtkSmartPointer<vtkMarchingCubes>::New();
    mcubes->SetInputConnection(importer->GetOutputPort());
    mcubes->ComputeNormalsOn();
    mcubes->SetValue(0, 50);
    // mcubes->SetValue(1, isovalue - 0.25);
    // mcubes->SetValue(2, isovalue + 0.25);
    mcubes->Update();
  }
}

void writeVTKData(std::vector<Mandelbulb>& mandelbulbList, int global_blocks, std::string fileName)
{
  int i = 0;
  for (auto& mandelbulb : mandelbulbList)
  {
    int* extents = mandelbulb.GetExtents();
    auto importer = vtkSmartPointer<vtkImageImport>::New();
    importer->SetDataSpacing(1.0 / global_blocks, 1, 1);
    importer->SetDataOrigin(mandelbulb.GetOrigin());
    // from 0 to the shape -1 or from lb to the ub??
    importer->SetWholeExtent(extents);
    importer->SetDataExtentToWholeExtent();
    importer->SetDataScalarTypeToDouble();
    importer->SetNumberOfScalarComponents(1);
    importer->SetImportVoidPointer((int*)(mandelbulb.GetData()));
    importer->Update();

    vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
    std::string newfileName = fileName + std::to_string(i);
    writer->SetFileName(newfileName.data());
    // get the specific polydata and check the results
    // writer->SetInputConnection(polyData->GetOutput());
    // writer->SetInputData(importer->GetOutputPort());
    // Optional - set the mode. The default is binary.
    // pay attention when to use set input data when to use get output
    writer->SetInputConnection(importer->GetOutputPort());
    writer->SetDataModeToBinary();
    // writer->SetDataModeToAscii();
    writer->Write();
    i++;
  }
}

void sampleExtraction(std::vector<Mandelbulb>& mandelbulbList, int global_blocks)
{
  // sample the data
  int i = 0;
  for (auto& mandelbulb : mandelbulbList)
  {
    int* extents = mandelbulb.GetExtents();
    auto importer = vtkSmartPointer<vtkImageImport>::New();
    importer->SetDataSpacing(1.0 / global_blocks, 1, 1);
    importer->SetDataOrigin(mandelbulb.GetOrigin());
    // from 0 to the shape -1 or from lb to the ub??
    importer->SetWholeExtent(extents);
    importer->SetDataExtentToWholeExtent();
    importer->SetDataScalarTypeToDouble();
    importer->SetNumberOfScalarComponents(1);
    importer->SetImportVoidPointer((int*)(mandelbulb.GetData()));
    importer->Update();

    vtkSmartPointer<vtkImageReslice> reslice = vtkSmartPointer<vtkImageReslice>::New();
    reslice->SetOutputExtent(0, 19, 0, 19, 0, 19);
    reslice->SetInputConnection(importer->GetOutputPort());
    reslice->Update();

    i++;
  }
}



void sampleisoExtraction(std::vector<Mandelbulb>& mandelbulbList, int global_blocks)
{
  // sample the data
  int i = 0;
  for (auto& mandelbulb : mandelbulbList)
  {
    int* extents = mandelbulb.GetExtents();
    auto importer = vtkSmartPointer<vtkImageImport>::New();
    importer->SetDataSpacing(1.0 / global_blocks, 1, 1);
    importer->SetDataOrigin(mandelbulb.GetOrigin());
    // from 0 to the shape -1 or from lb to the ub??
    importer->SetWholeExtent(extents);
    importer->SetDataExtentToWholeExtent();
    importer->SetDataScalarTypeToDouble();
    importer->SetNumberOfScalarComponents(1);
    importer->SetImportVoidPointer((int*)(mandelbulb.GetData()));
    importer->Update();

    vtkSmartPointer<vtkImageReslice> reslice = vtkSmartPointer<vtkImageReslice>::New();
    reslice->SetOutputExtent(0, 79, 0, 79, 0, 79);
    reslice->SetInputConnection(importer->GetOutputPort());
    reslice->Update();

    // Run the marching cubes algorithm for isocounter
    auto mcubes = vtkSmartPointer<vtkMarchingCubes>::New();
    mcubes->SetInputConnection(reslice->GetOutputPort());
    mcubes->ComputeNormalsOn();
    mcubes->SetValue(0, 60);
    mcubes->Update();

    i++;
  }
}

int putData(void* dataSourcePtr, size_t datasize, std::string dir, std::string filename)
{
  // how to check the file existance properly?
  // if file exist
  // std::ifstream f(fullfileName.c_str());
  // if (f.is_open())
  //{
  //  std::cerr << "file is open stats: " << fullfileName << std::endl;
  //  throw std::runtime_error("file open");
  //  return -1;
  //}

  // write from the file
  std::string fullfileName = dir + "/" + filename;
  std::ofstream wfile(fullfileName.c_str(), std::ios::binary);
  if (!wfile.good())
  {
    throw std::runtime_error("not good to write info the file");
    return -1;
  }

  if (datasize == 0)
  {
    throw std::runtime_error("file write size is 0 on disk");
    return -1;
  }

  wfile.write((char*)dataSourcePtr, datasize);
  wfile.close();
  return 0;
}

void StoreBlocksSharedMem(unsigned int timeStep, std::vector<Mandelbulb>& mandelbulbList)
{
  // put data into the shared memory
}

void StoreBlockIntransit()
{
  // put the data by in-transit way
}

void StoreBlockDisk(std::vector<Mandelbulb>& mandelbulbList, std::string dir, std::string filename)
{
  // put the data into the disk
  // iterate block and write
  int i = 0;
  for (auto& mandelbulb : mandelbulbList)
  {
    size_t dataSize = mandelbulb.GetNumberOfLocalCells() * sizeof(int);
    // the block id
    std::string newfilename = filename + "_" + std::to_string(i);
    if (dataSize != 0)
    {
      putData(mandelbulb.GetData(), dataSize, dir, newfilename);
    }
    i++;
  }
}

// grilaput
void grilaPut(std::vector<Mandelbulb>& mandelbulbList) {}

} // namespace InSitu
