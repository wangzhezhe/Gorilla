#include <iostream>
#include <sstream>

#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkMarchingCubes.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLDataSetWriter.h>

#include <thread>
#include "../simulation/timer.hpp"
#include "../../unimos/client/unimosclient.h"

void writeImageData(std::string fileName,
                    std::array<size_t, 3> &shape,
                    std::array<size_t, 3> &offset,
                    const std::vector<double> &field)
{
    auto importer = vtkSmartPointer<vtkImageImport>::New();
    importer->SetDataSpacing(1, 1, 1);
    importer->SetDataOrigin(1.0 * offset[2], 1.0 * offset[1],
                            1.0 * offset[0]);
    importer->SetWholeExtent(0, shape[2] - 1, 0,
                             shape[1] - 1, 0,
                             shape[0] - 1);
    importer->SetDataExtentToWholeExtent();
    importer->SetDataScalarTypeToDouble();
    importer->SetNumberOfScalarComponents(1);
    importer->SetImportVoidPointer(const_cast<double *>(field.data()));
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
    std::string serverAddr = loadMasterAddr();

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

        size_t blockID = (size_t)rank;
        std::string slaveAddr = dspaces_client_getaddr(clientEngine, serverAddr, VarNameU, step);
        //TODO add checking operation here, if there is no meta info, waiting
        BlockMeta blockmeta = dspaces_client_getblockMeta(clientEngine, slaveAddr, VarNameU, step, blockID);
        blockmeta.printMeta();
        std::vector<double> dataContainer(blockmeta.m_shape[0] * blockmeta.m_shape[1] * blockmeta.m_shape[2]);
        dspaces_client_get(clientEngine, slaveAddr, VarNameU, step, blockID, dataContainer);

#ifdef ENABLE_TIMERS
        double time_read = timer_read.stop();
        log << step << "\t" << time_read << "\t" << std::endl;
#endif


        //todo add the checking operation for the pdf
        //auto polyData = compute_isosurface(blockmeta.m_shape, blockmeta.m_offset, dataContainer, isovalue);

        char countstr[50];
        sprintf(countstr, "%02d_%04d", blockID, step);
        std::string fname = "./vtkdata/vtkiso_" + std::string(countstr) + ".vti";
        //writeImageData(fname, blockmeta.m_shape, blockmeta.m_offset, dataContainer);
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
