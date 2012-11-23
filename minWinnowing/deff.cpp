#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <utility>
#include <string>
#include <map>
#include <list>


#include "../util/wiki_corpus.h"
#include "../util/utility.h"
#include "partition.h"
#include "../util/indexer.h"
using namespace std;


#define MAXWORD 20000

int* wcounts;
posting* post_buf;
unsigned* frag_count_buf;

int compare(const void* a1, const void* a2)
{
    posting* e1 = (posting*)a1;
    posting* e2 = (posting*)a2;

    int ret = e1->wid - e2->wid;
    int ret2 = e1->pos - e2->pos;

    if ( ret2 == 0 )
        return ret;
    else
        return ret2;
}


int ndoc = 0;
indexer* ib;
partitions* d;
int* outs[2];
int* aux2;

FILE* fbase_frag;
FILE* fhash;
unsigned* mhbuf;

void dothejob(int* buf, int* wcounts, int id , int vs)
{
    int ptr = 0;
    int total_cost = MAXSIZE;
    int mysize = 0;
    int hash_count = 0;
    char fn[256];
    memset(fn, 0, 256);
    sprintf(fn, "meta/%d", id);
    FILE* fhashs = fopen(fn, "rb");
    for ( int i = 0; i < vs; i++)
    {
        if ( wcounts[i]>B)
        {
            int nread = fread(&mhbuf[hash_count], sizeof(unsigned), wcounts[i]-B+1, fhashs);
            hash_count += nread;
        }
    }
    fclose(fhashs);

    hash_count  =0;
    for ( int j = 20; j <= 40; j+=10){
        d->init();
        ptr = 0;
        hash_count = 0;
        for ( int i = 0; i < vs; i++)
        {
            d->fragment(i, &buf[ptr], wcounts[i], &mhbuf[hash_count], j, false);
            if (wcounts[i] > B)
                hash_count = hash_count+wcounts[i]-B+1;
            ptr += wcounts[i];
            //ib->insert_terms(d->add_list, d->add_list_len);
        }
        int size = d->cost_cal();
        if ( size < total_cost)
        {
            mysize = j;
            total_cost = size;
        }
        d->finish();
    }
    d->init();
    ptr = 0;

    fprintf(fbase_frag, "%d\n", d->fID);
    hash_count = 0;
    for ( int i = 0; i < vs; i++)
    {
        d->fragment(ndoc+i, &buf[ptr], wcounts[i], &mhbuf[hash_count], mysize, true);
        ptr += wcounts[i];
        if ( wcounts[i]>B)
            hash_count = hash_count + wcounts[i]-B+1;

        for ( int j = 0; j < d->add_list_len; j++)
            ib->insert_term(d->add_list[j].wid, d->add_list[j].pos, d->add_list[j].vid);

    }
    int count = d->dump_frag();
    frag_count_buf[id] = count;
    ndoc+=vs;
}

int main(int argc, char** argv)
{
    fbase_frag = fopen("base_frag", "w");

    frag_count_buf = new unsigned[250000];
    wiki_corpus wc("url.idx3");
    double total_cost, total_cost2;
    wcounts = new int[25000];
    post_buf = new posting[MAXSIZE];
    outs[0] = new int[MAXSIZE];
    outs[1] = new int[MAXSIZE];
    aux2 = new int[MAXSIZE];
    mhbuf = new unsigned[MAXSIZE];
    fhash= fopen("minWinnowing", "rb");

    int ret;
    //ib = new indexbase(20000000, 2, 1);
    int count = 0;
    int wsize = 20;
    d = new partitions(wsize);
    ib = new indexer(400000000, wsize, 1);
    int start, idx;
    start = 0;

    FILE* fsize = fopen("/data/jhe/wiki_access/word_size", "rb");
    int wsize_ptr = 0;
    int* buf;
    fread(&wsize, sizeof(unsigned), 1, fsize);
    buf = (int*)malloc(sizeof(int)*wsize);
    fread(buf, sizeof(unsigned), wsize, fsize);
    fclose(fsize);

    FILE* fword = fopen("/data/jhe/wiki_access/completeFile", "rb");
    int nd = 0;
    while ( wc.next())
    {
        item its = wc.current();
        int total = 0;
        for ( int i = 0; i < its.vs; i++)
            total += buf[wsize_ptr+i];

        fread(aux2, sizeof(unsigned), total, fword);
        dothejob(aux2, &buf[wsize_ptr], nd, its.vs);
        wsize_ptr+=its.vs;
        printf("Complete:%d\t%d\n", its.id, total);
        nd++;
        //if (nd > 100)
        //	break;
    }

    fclose(fword);

    ib->dump();
    FILE* f = fopen("SUCCESS", "w");
    if ( NULL == f)
        perror("Last!");

    fclose(f);
    fclose(fbase_frag);
    delete d;
    return 0;
}
