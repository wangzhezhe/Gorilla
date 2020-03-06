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
#include <adios2.h>

std::string test(FunctionManagerRaw *fmr, const BlockSummary &bs, void *inputData, const std::vector<std::string> &parameters)
{
    //bs.printSummary();
    std::cout << "test the default data execution" << std::endl;
    return "";
}

std::string valueRange(FunctionManagerRaw *fmr, const BlockSummary &bs, void *inputData, const std::vector<std::string> &parameters)
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

std::string testvtk(FunctionManagerRaw *fmr, const BlockSummary &bs, void *inputData, const std::vector<std::string> &parameters)
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



std::string adiosWrite(FunctionManagerRaw *fmr, const BlockSummary &bs, void *inputData, const std::vector<std::string> &parameters)
{
    // write the data into the adios bp file
    // the first parameter represent the status
    if (parameters.size() != 2)
    {
        throw std::runtime_error("the parameter len for adiosWrite should be 2");
    }

    if (fmr == NULL)
    {
        throw std::runtime_error("the function manager should not be null");
        return "FAIL";
    }

    char str[256];
    sprintf(str, "adios write step %s varName %s lb %d %d %d ub %d %d %d\n",
            parameters[0].c_str(), parameters[1].c_str(),
            bs.m_indexlb[0], bs.m_indexlb[1], bs.m_indexlb[2],
            bs.m_indexub[0], bs.m_indexub[1], bs.m_indexub[2]);
    //std::cout << str << std::endl;

    //simulate the writting process

    //int writeTime = 3.8;
    //usleep(writeTime * 1000000);

    //record time

    // there is still unsolved bug here
    /*
    
    adios2::Variable<double> var_u;
    adios2::Variable<int> var_step;

    size_t shapex = bs.m_indexub[0] - bs.m_indexlb[0] + 1;
    size_t shapey = bs.m_indexub[1] - bs.m_indexlb[1] + 1;
    size_t shapez = bs.m_indexub[2] - bs.m_indexlb[2] + 1;

    var_u = fmr->m_statefulConfig->m_io.DefineVariable<double>("U",
                                             {512, 512, 512},
                                             {(size_t)bs.m_indexlb[0], (size_t)bs.m_indexlb[1], (size_t)bs.m_indexlb[2]},
                                             {shapex, shapey, shapez});

    int step = atoi(parameters[0].c_str());
    var_step = fmr->m_statefulConfig->m_io.DefineVariable<int>("step");

    //fmr->m_writer.BeginStep();
    fmr->m_statefulConfig->m_writer.Put<int>(var_step, &step);
    fmr->m_statefulConfig->m_writer.Put<double>(var_u, (double *)inputData);
    //fmr->m_writer.EndStep();
    */

    //write the current block into the adios
    //the partition of the staging server is useully less than the partition of the writer

    return "OK";
}