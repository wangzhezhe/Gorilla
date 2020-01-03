#include "writer.h"
#include <iostream>


void Writer::write(const GrayScott &sim, size_t &step)
{

    std::vector<double> u = sim.u_noghost();

    std::string VarNameU = "grascott_u";

    std::array<int, 3> indexlb = {{(int)sim.offset_x, (int)sim.offset_y, (int)sim.offset_z}};
    std::array<int, 3> indexub = {{(int)(sim.offset_x + sim.size_x - 1), (int)(sim.offset_y + sim.size_y - 1), (int)(sim.offset_z + sim.size_z - 1)}};

    size_t elemSize = sizeof(double);
    size_t elemNum = sim.size_x*sim.size_y*sim.size_z;
    
    //generate raw data summary block
    BlockSummary bs(elemSize, elemNum,
                    DRIVERTYPE_RAWMEM,
                    3,
                    indexlb,
                    indexub);

    int status = m_uniclient->putrawdata(step, VarNameU, bs, u.data());

    if(status!=0){
        throw std::runtime_error("failed to put data for step " + std::to_string(step));
    }
}
