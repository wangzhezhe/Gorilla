#include "defaultfunc.h"
#include <string>
#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkMarchingCubes.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLDataSetWriter.h>
#include <unistd.h>

void test(const BlockSummary &bs, void *inputData)
{
    //bs.printSummary();
    std::cout << "test the default data execution" << std::endl;
    return;
}

void testvtk(const BlockSummary &bs, void *inputData)
{
    // load to vtk data and execute write option

    std::cout << "debug\n";

    std::cout << bs.m_indexlb[0] << "," <<bs.m_indexlb[1] << "," <<bs.m_indexlb[2] << std::endl;
    std::cout << bs.m_indexub[0] << "," <<bs.m_indexub[1] << "," <<bs.m_indexub[2] << std::endl;

    for(int i=0;i<1000;i++){
        double value = *((double *)(inputData) + i);
        std::cout << "index " << i << " value " <<  value << std::endl;
    }

    auto importer = vtkSmartPointer<vtkImageImport>::New();
    importer->SetDataSpacing(1, 1, 1);

    importer->SetDataOrigin(1.0 * bs.m_indexlb[2], 1.0 * bs.m_indexlb[1],
                            1.0 * bs.m_indexlb[0]);

    importer->SetWholeExtent(0, bs.m_indexub[2] , 0,
                             bs.m_indexub[1] , 0,
                             bs.m_indexub[0]);

    importer->SetDataExtentToWholeExtent();
    importer->SetDataScalarTypeToDouble();
    importer->SetNumberOfScalarComponents(1);
    importer->SetImportVoidPointer((double *)(inputData));
    importer->Update();

    // Write the file by vtkXMLDataSetWriter
    vtkSmartPointer<vtkXMLImageDataWriter> writer =
        vtkSmartPointer<vtkXMLImageDataWriter>::New();

    std::string fileName ="testvtk.vti";
    writer->SetFileName(fileName.data());

    // get the specific polydata and check the results
    writer->SetInputConnection(importer->GetOutputPort());
    //writer->SetInputData(importer->GetOutputPort());
    // Optional - set the mode. The default is binary.
    writer->SetDataModeToBinary();
    // writer->SetDataModeToAscii();
    writer->Write();

    return;
}
