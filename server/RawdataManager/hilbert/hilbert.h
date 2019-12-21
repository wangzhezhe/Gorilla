#if !defined(_hilbert_h_)
#define _hilbert_h_

#ifdef __cplusplus
extern "C" {
#endif


#ifndef bitmask_t_
#define bitmask_t_
#define bitmask_t 	unsigned long long
#endif

  
/*****************************************************************
 * hilbert_i2c
 * 
 * Convert an index into a Hilbert curve to a set of coordinates.
 * Inputs:
 *  nDims:      Number of coordinate axes.
 *  nBits:      Number of bits per axis.
 *  index:      The index, contains nDims*nBits bits (so nDims*nBits must be <= 8*sizeof(bitmask_t)).
 * Outputs:
 *  coord:      The list of nDims coordinates, each with nBits bits.
 * Assumptions:
 *      nDims*nBits <= (sizeof index) * (bits_per_byte)
 */

void hilbert_i2c(int nDims, int nBits, bitmask_t index, int coord[]);

/*****************************************************************
 * hilbert_c2i
 * 
 * Convert coordinates of a point on a Hilbert curve to its index.
 * Inputs:
 *  nDims:      Number of coordinates.
 *  nBits:      Number of bits/coordinate.
 *  coord:      Array of n nBits-bit coordinates.
 * Outputs:
 *  index:      Output index value.  nDims*nBits bits.
 * Assumptions:
 *      nDims*nBits <= (sizeof bitmask_t) * (bits_per_byte)
 */

bitmask_t hilbert_c2i(int nDims, int nBits, int coord[]);

/*****************************************************************
 * hilbert_cmp
 * 
 * Determine which of two points lies further along the Hilbert curve
 * Inputs:
 *  nDims:      Number of coordinates.
 *  nBytes:     Number of bytes/coordinate.
 * coord1:      Array of nDims nBytes-byte coordinates.
 * coord2:      Array of nDims nBytes-byte coordinates.
 * Return value:
 *      -1, 0, or 1 according to whether coord1<coord2, coord1==coord2, coord1>coord2
 * Assumptions:
 *      nBits <= (sizeof bitmask_t) * (bits_per_byte)
 */

int hilbert_cmp(int nDims, int nBytes, void const* coord1, void const* coord2);

/*****************************************************************
 * hilbert_box_vtx
 * 
 * Determine the first or last vertex of a box to lie on a Hilbert curve
 * Inputs:
 *  nDims:      Number of coordinates.
 *  nBytes:     Number of bytes/coordinate.
 *  findMin:    Is it the least vertex sought?
 *  coord1:     Array of nDims nBytes-byte coordinates - one corner of box
 *  coord2:     Array of nDims nBytes-byte coordinates - opposite corner
 * Output:
 *      c1 and c2 modified to refer to selected corner
 * Assumptions:
 *      nBits <= (sizeof bitmask_t) * (bits_per_byte)
 */
void
hilbert_box_vtx(int nDims, int nBytes, int findMin, void const* c1, void const* c2);

/*****************************************************************
 * hilbert_box_pt
 * 
 * Determine the first or last point of a box to lie on a Hilbert curve
 * Inputs:
 *  nDims:      Number of coordinates.
 *  nBytes:     Number of bytes/coordinate.
 *  findMin:    Is it the least vertex sought?
 *  coord1:     Array of nDims nBytes-byte coordinates - one corner of box
 *  coord2:     Array of nDims nBytes-byte coordinates - opposite corner
 * Output:
 *      c1 and c2 modified to refer to least point
 * Assumptions:
 *      nBits <= (sizeof bitmask_t) * (bits_per_byte)
 */
void
hilbert_box_pt(int nDims, int nBytes, int findMin, void const* coord1, void const* coord2);

/*****************************************************************
 * hilbert_nextinbox
 * 
 * Determine the first point of a box after a given point to lie on a Hilbert curve
 * Inputs:
 *  nDims:      Number of coordinates.
 *  nBytes:     Number of bytes/coordinate.
 *  coord1:     Array of nDims nBytes-byte coordinates - one corner of box
 *  coord2:     Array of nDims nBytes-byte coordinates - opposite corner
 *  point:      Array of nDims nBytes-byte coordinates - lower bound on point returned
 *  
 * Output:
      if returns 1:
 *      c1 and c2 modified to refer to least point after "point" in box
      else returns 0:
        arguments unchanged; "point" is beyond the last point of the box
 * Assumptions:
 *      nBits <= (sizeof bitmask_t) * (bits_per_byte)
 */
int
hilbert_nextinbox(int nDims, int nBytes,
		  void const* coord1, void const* coord2, void const* point);
#ifdef __cplusplus
}
#endif

#endif /* _hilbert_h_ */