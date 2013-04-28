/*
 * partition.h
 *
 *  Created on: Oct 8, 2009
 *      Author: jhe
 */

#ifndef PARTITION_H_
#define PARTITION_H_

#include <stdio.h>
#include <stdlib.h>
#include "winnow.h"
#include "vb.h"
#include "md5.h"
#include "mhash.h"
#include <list>
#include <map>

using namespace std;

typedef struct {
	int diffe;
	int total;
	float ratio;
	int diff_to;
	int diff_fre;
} diffes;

typedef struct
{
	int vid, offset;
}p;

typedef struct
{
	int fid, len;
	list<p> items;

}finfo;



class partitions {
private:
	unsigned* mhbuf;
	FILE* fhash;

	int minWinnow(int len, int wsize, int* start)
	{
		int num = 0;
		start[num++] = 0;
		for ( int i = 0; i < len-B-wsize+2; i++)
		{
			if (mhbuf[i]>= wsize){
				start[num++] = i;
			}
		}
		return num;
	}
public:
	void fragment(int vid, int* refer, int len, unsigned* hbuf, int wsize, bool indexable) {
		add_list_len = 0;
		unsigned hh, lh;
		int ins;
		int i;
		int block_num = 0;
		int error;
		int ep;

		w_size[0] = wsize;
		mhbuf = hbuf;
		if ( len > B){
			block_num = minWinnow(len, w_size[0], bound_buf);
		}
		else{
			bound_buf[0] = 0;
			block_num = 1;
		}
		if(indexable)
			total_meta+=block_num;
		for ( int j = 0; j < block_num; j++)
		{
			if ( j < block_num - 1)
			{
				if ( bound_buf[j+1] < bound_buf[j])
				{
					printf("error partition! %d,%d\t%d\n", j, bound_buf[j], bound_buf[j+1]);
					exit(0);
				}
				for ( int l = bound_buf[j]; l < bound_buf[j+1]; l++)
					md5_buf[l - bound_buf[j]] = refer[l];

				ep = bound_buf[j+1];
			}
			else
			{
				for ( int l = bound_buf[block_num-1]; l < len; l++)
						md5_buf[l-bound_buf[j]] = refer[l];

				ep = len;
			}

			md5_init(&state);
			md5_append(&state, (const md5_byte_t *) md5_buf, sizeof(unsigned) * (ep - bound_buf[j]));
			md5_finish(&state, digest);
			hh = *((unsigned *)digest) ;
			lh = *((unsigned *)digest + 1) ;
			ins = lookupHash (lh, hh, h[0]) ;
			if (ins == -1) {
				p p1;
				p1.vid = vid;
				p1.offset = bound_buf[j];
				insertHash(lh, hh, ptr, h[0]);
				infos[ptr].fid = fID;
				if ( j != block_num - 1)
					infos[ptr].len = ep - bound_buf[j];
				else
					infos[ptr].len = len - bound_buf[j];
					if ( infos[ptr].len < 0 )
					{
						printf("we have an error!%d\tprev bound:%d\tnow bound:%d\t%d\n", block_num, bound_buf[j], bound_buf[j+1], j);
						exit(0);
					}
					infos[ptr++].items.push_back(p1);

					if ( indexable){
						for ( int l = bound_buf[j]; l < ep; l++)
						{
							add_list[add_list_len].vid = fID;
							add_list[add_list_len].wid = refer[l];
							add_list[add_list_len++].pos = l - bound_buf[j];
						}
						//qsort(add_list, add_list_len, sizeof(posting), comparep);
						fID++;
					}
					total_post_count = total_post_count + ep-bound_buf[j];
				}
				else { //has sharing block
					p p2;
					p2.vid = vid;
					p2.offset = bound_buf[j];
					infos[ins].items.push_back(p2);
				}

			}
	}

	int total_post_count;
	int init()
	{
		add_list_len = 0;
		total_post_count = 0;
		ptr = 0;
		if ( h[0]!=NULL)
			destroyHash(h[0]);
		h[0] = initHash();
	
	}
	//Thomas Wang's 32 bit Mix Function
	inline unsigned char inthash(unsigned key) {
		int c2 = 0x9c6c8f5b; // a prime or an odd constant
		key = key ^ (key >> 15);
		key = (~key) + (key << 6);
		key = key ^ (key >> 4);
		key = key * c2;
		key = key ^ (key >> 16);
		return *(((unsigned char*) &key + 3));
	}
	char fn[256];
	
	int cost_cal()
	{
		int total_cost = total_post_count*2;
		for ( int i = 0; i < ptr; i++)
		{
			total_cost += 2; //the fid and lenth
			int size = infos[i].items.size();
			total_cost = total_cost + size*2;
		}
		return total_cost;
	}
	int finish()
	{
		for ( int i = 0; i < ptr; i++)
			infos[i].items.clear();

		int size = ptr;
		ptr = 0;
		return size;
	}

	unsigned total_meta;
	unsigned char* vbuf;
	int dump_frag()
	{
		unsigned char* vbuf_ptr = vbuf;
		memset(fn, 0, 256);
		int size;
		int total_size = 0;
		for ( int i = 0; i< ptr; i++){
			//fwrite(&infos[i].fid, sizeof(unsigned), 1, finfo);
			//fwrite(&infos[i].len, sizeof(unsigned), 1, finfo);
			//VBYTE_ENCODE(vbuf_ptr, infos[i].fid);
			VBYTE_ENCODE(vbuf_ptr, infos[i].len);

			size = infos[i].items.size();
			total_size += size;
			VBYTE_ENCODE(vbuf_ptr, size);
			//fwrite(&size, sizeof(unsigned), 1, finfo);
			for ( list<p>::iterator its = infos[i].items.begin(); its!=infos[i].items.end(); its++)
			{
				int a = (*its).vid;
				int b = (*its).offset;
				VBYTE_ENCODE(vbuf_ptr, a);
				VBYTE_ENCODE(vbuf_ptr, b);
			}

			infos[i].items.clear();
		}

		fwrite(vbuf, sizeof(unsigned char), vbuf_ptr-vbuf, ffrag);
		int temp = ptr;
		ptr =0;
		return total_size;
	}
	

	FILE* ffrag;

	partitions(int wsize)
	{
		total_meta = 0;
		vbuf = new unsigned char[10000000];
		winnow_buf = new unsigned char[5000000];
		bound_buf = new int[1000000];
		md5_buf = new unsigned[2000000];
		add_list = new posting[100000000];
		infos = new finfo[5000000];

		char fn[256];
		memset(fn, 0, 256);
		sprintf(fn, "frag_%d.info", wsize);
		w_size[0] = wsize;//50;
		ffrag = fopen(fn, "wb");
		fID = 0;
	}

~partitions()
{
	FILE* fs  =fopen("result", "w");
	fprintf(fs, "%d\t%d\n", w_size[0], total_meta);
	fclose(ffrag);
}

	posting* add_list;
	int add_list_len;
private:
	unsigned char* winnow_buf; //= new unsigned char[MAX_TERM_PER_DOC];
	int* bound_buf;// = new unsigned[MAX_TERM_PER_DOC];
	unsigned* md5_buf;
	static const unsigned W_SET = 4;
	unsigned w_size[W_SET];// = { 50, 100, 200, 300 };
	md5_state_t state;
	md5_byte_t digest[16];
	hStruct *h[W_SET];
public:
	int fID;
private:
	finfo* infos;
	int ptr;

	map<unsigned long, int> hashes;

};

#endif /* PARTITION_H_ */
