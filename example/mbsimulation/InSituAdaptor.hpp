#ifndef __IN_SITU_ADAPTOR_HEADER
#define __IN_SITU_ADAPTOR_HEADER

#include <mpi.h>
#include <string>
#include <vector>
#include <queue>
#include <memory>
class Mandelbulb;

namespace InSitu
{

struct Block
{
  Block(unsigned int timeStep,std::vector<Mandelbulb>&mandelbulbList):m_timeStep(timeStep),m_mandelbulbList(mandelbulbList){}
  unsigned int m_timeStep;
  std::vector<Mandelbulb> m_mandelbulbList;
};


void MPIInitialize(const std::string& script);

void MochiInitialize(const std::string& script);

void Finalize();

void MPICoProcess(Mandelbulb& mandelbulb, int nprocs, int rank, double time, unsigned int timeStep);

void MPICoProcessDynamic(MPI_Comm subcomm, std::vector<Mandelbulb>& mandelbulbList,
  int global_nblocks, double time, unsigned int timeStep);

void MochiCoProcess(
  Mandelbulb& mandelbulb, int nprocs, int rank, double time, unsigned int timeStep);

void StoreBlocks(unsigned int timeStep, std::vector<Mandelbulb>& mandelbulbList);

void StoreBlockDisk(std::vector<Mandelbulb>& mandelbulbList, std::string dir, std::string filename);

void isoExtraction(std::vector<Mandelbulb>& mandelbulbList, int global_blocks);

void sampleExtraction(std::vector<Mandelbulb>& mandelbulbList, int global_blocks);

void writeVTKData(std::vector<Mandelbulb>& mandelbulbList,int global_blocks, std::string fileName);

void sampleisoExtraction(std::vector<Mandelbulb>& mandelbulbList, int global_blocks);

}

#endif
