#ifndef _READWRITEBITS_H_
#define _READWRITEBITS_H_

#include <stdio.h>
#include <stdlib.h>
#include "bits_op.h"

/*****************************************************/ 
/* write a number of bits (at most 32) into a buffer */
/*****************************************************/
inline void writeBits(unsigned int *buf, unsigned int *bp,
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
inline unsigned int readBits(unsigned int *buf, unsigned int *bp, 
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
