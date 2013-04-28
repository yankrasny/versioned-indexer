/*
 * tree.h
 *
 *  Created on: May 13, 2011
 *      Author: jhe
 */

#ifndef TREE_H_
#define TREE_H_

#define TREE_LEVEL 3

typedef struct
{
	int ptr;
	int off;
}frag_ptr;

struct node
{
	frag_ptr fptr;
	int start, end;
	node* child_start;
	int child_len;
	node* parent;
};

bool operator != (const frag_ptr& f1, const frag_ptr& f2)
{
	if ( f1.ptr == f2.ptr && f1.off == f2.off)
		return false;

	return true;
}

bool operator == (const frag_ptr& f1, const frag_ptr& f2)
{
	return (f1.ptr == f2.ptr && f1.off == f2.off);
}

class tree
{
public:
	int total_level;
	node** node_array;
	int* len;//[TREE_LEVEL];
	int* ptr;//[TREE_LEVEL];
public:
	tree(){
		len = NULL;
		node_array = NULL;
		ptr = NULL;
	}

	void initalTree(int total_level){
		this->total_level = total_level;
		node_array = new node*[total_level];
		len = new int[total_level];
		ptr = new int[total_level];

	}

	int readTree(unsigned* buf)
	{
		int ptrs = 0;
		for ( int i = 0; i < total_level; i++)
		{
			len[i] = buf[ptrs++];
			node_array[i] = new node[len[i]];
			for (int j = 0; j < len[i]; j++)
			{
				node_array[i][j].fptr.ptr = buf[ptrs++];
				node_array[i][j].fptr.off = buf[ptrs++];
				node_array[i][j].start = buf[ptrs++];
				node_array[i][j].end = buf[ptrs++];
				node_array[i][j].child_len = buf[ptrs++];
				node_array[i][j].child_start = NULL;
				node_array[i][j].parent =NULL;

			}
		}
		return ptrs;

	}

	int setup_Connnection(finfo* infos)
	{
		for ( int i = 0; i < total_level; i++)
		{
			int child_ptr = 0;
			for ( int j = 0; j < len[i]; j++)
			{
				int ptr = node_array[i][j].fptr.ptr;
				int off = node_array[i][j].fptr.off;
				if ( infos[ptr].items.at(off).nptr == NULL)
					infos[ptr].items.at(off).nptr = &node_array[i][j];	
				if ( i != total_level-1)
				{
					node_array[i][j].child_start = &node_array[i+1][child_ptr];
					for ( int k = 0; k < node_array[i][j].child_len; k++)
						node_array[i+1][child_ptr++].parent = &node_array[i][j];
				}
			}
		}
		return 0;
	}

	int storeTree(unsigned* buf)
	{
		int ptr = 0;
		for ( int i = 0; i < total_level; i++)
		{
			buf[ptr++] = len[i];
//			fwrite(&len[i], sizeof(unsigned), 1, f);
			for ( int j = 0; j < len[i];j++)
			{
				buf[ptr++] = node_array[i][j].fptr.ptr;
				buf[ptr++] = node_array[i][j].fptr.off;
				buf[ptr++] = node_array[i][j].start;
				buf[ptr++] = node_array[i][j].end;
				buf[ptr++] = node_array[i][j].child_len;
			}
		}
		return ptr;
	}

	~tree()
	{
		if ( node_array!=NULL){
			for (int i = 0; i < total_level; i++)
				delete[] node_array[i];
			delete[] node_array;
		}

		if ( len != NULL)
			delete[] len;
		if ( ptr != NULL)
			delete[] ptr;
	}

	int init_level(int level, int lens){
		node_array[level] = new node[lens];
		len[level] = 0;
		ptr[level] = 0;
		return 0;
	}

	node* insert(int curr_level, const frag_ptr& finfo, int start, int end){
		int i = curr_level;
		if ( i > 0){
			node* nptr = node_array[i-1];
			for ( int j = ptr[i-1]; j < len[i-1]; j++)
			{
				if (nptr[j].start == start && nptr[j].end == end)
				{
					int ptrs = len[i];
					node_array[i][ptrs] = nptr[j];
					node_array[i][ptrs].child_start = NULL;
					node_array[i][ptrs].child_len = 0;
					len[i]++;

					if ( nptr[j].child_len == 0)
					{
						nptr[j].child_start = &node_array[i][ptrs];
						nptr[j].child_len++;
					}
					else
						nptr[j].child_len++;

					node_array[i][ptrs].parent = &nptr[j];
					ptr[i-1] = j;

					//basically doing nothing here
					//if ( finfo == nptr[j].fptr)
						return NULL;
					//else
					//	return &node_array[i][ptrs];

				}
				if(nptr[j].start <= start && nptr[j].end >= end)
				{
					int ptrs = len[i];
					node_array[i][ptrs].fptr = finfo;
					node_array[i][ptrs].start = start;
					node_array[i][ptrs].end = end;
					node_array[i][ptrs].child_start = NULL;
					node_array[i][ptrs].child_len = 0;
					len[i]++;

					if ( nptr[j].child_len == 0)
					{
						nptr[j].child_start = &node_array[i][ptrs];
						nptr[j].child_len++;
					}
					else
						nptr[j].child_len++;

					node_array[i][ptrs].parent = &nptr[j];
					ptr[i-1] = j;
					return &node_array[i][ptrs];
				}
			}
		}
		else{
			int ptr  = len[0];
			node_array[i][ptr].fptr = finfo;
			node_array[i][ptr].start = start;
			node_array[i][ptr].end = end;
			node_array[i][ptr].parent = NULL;
			node_array[i][ptr].child_len = 0;
			node_array[i][ptr].child_start = NULL;
			len[0]++;
			return &node_array[i][ptr];
		}
	}

	int going_down(node* n, frag_ptr* fbuf)
	{

		node* nodes = n->child_start;
		int ptr = 0;
		int size;
		if ( nodes != NULL){
			for ( int i = 0; i < n->child_len; i++)
			{
				if ( n->fptr != nodes->fptr)
					fbuf[ptr++] = nodes->fptr;

				size = going_down(nodes, &fbuf[ptr]);
				ptr += size;
				nodes++;
			}
		}

		return ptr;
	}

	int find_conflict(node* n, frag_ptr* fbuf){
		node* ntemp = n->parent;
		int ptr = 0;
		while ( ntemp != NULL)
		{
			if (ntemp->fptr != n->fptr)
				fbuf[ptr++] = ntemp->fptr;
			ntemp = ntemp->parent;
		}

		int size = going_down(n, &fbuf[ptr]);
		//qsort(fbuf, size+ptr, sizeof(frag_ptr), comparefrag);
		return size+ptr;
	}


};
#endif /* TREE_H_ */
