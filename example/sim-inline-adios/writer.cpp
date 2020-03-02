#include "writer.h"
#include <iostream>

void define_bpvtk_attribute(const Settings &s, adios2::IO &io)
{
    auto lf_VTKImage = [](const Settings &s, adios2::IO &io) {
        const std::string extent = "0 " + std::to_string(s.L) + " " + "0 " +
                                   std::to_string(s.L) + " " + "0 " +
                                   std::to_string(s.L);

        const std::string imageData = R"(
        <?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" + extent +
                                      R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent +
                                      R"(">
              <CellData Scalars="U">
                  <DataArray Name="U" />
                  <DataArray Name="V" />
                  <DataArray Name="TIME">
                    step
                  </DataArray>
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

        io.DefineAttribute<std::string>("vtk.xml", imageData);
    };

    if (s.mesh_type == "image")
    {
        lf_VTKImage(s, io);
    }
    else if (s.mesh_type == "structured")
    {
        throw std::invalid_argument(
            "ERROR: mesh_type=structured not yet "
            "   supported in settings.json, use mesh_type=image instead\n");
    }
    // TODO extend to other formats e.g. structured
}

Writer::Writer(const Settings &settings, const GrayScott &sim, adios2::IO io)
    : settings(settings), io(io)
{
    io.DefineAttribute<double>("F", settings.F);
    io.DefineAttribute<double>("k", settings.k);
    io.DefineAttribute<double>("dt", settings.dt);
    io.DefineAttribute<double>("Du", settings.Du);
    io.DefineAttribute<double>("Dv", settings.Dv);
    io.DefineAttribute<double>("noise", settings.noise);
    // define VTK visualization schema as an attribute
    if (!settings.mesh_type.empty())
    {
        define_bpvtk_attribute(settings, io);
    }

    var_u =
        io.DefineVariable<double>("U", {settings.L, settings.L, settings.L},
                                  {sim.offset_z, sim.offset_y, sim.offset_x},
                                  {sim.size_z, sim.size_y, sim.size_x});

    var_v =
        io.DefineVariable<double>("V", {settings.L, settings.L, settings.L},
                                  {sim.offset_z, sim.offset_y, sim.offset_x},
                                  {sim.size_z, sim.size_y, sim.size_x});

    if (settings.adios_memory_selection)
    {
        var_u.SetMemorySelection(
            {{1, 1, 1}, {sim.size_z + 2, sim.size_y + 2, sim.size_x + 2}});
        var_v.SetMemorySelection(
            {{1, 1, 1}, {sim.size_z + 2, sim.size_y + 2, sim.size_x + 2}});
    }

    //std::cout << "offset x: " << sim.offset_x << "offset y: " << sim.offset_y << "offset z: " << sim.size_z << std::endl;

    var_step = io.DefineVariable<int>("step");
}

void Writer::open(const std::string &fname)
{
    writer = io.Open(fname, adios2::Mode::Write);
    // the key is fixed here
    //io.SetParameters({{"verbose", "4"}, {"writerID", fname}});
}

void Writer::writeToVTK(std::string fname, const GrayScott &sim)
{

    // Create an image data
    vtkSmartPointer<vtkImageData> imageData =
        vtkSmartPointer<vtkImageData>::New();

    // Specify the size of the image data
    imageData->SetOrigin(sim.offset_x, sim.offset_y, sim.offset_z);
    imageData->SetDimensions(sim.size_x, sim.size_y, sim.size_z);
    imageData->AllocateScalars(VTK_DOUBLE, 1);
    //write u
    //using the image write

    //write the data u

    std::vector<double> u = sim.u_ghost();

    for (int z = 0; z < sim.size_z; z++)
    {
        for (int y = 0; y < sim.size_y; y++)
        {
            for (int x = 0; x < sim.size_x; x++)
            {
                int i = x + y * (sim.size_x + 2) + z * (sim.size_x + 2) * (sim.size_y + 2);
                double *pixel = static_cast<double *>(imageData->GetScalarPointer(x, y, z));
                pixel[0] = u[i];
            }
        }
    }

    vtkSmartPointer<vtkXMLImageDataWriter> writer =
        vtkSmartPointer<vtkXMLImageDataWriter>::New();
    writer->SetFileName(fname.c_str());
    writer->SetInputData(imageData);
    writer->Write();
}

void Writer::write(int &step, const GrayScott &sim)
{

    std::vector<double> u = sim.u_noghost();

    writer.BeginStep();
    writer.Put<int>(var_step, &step);
    writer.Put<double>(var_u, u.data());
    writer.EndStep();
}

void Writer::close() { writer.Close(); }
