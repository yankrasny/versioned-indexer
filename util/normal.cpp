#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <utility>
#include <string>
#include <map>
#include <list>


#include "wiki_corpus.h"
#include "partition.h"
#include "indexbase.h"
using namespace std;


#define MAXWORD 20000

int* wcounts;
posting* post_buf;

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
indexbase* ib;
indexbase* ib2;
partitions d;
int* outs[2];
int* aux2;

double dothejob(posting* post_buf, int* wcounts, int id, int vs, FILE* mark)
{
	d.init();
	int ptr = 0;

	int temp_len;
	int ret;

	for ( int i = 0; i < vs; i++)
	{
		if ( wcounts[i] > MAXWORD)
		{
			fprintf(mark, "%d\t%d\n", id, vs);
			fflush(mark);
			break;
		}
	}

	int* prev = outs[0];
	int* curr = outs[1];
	transform(&post_buf[ptr], wcounts[0], prev, 0);
	ib->insert_terms(&post_buf[ptr], wcounts[0]);
	ptr += wcounts[0];
	int ret_len, ret_len2;
	ndoc++;
	double total_cost = 0;
	double total_cost2 = 0;
	total_cost = wcounts[0];
	total_cost2 = wcounts[0];
	for ( int i = 1; i < vs; i++)
	{

		transform(&post_buf[ptr], wcounts[i], curr, i);
		ndoc = post_buf[ptr].vid;	
		ib->insert_terms(&post_buf[ptr], wcounts[i]);
		//d.fragment(ndoc, curr, wcounts[i]);
		//ib->insert_terms(d.add_list, d.add_list_len);
		ptr += wcounts[i];
		int* temp = prev;
		prev = curr;
		curr = temp;
		ndoc++;	
	}		
	printf("Complete:%d!!\n", id);
	d.dump_frag();
	FILE* fmark = fopen("FINISH_MARK", "w");
	fprintf(fmark, "%d\n", id);
	fflush(fmark);
	fclose(fmark);
}

int main(int argc, char** argv)
{
	wiki_corpus wc;
	double total_cost, total_cost2;
	wcounts = new int[25000];
	post_buf = new posting[MAXSIZE];
	outs[0] = new int[MAXSIZE];
	outs[1] = new int[MAXSIZE];
	aux2 = new int[MAXSIZE];

	int ret;
	ib = new indexbase(20000000, 2, 1);
	ib2 = new indexbase(20000000, 4, 1);
	int count = 0;
	int k = atoi(argv[1]);

	FILE* mark = fopen("large_file", "w");
	double total_post = 0;

	while ( wc.next())
	{
		item its = wc.current();
		if( its.id < k )
			continue;
		ret = wc.get_current_pos(wcounts,post_buf);
		if ( ret < 0)
			continue;
				
		for ( int i= 0; i < its.vs; i++)
			total_post+= wcounts[i];

		printf("doing:id:%d\tvs:%d\ttotal:%.1lf\t", its.id, its.vs, total_post);
		//dothejob(post_buf, wcounts, its.id, its.vs, mark);
		printf("Complete!\n");
		count++;
		

	}

	fclose(mark);

	ib->dump();
	FILE* f = fopen("SUCCESS", "a+");
	if ( NULL == f)
		perror("Last!");

	fclose(f);
	
	return 0;
}
