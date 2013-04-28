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
#include "vb.h"
#include <list>
#include <vector>
#include <map>
#include "heap.h"

#define MAXDIS 2000000

using namespace std;
struct node;

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
	node* nptr;
	bool isVoid; //if it's still effect
}p;

typedef struct
{
	int fid;
	int len;
	int size;
	double score;
	vector<p> items;
}finfo;

#include "tree.h"
int comparep(const void* a1, const void* a2)
{
	frag_ptr* f1 = (frag_ptr*)a1;
	frag_ptr* f2 = (frag_ptr*)a2;

	int ret = f1->ptr - f2->ptr;
	if ( ret == 0)
		return f1->off - f2->off;
	else
		return ret;
}

typedef struct
{
	unsigned wid, vid, pos;
}posting;


class partitions {
public:
	partitions(int para):myheap(MAXDIS)
	{
		selected_buf = new unsigned[MAXDIS];
		fbuf = new frag_ptr[2000000];
		refer_ptrs = new int*[20000];
		refer_ptrs2 = new unsigned char*[20000];
		vbuf = new unsigned char[50000000];
		winnow_buf = new unsigned char[5000000];
		bound_buf = new int[5000000];
		md5_buf = new unsigned[5000000];
		add_list = new posting[1000000];
		infos = new finfo[5000000];
		infos2 = new finfo[5000000];
		flens = new unsigned[5000000];
		block_info = new unsigned[5000000];
		id_trans = new unsigned[60*20000];
		fID = 0;
		base_frag = 0;

		char fn[256];
		sprintf(fn, "frag_%d.info", para);
		ffrag = fopen(fn, "wb");
	}

	~partitions(){
		delete[] fbuf;
		fclose(ffrag);
	}
	void completeCount(int wsize)
	{
		myheap.init();
		for ( int i = 0; i < ptr; i++){
			infos[i].size = infos[i].items.size();
		}
		//compute score for each piece.
		for ( int i = 0; i < ptr; i++){
			for (vector<p>::iterator its = infos[i].items.begin(); its != infos[i].items.end(); its++)
				its->isVoid = false;

			infos[i].score = static_cast<double>(infos[i].len*infos[i].size) /(1+infos[i].len+wsize*(1+infos[i].size)); 
			hpost ite;
			ite.ptr = i;
			ite.score = infos[i].score;
			myheap.push(ite);
		}

	}


	int compact(frag_ptr* fi, int len)
	{
		int ptr1 = 0;
		int ptr2 = 1;
		while ( ptr2 < len)
		{
			while ( ptr2 < len && fi[ptr2] == fi[ptr1])
				ptr2++;
			if ( ptr2==len)
				break;
			else
			{
				ptr1++;
				fi[ptr1] = fi[ptr2];
				ptr2++;
			}
		}
		return ptr1+1;
	}

	int select_ptr;
	void select(tree* root, int wsize)
	{
		select_ptr = 0;
		while ( myheap.size()>0)
		{
			hpost po = myheap.top();
			selected_buf[select_ptr++] = po.ptr;
			myheap.pop();
			int idx = po.ptr;
			int cptr = 0;
			for ( vector<p>::iterator its = infos[idx].items.begin(); its != infos[idx].items.end(); its++)
			{
				if ( its->isVoid == false){
					infos2[idx].items.push_back(*its);
					int len = root[its->vid].find_conflict(its->nptr, &fbuf[cptr]);
					cptr += len;
				}
			}
			infos2[idx].size = infos2[idx].items.size();
			infos2[idx].len = infos[idx].len;

			if ( cptr > 0 ){
				qsort(fbuf, cptr, sizeof(frag_ptr), comparep);
				cptr = compact(fbuf, cptr);
				for ( int i = 0; i < cptr; i++){
					
					int idx = fbuf[i].ptr;
					if ( i>0 && fbuf[i].ptr != fbuf[i-1].ptr){
						int idx2 = fbuf[i-1].ptr;
						if (infos[idx2].size >0)
						{
							infos[idx2].score = infos[idx2].len*infos[idx2].size /(1+infos[idx2].len+wsize*(1+infos[idx2].size));
							myheap.UpdateKey(idx2, infos[idx2].score);
						}
					}
					if ( infos[idx].items.at(fbuf[i].off).isVoid == false){
						infos[idx].size--;
						infos[idx].items.at(fbuf[i].off).isVoid = true;
						if ( infos[idx].size == 0)
							myheap.deleteKey(idx);
					}
				}

				idx = fbuf[cptr-1].ptr;
				if (infos[idx].size > 0)
				{
					infos[idx].score = infos[idx].len*infos[idx].size /(1+infos[idx].len+wsize*(1+infos[idx].size));
					myheap.UpdateKey(idx, infos[idx].score);
				}
			}
		}

	}


