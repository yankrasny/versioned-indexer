/* size of small window over which we hash - try between 7 and 20 or so */
#define B_WINNOW_VAR 10 
#define MIN(a,b) ((a)<(b))?(a):(b)

/*******************************************/
/* hash implementation based on Adler hack */
/*******************************************/
unsigned int t_hash(const unsigned char* buff, int len)

{
  unsigned int s1 = 0;
  unsigned int s2 = 0;
  unsigned int s3 = 0;
  unsigned int s4 = 0;
  int i;

  for(i = 0; i < len; i++)
  {
    s1 += *(buff++);
    s2 += s1;
    s3 += s2;
    s4 += s3;
  }

  s1 &= 0xff;  s2 &= 0xff;  s3 &= 0xff;  s4 &= 0xff;
  return (s4 << 24) | (s3 << 16) | (s2 << 8) | s1;
}


/******************************************************/
/* precompute 32-bit hash values for next BLOCK bytes */
/******************************************************/
int preComp(unsigned char *f, int fSize, unsigned int *h)
{
	unsigned int i;
	register unsigned int t0, t1, t2;
	unsigned int c1, c2, c3, c4;

	h[0] = t_hash(f, B_WINNOW_VAR);
	c1 = h[0] & 0xff;
	c2 = (h[0]>>8) & 0xff;
	c3 = (h[0]>>16) & 0xff;
	c4 = (h[0]>>24) & 0xff;
	t0 = B_WINNOW_VAR;
	t1 = (B_WINNOW_VAR * (B_WINNOW_VAR+1)) / 2;
	t2 = (t1 * (B_WINNOW_VAR+2))/3;
	for (i = 1; i < fSize; i++)
	{
		c1 = (c1 - f[i-1] + f[i-1+B_WINNOW_VAR]) & 0xff;
		c2 = (c2 - f[i-1]*B_WINNOW_VAR + c1) & 0xff;
		c3 = (c3 - f[i-1]*t1 + c2) & 0xff;
		c4 = (c4 - f[i-1]*t2 + c3) & 0xff;
		h[i] = ((c4<<24) | (c3<<16) | (c2<<8) | c1);
	}
	return fSize;
}


/***********************************************************************/
/* Computes set of boundaries under winnowing, given an input file.    */
/*               Returns # of created blocks, or -1 if error occurred  */
/*                                                                     */
/*   file:  input file. It is assumed each word hashed to char already */
/*   fSize: size of input file                                         */
/*   w:     size of large window where we select minimum (try 20-100)  */
/*   starts:  block boundaries - starts[i] is first element in block i */ 
/***********************************************************************/
int winnow(unsigned char *file, int fSize, int w, int *starts)
{
  unsigned int *hval;   /* temporary buffer for hash values */
  int min;              /* pointer to current minimum in window */
  int num;              /* number of blocks generated so far */
  int i, j;

  hval = (unsigned int *) malloc(fSize * sizeof(unsigned int));
  if (hval == NULL) return(-1); 
  preComp(file, fSize-B_WINNOW_VAR+1, hval);
  starts[0] = 0;
  num = 1;
  min = 0;
  for (i = 1; i < fSize-w-B_WINNOW_VAR+2; i++)
  {
    if (min == i-1)
    {
      for (j = i+1, min = i; j < i+w; j++)
        if (hval[j] <= hval[min])  min = j;
      starts[num++] = min;
    }
    else
      if (hval[i+w-1] <= hval[min])  starts[num++] = min = i+w-1;
  }

  free(hval);
  return(num);
}

int minWinnow(unsigned char* file, int fSize, int* starts)
{
	int num = 1;
	unsigned* hval;
	hval = (unsigned*)malloc(fSize*sizeof(unsigned));
	preComp(file, fSize-B_WINNOW_VAR+1, hval);
	starts[0] = 0;
	
	int i;
	for ( i = 1; i < fSize-B_WINNOW_VAR+1; i++)
	{
		int j = 1;
		while ( i-j>=0 && i+j < fSize-B_WINNOW_VAR+1 && hval[i+j] > hval[i] && hval[i-j] > hval[i])
			j++;

		j--;
		starts[num++] = j;
	}
	if ( num != fSize-B_WINNOW_VAR+1){
		printf("problems!\n");
		exit(0);
	}
	return num;
}

