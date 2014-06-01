#ifndef UTILITY_H_
#define UTILITY_H_

#include <stdio.h>
#include <stdlib.h>

#include "wiki_corpus.h"


int compare_pos(const void* a1, const void* a2)
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

int compare_pos2(const void* a1, const void* a2)
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

class docreader{
public:
	docreader(posting* pos_buf, int* wcounts, int vs, int id)
	{
		total =0;
		curr = -1;
		ptr = 0;
		this->pos_buf = pos_buf;
		this->wcounts = wcounts;
		this->id = id;
		this->vs = vs;
		for ( int i = 0; i < vs; i++)
			total += wcounts[i];
		qsort(pos_buf, total, sizeof(posting), compare_pos2);
	}

	bool next()
	{
		if ( curr == -1 )
		{
			ptr = 0;
			curr++;
			return true;
		}

		if ( curr < vs ){
			ptr += wcounts[curr];
			curr++;
			return true;
		}
		else
			return false;
	}

	int current(int* outs)
	{
		transform(&pos_buf[ptr], wcounts[curr], outs);
		return wcounts[curr];
	}

	int get_all(int* int_buf)
	{
		for ( int i = 0; i < total; i++)
			int_buf[i] = pos_buf[i].wid;

		return total;
	}

private:
	void transform(posting* p, int count, int* out)
	{
		//qsort(p, count, sizeof(posting), compare_pos);
		for ( int i = 0; i < count; i++){
			out[i] = p[i].wid;
		}
	}

	posting* pos_buf;
	int* wcounts;
	int ptr;
	int vs;
	int id;
	int curr;
	int total;
};
#endif