	unsigned char* tpbuf;
	void finish2(int* refer, int* wcounts, int id, int vs, binfo* bis)
	{
		if ( tpbuf ==NULL)
			tpbuf = new unsigned char[MAXSIZE];


		int start = 0;
		for ( int i = 0; i < vs; i++)
		{
			refer_ptrs2[i] = &tpbuf[start];
			start += wcounts[i];
		}
		memset(tpbuf, 0, start);

		add_list_len = 0;
		int new_fid = 0;
		int total = 0;
		bis->total_dis = select_ptr;
		int total_post = 0;
		for ( int i = 0; i < select_ptr; i++)
		{
			int myidx = selected_buf[i];
			total+= infos2[myidx].items.size();
			total_post += infos2[myidx].len;

			for ( vector<p>::iterator its = infos2[myidx].items.begin(); its!=infos2[myidx].items.end(); its++){
				int version = its->vid;
				int off = its->offset;
				int len = infos2[myidx].len;
				if ( its->isVoid == false){
					for ( int j = off; j < off+len; j++){
						if ( refer_ptrs2[version][j] > 0)
						{
							FILE* ferror = fopen("ERROR", "w");
							fprintf(ferror, "we got problems!%d, %d, %d\n", id, i, j);
							fclose(ferror);
							exit(0);
						}
						refer_ptrs2[version][j] = 1;
					}
				}
			}
			infos2[myidx].items.clear();
		}

		bis->total = total;
		bis->post = total_post;

		for ( int i = 0; i < vs; i++)
		{
			for ( int j = 0; j < wcounts[i]; j++){
				if (refer_ptrs2[i][j]==0){
					FILE* ferror = fopen("ERROR!!", "w");
					fprintf(ferror, "at doc:%d, version:%d\tpos:%d\n", id, i, j);
					fclose(ferror);
					exit(0);
				}
			}
		}

	}

	/*int cut(int* mhbuf, int len, int wsize, int* start)
	{
		int num = 0;
		start[num++] = 0;
		for ( int i = 0; i < len-B+2; i++)
		{
			if (mhbuf[i]>= wsize){
				start[num++] = i+B-2;
			}
			
		}
		return num;
	}*/

	int cut(int* mhbuf, int len, int wsize, int* start)
	{
		int num = 0;
		start[num++] = 0;
		for ( int i = 0; i < len-B+2; ++i)
		{
			if ( abs(mhbuf[i]-mhbuf[i+1])>=10)
			{
				start[num++] = i+B;
			}
		}
		return num;
	}

