/*
 * bits_op.h
 *
 *  Created on: Sep 27, 2010
 *      Author: jhe
 */

#ifndef BITS_OP_H_
#define BITS_OP_H_

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

#define TESTBIT(buffer, bitp) ((((buffer)[(bitp)>>5])>>((bitp)&31))&1)


#endif /* BITS_OP_H_ */
