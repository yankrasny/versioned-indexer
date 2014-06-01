/*
 * indexer.h
 *
 *  Created on: Jan 12, 2010
 *      Author: jhe
 */

#ifndef INDEXER_H_
#define INDEXER_H_

#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <map>
#include <string.h>
#include <string>
#include "tuple.h"
#include "indexwriter.h"

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

class indexer
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
    indexwriter* writer;
public:
    indexer(int buf_size, int type, int s)
    {
        MAX_TUPS_PER_BLK = buf_size;
        tups = new tuple[MAX_TUPS_PER_BLK];
        extra_postings = new post[MAX_TUPS_PER_BLK];
        nblocks = 0;
        ntups = 0;
        mytype = type;
        mysize = s;
        writer = new indexwriter();
    }

    ~indexer()
    {
        FILE* f = fopen("INDEX", "w");
        fprintf(f, "%d\n", nblocks); 
        fclose(f);
        delete[] tups;
        delete[] extra_postings;
        delete writer;
    }

    // wordID, position?, docID
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
        qsort(tups, ntups, sizeof(tuple), compare_tuple);

        sprintf(fname, "data/0_%d_%d_%d", mytype, mysize, nblocks);
        int ret = writer->open(fname, 0);
        if ( ret < 0 )
            return;

        int term_count = 0;
        unsigned last_term;

        last_term = tups[0].word;
        extra_postings[term_count].docid = tups[0].doc_id;
        extra_postings[term_count++].pos = tups[0].pos;

        for ( int i = 1; i < ntups; i++)
        {
            if ( last_term != tups[i].word )
            {
                writer->add_postings(last_term, extra_postings, term_count);
                term_count = 0;
                last_term = tups[i].word;
            }

            extra_postings[term_count].docid = tups[i].doc_id;
            extra_postings[term_count++].pos = tups[i].pos;
        }

        if ( term_count != 0 ){
            writer->add_postings(last_term, extra_postings, term_count);
        }

        writer->close();
        ntups = 0;
        nblocks++;
        return;

    }

    //set current index id number;
    inline void set_idx(int idx)
    {
        nblocks = idx;
    }

    //get current index id number;
    inline int get_idx()
    {
        return nblocks;
    }

};

#endif /* INDEXER_H_ */
