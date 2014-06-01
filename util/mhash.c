#include "mhash.h"

/*****************************************/
/* some basic parameters - do not change */
/*****************************************/

/* maximum bit strength of fragment ID - fragID must be less than (1<<30)-1 */
#define FRAGBITS 30
#define MAXFID ((1<<30)-1)
/* determines number of hash buckets */
#define BUCKBITS 22
/* size of linked list element */
#define ESIZE (72 + sizeof(void *))
//#define ESIZE 76
/* maximum number of chunks */
#define MAXCH 4000
/* size of a chunk */
#define CSIZE 1048192

/*************************************************************************/
/* Test program                                                          */
/*   Compile with  "gcc -O2 -o mhash mhash.c"                            */
/*   Run with  "./mhash 100000 20000" to insert 100000 random hashes and */
/*                    later look them up, and produce output every 20000 */
/*************************************************************************/
/* int main (int argc, char *argv[])
{
  int numh = atoi(argv[1]);
  int nump = atoi(argv[2]);
  unsigned int lh, hh;
  hStruct *h;
  hStruct *initHash();
  void destroyHash();
  int spaceHash();
  int lookupHash();
  int insertHash();
  int i, j;

  int sp, e;                              // some vars for reporting results 
  int found = 0, found2 = 0, overwrite = 0;

  double msrandom();   // this and next two lines for creating random input 
  int s = 123;
  int *seed = &s;
  int milliSecs();     // this and next line is for timing only 
  int t0, t1;

  t0 = milliSecs();
  h = initHash();

  for (i = 1; i <= numh; i++)
  {
    lh = (unsigned int) (65536.0 * (65536.0 * msrandom(seed)));
    hh = (unsigned int) (65536.0 * (65536.0 * msrandom(seed)));
    if (lookupHash(lh, hh, h) != -1)  found++;
    if (insertHash(lh, hh, i, h) == 1)  overwrite++;

    if ((i%nump) == 0)
    {
      t1 = milliSecs();
      for (e = 0, j = 0; j < (1<<BUCKBITS); j++) 
        if (h->buckets[j] != NULL) e++;
      sp = spaceHash(h);
      printf("N: %d  Found: %d  Overw: %d  T: %d  Mem: %d  Bucks: %d\n",
             i, found, overwrite, t1-t0, sp, e);
      t0 = milliSecs();
    }
  }

  printf("Entering second test phase:\n");
  s = 123;
  found = 0;
  overwrite = 0;
  t0 = milliSecs();
  for (i = numh; i > 0 ; i--)
  {
    lh = (unsigned int) (65536.0 * (65536.0 * msrandom(seed)));
    hh = (unsigned int) (65536.0 * (65536.0 * msrandom(seed)));
    j = lookupHash(lh, hh, h);
    if (j == numh-i+1)  found++;
    j = lookupHash(hh, lh, h);
    if (j != -1) found2++;
    if (insertHash(lh, hh, i, h) == 1)  overwrite++;

    if ((i%nump) == 0)
    {
      t1 = milliSecs();
      printf("N: %d  Found: %d  Found2: %d  Overw: %d  T: %d\n",
             i, found, found2, overwrite, t1-t0);
      t0 = milliSecs();
    }
  }

  printf("Entering third test phase:\n");
  s = 123;
  found = 0;
  t0 = milliSecs();
  for (i = numh; i > 0 ; i--)
  {
    lh = (unsigned int) (65536.0 * (65536.0 * msrandom(seed)));
    hh = (unsigned int) (65536.0 * (65536.0 * msrandom(seed)));
    j = lookupHash(lh, hh, h);
    if (j == i)  found++;

    if ((i%nump) == 0)
    {
      t1 = milliSecs();
      printf("N: %d  Found: %d  T: %d\n",
             i, found, t1-t0);
      t0 = milliSecs();
    }
  }
  destroyHash(h);
}
*/

/****************************************************************************/
/* to initialize hash table - returns pointer to struct, or NULL if problem */
/****************************************************************************/
hStruct *initHash()

{
  hStruct *h;
  int i;

  h = (hStruct *) malloc(sizeof(hStruct));
  if (h == NULL) { printf("Could not alloc hStruct\n"); return(NULL); }

  h->buckets = (void **) malloc((1<<BUCKBITS) * sizeof(void *));
  if (h->buckets == NULL) { printf("Could not alloc buckets\n"); return(NULL); }
  for (i = 0; i < (1<<BUCKBITS); i++) h->buckets[i] = NULL;

  h->chunks = (void **) malloc(MAXCH * sizeof(void *));
  if (h->chunks == NULL) { printf("Could not alloc chunks\n"); return(NULL); }
  for (i = 0; i < MAXCH; i++) h->chunks[i] = NULL;

  h->chunks[0] = (void *) malloc(CSIZE * sizeof(char));
  if (h->chunks[0] == NULL) { printf("Could not alloc chunk\n"); return(NULL); }
  h->numC = 0;
  h->numE = 0;

  return(h);
}


