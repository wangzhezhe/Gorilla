#include "writer.h"
#include <iostream>

//init the writer instance
//init the staging client
Writer::Writer(const Settings &settings)
{
    this->m_serverMasterAddr = loadMasterAddr(); 
    std::cout << "serverMaster addr " << m_serverMasterAddr << std::endl;   
}

//void Writer::open(const std::string &fname)
//{
//writer = io.Open(fname, adios2::Mode::Write);
// the key is fixed here
//io.SetParameters({{"verbose", "4"}, {"writerID", fname}});
//    return;
//}

//execute the data write operation
void Writer::write(const GrayScott &sim, tl::engine &myEngine, size_t &step, size_t &blockID)
{

    std::vector<double> u = sim.u_noghost();
    std::vector<double> v = sim.v_noghost();

    std::string VarNameU = "grascott_u";
    std::array<size_t, 3> varuShape = {{sim.size_x, sim.size_y, sim.size_z}};
    std::array<size_t, 3> offSet = {{sim.offset_x, sim.offset_y, sim.offset_z}};

    //put u to the staging service
    DataMeta dataMeta = DataMeta(VarNameU, step,  typeid(double).name(), sizeof(double), varuShape,offSet);

    std::string slaveAddr = dspaces_client_getaddr(myEngine, this->m_serverMasterAddr, VarNameU, step , blockID);
    std::cout << "the slave server addr for ds put is " << slaveAddr << std::endl;
    dspaces_client_put(myEngine, slaveAddr, dataMeta, blockID, u);
}
