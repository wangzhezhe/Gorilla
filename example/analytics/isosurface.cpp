#include <iostream>
#include <sstream>

#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkMarchingCubes.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataWriter.h>

#include <thread>
#include "../../unimos/client/unimosclient.h"

vtkSmartPointer<vtkPolyData>
compute_isosurface(std::array<size_t, 3> &shape,
                   std::array<size_t, 3> &offset,
                   const std::vector<double> &field, double isovalue)
{
    // Convert field values to vtkImageData
    auto importer = vtkSmartPointer<vtkImageImport>::New();
    importer->SetDataSpacing(1, 1, 1);
    importer->SetDataOrigin(1.0*offset[2], 1.0*offset[1],
                            1.0*offset[0]);
    importer->SetWholeExtent(0, shape[2] - 1, 0,
                             shape[1] - 1, 0,
                             shape[0] - 1);
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

void write_vtk(const std::string &fname,
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
    Timer timer_compute;
    Timer timer_write;

    std::ostringstream log_fname;
    log_fname << "isosurface_pe_" << rank << ".log";

    std::ofstream log(log_fname.str());
    //log << "step\ttotal_iso\tread_iso\tcompute_write_iso" << std::endl;
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

#ifdef ENABLE_TIMERS
        double time_read = timer_read.stop();
        MPI_Barrier(comm);
        timer_compute.start();
#endif

        //get blockMeta, extract the shape, offset, variableName

        //get variable

        size_t blockID = (size_t)rank;
        std::string slaveAddr = dspaces_client_getaddr(clientEngine, serverAddr, VarNameU, step);
        BlockMeta blockmeta = dspaces_client_getblockMeta(clientEngine, slaveAddr, VarNameU, step, blockID);
        blockmeta.printMeta();
        std::vector<double> dataContainer(blockmeta.m_shape[0] * blockmeta.m_shape[1] * blockmeta.m_shape[2]);
        dspaces_client_get(clientEngine, slaveAddr, VarNameU, step, blockID, dataContainer);

        //todo add the checking operation for the pdf
        //auto polyData = compute_isosurface(blockmeta.m_shape, blockmeta.m_offset, dataContainer, isovalue);
        
        char countstr[50];
        sprintf(countstr, "%02d_04%d", blockID, step);
        std::string fname = "./vtkdata/vtkiso_" + std::string(countstr) + ".vtk";
        write_vtk(fname,polyData);
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
