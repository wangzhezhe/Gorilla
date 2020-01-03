#include <iostream>
#include <sstream>
#include <mpi.h>

#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkMarchingCubes.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLDataSetWriter.h>

#include "../simulation/timer.hpp"
#include "../../client/unimosclient.h"

void writeImageData(std::string fileName,
                    std::array<int, 3> &indexlb,
                    std::array<int, 3> &indexub,
                    void* fielddata)
{
    auto importer = vtkSmartPointer<vtkImageImport>::New();
    importer->SetDataSpacing(1, 1, 1);
    importer->SetDataOrigin(1.0 * indexlb[2], 1.0 * indexlb[1],
                            1.0 * indexlb[0]);
    //from 0 to the shape -1 or from lb to the ub??
    importer->SetWholeExtent(indexlb[2], indexub[2], indexlb[1],
                             indexub[1], indexlb[0],
                             indexub[0]);
    importer->SetDataExtentToWholeExtent();
    importer->SetDataScalarTypeToDouble();
    importer->SetNumberOfScalarComponents(1);
    importer->SetImportVoidPointer((double *)(fielddata));
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

vtkSmartPointer<vtkPolyData>
    compute_isosurface(std::array<size_t, 3> &shape,
                       std::array<size_t, 3> &offset,
                       const std::vector<double> &field, double isovalue)
{
    // Convert field values to vtkImageData
    auto importer = vtkSmartPointer<vtkImageImport>::New();
    importer->SetDataSpacing(1, 1, 1);
    importer->SetDataOrigin(1.0 * offset[2], 1.0 * offset[1],
                            1.0 * offset[0]);
    importer->SetWholeExtent(0, shape[2] , 0,
                             shape[1] , 0,
                             shape[0] );
    importer->SetDataExtentToWholeExtent();
    importer->SetDataScalarTypeToDouble();
    importer->SetNumberOfScalarComponents(1);
    importer->SetImportVoidPointer(const_cast<double *>(field.data()));

    // Run the marching cubes algorithm
    auto mcubes = vtkSmartPointer<vtkMarchingCubes>::New();
    mcubes->SetInputConnection(importer->GetOutputPort());
    mcubes->ComputeNormalsOn();
    mcubes->SetValue(0, isovalue);
    mcubes->Update();

    // Return the isosurface as vtkPolyData
    return mcubes->GetOutput();
}

void writePolyvtk(const std::string &fname,
               const vtkSmartPointer<vtkPolyData> polyData)
{
    auto writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
    writer->SetFileName(fname.c_str());
    writer->SetInputData(polyData);
    writer->Write();
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int rank, procs;
    
    //assume only start one server here
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &procs);

    MPI_Comm comm=MPI_COMM_WORLD;

    if (argc < 3)
    {
        if (rank == 0)
        {
            std::cerr << "Too few arguments" << std::endl;
            std::cout << "Usage: isosurface input output isovalue" << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    const double steps = std::stoi(argv[1]);
    const double isovalue = std::stod(argv[2]);

    std::vector<double> variableU;
    int step;

#ifdef ENABLE_TIMERS
    Timer timer_total;
    Timer timer_read;

    std::ostringstream log_fname;
    log_fname << "isosurface_pe_" << rank << ".log";

    std::ofstream log(log_fname.str());
    log << "step\tread_iso" << std::endl;
#endif

    tl::engine clientEngine("verbs", THALLIUM_CLIENT_MODE);
    UniClient *uniclient = new UniClient(&clientEngine, "./unimos_server.conf");


    std::string VarNameU = "grascott_u";

    //gray scott simulation start from 1
    for (int step = 1; step <= steps; step++)
    {

#ifdef ENABLE_TIMERS

        MPI_Barrier(comm);
        timer_total.start();
        timer_read.start();
#endif

        //read data

        //get blockMeta, extract the shape, offset, variableName

        //get variable
        std::array<int, 3> indexlb = {{15,15,15}};
        std::array<int, 3> indexub = {{47,47,47}};

        MATRIXTOOL::MatrixView dataView = uniclient->getArbitraryData(step, VarNameU, sizeof(double), 3, indexlb, indexub);

        /*
    size_t step,
    std::string varName,
    size_t elemSize,
    size_t dims,
    std::array<int, 3> indexlb,
    std::array<int, 3> indexub)
        */



#ifdef ENABLE_TIMERS
        double time_read = timer_read.stop();
        log << step << "\t" << time_read << "\t" << std::endl;
#endif


        //todo add the checking operation for the pdf
        //auto polyData = compute_isosurface(blockmeta.m_shape, blockmeta.m_offset, dataContainer, isovalue);

        char countstr[50];
        sprintf(countstr, "%02d_%04d", rank, step);
        std::string fname = "./vtkdata/vtkiso_" + std::string(countstr) + ".vti";
        writeImageData(fname, indexlb, indexub, dataView.m_data);
        //writePolyvtk(fname, polyData);
        std::cout << "ok for ts " << step << std::endl;
    }

#ifdef ENABLE_TIMERS
    //log << "total\t" << timer_total.elapsed() << "\t" << timer_read.elapsed()
    //   << "\t" << timer_compute.elapsed() << std::endl;

    log.close();
#endif

    MPI_Finalize();

    return 0;
}
