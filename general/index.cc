/*
 * diff.cc
 *
 *  Created on: May 15, 2011
 *      Author: jhe
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>


//#define MAXSIZE 250939073
#define	MAXSIZE 81985059 			/*  */
struct binfo
{
    int total, total_dis, post;
};

#include "general_partition.h"
#include "../util/indexer.h"
#include	<fstream>

using namespace std;

//#define PRINTTREE 1
int* buf;
int* wcounts;
int* wsizes;
partitions<CutDocument2>* par;

int ndoc = 0;
int* mhbuf;
int* hbuf;
unsigned char* tree_buf;
indexer* ib;

int dothejob(int* buf, int* wcounts, int id, int vs, float wsize)
{
    char fn[256];
    memset(fn, 0, 256);
    sprintf(fn, "/data4/jhe/proximity/model/test/%d", id);
    FILE* f = fopen(fn, "r");
    int total = 1;
    int a,b,c,d,e,fs;
    while ( fscanf(f, "%d\t%d\t%d\t%d\t%d\t%d\n", &a, &b, &c, &d, &e, &fs)>0){
        wsizes[total] = a;
        total++;
    }
    wsizes[0] = 5;
    fclose(f);
    memset(fn, 0, 256);
    sprintf(fn, "/data4/jhe/proximity/general/meta/%d", id);
    f = fopen(fn, "rb");
    int size;
    fread(&size, sizeof(unsigned), 1, f);
    fread(mhbuf, sizeof(unsigned), size, f);
    fclose(f);
     memset(fn, 0, 256);
        sprintf(fn, "/data4/jhe/proximity/minWinnowing/meta/%d", id);
        f = fopen(fn, "rb");
        fread(&size, sizeof(unsigned), 1, f);
        fread(hbuf, sizeof(unsigned), size, f);
        fclose(f);


    iv* trees = new iv[vs];
    int wptr = 0;
    int hptr = 0;
    par->init();
    for ( int i = 0; i < vs; i++)
    {
        if ( wcounts[i] > 0 )
            par->fragment(i, &buf[wptr], wcounts[i], &mhbuf[hptr], &hbuf[hptr],  &trees[i], wsizes, total);
        wptr += wcounts[i];
        if (wcounts[i] > B)
        {
            hptr += (wcounts[i]-B+1);
        }

        trees[i].complete();
    }

    printf("finish sorting!\n");
    par->completeCount(wsize);
    par->select(trees, wsize);
    par->finish3(buf, wcounts, id, vs);

    for ( int i = 0; i < par->add_list_len; i++)
    {
        // par->add_list is a posting*
        // wid is wordID
        // pos is position (most likely)
        // vid is versionID, and we're using version IDs as doc IDs
        ib->insert_term(par->add_list[i].wid, par->add_list[i].pos, par->add_list[i].vid);
    }
    delete[] trees;

    ndoc += vs;
    return par->add_list_len;
}


int main(int argc, char**argv)
{
    if ( argc < 2)
    {
        printf("diff:para\n");
        exit(0);
    }
    par = new partitions<CutDocument2>(atoi(argv[1]),atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
    wsizes = new int[1000];
    buf = new int[MAXSIZE];
    mhbuf = new int[MAXSIZE];
    hbuf = new int[MAXSIZE];
    tree_buf = new unsigned char[MAXSIZE];

    FILE* fcomplete = fopen("/data/jhe/wiki_access/completeFile", "rb");
    FILE* fnumv = fopen("/data/jhe/wiki_access/numv", "rb");
    int doccount;
    fread(&doccount, sizeof(unsigned), 1, fnumv);
    int* numv = new int[doccount];
    fread(numv, sizeof(unsigned), doccount, fnumv);
    ib = new indexer(400000000, atoi(argv[5]), 1);

    FILE* fword_size = fopen("/data/jhe/wiki_access/word_size", "rb");
    unsigned total_ver;
    fread(&total_ver, sizeof(unsigned), 1, fword_size);
    wcounts = new int[total_ver];
    fread(wcounts, sizeof(unsigned), total_ver, fword_size);
    fclose(fword_size);

    int ptr = 0;
    FILE* f2 = fopen("result", "rb");
    fread(&doccount, sizeof(unsigned), 1, f2);
    float* result_buf = new float[doccount];
    fread(result_buf, sizeof(unsigned), doccount, f2);
    fclose(f2);

    ofstream fout("index_list");
    for ( int i = 0; i < doccount; i++)
    {
        int total = 0;
        for ( int j = 0; j < numv[i]; j++)
            total+=wcounts[ptr+j];

        fread(buf, sizeof(unsigned), total, fcomplete);
        //if ( i < atoi(argv[2])){
          //  ptr += numv[i];
            //continue;
       // }
        int totalCount = dothejob(buf, &wcounts[ptr], i, numv[i], result_buf[i]);
        fout << result_buf[i]<<'\t'<<totalCount<<endl;
        printf("Complete:%d\n", i);
        ptr += numv[i];
        //if( i >= atoi(argv[3]))
          //  break;
    }
    fout.close();

    ib->dump();
    FILE* fscud = fopen("SUCCESS!", "w");
    fclose(fscud);
    delete par;
    return 0;
}
