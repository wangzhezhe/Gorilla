#include "writer.h"
#include <iostream>
#include <utils/uuid.h>

#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkMarchingCubes.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLDataSetWriter.h>


void Writer::writeImageData(const GrayScott &sim, std::string fileName)
{

    std::vector<double> u = sim.u_noghost();
    std::array<int, 3> indexlb = {{(int)sim.offset_x, (int)sim.offset_y, (int)sim.offset_z}};
    std::array<int, 3> indexub = {{(int)(sim.offset_x + sim.size_x - 1), (int)(sim.offset_y + sim.size_y - 1), (int)(sim.offset_z + sim.size_z - 1)}};


    auto importer = vtkSmartPointer<vtkImageImport>::New();
    importer->SetDataSpacing(1, 1, 1);
    importer->SetDataOrigin(0,0,0);
    //from 0 to the shape -1 or from lb to the ub??
    importer->SetWholeExtent(indexlb[0], indexub[0], indexlb[1],
                             indexub[1], indexlb[2],
                             indexub[2]);
    importer->SetDataExtentToWholeExtent();
    importer->SetDataScalarTypeToDouble();
    importer->SetNumberOfScalarComponents(1);
    importer->SetImportVoidPointer((double *)(u.data()));
    importer->Update();

    // Write the file by vtkXMLDataSetWriter
    vtkSmartPointer<vtkXMLImageDataWriter> writer =
        vtkSmartPointer<vtkXMLImageDataWriter>::New();
    writer->SetFileName(fileName.data());

    // get the specific polydata and check the results
    writer->SetInputConnection(importer->GetOutputPort());
    //writer->SetInputData(importer->GetOutputPort());
    // Optional - set the mode. The default is binary.
    writer->SetDataModeToBinary();
    // writer->SetDataModeToAscii();
    writer->Write();
}



void Writer::write(const GrayScott &sim, size_t step, int rank, std::string recordInfo)
{
    std::vector<double> u = sim.u_noghost();

    std::string VarNameU = "grascott_u";

    std::array<int, 3> indexlb = {{(int)sim.offset_x, (int)sim.offset_y, (int)sim.offset_z}};
    std::array<int, 3> indexub = {{(int)(sim.offset_x + sim.size_x - 1), (int)(sim.offset_y + sim.size_y - 1), (int)(sim.offset_z + sim.size_z - 1)}};

    size_t elemSize = sizeof(double);
    size_t elemNum = sim.size_x*sim.size_y*sim.size_z;
    int blockindex = 0;
    std::string blockid = VarNameU+"_"+std::to_string(step)+"_"+std::to_string(rank)+"_"+std::to_string(blockindex);
    
    //generate raw data summary block
    BlockSummary bs(elemSize, elemNum,
                    DATATYPE_RAWMEM,
                    blockid,
                    3,
                    indexlb,
                    indexub,
                    recordInfo);
     
    int status = m_uniclient->putrawdata(step, VarNameU, bs, u.data());

    if(status!=0){
        throw std::runtime_error("failed to put data for step " + std::to_string(step));
    }
}