/***************************************/
/* to deallocate hash table at the end */
/***************************************/
void destroyHash(hStruct *h)

{
  int i;

  for (i = 0; i <= h->numC; i++)  free(h->chunks[i]);
  free(h->chunks);
  free(h->buckets);
  free(h);
}


/*******************************************************************/
/* returns estimate of current space consumption of hash structure */
/*******************************************************************/
int spaceHash(hStruct *h)

{ 
  return((h->numC+1)*CSIZE + (1<<BUCKBITS)*4 + MAXCH*4); 
}


/*******************************************************************/
/* do lookup in hash table - return fragID if exists, -1 otherwise */
/*******************************************************************/
int lookupHash(unsigned int lh, unsigned int hh, hStruct *h)

{
  void *hptr;
  int i;
  
  /* find the start of the bucket */
  hptr = h->buckets[lh>>(32-BUCKBITS)];

  /* now search for the entry */
  while (hptr != NULL)
  {
    for (i = 0; i < 8; i++)
      if (((unsigned int *)(hptr))[i] == hh)
        if (((unsigned char *)(hptr))[32+i] == (lh&255))
          if ((((unsigned int *)(hptr))[10+i]&3) == ((lh>>8)&3))
            if ((((unsigned int *)(hptr))[10+i]>>2) != MAXFID)
              return((int)(((unsigned int *)(hptr))[10+i]>>2));
    //hptr = ((void **)(hptr))[18];
    hptr = *((void **)(hptr+72));
  }
  return(-1);
}


/****************************************************************************/
/* insert in hash table - return 1 if overwrite, 0 if new hash, -1 if error */
/****************************************************************************/
int insertHash(unsigned int lh, unsigned int hh, int fragID, hStruct *h)

{
  void **optr;
  void *hptr;
  unsigned int nv;
  int i;
  
  /* find the start of the bucket */
  optr = h->buckets+(lh>>(32-BUCKBITS));

  /* now search for the entry */
  while (*optr != NULL)
  {
    hptr = *optr;

    /* check if hash value already has entry - if so, overwrite fragID */
    for (i = 0; i < 8; i++)
      if (((unsigned int *)(hptr))[i] == hh)
        if (((unsigned char *)(hptr))[32+i] == (lh&255))
          if ((((unsigned int *)(hptr))[10+i]&3) == ((lh>>8)&3))
            if ((((unsigned int *)(hptr))[10+i]>>2) != MAXFID)
            {
              nv = (fragID<<2) + (((unsigned int *)(hptr))[10+i]&3);
              ((unsigned int *)(hptr))[10+i] = nv;
              return(1);
            }

    /* if this is last list element in bucket, check if still space */
    optr = ((void **)(hptr+72));
    //optr = ((void **)(hptr))+18;
    if (*optr == NULL)
    {
      for (i = 7; i >= 0; i--)
        if ((((unsigned int *)(hptr))[10+i]>>2) != MAXFID) break;
      if (i < 7)
      {
        i++;
        ((unsigned int *)(hptr))[i] = hh;
        ((unsigned char *)(hptr))[32+i] = lh&255;
        ((unsigned int *)(hptr))[10+i] = (fragID<<2) + ((lh>>8)&3);
        return(0);
      }
    }
  }

  /* need to allocate a new element for this bucket */
  *optr = h->chunks[h->numC] + (ESIZE * h->numE);

  /* and now put new entry into first slot, and initialize other slots */
  hptr = *optr;
  ((unsigned int *)(hptr))[0] = hh;
  ((unsigned char *)(hptr))[32] = lh&255;
  ((unsigned int *)(hptr))[10] = (fragID<<2) + ((lh>>8)&3);
  for (i = 1; i < 8; i++)
    ((unsigned int *)(hptr))[10+i] = (MAXFID<<2);
  *((void **)(hptr+72)) = NULL; 

  /* move pointers to next available space, if necessary alloc new chunk */
  h->numE++;
  if ((h->numE+1)*ESIZE > CSIZE)
  {
    h->numC++;
    h->numE = 0;
    h->chunks[h->numC] = (void *) malloc(CSIZE * sizeof(char));
    if (h->chunks[h->numC] == NULL) { printf("Out of chunks\n"); return(-1); }
  }
  return(0);
}

