/* See LICENSE below for information on rights to use, modify and distribute
   this code. */

/* 
 * hilbert.c - Computes Hilbert space-filling curve coordinates, without
 * recursion, from integer index, and vice versa, and other Hilbert-related
 # calculations.
 * 
 * Author:      Doug Moore
 *              Dept. of Computational and Applied Math
 *              Rice University
 *              http://www.caam.rice.edu/~dougm
 * Date:        Wed Jul  15 1998
 * Copyright (c) 1998, Rice University
 *
 * Acknowledgement:
 * This implementation is based on the work of A. R. Butz ("Alternative
 * Algorithm for Hilbert's Space-Filling Curve", IEEE Trans. Comp., April,
 * 1971, pp 424-426) and its interpretation by Spencer W. Thomas, University
 * of Michigan (http://www-personal.umich.edu/~spencer/Home.html) in his widely
 * available C software.  While the implementation here differs considerably
 * from his, the first two interfaces and the style of some comments are very
 * much derived from his work. */

#include "hilbert.h"

#define adjust_rotation(rotation,nDims,bits)				\
do {									\
      /* rotation = (rotation + 1 + ffs(bits)) % nDims; */		\
      bits &= -bits & ((1 << (nDims-1)) - 1);				\
      while (bits)							\
	bits >>= 1, ++rotation;						\
      if ( ++rotation >= nDims )					\
	rotation -= nDims;						\
} while (0)

#define rotateRight(arg, nRots, nDims)					\
((((arg) >> (nRots)) | ((arg) << ((nDims)-(nRots)))) & ((1 << (nDims)) - 1))


     
/*****************************************************************
 * hilbert_i2c
 * 
 * Convert an index into a Hilbert curve to a set of coordinates.
 * Inputs:
 *  nDims:      Number of coordinate axes.
 *  nBits:      Number of bits per axis.
 *  index:      The index, contains nDims*nBits bits
 *              (so nDims*nBits must be <= 8*sizeof(bitmask_t)).
 * Outputs:
 *  coord:      The list of nDims coordinates, each with nBits bits.
 * Assumptions:
 *      nDims*nBits <= (sizeof index) * (bits_per_byte)
 */
void
hilbert_i2c(int nDims, int nBits, bitmask_t index, int coord[])
{
  bitmask_t const one = 1;
  bitmask_t const ndOnes = (one << nDims) - 1;
  bitmask_t const nthbits = (((one << nDims*nBits) - one) / ndOnes) >> 1;
  int b,d;
  int rotation = 0; /* or (nBits * (nDims-1)) % nDims; */
  bitmask_t reflection = 0;

  for ( d = 0; d < nDims; d++ )
    coord[d] = 0;

  index ^= index >> 1;
  index ^= nthbits;
  
  for (b = nBits; b--;)
    {
      bitmask_t bits = (index >> nDims*b) & ndOnes;
      reflection ^= rotateRight(bits, nDims-rotation, nDims);
      for ( d = 0; d < nDims; d++ )
	coord[d] |= ((reflection >> d) & 1) << b;
      reflection ^= one << rotation;

      adjust_rotation(rotation,nDims,bits);
    }
}

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
/* MODIFIED by A. Donev--A Fortran-like entry point to hilbert_c2i */
bitmask_t
fhilbert_c2i(int *nDims, int *nBits, int *coord)
{
return hilbert_c2i(*nDims, *nBits, coord) ;
}
 
