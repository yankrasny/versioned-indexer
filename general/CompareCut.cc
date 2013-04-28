/*
 * =====================================================================================
 *
 *       Filename:  CutDistribution.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  01/06/2012 01:12:11 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Jinru He (jhe), mikehe1117@gmail.com
 *        Company:  NYU-Poly
 *
 * =====================================================================================
 */
#include	"CompareCut.h"
#include	<fstream>

using namespace std;

#define	MAXSIZE 250939073
			/*  */

int* mhbuf;
int* hbuf;
int* buf;
int* wsizes;
int* wcounts;
partitions<CutDocument3>* par;
partitions<CutDocument4>* par2;

int dothejob(int* buf, int* wcounts, int id, int vs)
{
    char fn[256];
    memset(fn, 0, 256);
    sprintf(fn, "/data4/jhe/proximity/general/meta/%d", id);
    FILE* f = fopen(fn, "rb");
    int size;
    fread(&size, sizeof(unsigned), 1, f);
    fread(mhbuf, sizeof(unsigned), size, f);

    fclose(f);

    sprintf(fn, "/data4/jhe/proximity/minWinnowing/meta/%d", id);
    f = fopen(fn, "rb");
    fread(&size, sizeof(unsigned), 1, f);
    fread(hbuf, sizeof(unsigned), size, f);
    fclose(f);
    int wptr = 0;
    int hptr = 0;
    memset(fn, 0, 256);
    sprintf(fn, "test/%d.compare", id);
    ofstream fout(fn);

    for ( int i = 0; i < vs; i++){
        if ( wcounts[i] > 0 )
        {
            int count1 = par->fragment(i, &buf[wptr], wcounts[i], &mhbuf[hptr], &hbuf[hptr]);
            int count2 = par2->fragment(i, &buf[wptr], wcounts[i], &mhbuf[hptr], &hbuf[hptr]);

            if (count1 > count2)
            {
                fout << i <<'\t'<<wcounts[i]<<'\t'<<count1<<'\t'<<count2<<endl;
            }
        }
        if (wcounts[i] >B)
        {
            hptr += (wcounts[i]-B+1);
        }
        wptr += wcounts[i];
    }	

    fout.close();
    printf("finish sorting!\n");
    return 0;
}

int main(int argc, char**argv)
{
    if ( argc < 2)
    {
        printf("diff:para startIdx endIdx basicLayer\n");
        exit(0);
    }
    par = new partitions<CutDocument3>(); 
    par2 = new partitions<CutDocument4>();
    wsizes = new int[1000];
    buf = new int[MAXSIZE];
    mhbuf = new int[MAXSIZE];
    hbuf = new int[MAXSIZE];
    FILE* fcomplete = fopen("/data/jhe/wiki_access/completeFile", "rb");
    FILE* fnumv = fopen("/data/jhe/wiki_access/numv", "rb");
    int doccount;
    fread(&doccount, sizeof(unsigned), 1, fnumv);
    int* numv = new int[doccount];
    fread(numv, sizeof(unsigned), doccount, fnumv);

    FILE* fword_size = fopen("/data/jhe/wiki_access/word_size", "rb");
    unsigned total_ver;
    fread(&total_ver, sizeof(unsigned), 1, fword_size);
    wcounts = new int[total_ver];
    fread(wcounts, sizeof(unsigned), total_ver, fword_size);
    fclose(fword_size);

    int ptr = 0;
    
    for ( int i = 0; i < doccount; i++)
    {
        int total = 0;
        for ( int j = 0; j < numv[i]; j++)
                total+=wcounts[ptr+j];

        fread(buf, sizeof(unsigned), total, fcomplete);
        if ( i < atoi(argv[1])){
                ptr += numv[i];
                continue;
        }
        dothejob(buf, &wcounts[ptr], i, numv[i]);
        printf("Complete:%d\n", i);
        ptr += numv[i];
        if( i >= atoi(argv[2]))
            break;
    }

    delete par;
    delete par2;
    FILE* fscud = fopen("SUCCESS!", "w");
    fclose(fscud);

    return 0;
}

