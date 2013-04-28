/*
 * selection.h
 *
 *  Created on: May 8, 2011
 *      Author: jhe
 */

#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include	<algorithm>


using namespace std;
typedef struct
{
    int total, total_dis;
    int post;
    float wsize;
}linfo;

struct linfo2
{
    int did, ptr;
    int meta, size;
    double rate;
    float wsize;
};

bool operator < (const linfo2& l1, const linfo2& l2){
    if (l1.rate < l2.rate)
        return true;
    else
        return false;
}


typedef struct
{
    int did;
    int ptr;
    int len;
    linfo2* lbuf;
}par_reader;

class selection
{
private:
    int threshold;
    unsigned* linfo_lens;
    unsigned* curr_off;
    par_reader* preaders;
    priority_queue<linfo2> myqueue;
    linfo2*  lbuf2;

    int load(){
        FILE* f2 = fopen("linfos", "rb");
        int size;
        fread(&size, sizeof(unsigned), 1, f2);
        linfo_lens = new unsigned[size+1];
        curr_off = new unsigned[size+1];
        lbuf2 = new linfo2[size];

        fread(linfo_lens, sizeof(unsigned), size, f2);
        fclose(f2);
        linfo_lens[size] = 0;
        int off = 0;
        int i = 0;
        do
        {
            curr_off[i] = off;
            off+=linfo_lens[i];
            i++;
        }while (i <= size);

        FILE* f1 = fopen("finfos", "rb");
        linfo* lbuf = new linfo[curr_off[size]];
        int nread = fread(lbuf, sizeof(linfo), off, f1);
        fclose(f1);
        printf("nread:%d\n", nread);

        preaders = new par_reader[size];
        int index_size = 0;
        char fn[256];
        
        printf("Total size;%d\n", size);
        for ( int i = 0; i < size; i++){

            preaders[i].did = i;
            preaders[i].len = linfo_lens[i];//linfo_lens[i+1] - linfo_lens[i];
            preaders[i].ptr = 1;
            if ( preaders[i].len == 0 )
            {
                    printf("%d\n", i);
                    continue;
            }
            preaders[i].lbuf = new linfo2[preaders[i].len];
            int j = curr_off[i];
            preaders[i].lbuf[0].size = lbuf[j].post;//lbuf[j].index_size;
            preaders[i].lbuf[0].meta = lbuf[j].total;//lbuf[j].meta_size;
            preaders[i].lbuf[0].rate = 1;
            preaders[i].lbuf[0].wsize = lbuf[j].wsize;
            preaders[i].lbuf[0].did = i;
            threshold -= preaders[i].lbuf[0].meta;
            if ( threshold < 0)
            {
                printf("I'm less than 0 now:%d, at:%d\n", threshold,i);
                exit(0);
            }
            index_size += preaders[i].lbuf[0].size;

            for ( int j = curr_off[i]+1; j< curr_off[i+1]; j++){
                preaders[i].lbuf[j-curr_off[i]].did = i;
                preaders[i].lbuf[j-curr_off[i]].size = lbuf[j-1].post-lbuf[j].post;
                preaders[i].lbuf[j-curr_off[i]].meta = lbuf[j].total - lbuf[j-1].total;
                preaders[i].lbuf[j-curr_off[i]].wsize = lbuf[j].wsize;
                preaders[i].lbuf[j-curr_off[i]].ptr = j-curr_off[i];
                if ( lbuf[j].total == lbuf[j-1].total)
                    preaders[i].lbuf[j-curr_off[i]].rate = lbuf[j].post - lbuf[j-1].post;
                else
                    preaders[i].lbuf[j-curr_off[i]].rate = (lbuf[j].post - lbuf[j-1].post)/(lbuf[j-1].total - lbuf[j].total);
            }
            if ( preaders[i].len > 1)
                myqueue.push(preaders[i].lbuf[1]);
            else
                preaders[i].ptr = 0;
        }

        printf("index size;%d\n", index_size);
        int count = 0;
        double rate;
        while ( myqueue.size() > 0)
        {
            count = 0;
            lbuf2[count++] = myqueue.top();

            rate = lbuf2[0].rate;
            myqueue.pop();
            while (myqueue.size()> 0 && rate == myqueue.top().rate )
            {
                lbuf2[count++] = myqueue.top();
                myqueue.pop();
            }

            while ( count > 0 )
            {
                linfo2 ite = lbuf2[--count];
                int seq = ite.did;
                if ( threshold < ite.meta) 
                {
                    preaders[seq].ptr--;
                }
                else
                {
                    threshold -= ite.meta;
                    index_size -= ite.size;
                    if ( preaders[seq].ptr < preaders[seq].len-1)
                    {
                        preaders[seq].ptr++;
                        int idx = preaders[seq].ptr;
                        myqueue.push(preaders[seq].lbuf[idx]);
                    }
                }
            }
        }

        FILE* fout = fopen("result", "wb");
        fwrite(&size, sizeof(unsigned), 1, fout);
        for ( int i = 0; i < size; i++){
            int idx = preaders[i].ptr;
            fwrite(&preaders[i].lbuf[idx].wsize, sizeof(float), 1, fout);
        }
        fclose(fout); 
        printf("Threshold:%d, index_size:%d\n", threshold, index_size);
        return 0;
    }
public:
    selection(int threshold){
        this->threshold = threshold;
        load();
    }

};


int main(int argc, char** argv)
{
    selection s(atoi(argv[1]));
    return 0;
}

