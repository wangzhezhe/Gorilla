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

    if (s.mesh_type == "image") {
        lf_VTKImage(s, io);
    } else if (s.mesh_type == "structured") {
        throw std::invalid_argument(
            "ERROR: mesh_type=structured not yet "
            "   supported in settings.json, use mesh_type=image instead\n");
    }
    // TODO extend to other formats e.g. structured
}

//init the writer instance
//init the staging client
Writer::Writer(const Settings &settings, const GrayScott &sim, size_t steps, size_t blockID)
    : settings(settings)
{

    //put var_u and var_v into the staging services

}

void Writer::open(const std::string &fname)
{
    //writer = io.Open(fname, adios2::Mode::Write);
    // the key is fixed here
    //io.SetParameters({{"verbose", "4"}, {"writerID", fname}});
    return;
}

//execute the data write operation
void Writer::write(const GrayScott &sim, size_t & step, size_t & blockID)
{


        std::vector<double> u = sim.u_noghost();
        std::vector<double> v = sim.v_noghost();


        //put u

        //put v


}

void Writer::close() { writer.Close(); }
