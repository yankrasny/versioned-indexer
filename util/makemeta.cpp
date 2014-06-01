#include <stdio.h>
#include <stdlib.h>
#include "md5.h"
#include "mhash.h"
#define MAXPOS 560707643
#define MAXPOS2 23665946
#include <list>

using namespace std;

typedef struct
{
	int vid, fid, offset, len;
}post;

typedef struct
{
	int fid;
	unsigned long long hash_val;
	int ptr, count; 
}item;

int compare_pos(const void* a1, const void* a2)
{
	int ret = ((post*)a1)->vid - ((post*)a2)->vid;
	int ret2 = ((post*)a1)->fid - ((post*)a2)->fid;
	if ( ret == 0)
		return ret2;
	else
		return ret;
}

int compare_item(const void*a1, const void*a2)
{
	int ret = ((item*)a1)->hash_val - ((item*)a2)->hash_val;
	int ret2 = ((item*)a1)->fid - ((item*)a2)->fid;

	if ( ret == 0 )
		return ret2;
	else
		return ret;
}

int compare_pos2(const void* a1, const void* a2)
{
	int ret = ((post*)a1)->fid - ((post*)a2)->fid;
	int ret2 = ((post*)a1)->vid - ((post*)a2)->vid;
	if ( ret2 == 0)
		return ret;
	else
		return ret2;

}



int main(int argc, char** argv)
{
	post* p = (post*)malloc(sizeof(post)*MAXPOS);
	if ( NULL == p )
		perror("p");
	FILE* f = fopen64("frag.info", "r");
	unsigned* md5_buf = (unsigned*)malloc(sizeof(unsigned)*100000);
	item* its = (item*)malloc(sizeof(item)*MAXPOS2);
	if ( NULL == its )
		perror("its");
	int ptr = 0;	
	int ptr2 =0;
	int ptr3 = 0;
	int fid, len, count;
	int i, j;
	int vid, offset;
	unsigned long long ret;
	md5_state_t state;
	md5_byte_t digest[16];

	while ( !feof(f))
	{
		fread(&fid, sizeof(int), 1, f);
		fread(&len, sizeof(int), 1, f);		
		fread(&count, sizeof(int), 1, f);

		ptr2 = 0;
		its[ptr3].fid = fid;
		its[ptr3].ptr = ptr;
 		its[ptr3].count = count;
		for (i = 0; i < count; i++){
			fread(&vid, sizeof(int), 1, f);
			fread(&offset, sizeof(int), 1, f);
			p[ptr].vid = vid;
			p[ptr].fid = fid;
			p[ptr].offset = offset;
			p[ptr].len = len;
			ptr++;
			md5_buf[ptr2++] = vid;
		}

		md5_init(&state);
		md5_append(&state, (const md5_byte_t *) md5_buf, sizeof(unsigned) * ptr2);
		md5_finish(&state, digest);

		ret = *((unsigned long long*)digest);


		its[ptr3++].hash_val = ret;
	//	ptr3++;
		

	}
	printf("ptr:%d\t%d\n", ptr, ptr3);
	//exit(0);
	qsort(p, ptr, sizeof(post), compare_pos2);

	list<int> temp;
	temp.push_back(p[0].fid);
	double average = 0.0;
	int total = 0;
	FILE* fs = fopen("meta", "w");
	for ( int i = 1; i < ptr; i++)
	{
		if (p[i].vid == p[i-1].vid)
		{
			temp.push_back(p[i].fid);
		}
		else
		{

			fprintf(fs, "%d\t%d\t", p[i-1].vid, temp.size());
			for ( list<int>::iterator its = temp.begin(); its!=temp.end(); its++)
				fprintf(fs, "%d,", (*its));
			fprintf(fs, "\n");
			total += temp.size();
			temp.clear();
			temp.push_back(p[i].fid);
		}
	}

	fclose(fs);
	printf("Total:%d\n", total);
	return 0;
}
