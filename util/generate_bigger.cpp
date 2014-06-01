#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <utility>
#include <string>
#include <map>
#include <list>


#include "../util/wiki_corpus.h"
#include "../util/utility.h"
using namespace std;


#define MAXWORD 20000

int* wcounts;
posting* post_buf;

int compare(const void* a1, const void* a2)
{
	posting* e1 = (posting*)a1;
	posting* e2 = (posting*)a2;

	int ret = e1->vid - e2->vid;
	int ret2 = e1->pos - e2->pos;

	if ( ret == 0 )
		return ret2;
	else
		return ret;
}


int contrast(int* prev, int* curr, int len1, int len2)
{
	if (len1 != len2 ){
		printf("%d\t%d\t%d\t%d\n", len1, len2, prev[len1-1], curr[len2-2]);
		
		return 0;
	}

	for ( int i = 0; i < len1; i++)
	{
		if ( prev[i] != curr[i]){
			printf("number:%d\t%d\t%d\n", i, prev[i], curr[i]);
			return 0;
		}
	}

	return 1;
}
int transform(posting* p, int count, int* out, int vs)
{
	
	qsort(p, count, sizeof(posting), compare);
	for ( int i = 0; i < count; i++){
		out[i] = p[i].wid;
	}

}

int ndoc = 0;
int* outs[2];
int* aux2;
FILE* fword;
FILE* fsize;

int* wsize;
int wsize_ptr = 0;

void dothejob(posting* post_buf, int* wcounts, int id, int vs)
{
	docreader reader(post_buf, wcounts, vs, id);
	int vid = 0;
	while ( reader.next() )
	{
		int count = reader.current(aux2);
		wsize[wsize_ptr++] = count;
		fwrite(aux2, sizeof(int), count, fword);
	}

	printf("%d\t%d\n", wsize_ptr, vs);
	//exit(0);
}

int main(int argc, char** argv)
{
	wiki_corpus wc("url.idx3");
	double total_cost, total_cost2;
	wcounts = new int[25000];
	post_buf = new posting[MAXSIZE];
	outs[0] = new int[MAXSIZE];
	outs[1] = new int[MAXSIZE];
	aux2 = new int[MAXSIZE];
	wsize = new int[8600000];

	int ret;
	//ib = new indexbase(20000000, 2, 1);
	int count = 0;

	fword = fopen64("completeFile", "wb");
	wsize_ptr = 0;
	
	while ( wc.next())
	{
		item its = wc.current();
		ret = wc.get_current_pos(wcounts,post_buf);
		printf("doing:id:%d\tvs:%d\t", its.id, its.vs);
		dothejob(post_buf, wcounts, its.id, its.vs);
		printf("Complete!\n");
		count++;
		

	}

	fsize = fopen64("word_size", "wb");
	fwrite(&wsize_ptr, sizeof(unsigned), 1, fsize);
	fwrite(wsize, sizeof(int), wsize_ptr, fsize);
	fclose(fsize);

	return 0;
}
