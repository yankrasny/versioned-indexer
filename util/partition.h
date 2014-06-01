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
public:
	void fragment(int vid, int* refer, int len) {
		add_list_len = 0;
		int k = 0;
		for (; k < len; k++)
			*(winnow_buf + k) = inthash(refer[k]);

		unsigned hh, lh;
		int ins;
		int i;
		int block_num = 0;
		int error;
		int ep;

		if (k <= (w_size[0] + B - 1)) {
			bound_buf[0] = 0;
			block_num = 1;
		} else {
			if ((block_num = winnow(winnow_buf, k, w_size[0], bound_buf))== -1){
				error = 1;
				printf("winnow error.\n");
			}
		}

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
					fID++;

					for ( int l = bound_buf[j]; l < ep; l++)
					{
						add_list[add_list_len].vid = fID;
						add_list[add_list_len].wid = refer[l];
						add_list[add_list_len++].pos = l;
					}
				}
				else { //has sharing block
					p p2;
					p2.vid = vid;
					p2.offset = bound_buf[j];
					infos[ins].items.push_back(p2);
				}

			}

			

			
		//}
	}

	int init()
	{
		add_list_len = 0;
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
	int dump_frag()
	{
		memset(fn, 0, 256);
		sprintf(fn, "frag_%d.info", w_size[0]);
		FILE* finfo = fopen(fn, "a+");
		int size;
		for ( int i = 0; i< ptr; i++){
			fwrite(&infos[i].fid, sizeof(unsigned), 1, finfo);
			fwrite(&infos[i].len, sizeof(unsigned), 1, finfo);
			size = infos[i].items.size();
			fwrite(&size, sizeof(unsigned), 1, finfo);
			for ( list<p>::iterator its = infos[i].items.begin(); its!=infos[i].items.end(); its++)
			{
				fwrite(&(*its).vid, sizeof(int), 1, finfo);
				fwrite(&(*its).offset, sizeof(int), 1, finfo);
			}

			infos[i].items.clear();
		}

		fclose(finfo);

		ptr =0;

	}

	partitions(int wsize)
	{
		winnow_buf = new unsigned char[5000000];
		bound_buf = new int[1000000];
		md5_buf = new unsigned[100000];
		add_list = new posting[1000000];
		infos = new finfo[5000000];

		w_size[0] = wsize;//50;
		//w_size[1] = 100;
		//w_size[2] = 200;
		//w_size[3] = 300;
		fID = 0;
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
	int fID;
	finfo* infos;
	int ptr;

	map<unsigned long, int> hashes;

};

#endif /* PARTITION_H_ */
