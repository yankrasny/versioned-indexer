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
#include        "heap.h"
#include        <cmath>
#include	<algorithm>

#define MAXDIS 20000000

using namespace std;
struct node;
 
// I think this is a fragment application
struct p
{
    int vid, offset;
    int nptr;
    bool isVoid; //if it's still effect
};

// Looks like my FragInfo
struct finfo
{
    int fid;
    int len;
    int size;
    double score;
    vector<p> items;
};

#include "iv.h"
#include	"lowerpartition.h"

struct PCompare:public binary_function<frag_ptr, frag_ptr, bool>
{
    inline bool operator() (const frag_ptr& f1, const frag_ptr& f2)
    {
        if (f1.ptr == f2.ptr)
            return f1.off < f2.off;
        else
            return f1.ptr < f2.ptr;
    }
}comparep;

struct posting
{
    unsigned wid, vid, pos;
};

template<typename T>
class partitions {
private:
    int total_level;
public:
    partitions(int para,int maxblock, int blayer, int pTotal):myheap(MAXDIS),
    docCutter(para,maxblock, blayer),
    total_level(pTotal)
    {
        selected_buf = new unsigned[MAXDIS];
        fbuf = new frag_ptr[5000000];
        refer_ptrs = new int*[20000];
        refer_ptrs2 = new unsigned char*[20000];
        vbuf = new unsigned char[50000000];
        bound_buf = new int[5000000];
        md5_buf = new unsigned[5000000];
        add_list = new posting[1000000];
        infos = new finfo[5000000];
        infos2 = new finfo[5000000];
        flens = new unsigned[5000000];
        block_info = new unsigned[5000000];
        block_info2 = new block[5000000];
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
        for ( int i = 0; i < ptr; i++)
        {
            infos[i].size = infos[i].items.size();
        }

        for ( int i = 0; i < ptr; i++)
        {
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
    void select(iv* root, double wsize)
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
                    int len = root[its->vid].find_conflict(its->nptr, infos[idx].items, po.ptr, &fbuf[cptr]);
                    cptr += len;
                }
            }
            infos2[idx].size = infos2[idx].items.size();
            infos2[idx].len = infos[idx].len;

