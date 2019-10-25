#ifndef __WRITER_H__
#define __WRITER_H__

#include <mpi.h>

#include "gray-scott.h"
#include "settings.h"
#include "../../unimos/client/unimosclient.h"
#include <vector>

namespace tl = thallium;

class Writer
{
public:
    Writer(const Settings &settings);
    //use this if write to the files
    //void open(const std::string &fname);
    void write(const GrayScott &sim, tl::engine &myEngine, size_t &step, size_t &blockID);
    //oid close();

protected:
    std::string m_serverMasterAddr;
};

#endif
