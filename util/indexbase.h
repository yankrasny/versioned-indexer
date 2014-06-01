/*
 * indexbase.h
 *
 *  Created on: Sep 3, 2009
 *      Author: jhe
 */

#ifndef INDEXBASE_H_
#define INDEXBASE_H_

#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <map>
#include <string.h>
#include <string>
#include "tuple.h"

//#include "wiki_corpus.h"
using namespace std;


int compare_tuple(const void* e1, const void* e2)
{
	tuple* a = (tuple*)e1;
	tuple* b = (tuple*)e2;

	int ret = a->word - b->word;
	if ( ret != 0 )
		return ret;

	if ( a->doc_id < b->doc_id)
		return -1;
	else if ( a->doc_id > b->doc_id)
		return 1;

	if ( a->pos > b->pos)
		return 1;
	else
		return -1;

}

class indexbase
{
private:
	unsigned nblocks;
	int ntups;
	int mytype;
	tuple* tups;
	int MAX_TUPS_PER_BLK;
	post* extra_postings;
	char fname[256];
	int mysize;

public:
	indexbase(int buf_size, int type, int s)
	{
		MAX_TUPS_PER_BLK = buf_size;

		tups = new tuple[MAX_TUPS_PER_BLK];

		extra_postings = new post[MAX_TUPS_PER_BLK];

		nblocks = 0;
		ntups = 0;
		mytype = type;
		mysize = s;

	}

	void insert_terms(posting* pbuf, int len)
	{
		int i = 0;
		//printf("type:%d\t%d\t%d\t%d\n", mytype, pbuf[i].wid, pbuf[i].pos, pbuf[i].vid);
		for ( int i = 0; i < len; i++)
			insert_term(pbuf[i].wid, pbuf[i].pos, pbuf[i].vid);
	}

	void insert_term(int wordid, int pos, int docid)
	{

		tups[ntups].word = wordid;
		tups[ntups].doc_id = docid;
		tups[ntups].pos = pos;

		ntups++;

		if ( ntups >= MAX_TUPS_PER_BLK )
		{
			dump();
			ntups = 0;
		}

		return;

	}

	void dump()
	{

		FILE*data, *lexs;

		qsort(tups, ntups, sizeof(tuple), compare_tuple);

		sprintf(fname, "data/0_%d_%d_%d.idx", mytype, mysize, nblocks);

		if ((lexs = fopen(fname, "wb")) == NULL)
		{
			perror(fname);
			return;
		}

		sprintf(fname, "data/0_%d_%d_%d.dat", mytype, mysize, nblocks);

		if ((data = fopen(fname, "wb")) == NULL)
		{
			perror("parser_dump: unable to open inf file to write");
			return;
		}

		int term_count = 0;

		unsigned last_term;

		last_term = tups[0].word;

		long pos = 0;

		extra_postings[term_count].docid = tups[0].doc_id;

		extra_postings[term_count++].pos = tups[0].pos;

		for ( int i = 1; i < ntups; i++)
		{

			if ( last_term != tups[i].word )
			{

				fwrite(extra_postings, sizeof(post), term_count, data);
				//fprintf(lexs, "%ld\t%d\t%d\n", pos, term_count, last_term);
				fwrite(&pos, sizeof(long long), 1, lexs);
				fwrite(&term_count, sizeof(int), 1, lexs);
				fwrite(&last_term, sizeof(int), 1, lexs);
				pos += sizeof(post)*term_count;
				term_count = 0;
				last_term = tups[i].word;
			}

			extra_postings[term_count].docid = tups[i].doc_id;
			extra_postings[term_count++].pos = tups[i].pos;

		}

		if ( term_count != 0 ){
			fwrite(extra_postings, sizeof(post), term_count, data);
			//fprintf(lexs, "%ld\t%d\t%d\n", pos, term_count, last_term);
			fwrite(&pos, sizeof(long long), 1, lexs);
			fwrite(&term_count, sizeof(int), 1, lexs);
			fwrite(&last_term, sizeof(int), 1, lexs);
		}

		fclose(lexs);
		fclose(data);

		ntups = 0;

		nblocks++;
		return;

	}

	int get_idx()
	{
		return nblocks;
	}

	void set_idx(int idx)
	{
		nblocks = idx;
	}

};

#endif /* INDEXBASE_H_ */
