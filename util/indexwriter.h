/*
 * indexwriter.h
 *
 *  Created on: Jan 12, 2010
 *      Author: jhe
 */

#ifndef INDEXWRITER_H_
#define INDEXWRITER_H_
#include "tuple.h"
#define NOLEX 1
#define NORMO 0

class indexwriter {
public:
	indexwriter()
	{
		its = new index_item[DEFAULT_BUF_SIZE];
		its_ptr = 0;
	}


	int open(char* fns, int mode)
	{
		curr_offset = 0;
		sprintf(fn, "%s.dat", fns);
		fdat = fopen(fn, "wb");
		if ( NULL == fdat )
		{
			perror(fn);
			return -1;
		}
		nolex =true;// false;
		//if ( mode & 0x1 == 0 )
		//{
			memset(fn, 0, 256);
			sprintf(fn, "%s.idx", fns);
			fidx = fopen(fn, "wb");
			if ( NULL == fidx)
			{
				perror(fn);
				return -1;
			}
			nolex = true;
		//}

		no_comp = true;
		return 0;
	}

	int add_postings(int wid, post* buf, int freq)
	{
		if (nolex)
			insert_idx_item(curr_offset, wid, freq);
		if ( no_comp )
		{
			if ( NULL != fdat ){
				fwrite(buf, sizeof(post), freq, fdat);
				curr_offset += sizeof(post)*freq;
				return 0;
			}
			else
				return -1;
		}
		else
			return 0;

	}

	int close()
	{
		if ( nolex){
			dump_idx();
			fclose(fidx);
		}

		fclose(fdat);
	}

	virtual ~indexwriter()
	{
		delete[] its;
	}
private:

	char fn[256];
	FILE* fdat;
	FILE* fidx;
	unsigned* dat_buf;
	unsigned* idx_buf;
	index_item* its;
	long curr_offset; //indicate the current offset in the dat file.
	static const int DEFAULT_BUF_SIZE = 10000000;
	bool no_comp; //indicate if we need to compress the index
	bool nolex; //indicate if we need the lexicon file in the index
	int its_ptr;
	inline void insert_idx_item(long offset, int wid, int freq)
	{
		its[its_ptr].offset = offset;
		its[its_ptr].freq = freq;
		its[its_ptr++].wid = wid;

		if ( its_ptr >= DEFAULT_BUF_SIZE )
			dump_idx();

	}

	inline void dump_idx()
	{
		if ( its_ptr != 0 ){
			fwrite(its, sizeof(index_item), its_ptr, fidx);
			its_ptr = 0;
		}
	}
};

#endif /* INDEXWRITER_H_ */
