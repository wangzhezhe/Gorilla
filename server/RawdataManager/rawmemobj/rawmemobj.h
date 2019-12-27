

#include "../blockManager.h"
#include <iostream>


struct RawMemObj : public DataBlockInterface {

  RawMemObj(BlockSummary &blockSummary) : DataBlockInterface(blockSummary){
      std::cout << "RawMemObj is initialised" << std::endl;
  };

  BlockSummary getData(void *&dataContainer);

  // put data into coresponding data structure for specific implementation
  int putData(void *dataSourcePtr);

   BlockSummary getDataSubregion(std::array<size_t, 3> subregionlb,
                                        std::array<size_t, 3> subregionub,
                                        void *&dataContainer);

  void *m_rawMemPtr = NULL;

  ~RawMemObj(){
    if(m_rawMemPtr!=NULL){
      free(m_rawMemPtr);
    }
  };

};