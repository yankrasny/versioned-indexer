#include <stdio.h>
#include <stdlib.h>
#include "md5.h"
#include "mhash.h"
#define MAXPOS 560707643
#define MAXPOS2 23665946

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
	if ( ret == 0)
		return ret2;
	else
		return ret;

}


int mergeable(item* its, post* p, int start, int len)
{
	int count = its[0].count;
	int i, j;
	int ptr, offset, lens;
	for (i = 0; i < count; i++)
	{
		j = start;
		ptr = its[j].ptr;
		offset = p[ptr].offset;
		lens = p[ptr].len;
		j++;		
		for (; j < start+len; j++)
		{
			ptr = its[j].ptr;
			if ( p[ptr].offset == offset+lens || p[ptr].offset == offset+lens+1) 
			{
			
				offset = p[ptr].offset;
				lens = p[ptr].len;
				printf("start:%d\t%d\t%d\t\n", p[ptr].fid, offset, lens);
			}
			else{
				printf("failed:%d\t%d\t%d\t%d\n", offset+lens, p[ptr].offset, p[ptr].fid, i);
				return 0;
			}

			
		}
	}
	
	
	return 1;
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

	}
	printf("ptr:%d\t%d\n", ptr, ptr3);
	//exit(0);
	//qsort(p, ptr, sizeof(post), compare_pos2);
	qsort(its, ptr3, sizeof(item), compare_item);

	int ret_len = 1;
	int start_ptr = 0;
	unsigned long long temp = its[0].hash_val;
	FILE* fs = fopen("mergeable", "w");
 int rets;
	for ( j = 1; j < ptr3; j++)
	{
		if ( temp != its[j].hash_val)
		{
			printf("Complete:start:%d\tlens:%d\t", start_ptr, ret_len);
			rets = mergeable(its, p, start_ptr, ret_len);
			if ( rets == 1)
			{
				printf("Mergeable!\n");
				for ( i = start_ptr; i< start_ptr+ret_len; i++)
					fprintf(fs, "%d,", its[i].fid);
				fprintf(fs, "\n");
			}
			else
			{
				printf("NOOOOO!\n");
			}
			start_ptr = j;
			ret_len = 1;
			temp = its[j].hash_val;
		}
		else
			ret_len++;			
		printf("%d\t%lu\n", its[j].fid, its[j].hash_val);
	}

	rets = mergeable(its, p, start_ptr, ret_len);
	if ( rets == 1)
	{
		printf("Mergeable!\n");
		for ( i = start_ptr; i< start_ptr+ret_len; i++)
			fprintf(fs, "%d,", its[i].fid);
		fprintf(fs, "\n");
		
	}

	fclose(fs);
	return 0;
}
