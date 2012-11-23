#include <stdio.h>
#include <stdlib.h>
#define MAXSIZE 130939073

#include "winnow.h"

unsigned* buf;
int* wcounts;
unsigned char* hbuf;
int* mhbuf;

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
	int off = 0;
	int out_off = 0;
	int total_count = 0;
	for ( int i = 0; i < vs; i++){
		for ( int j = 0; j < wcounts[i]; j++)
		{
			hbuf[j] = inthash(buf[off+j]);
		}
		int count;
		if ( wcounts[i] > B){
			count = minWinnow(hbuf, wcounts[i], &mhbuf[out_off]);
			out_off+=count;
			total_count+=count;
		}
		off += wcounts[i];
	}

	fwrite(mhbuf, sizeof(unsigned), total_count, fhashes);
}


int main(int argc, char** argv)
{
	buf = new unsigned[MAXSIZE];
	hbuf = new unsigned char[MAXSIZE];
	mhbuf = new int[MAXSIZE];

	fhashes = fopen64("minWinnowing", "wb");
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
	for ( int i = 0; i < 1000; i++)
	{
		int total_bytes = 0;
		for ( int j = 0; j < numv[i]; j++)
			total_bytes+=wlens[ptr+j];

		fread(buf, sizeof(unsigned), total_bytes, f1);
		process(buf, &wlens[ptr], i, numv[i]);
		ptr+=numv[i];
	}

	fclose(fhashes);
	return 0;
}
