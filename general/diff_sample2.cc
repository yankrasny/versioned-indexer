/*
 * diff.cc
 *
 *  Created on: May 15, 2011
 *      Author: jhe
 */

#include        <cstdio>
#include        <cstdlib>
#include        <cstring>
#include        <map>
#include	<iterator>
#include	<fstream>
#include	<vector>

#define	MAXSIZE 81985059 			/*  */

struct binfo
{
    int total;
    int total_dis;
    int post;
    float wsize;
};

struct CompareFunctor
{
    bool operator()(const binfo& l1, const binfo& l2)
    {
        if (l1.total == l2.total)
        {
            return l1.post < l2.post;
        }
        else
            return l1.total < l2.total;
    }
}compareb;

#include "general_partition.h"

int* buf;
int* wcounts;
int* wsizes;
int* hbuf;
int* mhbuf;
FILE* f2;

partitions<CutDocument4>* par;
int dothejob(int* buf, int* wcounts, int id, int vs, const vector<double>& wsizes2)
{
    char fn[256];
    memset(fn, 0, 256);
    sprintf(fn, "test/%d.2", id);
    FILE* fout = fopen(fn, "w");
    if (vs != 1)
    {
        memset(fn, 0, 256);
        sprintf(fn, "/data4/jhe/proximity/model/test/%d", id);
        FILE* f = fopen(fn, "r");
        int total = 1;
        int a,b,c,d,e,fs;
        while ( fscanf(f, "%d\t%d\t%d\t%d\t%d\t%d\n", &a, &b, &c, &d, &e, &fs)>0)
        {
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

        //build table for each document recording total number of block and total postings
        binfo binf;
        int length = wsizes2.size();
        for ( int i = 0; i < length; i++)
        {
            double wsize = wsizes2[i];
            par->completeCount(wsize);
            par->select(trees, wsize);
            par->PushBlockInfo(buf, wcounts,id, vs, &binf);
            binf.wsize = wsize;
            fprintf(fout, "%.2lf\t%d\t%d\t%d\n", wsize, binf.total_dis, binf.total, binf.post);
        }
        par->dump_frag();
        /*  sort(lbuf.begin(), lbuf.end(), compareb);
            int ptr1 = 0;
            int ptr2 = 1;
            int ptr = lbuf.size();

        //compact 
        while (ptr2 < ptr)
        {
        while ( ptr2 < ptr && lbuf[ptr2].post >= lbuf[ptr1].post)
        ptr2++;

        if ( ptr2 >= ptr)
        break;

        ptr1++;
        lbuf[ptr1] = lbuf[ptr2];
        ptr2++;
        }
        ptr1++;
        for ( int j = 0; j < ptr1; j++)
        {
        fprintf(f, "%.2f\t%d\t%d\t%d\n", lbuf[j].wsize, lbuf[j].total,lbuf[j].total_dis, lbuf[j].post);
        fwrite(&lbuf[j], sizeof(binfo), 1, f2);
        }
        */     
        fclose(fout);
        delete[] trees;
        return 0;
    }
    else
    {
        fprintf(fout, "%.2lf\t%d\t%d\t%d\n", 1, 1, 1, wcounts[0]);
        fclose(fout);
        return 1;
    }
}


int main(int argc, char**argv)
{
    if ( argc < 2)
    {
        printf("diff:para startIdx endIdx basicLayer\n");
        exit(0);
    }
    par = new partitions<CutDocument4>(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
    wsizes = new int[1000];
    buf = new int[MAXSIZE];
    mhbuf = new int[MAXSIZE];
    hbuf = new int[MAXSIZE];
    FILE* fcomplete = fopen("/data/jhe/wiki_access/completeFile", "rb");
    FILE* fnumv = fopen("/data/jhe/wiki_access/numv", "rb");
    int doccount;
    fread(&doccount, sizeof(unsigned), 1, fnumv);
    int* numv = new int[doccount];
    unsigned* lens = new unsigned[doccount];
    fread(numv, sizeof(unsigned), doccount, fnumv);

    FILE* fword_size = fopen("/data/jhe/wiki_access/word_size", "rb");
    unsigned total_ver;
    fread(&total_ver, sizeof(unsigned), 1, fword_size);
    wcounts = new int[total_ver];
    fread(wcounts, sizeof(unsigned), total_ver, fword_size);
    fclose(fword_size);

    f2 = fopen("finfos", "wb");

    int ptr = 0;

    ifstream fin("options");
    istream_iterator<double> data_begin(fin);
    istream_iterator<double> data_end;
    vector<double> wsizes2(data_begin, data_end);
    fin.close();

    FILE* fsamples = fopen("samples", "rb");
    int scount;
    fread(&scount, sizeof(unsigned), 1, fsamples);
    unsigned* samples = new unsigned[scount];
    fread(samples,sizeof(unsigned), scount, fsamples);
    fclose(fsamples);

    int sptr = 0;
    int nd = 0;
    for ( int i = 0; i < doccount; i++)
    {
        int total = 0;
        for ( int j = 0; j < numv[i]; j++)
            total+=wcounts[ptr+j];

        fread(buf, sizeof(unsigned), total, fcomplete);
        if ( i == samples[sptr] )
        {
            sptr++;
            lens[nd] = dothejob(buf, &wcounts[ptr], i, numv[i], wsizes2);
            nd++;
            printf("Complete:%d\n", i);
            if (sptr >= scount)
                break;
        }
        ptr += numv[i];

    }

    fclose(f2);
    FILE* fscud = fopen("SUCCESS!", "w");
    fclose(fscud);

    FILE* fsize =fopen("binfos", "wb");
    fwrite(&nd,  sizeof(unsigned), 1, fsize);
    fwrite(lens, sizeof(unsigned), nd, fsize);
    fclose(fsize);

    delete par;
    return 0;
}
