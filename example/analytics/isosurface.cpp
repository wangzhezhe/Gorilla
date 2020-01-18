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
#include "../simulation/settings.h"
#include "../../client/unimosclient.h"

void writeImageData(std::string fileName,
                    std::array<int, 3> &indexlb,
                    std::array<int, 3> &indexub,
                    void* fielddata)
{
    auto importer = vtkSmartPointer<vtkImageImport>::New();
    importer->SetDataSpacing(1, 1, 1);
    importer->SetDataOrigin(0, 0, 0);
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

    int rank, procs, wrank;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &procs);


    if (argc != 4)
    {
        if (rank == 0)
        {
            std::cerr << "Too few arguments" << std::endl;
            std::cout << "Usage: isosurface setting.json steps isovalue" << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD, -1);
    }


    int dims[3] = {0};
    MPI_Dims_create(procs, 3, dims);
    int npx = dims[0];
    int npy = dims[1];
    int npz = dims[2];

    int coords[3] = {0};
    int periods[3] = {0};
    MPI_Comm cart_comm;
    MPI_Cart_create(comm, 3, dims, periods, 0, &cart_comm);
    MPI_Cart_coords(cart_comm, rank, 3, coords);
    uint64_t px = coords[0];
    uint64_t py = coords[1];
    uint64_t pz = coords[2];

    //TODO this info is loaded from the setting.json

    Settings settings = Settings::from_json(argv[1]);
    int dimValue = settings.L;

    int shape[3] = {dimValue, dimValue, dimValue};

    int size_x = (shape[0] + npx - 1) / npx;
    int size_y = (shape[1] + npy - 1) / npy;
    int size_z = (shape[2] + npz - 1) / npz;

    int offset_x = size_x * px;
    int offset_y = size_y * py;
    int offset_z = size_z * pz;

    if (px == npx - 1)
    {
        size_x -= size_x * npx - shape[0];
    }
    if (py == npy - 1)
    {
        size_y -= size_y * npy - shape[1];
    }
    if (pz == npz - 1)
    {
        size_z -= size_z * npz - shape[2];
    }



    const double steps = std::stoi(argv[2]);
    const double isovalue = std::stod(argv[3]);

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
        /*
        std::array<int, 3> indexlb = {{15,15,15}};
        std::array<int, 3> indexub = {{47,47,47}};
        */
        std::array<int, 3> indexlb = {{offset_x,offset_y,offset_z}};
        std::array<int, 3> indexub = {{offset_x + size_x - 1,offset_y + size_y - 1,offset_z + size_z - 1}};
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

        //char countstr[50];
        //sprintf(countstr, "%03d_%04d", step, rank);
        //std::string fname = "./vtkdata/vtkiso_" + std::string(countstr) + ".vti";
        //writeImageData(fname, indexlb, indexub, dataView.m_data);
        //writePolyvtk(fname, polyData);
        //std::cout << "ok for ts " << step << std::endl;
    }

#ifdef ENABLE_TIMERS
    //log << "total\t" << timer_total.elapsed() << "\t" << timer_read.elapsed()
    //   << "\t" << timer_compute.elapsed() << std::endl;

    log.close();
#endif

    MPI_Finalize();

    return 0;
}
