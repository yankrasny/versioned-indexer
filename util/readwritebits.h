#ifndef _READWRITEBITS_H_
#define _READWRITEBITS_H_

#include <stdio.h>
#include <stdlib.h>

/* And'ing with bit_mask[n] masks the lower n bits */
#define MASK(j) bit_mask[j]
static unsigned int bit_mask[33] = {
    0x00000000,
    0x00000001, 0x00000003, 0x00000007, 0x0000000f, 0x0000001f, 0x0000003f,
    0x0000007f, 0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
    0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff, 0x0001ffff, 0x0003ffff,
    0x0007ffff, 0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff, 0x1fffffff, 0x3fffffff,
    0x7fffffff, 0xffffffff
};


/*****************************************************/ 
/* write a number of bits (at most 32) into a buffer */
/*****************************************************/
inline void writeBits(unsigned int *buf, unsigned *bp,
               unsigned int val, unsigned int bits)
{                                     
	unsigned int bPtr;
	unsigned int w;

	bPtr = (*bp)&31;

	if (bPtr == 0)
		buf[(*bp)>>5] = 0;

	w = (32 - bPtr > bits)? bits : (32 - bPtr);
	buf[(*bp)>>5] |= ((val&MASK(w))<<bPtr);
	(*bp) += w;

	if (bits > w)
	{
		buf[(*bp)>>5] = (val>>w)&MASK(bits-w);
		(*bp) += (bits-w);
	}
	return;
}

/********************************************************************/
/* read a number of bits (at most 32) from a buffer and return them */
/********************************************************************/
inline unsigned int readBits(unsigned int *buf, unsigned *bp, 
                             unsigned int bits)

{
  unsigned int bPtr;
  unsigned int w;
  unsigned int v;

  bPtr = (*bp)&31;
  w = (32 - bPtr > bits)? bits : (32 - bPtr);
  v = ((buf[(*bp)>>5]>>bPtr) & MASK(w));
  (*bp) += w;

  if (bits == w)  return(v);

  v = v | ((buf[(*bp)>>5] & MASK(bits-w))<<w);
  (*bp) += (bits-w);
  return(v);
}
#endif
