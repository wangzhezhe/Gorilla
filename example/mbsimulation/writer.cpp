#include "writer.hpp"
#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkSmartPointer.h>

void Writer::writetoStaging(
  size_t step, std::vector<Mandelbulb>& mandelbulbList, int block_number_offset, int global_blocks)
{

  // send array or the image data?
  int index = 0;

  for (auto& mandelbulb : mandelbulbList)
  {
    // change the data into the vtkimage
    // transfer the vtk image
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

    vtkSmartPointer<vtkImageData> imageData = importer->GetOutput();

    int blockIndex = block_number_offset + index;

    std::string blockName = "mandelbulb_" + std::to_string(blockIndex) + "_" + std::to_string(step);

    std::array<int, 3> indexlb = { { blockIndex + DEPTH, 0, 0 } };
    std::array<int, 3> indexub = { { (blockIndex + 1) * DEPTH, HEIGHT, WIDTH } };
    
    //it is ok to provide the empty aslist here
    //the addsumary operation will be exeuted in client
    std::vector<ArraySummary> aslist;
    BlockSummary bs(aslist, DATATYPE_VTKPTR, blockName, 3, indexlb, indexub);

    int status = this->m_uniclient->putrawdata(step, "mandelbulb", bs, (void*)imageData);

    if (status != 0)
    {
      throw std::runtime_error("failed to put data for block " + blockName);
    }

    // trigger specific task
    std::vector<std::string> parameters;
    std::string resutls = this->m_uniclient->executeRawFunc(
      this->m_uniclient->m_associatedDataServer, blockName, "test", parameters);

    std::cout << "exec resuts for block: " << blockName << ": " << resutls << std::endl;

    index++;
  }
}