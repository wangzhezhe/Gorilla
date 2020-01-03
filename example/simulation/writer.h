#ifndef __WRITER_H__
#define __WRITER_H__

#include <mpi.h>

#include "gray-scott.h"
#include "settings.h"
#include <vector>
#include "../../client/unimosclient.h"

namespace tl = thallium;

class Writer
{
public:
    Writer( tl::engine* clientEnginePtr ){
        m_uniclient = new UniClient(clientEnginePtr, "./unimos_server.conf");
    };

    void write(const GrayScott &sim, size_t &step);
    
    UniClient * m_uniclient = NULL;
};

#endif
