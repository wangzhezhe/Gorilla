#include "../hilbert/hilbert.h"
#include <iostream>

int main()
{

    /*
    hilbert_c2i(	unsigned nDims, 
		unsigned nBits, 
		bitmask_t const coord[])

    */
    unsigned nDims = 2;
    unsigned nBits = 3;
    int partitionNum = 2;

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            int sfc_coord[2] = {i, j};
            bitmask_t index = hilbert_c2i(nDims, nBits, sfc_coord);
            bitmask_t reminder = index/(64/partitionNum);
            std::cout << index << "(" << reminder << ")" << "\t";
        }
        std::cout << " " << std::endl;
    }

    return 0;
}