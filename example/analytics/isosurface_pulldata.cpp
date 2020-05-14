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
#include "../client/unimosclient.h"
#include "../putgetMeta/metaclient.h"

#ifdef USE_GNI
extern "C"
{
#include <rdmacred.h>
}
#include <mercury.h>
#include <margo.h>
#define DIE_IF(cond_expr, err_fmt, ...)                                                                           \
    do                                                                                                            \
    {                                                                                                             \
        if (cond_expr)                                                                                            \
        {                                                                                                         \
            fprintf(stderr, "ERROR at %s:%d (" #cond_expr "): " err_fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
            exit(1);                                                                                              \
        }                                                                                                         \
    } while (0)
#endif

const std::string serverCred = "Gorila_cred_conf";

void writeImageData(std::string fileName,
                    std::array<int, 3> &indexlb,
                    std::array<int, 3> &indexub,
                    void *fielddata)
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
    importer->SetWholeExtent(0, shape[2], 0,
                             shape[1], 0,
                             shape[0]);
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

    if (argc != 5)
    {
        if (rank == 0)
        {
            std::cerr << "Too few arguments" << std::endl;
            std::cout << "Usage: isosurface setting.json steps isovalue protocol" << std::endl;
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
    std::string protocol = argv[4];

    std::vector<double> variableU;
    int step;

#ifdef USE_GNI
    //get the drc id from the shared file
    std::ifstream infile(serverCred);
    std::string cred_id;
    std::getline(infile, cred_id);
    if (rank == 0)
    {
        std::cout << "load cred_id: " << cred_id << std::endl;
    }

    struct hg_init_info hii;
    memset(&hii, 0, sizeof(hii));
    char drc_key_str[256] = {0};
    uint32_t drc_cookie;
    uint32_t drc_credential_id;
    drc_info_handle_t drc_credential_info;
    int ret;
    drc_credential_id = (uint32_t)atoi(cred_id.c_str());

    ret = drc_access(drc_credential_id, 0, &drc_credential_info);
    DIE_IF(ret != DRC_SUCCESS, "drc_access %u", drc_credential_id);
    drc_cookie = drc_get_first_cookie(drc_credential_info);

    sprintf(drc_key_str, "%u", drc_cookie);
    hii.na_init_info.auth_key = drc_key_str;
    //printf("use the drc_key_str %s\n", drc_key_str);

    margo_instance_id mid;
    mid = margo_init_opt("gni", MARGO_CLIENT_MODE, &hii, 0, -1);
    tl::engine clientEngine(mid);
#else
    tl::engine clientEngine(protocol, THALLIUM_CLIENT_MODE);
#endif

    UniClient *uniclient = new UniClient(&clientEngine, "./unimos_server.conf", rank);
    uniclient->getAllServerAddr();
    uniclient->m_totalServerNum = uniclient->m_serverIDToAddr.size();

    std::string VarNameU = "grascott_u";
    //gray scott simulation start from 1
    for (int step = 1; step <= steps; step++)
    {

        //read data

        //get blockMeta, extract the shape, offset, variableName
        //get variable
        /*
        std::array<int, 3> indexlb = {{15,15,15}};
        std::array<int, 3> indexub = {{47,47,47}};
        */
        std::array<int, 3> indexlb = {{offset_x, offset_y, offset_z}};
        std::array<int, 3> indexub = {{offset_x + size_x - 1, offset_y + size_y - 1, offset_z + size_z - 1}};

#ifdef ENABLE_TIMERS
        MPI_Barrier(comm);
        //timer_total.start();
        //timer_read.start();

        struct timespec start, end;
        double diff;
        clock_gettime(CLOCK_REALTIME, &start); /* mark start time */
#endif
        std::cout << "debug " << "id " << rank << " indexlb " << indexlb[0] << ","<< indexlb[1] << ","<< indexlb[2]<< " indexub " << indexub[0] << ","<< indexub[1] << ","<< indexub[2] << std::endl;

        //if no metadata is updated, this API will block there 
        MATRIXTOOL::MatrixView dataView = uniclient->getArbitraryData(step, VarNameU, sizeof(double), 3, indexlb, indexub);

#ifdef ENABLE_TIMERS
        //double time_read = timer_read.stop();
        MPI_Barrier(comm);
        //some functions here
        clock_gettime(CLOCK_REALTIME, &end); /* mark end time */
        diff = (end.tv_sec - start.tv_sec) * 1.0 + (end.tv_nsec - start.tv_nsec) * 1.0 / BILLION;

        //caculate the avg
        double time_sum_read;
        MPI_Reduce(&diff, &time_sum_read, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
        if (rank == 0)
        {
            std::cout << "step " << step << " avg read " << time_sum_read / procs << std::endl;
        }
#endif

        if (rank == 0)
        {
            //std::string recordKey = "Trigger_" + std::to_string(step);
            //MetaClient *metaclient = new MetaClient(&clientEngine);
            //metaclient->Recordtime(recordKey);
            std::cout << "read ok for step " << step << std::endl;
        }

        //todo add the checking operation for the pdf
        //auto polyData = compute_isosurface(blockmeta.m_shape, blockmeta.m_offset, dataContainer, isovalue);

        //char countstr[50];
        //sprintf(countstr, "%03d_%04d", step, rank);
        //std::string fname = "./vtkdata/vtkiso_" + std::string(countstr) + ".vti";
        //writeImageData(fname, indexlb, indexub, dataView.m_data);
        //writePolyvtk(fname, polyData);
        //std::cout << "ok for ts " << step << std::endl;
    }

    delete uniclient;
    MPI_Finalize();

    return 0;
}
