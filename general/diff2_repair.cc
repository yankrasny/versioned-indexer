/*
 * diff2_repair.cc
 *
 *  Created on: May 11, 2013
 *  Adapted from diff2.cc by Jinru He    
 *  Author: Yan Krasny
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <iterator>
#include <fstream>
#include <vector>

#define MAXSIZE 81985059            /*  */

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
        {
            return l1.total < l2.total;
        }
    }
}compareb;

#include "general_partition_repair.h"

int* buf;
int* wcounts;
int* wsizes;
int* hbuf;
int* mhbuf;
// FILE* f2;

partitions<CutDocument2>* par;
int dothejob(int* buf, int* wcounts, int id, int vs, const vector<double>& wsizes2)
{
    char fn[256];
    if (vs != 1)
    {
        memset(fn, 0, 256);
        sprintf(fn, "/research/run/jhe/proximity/model/test/%d", id);
        FILE* f = fopen(fn, "r");
        
        // int total = 1;
        // int a,b,c,d,e,fs;
        // while ( fscanf(f, "%d\t%d\t%d\t%d\t%d\t%d\n", &a, &b, &c, &d, &e, &fs)>0){
        //     wsizes[total] = a;
        //     total++;
        // }
        // wsizes[0] = 5;
        // fclose(f);
        // memset(fn, 0, 256);
        // sprintf(fn, "/data4/jhe/proximity/general/meta/%d", id);
        // f = fopen(fn, "rb");
        // int size;
        // fread(&size, sizeof(unsigned), 1, f);
        // fread(mhbuf, sizeof(unsigned), size, f);
        // fclose(f);
        // memset(fn, 0, 256);
        // sprintf(fn, "/data4/jhe/proximity/minWinnowing/meta/%d", id);
        // f = fopen(fn, "rb");
        // fread(&size, sizeof(unsigned), 1, f);
        // fread(hbuf, sizeof(unsigned), size, f);
        // fclose(f);

        iv* trees = new iv[vs];
        int wptr = 0;
        int hptr = 0;
        par->init();

        // I think wsizes is window sizes, and total is the size of that array
        // TODO these vars should not be hardcoded, if time permits we'll change it
        float fragmentationCoefficient = 1.0;
        float minFragSize = 100;
        unsigned method = 1;
        int numFrags = par->fragment(buf, wcounts, vs, trees, fragmentationCoefficient, minFragSize, method);
        if (numFrags == -1)
        {
            printf("warning: doc %d, too large, skipping...\n", id);
            par->exitEarly();
        }
        printf("finish sorting!\n");

        // build table for each document recording total number of block and total postings
        vector<binfo> lbuf;
        binfo binf;
        memset(fn, 0, 256);
        sprintf(fn, "test/%d.2", id);
        f = fopen(fn, "w");
        int length = wsizes2.size();
        for (int i = 0; i < length; i++)
        {
            double wsize = wsizes2[i];
            par->completeCount(wsize);
            par->select(trees, wsize);
            par->PushBlockInfo(buf, wcounts, id, vs, &binf);
            binf.wsize = wsize;
            lbuf.push_back(binf);
            fprintf(f, "%.2lf\t%d\t%d\t%d\n", wsize, binf.total_dis, binf.total, binf.post);
        }
        fclose(f);
        par->dump_frag();
        delete[] trees;
    }
    else
    {
        memset(fn, 0, 256);
        sprintf(fn, "test/%d.2", id);
        FILE* f = fopen(fn, "w");
        fprintf(f, "%.2lf\t%d\t%d\t%d\n", wcounts[0], 1, 1, wcounts[0]);
        fclose(f);
        return 1;
    }
}

/*
The Repair Code uses extern IDs
    This is what we call bad design; now the calling code 
    has to worry about setting these up, and it shouldn't.
    I will change this if I get the chance -YK
*/
unsigned currentFragID = 0;
unsigned currentWordID = 0;
unsigned currentOffset = 0;

int main(int argc, char**argv)
{
    if ( argc < 2)
    {
        printf("diff:para startIdx endIdx basicLayer\n");
        exit(0);
    }

    par = new partitions<CutDocument2>(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
    wsizes = new int[1000];
    buf = new int[MAXSIZE]; // the contents of one document at a time (the fread below seems to overwrite for each doc)
    mhbuf = new int[MAXSIZE]; // a buffer for some sort of hashes
    hbuf = new int[MAXSIZE]; // another buffer for some sort of hashes
    FILE* fcomplete = fopen("/data/jhe/wiki_access/completeFile", "rb");
    FILE* fnumv = fopen("/data/jhe/wiki_access/numv", "rb");
    int doccount; // number of documents -yan
    fread(&doccount, sizeof(unsigned), 1, fnumv);
    int* numv = new int[doccount]; // number of versions for each document -yan
    unsigned* lens = new unsigned[doccount]; // lengths? -yan
    fread(numv, sizeof(unsigned), doccount, fnumv);

    FILE* fword_size = fopen("/data/jhe/wiki_access/word_size", "rb");
    unsigned total_ver;
    fread(&total_ver, sizeof(unsigned), 1, fword_size);
    wcounts = new int[total_ver]; // wcounts[i] is the length of version i (the number of ints)
    fread(wcounts, sizeof(unsigned), total_ver, fword_size);
    fclose(fword_size);

    int ptr = 0;

    ifstream fin("options");
    istream_iterator<double> data_begin(fin);
    istream_iterator<double> data_end;
    vector<double> wsizes2(data_begin, data_end);
    fin.close();

    int sptr = 0;
    int nd = 0;
    for (int i = 0; i < doccount; i++) // for each document -YK
    {
        int total = 0; //the number of words in all versions of this doc
        for (int j = 0; j < numv[i]; j++) // for each version of document[i] -YK
        {
            total += wcounts[ptr + j];
        }
        fread(buf, sizeof(unsigned), total, fcomplete);
        lens[nd] = dothejob(buf, &wcounts[ptr], i, numv[i], wsizes2);
        currentWordID = 0; // This is a global used by repair, see repair-algorithm/Util.h -YK
        nd++;
        printf("Complete:%d\n", i);
        ptr += numv[i];
    }

    // fclose(f2);
    FILE* fscud = fopen("SUCCESS!", "w");
    fclose(fscud);

    delete par;
    return 0;
}
