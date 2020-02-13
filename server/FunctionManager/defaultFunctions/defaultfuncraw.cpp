#include "defaultfuncraw.h"
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
#include <sstream>

std::string test(const BlockSummary &bs, void *inputData, const std::vector<std::string> &parameters)
{
    //bs.printSummary();
    std::cout << "test the default data execution" << std::endl;
    return "";
}

std::string valueRange(const BlockSummary &bs, void *inputData, const std::vector<std::string> &parameters)
{

    if (parameters.size() < 1)
    {
        throw std::runtime_error("the parameter of value range should larger than 3");
    }
    
   
    std::string type = parameters[0];
    
    /*
    for (int i=0;i<parameters.size();i++){
        std::cout << "paras " << i << ": " << parameters[i] << std::endl;
    }
    */
    //range all the elements
    
    for (int i = 0; i < bs.m_elemNum; i++)
    {
        if (bs.m_elemSize == 8)
        {
            double tempData = *((double *)inputData + i);
            double threshA, threshB;

            //std::cout << "check data " << tempData << std::endl;

            if (type.compare("G") == 0)
            {
                threshA = atof(parameters[1].c_str());

                if (tempData >= threshA)
                {
                    return "1";
                }
            }
            else if (type.compare("L") == 0)
            {
                threshA = atof(parameters[1].c_str());
                if (tempData <= threshA)
                {
                    return "1";
                }
            }
            else if (type.compare("B") == 0)
            {
                if (parameters.size() != 3)
                {
                    throw std::runtime_error("the length of parameters should be 3 when the type is B");
                }

                threshA = atof(parameters[1].c_str());
                threshB = atof(parameters[2].c_str());

                if (tempData >= threshA && tempData <= threshB)
                {
                    return "1";
                }
            }
            else
            {
                throw std::runtime_error("unsuported type " + type);
            }
        }
        else
        {
            throw std::runtime_error("unsuported data length " + std::to_string(bs.m_elemSize));
        }
    }

    return "0";
}

std::string testvtk(const BlockSummary &bs, void *inputData, const std::vector<std::string> &parameters)
{
    // load to vtk data and execute write option

    std::cout << "debug\n";

    std::cout << bs.m_indexlb[0] << "," << bs.m_indexlb[1] << "," << bs.m_indexlb[2] << std::endl;
    std::cout << bs.m_indexub[0] << "," << bs.m_indexub[1] << "," << bs.m_indexub[2] << std::endl;

    for (int i = 0; i < 1000; i++)
    {
        double value = *((double *)(inputData) + i);
        std::cout << "index " << i << " value " << value << std::endl;
    }

    auto importer = vtkSmartPointer<vtkImageImport>::New();
    importer->SetDataSpacing(1, 1, 1);

    importer->SetDataOrigin(1.0 * bs.m_indexlb[2], 1.0 * bs.m_indexlb[1],
                            1.0 * bs.m_indexlb[0]);

    importer->SetWholeExtent(0, bs.m_indexub[2], 0,
                             bs.m_indexub[1], 0,
                             bs.m_indexub[0]);

    importer->SetDataExtentToWholeExtent();
    importer->SetDataScalarTypeToDouble();
    importer->SetNumberOfScalarComponents(1);
    importer->SetImportVoidPointer((double *)(inputData));
    importer->Update();

    // Write the file by vtkXMLDataSetWriter
    vtkSmartPointer<vtkXMLImageDataWriter> writer =
        vtkSmartPointer<vtkXMLImageDataWriter>::New();

    std::string fileName = "testvtk.vti";
    writer->SetFileName(fileName.data());

    // get the specific polydata and check the results
    writer->SetInputConnection(importer->GetOutputPort());
    //writer->SetInputData(importer->GetOutputPort());
    // Optional - set the mode. The default is binary.
    writer->SetDataModeToBinary();
    // writer->SetDataModeToAscii();
    writer->Write();

    return "";
}
