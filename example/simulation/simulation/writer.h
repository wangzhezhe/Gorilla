#ifndef __WRITER_H__
#define __WRITER_H__

#include <mpi.h>

#include "gray-scott.h"
#include "settings.h"
#include <vector>

class Writer
{
public:
    Writer(const Settings &settings, const GrayScott &sim, adios2::IO io);
    //use this if write to the files
    void open(const std::string &fname);
    void write(int &step, const GrayScott &sim);
    void close();

protected:
    Settings settings;
};

#endif
