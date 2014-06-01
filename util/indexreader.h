/*
 * indexreader.h
 *
 *  Created on: Jan 11, 2010
 *      Author: jhe
 */

#ifndef INDEXREADER_H_
#define INDEXREADER_H_
#include <stdlib.h>
#include <stdio.h>
#include "tuple.h"
#include <iostream>


using namespace std;

class indexreader {
public:
	indexreader()
	{
		fidx = NULL;
		fdat = NULL;
		lex_size = 0;
		it_buf_ptr = -1;
	}

	indexreader(int l_size)
	{
		fidx = NULL;
		fdat = NULL;
		set_buf(l_size);
		it_buf_ptr = -1;
	}

	inline int set_buf(int l_size)
	{
		lex_size = l_size;
		it_buf = new index_item[lex_size];
		return 0;
	}

	inline int open(char* fns)
	{
		sprintf(fn, "%s.idx", fns);
		fidx = fopen64(fn, "rb");
		if ( NULL == fidx)
		{
			perror(fn);
			return -1;
		}

		sprintf(fn, "%s.dat", fns);
		fdat = fopen64(fn, "rb");
		if ( NULL == fdat )
		{
			perror(fn);
			return -1;
		}
		it_buf_ptr = -1;
		item_read = fread(it_buf, sizeof(index_item), lex_size, fidx);
		printf("item_read:%d\t%d\n", item_read, it_buf[item_read-1].wid);
		return 1;
	}

	void close()
	{
		fclose(fdat);
		fclose(fidx);
	}

	indexreader(char* fn, int l_size)
	{
		fidx = NULL;
		fdat = NULL;
		set_buf(l_size);
		open(fn);
		it_buf_ptr = -1;
	}

	~indexreader()
	{
		delete[] it_buf;
	}

	bool next()
	{
		if ( fidx == NULL || fdat == NULL)
			return false;

		it_buf_ptr++;
		if ( it_buf_ptr < item_read )
			return true;
		else
		{
			item_read = fread(it_buf, sizeof(index_item), lex_size, fidx);
			it_buf_ptr = 0;
			if ( item_read <= 0 )
				return false;
			else
				return true;
		}
	}

	inline index_item current()
	{
		return it_buf[it_buf_ptr];
	}


	int get_current_posting(post* buf)
	{
		if ( fdat != NULL )
			return fread(buf, sizeof(post), it_buf[it_buf_ptr].freq, fdat);
		else
			return 0;
	}

private:
	char fn[256];
	FILE* fidx;
	FILE* fdat;
	index_item* it_buf;
	int it_buf_ptr;
	int item_read;
	int list_size;
	int lex_size;
};

#endif /* INDEXREADER_H_ */
