#ifndef __WRITER_H__
#define __WRITER_H__

#include <mpi.h>

#include "../../commondata/metadata.h"
#include "../client/unimosclient.h"
#include <vector>
#include "Mandelbulb_dynamic.hpp"

namespace tl = thallium;
using namespace GORILLA;

class Writer
{
public:
  Writer(tl::engine* clientEnginePtr, int rank)
  {
    m_uniclient = new UniClient(clientEnginePtr, "unimos_server.conf", rank);
    // the cache should be attached with the client firstly
    m_uniclient->getAllServerAddr();
    m_uniclient->m_totalServerNum = m_uniclient->m_serverIDToAddr.size();
  };

  void writetoStaging(size_t step,
    std::vector<Mandelbulb>& mandelbulbList, int block_number_offset, int global_blocks);

  UniClient* m_uniclient = NULL;

  ~Writer()
  {
    if (m_uniclient != NULL)
    {
      delete m_uniclient;
    }
  }
};

#endif