bitmask_t
hilbert_c2i(int nDims, int nBits, int coord[])
{
  bitmask_t const one = 1;
  bitmask_t const ndOnes = (one << nDims) - 1;
  bitmask_t const nthbits = (((one << nDims*nBits) - one) / ndOnes) >> 1;
  int b, d;
  int rotation = 0; /* or (nBits * (nDims-1)) % nDims; */
  bitmask_t reflection = 0;
  bitmask_t index = 0;

  for (b = nBits; b--;)
    {
      bitmask_t bits = reflection;
      reflection = 0;
      for ( d = 0; d < nDims; d++ )
	reflection |= ((coord[d] >> b) & 1 ) << d;
      bits ^= reflection;
      bits = rotateRight(bits, rotation, nDims);
      index |= bits << nDims*b;
      reflection ^= one << rotation;

      adjust_rotation(rotation,nDims,bits);
    }
  index ^= nthbits;
/*
  for (d = 1; index >> d; d *= 2)
    index ^= index >> d;
*/
  for (d = 1; ; d *= 2) {
    bitmask_t t;
    if (d <= 32) {
       t = index >> d;
       if (!t)
          break;
       }
    else {
       t = index >> 32;
       t = t >> (d - 32);
       if (!t)
          break;
       }
    index ^= t;
    }
  return index;
}

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
 *      -1, 0, or 1 according to whether
           coord1<coord2, coord1==coord2, coord1>coord2
 * Assumptions:
 *      nBits <= (sizeof bitmask_t) * (bits_per_byte)
 */