	int fragment(int vid, int* refer, int len, int* mhbuf, tree* root, int* w_size, int wlen) {

		if (len == 0){
			return 0;
		}
		unsigned hh, lh;
		int ins;
		int block_num = 0;
		int error;
		int ep;
		

		for ( int i = 0; i < wlen; i++){
			if (len <= (w_size[i] + B - 1)) {
				bound_buf[0] = 0;
				block_num = 1;
			}
			else {
				if ((block_num = cut(mhbuf, len, w_size[i], bound_buf))== -1){
					error = 1;
					printf("winnow error.\n");
				}
			}
			root->init_level(i, block_num);
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
				ins = lookupHash (lh, hh, h[0]);
			
				if (ins == -1) {
					p p1;
					p1.vid = vid;
					p1.offset = bound_buf[j];
					p1.isVoid = false;

					insertHash(lh, hh, ptr, h[0]);
					infos[ptr].fid = fID;
					infos[ptr].size = 0;

					frag_ptr fptr;
					fptr.ptr = ptr;
					fptr.off = infos[ptr].size;

					p1.nptr = root->insert(i, fptr, bound_buf[j], ep);
		
					if ( p1.nptr != NULL){
						if ( j != block_num - 1)
							infos[ptr].len = ep - bound_buf[j];
						else
							infos[ptr].len = len - bound_buf[j];
					
						if ( infos[ptr].len < 0 )
						{
							printf("we have an error!%d\tprev bound:%d\tnow bound:%d\t%d\n", block_num, bound_buf[j], bound_buf[j+1], j);
							return -1;
						}
						infos[ptr].items.push_back(p1);
						infos[ptr].size++;
						ptr++;
						if ( ptr >= MAXDIS)
						{
							printf("too many fragment !\n");
							exit(0);
						}
						fID++;
					}
				}
				else { //has sharing block

					frag_ptr fptr;
					fptr.ptr = ins;
					fptr.off = infos[ins].size;
					p p2;
					p2.vid = vid;
					p2.offset = bound_buf[j];
					p2.isVoid = false;
					p2.nptr = root->insert(i, fptr, bound_buf[j], ep);

					if ( p2.nptr != NULL){
						infos[ins].items.push_back(p2);
						infos[ins].size++;
					}
				}
			}
			if ( i > 0)
			{	
					int total = 0;
					node* n = root->node_array[i-1][0].child_start;
					for ( int k = 0; k < root->len[i-1]; k++){
						if ( n != root->node_array[i-1][k].child_start)
						{
							printf("wrong!\n");
							exit(0);
						}
						total+=root->node_array[i-1][k].child_len;
						for ( int m = 0; m < root->node_array[i-1][k].child_len; m++)
							n++;
					}
					if ( total != root->len[i])
					{
						for ( int m = 0; m < ptr; m++)
							infos[m].items.clear();

						printf("we got a problem during winnowing!%d\t%d\n", total, root->len[i]);
						exit(0);
						return -1;
					}
			}
			
		}	
		return 0;
	}


	int init()
	{
		add_list_len = 0;
		flens_count = 0;
		block_info_ptr = 0;
		ptr = 0;
		fID = 0;
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
	unsigned char* vbuf;
public:
	posting* add_list;
	unsigned* block_info;
	int block_info_ptr;
	int add_list_len;
	unsigned* flens;
	unsigned flens_count;
	unsigned base_frag;


	int dump_frag(int id)
	{
	
		char fn[256];
		unsigned char* vbuf_ptr = vbuf;
		memset(fn, 0, 256);
		sprintf(fn, "meta2/%d.block", id);
		FILE* ff =fopen(fn, "wb");
		int size;
		int total_size = 0;
		for ( int i = 0; i< ptr; i++){
			VBYTE_ENCODE(vbuf_ptr, infos[i].len);
			size = infos[i].items.size();
			total_size += size;
			VBYTE_ENCODE(vbuf_ptr, size);
			//fwrite(&size, sizeof(unsigned), 1, finfo);
			for ( vector<p>::iterator its = infos[i].items.begin(); its!=infos[i].items.end(); its++)
			{
				int a = (*its).vid+base_frag;
				int b = (*its).offset;
				VBYTE_ENCODE(vbuf_ptr, a);
				VBYTE_ENCODE(vbuf_ptr, b);
			}

			infos[i].size = 0;
			infos[i].items.clear();
		}
		int mysize = vbuf_ptr-vbuf;
		fwrite(&mysize,sizeof(unsigned), 1, ff);
		fwrite(vbuf, sizeof(unsigned char), vbuf_ptr-vbuf, ff);
		fclose(ff);
		int temp = ptr;
		ptr =0;
		return total_size;
	}
private:
	unsigned char* winnow_buf; //= new unsigned char[MAX_TERM_PER_DOC];
	unsigned* hval; //the hash buffer prepared for the winnow
	int* bound_buf;// = new unsigned[MAX_TERM_PER_DOC];
	unsigned* md5_buf;
	static const unsigned W_SET = 4;
	unsigned w_size[W_SET];// = { 50, 100, 200, 300 };
	md5_state_t state;
	md5_byte_t digest[16];
	hStruct *h[W_SET];
	int fID;
	finfo* infos;
	finfo* infos2;
	int ptr;
	map<unsigned long, int> hashes;
	frag_ptr* fbuf;
	int** refer_ptrs;
	unsigned char** refer_ptrs2;
	unsigned* id_trans;
	heap myheap;
	unsigned* selected_buf;
	FILE* ffrag; //the file to write meta inforation
};

#endif /* PARTITION_H_ */
