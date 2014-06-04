#ifndef __VARBYTE_H_ 
#define __VARBYTE_H_ 

#include "varbyte.cpp"

/**
 * Takes an integer, and varbytes it adding it to the buffer provided.
 *
 * param: num - unsigned int - the number to be recorded
 * param: buff - unsigned char* - the array into which the number is written.
 *
 * return: unsigned - A position into the provided buff 1 index after the compressed number.
 *
 */
unsigned iToV(unsigned num, unsigned char* buff);

/**
 * Takes a buffer and reads one number varbyten number from it.
 *
 * param: buff - unsigned char* - the array containing then numbers that should be read.
 * param: num - the number to store the result in.
 *
 * return: unsigned - The number of bits read.
 */ 
unsigned vToI(unsigned char* buff, unsigned& num);

#endif /* __VARBYTE_H_ */
