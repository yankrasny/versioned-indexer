#include <stdio.h>
#include <string.h>
#include <iostream>
#include <list>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
//#include <dirent.h>
#include <algorithm>
#include <queue>
#include "tuple.h"
#include "indexreader.h"

#define MAXDOC 400000000
//#define BUFSIZE 3200000000
using namespace std;



typedef struct
{
	index_item its;
	int seq;
}merge_item;

post* p;

unsigned* dat_buf;


bool operator < (const merge_item& e1, const merge_item& e2)
{

	int ret = e1.its.wid - e2.its.wid;

	if ( ret == 0 )
	{
		if ( e1.seq > e2.seq)
			return true;
		else
			return false;
	}
	else if ( ret > 0 )
		return true;
	else
		return false;
}

class multi_merge
{
public:
	multi_merge(int kway)
	{
		kways = kway;
		buf = new merge_item[kway+1];
		readers = new indexreader[kway];
		//dat_buf = new post[85000000];
		dat_buf = new post[400000000];
	}

	int merge(char* dir, int start_id, int dst, int level, int type, int version)
	{
		sprintf(fn, "%s/%d_%d_%d_%d.idx", dir, level+1, type, version, dst);
		FILE* fidx_out = fopen64(fn, "w");
		memset(fn, 0, 256);
		sprintf(fn, "%s/%d_%d_%d_%d.dat", dir, level+1, type, version, dst);
		FILE* fdat_out = fopen64(fn, "w");

		priority_queue<merge_item> queues;

		for ( int i = 0; i < kways; i++)
		{
			memset(fn, 0, 256);
			sprintf(fn, "%s/%d_%d_%d_%d", dir, level, type, version, start_id+i);
			readers[i].set_buf(10000);
			if (readers[i].open(fn)<0)
				continue;
			merge_item newitem;
			newitem.seq = i;
			if (readers[i].next()){
				newitem.its = readers[i].current();
				queues.push(newitem);
			}
		}

		int word;
		int count = 0;
		int curr_freq = 0;
		long curr_offset = 0;
		index_item temp;

		while (queues.size() > 0 )
		{
			curr_freq = 0;
			count = 0;
			buf[count++] = queues.top();

			word = buf[0].its.wid;
			queues.pop();

			while (queues.size()> 0 && word == queues.top().its.wid )
			{
					buf[count++] = queues.top();
					queues.pop();
			}

			curr_offset = ftell(fdat_out);

			for ( int i = 0; i < count; i++)
			{
				curr_freq += buf[i].its.freq;
				if (buf[i].its.freq >MAXDOC)
				{
					printf("ERROR!!!\nbuf size:%d\n", buf[i].its.freq);
					exit(0);
				}

				readers[buf[i].seq].get_current_posting(dat_buf);
				fwrite(dat_buf, sizeof(post), buf[i].its.freq, fdat_out);
			}

			temp.offset = curr_offset;
			temp.freq = curr_freq;
			temp.wid = word;
			fwrite(&temp, sizeof(index_item), 1, fidx_out);

			while ( count > 0 )
			{
				merge_item ite;
				ite.seq = buf[--count].seq;

				if ( readers[ite.seq].next() ){
					ite.its = readers[ite.seq].current();
					queues.push(ite);
				}
			}
		}

		for ( int i = 0; i < kways; i++)
			readers[i].close();


		fclose(fdat_out);
		fclose(fidx_out);
		return 0;
	}

	~multi_merge()
	{
		delete[] readers;
		delete[] dat_buf;
		delete[] buf;
	}
private:
	char fn[256];
	int kways;
	merge_item* buf;
	indexreader* readers;
	post* dat_buf;
};


//int main(int argc,  char** argv)
//{
//	
//	if ( argc != 8)
//	{
//		printf("merger start end lower_level k-way type version dir\n");
//		exit(1);
//	}
//
//	//FILE* f = fopen(argv[1], "r");
//
//	int start, end, lower, k, type, version;
//	//fscanf(f, "%d\t%d\t%d\t%d\t%d\t%d\n", &start, &end, &lower, &k, &type, &version);
//	
//	start = atoi(argv[1]);
//	end = atoi(argv[2]);
//	lower = atoi(argv[3]);
//	k = atoi(argv[4]);
//	type = atoi(argv[5]);
//	version = atoi(argv[6]);
//	int j = start/k;
//	
//	printf("start:%d, k:%d\n", start, k);
//	multi_merge m(k);
//
//
//	for ( int i = start; i<= end; i += k)
//	{
//		m.merge("./", i, j, lower, type, version);
//		j++;
//		printf("complete:%d\n", j);
//	}
//	exit(0);
//	
//
//	return 0;
//}