int
hilbert_cmp(int nDims, int nBytes, void const* c1, void const* c2)
{
  bitmask_t const one = 1;
  int y, b, d;
  int rotation = 0; /* or (nBits * (nDims-1)) % nDims; */
  bitmask_t reflection = 0;
  bitmask_t index = 0;

  for (y = 0; y < nBytes; ++y)
    for (b = 8; b--; )
      {
	bitmask_t bits = reflection, bts2 = bits;
	bitmask_t reflexion2 = 0;
	reflection = 0;
	for ( d = 0; d < nDims; d++ )
	  {
	    reflection |= ((((char*)c1)[y+d*nBytes] >> b) & 1 ) << d;
	    reflexion2 |= ((((char*)c2)[y+d*nBytes] >> b) & 1 ) << d;
	  }
	bits ^= reflection;
	bits = rotateRight(bits, rotation, nDims);
	if (reflection != reflexion2)
	  {
	    bts2 ^= reflexion2;
	    bts2 = rotateRight(bts2, rotation, nDims);

	    for (d = 1; d < nDims; d *= 2)
	      {
		index ^= index >> d;
		bits  ^= bits  >> d;
		bts2  ^= bts2  >> d;
	      }
	    return (((index ^ b) & 1) == (bits < bts2))? -1: 1;
	  }
	index ^= bits;
	reflection ^= one << rotation;
	adjust_rotation(rotation,nDims,bits);
      }
  return 0;
}

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
hilbert_box_vtx(int nDims, int nBytes, int findMin,
		void const* c1, void const* c2)
{
  bitmask_t const one = 1;
  bitmask_t const ndOnes = (one << nDims) - 1;
  int y, b, d;
  int rotation = 0; /* or (nBits * (nDims-1)) % nDims; */
  bitmask_t reflection = 0;
  bitmask_t index = 0;
  bitmask_t bitsFolded = 0;

  for (y = 0; y < nBytes; ++y)
    for (b = 8; b--; )
      {
	bitmask_t bits = reflection;
	bitmask_t diff = 0;
	reflection = 0;
	for ( d = 0; d < nDims; d++ )
	  {
	    reflection |= ((((char*)c1)[y+d*nBytes] >> b) & 1 ) << d;
	    diff       |= ((((char*)c2)[y+d*nBytes] >> b) & 1 ) << d;
	  }
	diff ^= reflection;
	if (diff)
	  {
	    bitmask_t smear = rotateRight(diff, rotation, nDims) >> 1;
	    bitmask_t digit = bits;

	    digit ^= reflection;
	    digit = rotateRight(digit, rotation, nDims);
	    for (d = 1; d < nDims; d *= 2)
	      {
		index ^= index >> d;
		digit ^= (digit  >> d) &~ smear;
		smear |= smear >> d;
	      }
	    if ((index ^ b ^ findMin) & 1)
	      digit ^= smear+1;
	    digit = rotateRight(digit, nDims-rotation, nDims) & diff;

	    for (d = 0; d < nDims; ++d)
	      if ((diff >> d) & 1)
		{
		  unsigned yy;
		  char* src, * dst;
		  if ((digit >> d) & 1)
		    { src = (char*) c2; dst = (char*) c1; }
		  else
		    { src = (char*) c1; dst = (char*) c2; }
		  for (yy = y; yy < nBytes; ++yy)
		    dst[yy+d*nBytes] = src[yy+d*nBytes];
		}
	    
	    bitsFolded |= diff;
	    if (bitsFolded == ndOnes)
	      return;
	    index &= 1;
	    reflection ^= digit;
	  }
	
	bits ^= reflection;
	bits = rotateRight(bits, rotation, nDims);
	index ^= bits;
	reflection ^= one << rotation;
	adjust_rotation(rotation,nDims,bits);
      }
}

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
hilbert_box_pt(int nDims, int nBytes, int findMin,
	       void const* c1, void const* c2)
{
  bitmask_t const one = 1;
  int y, b, d;
  int rotation = 0; /* or (nBits * (nDims-1)) % nDims; */
  bitmask_t reflection = 0;
  bitmask_t index = 0;
  bitmask_t fold1 = 0, fold2 = 0;
  bitmask_t valu1 = 0, valu2 = 0;

  for (y = 0; y < nBytes; ++y)
    for (b = 8; b--; )
      {
	char const bthbit = 1 << b;
	bitmask_t bits = reflection;
	bitmask_t diff = 0;
	reflection = 0;
	for ( d = 0; d < nDims; d++ )
	  {
	    char* cc1 = &((char*)c1)[y+d*nBytes];
	    char* cc2 = &((char*)c2)[y+d*nBytes];
	    if ((fold1 >> d) & 1)
	      if ((valu1 >> d) & 1) *cc1 |= bthbit;
	      else		    *cc1 &= ~bthbit;
	    if ((fold2 >> d) & 1)
	      if ((valu2 >> d) & 1) *cc2 |= bthbit;
	      else		    *cc2 &= ~bthbit;
	    reflection |= ((*cc1 >> b) & 1 ) << d;
	    diff       |= ((*cc2 >> b) & 1 ) << d;
	  }
	/*	reflection = (reflection &~ fold1) | valu1; */
	/*	diff       = (diff       &~ fold2) | valu2; */
	diff ^= reflection;
	if (diff)
	  {
	    bitmask_t smear = rotateRight(diff, rotation, nDims) >> 1;
	    bitmask_t digit = bits;

	    digit ^= reflection;
	    digit  = rotateRight(digit, rotation, nDims);
	    for (d = 1; d < nDims; d *= 2)
	      {
		index ^= index >> d;
		digit ^= (digit  >> d) &~ smear;
		smear |= smear >> d;
	      }
	    if ((index ^ b ^ findMin) & 1)
	      digit ^= smear+1;
	    digit = rotateRight(digit, nDims-rotation, nDims) & diff;

	    for (d = 0; d < nDims; ++d)
	      if ((diff >> d) & 1)
		((char*) (((digit >> d) & 1)? c1: c2))[y+d*nBytes] ^= bthbit;
	    
	    diff ^= digit;
	    fold1 |= digit;
	    fold2 |= diff;
	    reflection ^= digit;
	    valu1 |= ~reflection & digit;
	    valu2 |= ~reflection & diff;
	    index &= 1;
	  }
	
	bits ^= reflection;
	bits = rotateRight(bits, rotation, nDims);
	index ^= bits;
	reflection ^= one << rotation;
	adjust_rotation(rotation,nDims,bits);
      }
}

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
		  void const* c1, void const* c2, void const* pt)
{
  bitmask_t const one = 1;
  int y, b, d;
  int rotation = 0; /* or (nBits * (nDims-1)) % nDims; */
  bitmask_t point = 0;
  bitmask_t index = 0;
  bitmask_t fold1 = 0, fold2 = 0;
  bitmask_t valu1 = 0, valu2 = 0;
  int p_y, p_b;
  bitmask_t p_clash = 0, p_clashstart;
  bitmask_t p_diff, p_point, p_reflection;
  bitmask_t p_fold1, p_fold2, p_valu1, p_valu2;

  for (y = 0; y < nBytes; ++y)
    for (b = 8; b--; )
      {
	bitmask_t clash, clashstart;
	bitmask_t bits = point;
	bitmask_t reflection = 0;
	bitmask_t diff = 0;
	bitmask_t digit;
	point = 0;
	for ( d = 0; d < nDims; d++ )
	  {
	    reflection |= ((((char*)c1)[y+d*nBytes] >> b) & 1 ) << d;
	    diff       |= ((((char*)c2)[y+d*nBytes] >> b) & 1 ) << d;
	    point      |= ((((char*)pt)[y+d*nBytes] >> b) & 1 ) << d;
	  }
	reflection = (reflection &~ fold1) | valu1;
	diff       = (diff       &~ fold2) | valu2;
	diff ^= reflection;
	clash = (reflection ^ point) &~ diff;

	if (clash || diff)
	  /* compute (the complement of) a "digit" in the integer index of this
	     point */
	  {
	    digit = bits ^ point;
	    digit  = rotateRight(digit, rotation, nDims);
	    clash = rotateRight(clash,rotation,nDims);
	    for (d = 1; d < nDims; d *= 2)
	      {
		index ^= index >> d;
		digit ^= digit >> d;
		clash |= clash >> d;
	      }
	    if ((index ^ b) & 1)
	      digit ^= (one << nDims) - 1;
	    index &= 1;

	    clashstart = clash - (clash >> 1);
	    clash = rotateRight(clash,nDims-rotation,nDims);
	    clashstart = rotateRight(clashstart,nDims-rotation,nDims);
	    diff &= ~clash;
	  }
	
	if (diff)
	  {
	    bitmask_t p_p_clash = digit & rotateRight(diff, rotation, nDims);
	    if (p_p_clash)
	      {
		p_clashstart = p_p_clash & -p_p_clash;
		p_clash = 2*p_clashstart-1;
		p_clash = rotateRight(p_clash, nDims-rotation, nDims);
		p_clashstart = rotateRight(p_clashstart, nDims-rotation, nDims);
		p_diff = diff &~ (p_clash ^ p_clashstart);
		p_reflection = reflection;
		p_y = y;
		p_b = b;
		p_point = point ^ p_clashstart;
		p_fold1 = fold1;
		p_fold2 = fold2;
		p_valu1 = valu1;
		p_valu2 = valu2;
	      }
	  }

	if (digit < (digit^(rotateRight(clash,rotation,nDims))))
	  /* use next box */
	  {
	    if (!p_clash) return 0; /* no next point */
	    clash = p_clash;
	    clashstart = p_clashstart;
	    y = p_y;
	    b = p_b;
	    diff = p_diff;
	    point = p_point;
	    reflection = p_reflection;
	    fold1 = p_fold1;
	    fold2 = p_fold2;
	    valu1 = p_valu1;
	    valu2 = p_valu2;
	  }
	
	if (diff)
	  {
	    /* reduce currbox */
	    reflection = (reflection ^ point) & diff;
	    diff ^= reflection;
	    fold1 |= reflection;
	    fold2 |= diff;
	    valu1 |= ~point & reflection;
	    valu2 |= ~point & diff;
	  }
	
	if (clash)
	  {
	    for ( d = 0; d < nDims; d++ )
	      {
		int yy;
		char hipart, lo1, lo2, hibits;
		char* cc1 = &((char*)c1)[d*nBytes];
		char* cc2 = &((char*)c2)[d*nBytes];
		char* pnt = &((char*)pt)[d*nBytes];
		for (yy = 0; yy < y; ++yy)
		  cc1[yy] = cc2[yy] = pnt[yy];

		hibits = ~0 << b;
		if (((clash^clashstart) >> d) & 1)
		  hibits <<= 1;
		hipart = pnt[y] & hibits;
		if ((clashstart >> d) & 1)
		  hipart ^= 1<<b;

		if ((fold1 >> d) & 1)
		  {
		    lo1 = -((valu1 >> d) & 1);
		    for (yy = y+1; yy < nBytes; ++yy)
		      cc1[yy] = lo1;
		  }
		else lo1 = cc1[y];
		cc1[y] = hipart | (lo1 &~ hibits);

		if ((fold2 >> d) & 1)
		  {
		    lo2 = -((valu2 >> d) & 1);
		    for (yy = y+1; yy < nBytes; ++yy)
		      cc2[yy] = lo2;
		  }
		else lo2 = cc2[y];
		cc2[y] = hipart | (lo2 &~ hibits);
	      }

	    hilbert_box_pt(nDims, nBytes, 1, c1, c2);
	    return 1;
	  }
	
	bits ^= point;
	bits = rotateRight(bits, rotation, nDims);
	index ^= bits;
	point ^= one << rotation;
	adjust_rotation(rotation,nDims,bits);
      }

  /* point is in box */
  {
    char* cc1 = &((char*)c1)[d*nBytes];
    char* cc2 = &((char*)c2)[d*nBytes];
    char* pnt = &((char*)pt)[d*nBytes];
    for (y = 0; y < nDims; ++y)
      cc1[y] = cc2[y] = pnt[y];
  }
  return 1;
}



