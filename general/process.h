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

struct node;

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

typedef struct
{
	unsigned wid, vid, pos;
}posting;

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
		memset(fn, 0, 256);
		sprintf(fn, "base_frag_%d", para);
		fbase_frag = fopen(fn, "w");
		load_global();
	}

	~partitions(){
		delete[] fbuf;
		fclose(ffrag);
		fclose(fbase_frag);

	}
	void completeCount(double wsize)
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
	void select(tree* root, double wsize)
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
	void finish3(int* refer, int* wcounts, int id, int vs, binfo* bis)
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

	void finish2(int* refer, int* wcounts, int id, int vs)
	{
		int start = 0;
		for ( int i = 0; i < vs; i++)
		{
			refer_ptrs[i] = &refer[start];
			start += wcounts[i];
		}

		add_list_len = 0;
		int new_fid = 0;
		unsigned char* vbuf_ptr = vbuf;

		for ( int i = 0; i < select_ptr; i++)
		{
			int myidx = selected_buf[i];
			p po = infos2[myidx].items.at(0);
			int version = po.vid;
			int off = po.offset;
			int len = infos[myidx].len;
			for ( int j = off; j < off+len; j++){
				add_list[add_list_len].vid = i+base_frag;
				add_list[add_list_len].pos = j-off;
				add_list[add_list_len++].wid = refer_ptrs[version][j];
			}

			VBYTE_ENCODE(vbuf_ptr, len);
			int size = infos2[myidx].size;//items.size();
			VBYTE_ENCODE(vbuf_ptr, size);
			for ( vector<p>::iterator its = infos2[myidx].items.begin(); its!=infos2[myidx].items.end(); its++)
			{
				int a = (*its).vid+currid[id];
				int b = (*its).offset;
				VBYTE_ENCODE(vbuf_ptr, a);
				VBYTE_ENCODE(vbuf_ptr, b);
			}

			infos2[myidx].items.clear();
		}

		for ( int i = 0; i < vs; i++)
			fprintf(fbase_frag, "%d\n", base_frag);

		base_frag += select_ptr;
		fwrite(vbuf, sizeof(unsigned char), vbuf_ptr-vbuf, ffrag);

		for ( int i = 0; i < ptr; i++){
			infos[i].size = 0;
			infos[i].items.clear();
		}

	}

	int dump_frag(int id)
	{
		for ( int i = 0; i< ptr; i++){
			infos[i].size = 0;
			infos[i].items.clear();
		}
		return 0;
	}

	void load(int id, int*wcounts, int vs, tree* trees, unsigned* tree_buf)
	{
		int size;
		char fn[256];
		memset(fn, 0, 256);
		sprintf(fn, "meta2/%d.block", id);
		FILE* f = fopen(fn, "rb");
		fread(&size, sizeof(unsigned), 1, f);
		int nread = fread(vbuf, sizeof(unsigned char), size, f);
		fclose(f);
		unsigned char* vbuf_ptr = vbuf;
		ptr = 0;
		int block_len, block_size;
		int vid, off;
		while ( vbuf_ptr - vbuf < nread)
		{
			VBYTE_DECODE(vbuf_ptr, block_len);
			VBYTE_DECODE(vbuf_ptr, block_size);
			infos[ptr].len = block_len;
			infos[ptr].size = block_size;
			infos[ptr].items.clear();
			for ( int i = 0; i < block_size; i++){
				p p1;
				p1.isVoid = false;
				VBYTE_DECODE(vbuf_ptr, vid);
				VBYTE_DECODE(vbuf_ptr, off);
				p1.vid = vid;
				p1.offset = off;	
				p1.nptr = NULL;
				infos[ptr].items.push_back(p1);
			}
			ptr++;
		}

		int tptr = 0;
		int total_level = tree_buf[tptr++];
		for ( int i = 0; i < vs; i++)
		{
			if ( wcounts[i] > 0){
				trees[i].initalTree(total_level);
				int size = trees[i].readTree(&tree_buf[tptr]);
				trees[i].setup_Connnection(infos);
				tptr+=size;
			}
		}

		return;
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


private:
	int load_global(){
		FILE* f = fopen("maxid", "rb");
		int size;
		fread(&size,sizeof(unsigned), 1, f);
		maxid = new unsigned[size];
		currid = new unsigned[size];
		fread(maxid, sizeof(unsigned), size, f);
		fclose(f);
		
		currid[0] = 0;
		for ( int i = 1; i < size; i++)
			currid[i] = currid[i-1]+maxid[i-1];

	}
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
	FILE* fbase_frag;
	unsigned* maxid;
	unsigned* currid;
};

#endif /* PARTITION_H_ */
