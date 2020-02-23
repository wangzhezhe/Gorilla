// The solver is based on Hiroshi Watanabe's 2D Gray-Scott reaction diffusion
// code available at:
// https://github.com/kaityo256/sevendayshpc/tree/master/day5

#include <mpi.h>
#include <random>
#include <vector>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#include "json.hpp"
#include "sim_single.h"
#include "settings_single.h"

SingleSim::SingleSim(const SingleSettings &settings, int rank, int totalNumber)
    : settings(settings)
    {

             m_rank = rank;
             m_procs = totalNumber;

    }

SingleSim::~SingleSim() {}

void SingleSim::init(const SingleSettings &settings, int coords[3], double value)
{
    init_offset(settings, coords);
    init_field(value);
}

void SingleSim::iterate(int calcutime)
{
    this->calc(calcutime);
}

const std::vector<double> &SingleSim::u_ghost() const { return u; }

std::vector<double> SingleSim::u_noghost() const { return data_noghost(u); }


std::vector<double>
SingleSim::data_noghost(const std::vector<double> &data) const
{
    std::vector<double> buf(size_x * size_y * size_z);
    data_no_ghost_common(data, buf.data());
    return buf;
}

void SingleSim::data_noghost(const std::vector<double> &data,
                             double *data_no_ghost) const
{
    data_no_ghost_common(data, data_no_ghost);
}

void SingleSim::init_field(double value)
{
    const int V = (size_x + 2) * (size_y + 2) * (size_z + 2);
    u.resize(V, value);
      
    /*
    const int d = 6;
    for (int z = gridz / 2 - d; z < gridz/ 2 + d; z++) {
        for (int y = gridy / 2 - d; y < gridy / 2 + d; y++) {
            for (int x = gridx / 2 - d; x < gridx / 2 + d; x++) {
                if (!is_inside(x, y, z)) continue;
                int i = g2i(x, y, z);
                u[i] = 0.5;
            }
        }
    }
    */
}

void SingleSim::calc(int updating_time)
{
    //simulate data updating
    std::this_thread::sleep_for (std::chrono::seconds(updating_time));
    return;
}

void SingleSim::init_offset(const SingleSettings &settings, int coords[3])
{
    const int periods[3] = {1, 1, 1};
    
    gridx=settings.lenArray[0];
    gridy=settings.lenArray[1];
    gridz=settings.lenArray[2];
    
    npx = settings.processLayout[0];
    npy = settings.processLayout[1];
    npz = settings.processLayout[2];
    
    //number of the processors at each dimention 
    //(this represent the process layout)
    //the coordinates for the process
    px = coords[0];
    py = coords[1];
    pz = coords[2];

    size_x = (gridx + npx - 1) / npx;
    size_y = (gridy + npy - 1) / npy;
    size_z = (gridz + npz - 1) / npz;

    offset_x = size_x * px;
    offset_y = size_y * py;
    offset_z = size_z * pz;

    if (px == npx - 1) {
        size_x -= size_x * npx - gridx;
    }
    if (py == npy - 1) {
        size_y -= size_y * npy - gridy;
    }
    if (pz == npz - 1) {
        size_z -= size_z * npz - gridz;
    }
}

void SingleSim::data_no_ghost_common(const std::vector<double> &data,
                                     double *data_no_ghost) const
{
    for (int z = 1; z < size_z + 1; z++) {
        for (int y = 1; y < size_y + 1; y++) {
            for (int x = 1; x < size_x + 1; x++) {
                data_no_ghost[(x - 1) + (y - 1) * size_x +
                              (z - 1) * size_x * size_y] = data[l2i(x, y, z)];
            }
        }
    }
}