/*****************************************************************
 * hilbert_incr
 * 
 * Convert coordinates of a point on a Hilbert curve to its index.
 * Inputs:
 *  nDims:      Number of coordinates.
 *  nBits:      Number of bits/coordinate.
 *  coord:      Array of nDims nBits-bit coordinates.
 * Outputs:
 *  coord:      Next point on Hilbert curve
 * Assumptions:
 *      nDims*nBits <= (sizeof bitmask_t) * (bits_per_byte)
 */

/*todo*/

/* LICENSE
 *
 * This software is copyrighted by Rice University.  It may be freely copied,
 * modified, and redistributed, provided that the copyright notice is 
 * preserved on all copies.
 * 
 * There is no warranty or other guarantee of fitness for this software,
 * it is provided solely "as is".  Bug reports or fixes may be sent
 * to the author, who may or may not act on them as he desires.
 *
 * You may include this software in a program or other software product,
 * but must display the notice:
 *
 * Hilbert Curve implementation copyright 1998, Rice University
 *
 * in any place where the end-user would see your own copyright.
 * 
 * If you modify this software, you should include a notice giving the
 * name of the person performing the modification, the date of modification,
 * and the reason for such modification.
 */


/* What remains is test code that won't be compiled unless you define the
   TEST_HILBERT preprocessor symbol */

