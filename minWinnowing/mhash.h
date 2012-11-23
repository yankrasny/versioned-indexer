#ifndef __MHASH_H__
#define __MHASH_H__
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct { void **buckets; void **chunks; int numC; int numE; } hStruct;

#ifdef __cplusplus
extern "C"
{
#endif
hStruct *initHash();
void destroyHash(hStruct *h);
int spaceHash(hStruct *h);
int lookupHash(unsigned int lh, unsigned int hh, hStruct *h);
int insertHash(unsigned int lh, unsigned int hh, int fragID, hStruct *h);

#ifdef __cplusplus
}  /* end extern "C" */
#endif
#endif
