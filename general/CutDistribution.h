/*
 * partition.h
 *
 *  Created on: Oct 8, 2009
 *      Author: jhe
 */

#ifndef PARTITION_H_
#define PARTITION_H_

#include        <cstdio>
#include        <cstdlib>
#include        <list>
#include        <vector>
#include        <map>
#include        <cmath>
#include	<algorithm>
#include	<fstream>
#include	<cstring>
#include	"lowerpartition.h"

#define MAXDIS 20000000

using namespace std;
struct node;

template <class T>
class partitions {
private:
    int total_level;
    int static const MAXDIFF = 4000000;
public:
    partitions()
    {
        bound_buf = new int[5000000];
        block_info2 = new block[5000000];
        stats = new unsigned[4000001];
        memset(stats, 0, 4000001);
    }

    ~partitions()
    {
        delete[] bound_buf;
        delete[] block_info2;
        ofstream fout("distribute.xls");
        for ( int i = 0; i < 4000000; ++i)
        {
            if (stats[i]!=0)
                fout << i <<'\t'<<stats[i]<<endl;
        }
        fout.close();
    }


    int fragment(int vid, int* refer, int len, int* mhbuf, int* hbuf) 
    {
        if (len == 0){
            return 0;
        }
        int block_num = 0;

        block_num = docCutter.cut(mhbuf, hbuf, len, 10, bound_buf);
        for ( int j = 0; j < block_num-1; j++)
        {
            block_info2[j].start = bound_buf[j];
            block_info2[j].end = bound_buf[j+1];
            int idx = block_info2[j].end-block_info2[j].start+1;
            stats[idx]++;
        }
        return 0;
    }
private:
    block* block_info2;
    int* bound_buf;
    unsigned* stats;
    T docCutter; 
};

#endif /* PARTITION_H_ */
