#include <stdio.h>
#include <stdlib.h>
#define MAXSIZE 130939073
#include <string.h>
#include "winnow.h"
#include <map>


using namespace std;

unsigned* buf;
unsigned* hval;
int* wcounts;
unsigned char* hbuf;
int* mhbuf;
int* difbuf;

inline unsigned char inthash(unsigned key) {
    int c2 = 0x9c6c8f5b; // a prime or an odd constant
    key = key ^ (key >> 15);
    key = (~key) + (key << 6);
    key = key ^ (key >> 4);
    key = key * c2;
    key = key ^ (key >> 16);
    return *(((unsigned char*) &key + 3));
}

FILE* fhashes;

int process(unsigned* buf, int* wcounts, int id, int vs)
{
    map<int, int> dict;
    int off = 0;
    int out_off = 0;
    int total_count = 0;
    int hash_off = 0;
    for ( int i = 0; i < vs; i++){
        for ( int j = 0; j < wcounts[i]; j++)
            hbuf[off+j] = inthash(buf[off+j]);

        int count;
        if ( wcounts[i] > B)
        {
            int size = preComp(&hbuf[off], wcounts[i]-B+1, &hval[hash_off]);	
            for ( int j = 0; j < wcounts[i]-B+1; j++)
                dict[hval[j+hash_off]]++;

            hash_off += size;
        }
        off += wcounts[i];
    }

    hash_off = 0;
    int size;
    map<int, int>::iterator its;
    for ( int i = 0; i < vs; i++)
    {
        if ( wcounts[i] > B)
        {
            size = wcounts[i]-B+1;
            for ( int j = 0; j < size; j++)
            {
                its = dict.find(hval[j+hash_off]);
                if (its == dict.end())
                {
                    printf("ERROR!\n");
                    exit(0);
                }
                else
                    mhbuf[hash_off+j] = its->second;
            }

            for ( int j = 0; j < size-1; ++j)
            {
                difbuf[hash_off+j] = mhbuf[hash_off+j+1] - mhbuf[hash_off+j];
            }
            difbuf[hash_off+size-1] = 0;

            hash_off += size;
            total_count += size;
        }
    }
    char fn[256];
    memset(fn, 0, 256);
    sprintf(fn, "meta/%d", id);
    fhashes = fopen(fn, "wb");
    fwrite(&total_count, sizeof(unsigned), 1, fhashes);
    fwrite(mhbuf, sizeof(unsigned), total_count, fhashes);
    fclose(fhashes);
}


int main(int argc, char** argv)
{
    buf = new unsigned[MAXSIZE];
    hbuf = new unsigned char[MAXSIZE];
    hval = new unsigned[MAXSIZE];
    mhbuf = new int[MAXSIZE];
    difbuf = new int[MAXSIZE];

    FILE* f1 = fopen64("/data/jhe/wiki_access/completeFile", "rb");
    FILE* f2 = fopen64("/data/jhe/wiki_access/word_size", "rb");
    FILE* f3 = fopen64("/data/jhe/wiki_access/numv", "rb");
    unsigned tdoc;
    fread(&tdoc, sizeof(unsigned), 1, f3);
    unsigned* numv = new unsigned[tdoc];
    fread(numv, sizeof(unsigned), tdoc, f3);
    fclose(f3);

    unsigned tversion;
    int* wlens;
    fread(&tversion, sizeof(unsigned), 1, f2);
    wlens = new int[tversion];
    fread(wlens, sizeof(unsigned), tversion, f2);
    fclose(f2);

    int ptr = 0;
    for ( int i = 0; i < 240179; i++)
    {
        int total_bytes = 0;
        for ( int j = 0; j < numv[i]; j++)
            total_bytes+=wlens[ptr+j];

        fread(buf, sizeof(unsigned), total_bytes, f1);
        process(buf, &wlens[ptr], i, numv[i]);
        ptr+=numv[i];
        printf("Complete:%d\n", i);
    }

    FILE* fw = fopen("FINISH!", "w");
    fclose(fw);
    return 0;
}
