#ifndef __SIM_SYNTHETIC_H__
#define __SIM_SYNTHETIC_H__

#include <random>
#include <vector>
#include "settings_single.h"


class SingleSim
{
public:
    // Dimension of process grid
    size_t npx, npy, npz;
    // Coordinate of this rank in process grid
    size_t px, py, pz;
    // Dimension of local array
    size_t size_x, size_y, size_z;
    // Offset of local array in the global array
    size_t offset_x, offset_y, offset_z;
    // total grid number
    int gridx,gridy,gridz;

    SingleSim(const SingleSettings &settings, int rank, int totalNumber);
    ~SingleSim();

    void init(const SingleSettings &settings, int coords[3], double value);

    void iterate(int calcutime);

    const std::vector<double> &u_ghost() const;

    std::vector<double> u_noghost() const;

    void u_noghost(double *u_no_ghost) const;


protected:
    SingleSettings settings;

    std::vector<double> u;

    int m_rank, m_procs;

    // Setup cartesian communicator data types
    void init_offset(const SingleSettings &settings, int coords[3]);
    // Setup initial conditions
    void init_field(double value);

    // Progess simulation for one timestep
    void calc(int updating_time);

    // Check if point is included in my subdomain
    inline bool is_inside(int x, int y, int z) const
    {
        if (x < offset_x) return false;
        if (x >= offset_x + size_x) return false;
        if (y < offset_y) return false;
        if (y >= offset_y + size_y) return false;
        if (z < offset_z) return false;
        if (z >= offset_z + size_z) return false;

        return true;
    }

    std::vector<double> data_noghost(const std::vector<double> &data) const;

    void data_noghost(const std::vector<double> &data,
                             double *data_no_ghost) const;

    // Convert global coordinate to local index
    inline int g2i(int gx, int gy, int gz) const
    {
        int x = gx - offset_x;
        int y = gy - offset_y;
        int z = gz - offset_z;

        return l2i(x + 1, y + 1, z + 1);
    }
    // Convert local coordinate to local index
    inline int l2i(int x, int y, int z) const
    {
        return x + y * (size_x + 2) + z * (size_x + 2) * (size_y + 2);
    }

private:
    void data_no_ghost_common(const std::vector<double> &data,
                              double *data_no_ghost) const;
};

#endif