            if ( cptr > 0 ){
                sort(fbuf, fbuf+cptr, comparep);
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

    void finish3(int* refer, int* wcounts, int id, int vs)
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
            int len = infos2[myidx].len;
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

    void PushBlockInfo(int* refer, int* wcounts, int id, int vs, binfo* bis)
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
                if (!its->isVoid){
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
                    FILE* ferror = fopen("ERRORS", "w");
                    fprintf(ferror, "at doc:%d, version:%d\tpos:%d\n", id, i, j);
                    fclose(ferror);
                    exit(0);
                }
            }
        }

    }


    /*
        My version of fragment, works on all versions at once using RePair

        Params
            int* buf becomes int** buf: the contents of all the versions


        Objectives
            Set bound_buf (same as offsetsAllVersions)
            Set root (object type: iv, see iv.h): Looks like inverted list!

    */
    // int fragmentUsingRepair(int* buf, int* versionSizes, int numVersions, iv* root, unsigned minFragSize, unsigned repairStoppingPoint)
    // {
    //     // Convert buf and versionSizes into a vector of vectors
    //     // Choosing to do this instead of changing the Repair code
    //     vector<vector<unsigned> > versions = vector<vector<unsigned> >();
    //     int current(0);
    //     for (int i = 0; i < numVersions; i++)
    //     {
    //         versions[i] = <vector<unsigned>();
    //         for (int j = current; j < versionSizes[i]; j++)
    //         {
    //             versions[i].push_back(buf[j]);
    //         }
    //         current = versionSizes[i];
    //     }

    //     // Keeping my own variables for sanity
    //     vector<Association> associations;
    //     unsigned* offsetsAllVersions(NULL);
    //     unsigned* versionPartitionSizes(NULL);

    //     // Run Repair
    //     double score = runRepairPartitioning(versions, offsetsAllVersions, versionPartitionSizes, associations, minFragSize, repairStoppingPoint);

    //     // Set the variables that Jinru needs
    //     bound_buf = offsetsAllVersions;
    //     offsetsAllVersions = NULL;

    //     // TODO what about versionPartitionSizes? Ask Jinru.

    //     // Do we want to use our scoring scheme or his? 
    //         // Ours for now. Just check out how he does it below, you see he runs a cutting alg inside the while loop, and we can do the same.
    //             // Now we know what total_level is: it's the amount of iterations he's willing to optimize for, aka the # of times he calls the cutting alg




    //     return 0;
    // }

    /*

        We need to make repair set these variables -yan

        Example of wcounts, the size of each version. This means that version 0 contains 50 words, version 1 contains 52 words, etc.
        wcounts = [50, 52, 55, 49, 55]

        int vid: versionID (0, 1, 2, ...)
        int* refer: the contents of the current version (what I call wordIDs)
        int len: the length of the current version (wordIDs.size())
        int* mhbuf: hash stuff that I don't need
        int* hbuf: hash stuff that I don't need
        iv* root: passed as &trees[i] (inverted list but why is it called root?)
        int* w_size:
        int wlen: size of w_size

        par->fragment(i, &buf[wptr], wcounts[i], &mhbuf[hptr], &hbuf[hptr],  &trees[i], wsizes, total);

    */
    int fragment(int vid, int* refer, int len, int* mhbuf, int* hbuf, iv* root, int* w_size, int wlen) {
        if (len == 0)
        {
            return 0;
        }
        unsigned hh, lh;
        int ins;
        int block_num = 0;
        int error;
        int ep;

        /*
        TODO define:
            w_size
            bound_buf: the partition boundary buffer
            total_level: the amount of iterations he's willing to optimize for, aka the # of times he calls the cutting alg
            md5_buf: a buffer for storing fragment hashes?
            type p (posting?)
            infos: an array of finfo
            block_info2

        Sources:
            docCutter.cut()
            docCutter.cut3()
            this class's vars


        */

        int merge_size = 1;
        block_num = docCutter.cut(mhbuf, hbuf, len, w_size[wlen-1], bound_buf);
        for ( int j = 0; j < block_num-1; j++){
            block_info2[j].start = bound_buf[j];
            block_info2[j].end = bound_buf[j+1];
        }

        block_info2[block_num-1].start = bound_buf[block_num-1];
        block_info2[block_num-1].end = len;

        int counts = 0;
        while (counts < total_level)
        {
            // for all the blocks returned by the cutting alg
            for ( int j = 0; j < block_num; j++)
            {
                // Using refer as the version, so in ours, we need to pass a versionId!!!!!!!!!!
                for ( int l = block_info2[j].start; l < block_info2[j].end; l++)
                    md5_buf[l - bound_buf[j]] = refer[l];

                ep = block_info2[j].end;

                md5_init(&state);
                md5_append(&state, (const md5_byte_t *) md5_buf, sizeof(unsigned) * (ep - block_info2[j].start));
                md5_finish(&state, digest);
                hh = *((unsigned *)digest) ;
                lh = *((unsigned *)digest + 1) ;
                ins = lookupHash (lh, hh, h[0]);

                // Didn't find hash for this fragment
                if (ins == -1) {
                    p p1;
                    p1.vid = vid;

                    // TODO this can tell you a lot
                    p1.offset = block_info2[j].start;
                    p1.isVoid = false;

                    insertHash(lh, hh, ptr, h[0]);
                    infos[ptr].fid = fID;
                    infos[ptr].size = 0;

                    frag_ptr fptr;
                    fptr.ptr = ptr;
                    fptr.off = infos[ptr].size;

                    // TODO what's nptr? look at the insert() function
                    p1.nptr = root->insert(fptr, block_info2[j].start, ep);

                    if ( p1.nptr != -1){
                        infos[ptr].len = block_info2[j].end - block_info2[j].start;
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
                else { // has sharing block (or fragment as I call it -YK)

                    frag_ptr fptr;
                    fptr.ptr = ins;
                    fptr.off = infos[ins].size;
                    p p2;
                    p2.vid = vid;
                    p2.offset = block_info2[j].start;
                    p2.isVoid = false;
                    p2.nptr = root->insert(fptr, block_info2[j].start, ep);

                    if ( p2.nptr != -1){
                        infos[ins].items.push_back(p2);
                        infos[ins].size++;
                    }
                }
            }

            int prev_num = block_num;

            block_num = docCutter.cut3(block_info2, block_num, counts);
            counts++;
            if ( prev_num == block_num)
                break;
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
        return 0;
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
    block* block_info2;
    int block_info_ptr;
    int add_list_len;
    unsigned* flens;
    unsigned flens_count;
    unsigned base_frag;


    int dump_frag()
    {
        for ( int i = 0; i< ptr; i++){
            infos[i].size = 0;
            infos[i].items.clear();
        }
        ptr =0;
        return 0;
    }
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
    int ptr;
    int fID;
    heap myheap;
    static const unsigned W_SET = 4;
    md5_state_t state;
    md5_byte_t digest[16];
    FILE* ffrag; //the file to write meta inforation
    FILE* fbase_frag;

    T docCutter;

    /**************************
     * buffer used in the class
     * ***********************/
    hStruct *          h[W_SET];
    unsigned*          maxid; 
    unsigned*          currid;          // current ID of ...
    int*               bound_buf;       // the partition boundary buffer
    unsigned*          md5_buf;         // 
    finfo*             infos;           // an array of finfo
    finfo*             infos2;          // an array of finfo
    frag_ptr*          fbuf;            // an array of frag_ptr
    int**              refer_ptrs;      // versions (so each inner array is like wordIDs for one version)
    unsigned char**    refer_ptrs2;     // versions (so each inner array is like wordIDs for one version)
    unsigned*          id_trans;        // 
    unsigned*          selected_buf;    //
    unsigned char*     tpbuf;           // 
};

#endif /* PARTITION_H_ */
