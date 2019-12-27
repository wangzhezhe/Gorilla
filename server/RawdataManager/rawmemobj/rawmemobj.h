

#include "../blockManager.h"
#include <iostream>

struct RawMemObj : public DataBlockInterface {

  RawMemObj(BlockSummary &blockSummary) : DataBlockInterface(blockSummary){
      std::cout << "RawMemObj is initialised" << std::endl;
  };

  BlockSummary getData(void *&dataContainer){};

  // put data into coresponding data structure for specific implementation
  int putData(void *dataContainer){};

  BlockSummary getDataRegion(std::array<size_t, 3> baseOffset,
                             std::array<size_t, 3> regionShape,
                             void *&rawData){};

  void *m_rawMemPtr = NULL;

  ~RawMemObj(){};

};