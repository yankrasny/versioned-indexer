#include <stdio.h>
#include <stdlib.h>
#define MAX_FRAG 10000000
#include "md5.h"
#include "mhash.h"

#define MAXPOS 25171899

typedef struct
{
	int fid, len, count;
	long fptr;
}frag;

typedef struct
{
	unsigned vid, offset;
}cp;

typedef struct
{
	int fid;
	unsigned long long hash_val;
	int ptr; 
}item;

int compare_frag(const void* a1, const void* a2)
{
	int ret = ((frag*)a1)->count - ((frag*)a2)->count;
	int ret2 = ((frag*)a1)->len - ((frag*)a2)->len;

	if ( ret == 0 )
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

int mergable(frag* buf, item* buf2, int len)
{
	int i ;
	int count;
	long long fptr;
	for ( i = 0; i < len; i++)
	{
		count = buf[buf2[i].ptr].count;
		fptr= buf[buf2[i].ptr].fptr;
		
	}
}

int main(int argc, char** argv)
{
	FILE* f = fopen("frag.info", "r");
	frag* fs = (frag*)malloc(sizeof(frag)*MAX_FRAG);
	unsigned* md5_buf = (unsigned*)malloc(sizeof(unsigned)*100000);
	item* its = (item*)malloc(sizeof(item)*MAX_FRAG);

	int ptr = 0;	
	int ptr2 = 0;
	int ptr3 = 0;

	int fid, len, count;
	int i, j;
	int vid, offset;
	int total_item = 0;

	md5_state_t state;
	md5_byte_t digest[16];
	unsigned long long ret;

	while ( !feof(f))
	{
		fread(&fs[ptr].fid, sizeof(int), 1, f);
		fread(&fs[ptr].len, sizeof(int), 1, f);		
		fread(&fs[ptr].count, sizeof(int), 1, f);
		fs[ptr].fptr = ftell(f);

		ptr2 = 0;
		for (i = 0; i < fs[ptr].count; i++){
			fread(&md5_buf[ptr2], sizeof(int), 1, f);
			fread(&offset, sizeof(int), 1, f);
			ptr2++;
		}

		md5_init(&state);
		md5_append(&state, (const md5_byte_t *) md5_buf, sizeof(unsigned) * ptr2);
		md5_finish(&state, digest);
		ret = *((unsigned long long*)digest);
		
		its[ptr3].fid = fs[ptr].fid;
		its[ptr3].hash_val = ret;
		its[ptr3].ptr = ptr;

		ptr++;
		ptr3++;
	}

	qsort(its, ptr3, sizeof(item), compare_item);

	int ret_len = 1;
	int start_ptr = 0;
	unsigned long long temp = its[0].hash_val;
 
	for ( j = 1; j < ptr3; j++)
	{
		if ( temp != its[i].hash_val)
		{

			start_ptr = i;
			ret_len = 1;
		}
		else
			ret_len++;			
		printf("%d\t%lu\n", its[j].fid, its[j].hash_val);
	}
	fclose(f);

	return 0;
}