#ifdef TEST_HILBERT
#include <stdio.h>

main()
{
#define maxDim (8*sizeof(bitmask_t))
  int coord[maxDim], coord1[maxDim], nDims, nBits, nBytes, i;
  bitmask_t r, r1;

  for (;;)
    {
      printf( "Enter nDims, nBits: " );
      scanf( "%d", &nDims );
      if ( nDims == 0 )
	break;
      scanf( "%d", &nBits );
      while ( (i = getchar()) != '\n' && i != EOF )
	;
      if ( i == EOF )
	break;
      nBytes = (nBits+31)/32*4;
      
      for ( r = 0; r < 1 << (nDims*nBits); r++ )
        {
	  hilbert_i2c( nDims, nBits, r, coord );
	  printf("%d: ", (unsigned)r);
	  for ( i = 0; i < nDims; i++ )
	    printf(" %d", coord[i]);
	  printf("\n");
	  r1 = hilbert_c2i( nDims, nBits, coord );
	  if ( r != r1 )
	    printf( "r = 0x%x; r1 = 0x%x\n", (unsigned)r, (unsigned)r1);

	  for (r1 = 0; r1 < r; ++r1 )
	    {
	      int j;
	      int inf_dist = 0;
	      int ans;
	      hilbert_i2c( nDims, nBits, r1, coord1 );
	      ans = hilbert_cmp( nDims, nBytes, coord, coord1);
	      if (ans != 1)
		printf( "cmp r = 0x%0*x; r1 = 0x%0*x, ans = %2d\n", (nDims*nBits+3)/4, (unsigned)r,
                        (nDims*nBits+3)/4, (unsigned)r1, ans );
	    }
	  hilbert_i2c( nDims, nBits, r1, coord1 );
	  if (hilbert_cmp( nDims, nBytes, coord, coord1) != 0)
	    printf( "cmp r = 0x%0*x; r1 = 0x%0*x\n", (nDims*nBits+3)/4, (unsigned)r,
		    (nDims*nBits+3)/4, (unsigned)r1 );

        }
    }
}

#endif