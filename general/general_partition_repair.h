/*
 * general_partition_repair.h
 *
 *  Created on: May 11, 2013
 *  Adapted from general_partition.h by Jinru He
 *  Author: Yan Krasny
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
#include "heap.h"
#include <cmath>
#include <algorithm>

// Integration with Repair Partitioning code
#include "Repair-Partitioning/prototype/RepairPartitioningPrototype.h"

#define MAXDIS 20000000
#define MAX_NUM_WORDS_PER_DOC 200000

using namespace std;
struct node;
 
// I think this is a fragment application -YK
struct p
{
    int vid, offset;
    int nptr;
    bool isVoid; //if it's still effect
};

// Looks like my FragInfo -YK
struct finfo
{
    int fid;
    int len;
    int size;
    double score;
    vector<p> items;
};

#include "iv.h"
#include "lowerpartition.h"

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
    int load_global() {
        FILE* f = fopen("maxid", "rb");
        int size;
        fread(&size, sizeof(unsigned), 1, f);
        maxid = new unsigned[size];
        currid = new unsigned[size];
        fread(maxid, sizeof(unsigned), size, f);
        fclose(f);

        currid[0] = 0;
        for (int i = 1; i < size; i++)
        {
            currid[i] = currid[i-1]+maxid[i-1];
        }

    }

    int ptr;
    int fID;
    heap myheap;
    static const unsigned W_SET = 4;
    md5_state_t state;
    md5_byte_t digest[16];
    FILE* ffrag; //the file to write meta inforation
    FILE* fbase_frag;

    // T docCutter;

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

    // int total_level; // TODO where is this initialized?

public:
    partitions(int para = 0, int maxblock = 0, int blayer = 0, int pTotal = 0) : 
        myheap(MAXDIS)
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
        delete [] selected_buf;

        // TODO add all the delete [] that the previous author forgot.
        // "Forgot..." I hate that. I have to assume that's what happened.

        delete[] fbuf;
        fclose(ffrag);
        fclose(fbase_frag);
    }

    void completeCount(double wsize)
    {
        myheap.init();
        for (int i = 0; i < ptr; i++)
        {
            infos[i].size = infos[i].items.size();
        }

        for (int i = 0; i < ptr; i++)
        {
            for (vector<p>::iterator its = infos[i].items.begin(); its != infos[i].items.end(); its++)
            {
                its->isVoid = false;
            }

            infos[i].score = static_cast<double>(infos[i].len * infos[i].size) / (1 + infos[i].len + wsize * (1 + infos[i].size)); 
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
            while (ptr2 < len && fi[ptr2] == fi[ptr1])
            {
                ptr2++;
            }
            
            if (ptr2 == len)
            {
                break;
            }
            else
            {
                ptr1++;
                fi[ptr1] = fi[ptr2];
                ptr2++;
            }
        }
        return ptr1 + 1;
    }

    int select_ptr;
    void select(iv* root, double wsize)
    {
        select_ptr = 0;
        while (myheap.size() > 0)
        {
            hpost po = myheap.top();
            selected_buf[select_ptr++] = po.ptr;
            myheap.pop();
            int idx = po.ptr;
            int cptr = 0;
            for (vector<p>::iterator its = infos[idx].items.begin(); its != infos[idx].items.end(); its++)
            {
                if ( its->isVoid == false)
                {
                    infos2[idx].items.push_back(*its);
                    int len = root[its->vid].find_conflict(its->nptr, infos[idx].items, po.ptr, &fbuf[cptr]);
                    cptr += len;
                }
            }
            infos2[idx].size = infos2[idx].items.size();
            infos2[idx].len = infos[idx].len;

            if (cptr > 0)
            {
                sort(fbuf, fbuf+cptr, comparep);
                cptr = compact(fbuf, cptr);
                for (int i = 0; i < cptr; i++)
                {
                    int idx = fbuf[i].ptr;
                    if (i > 0 && fbuf[i].ptr != fbuf[i-1].ptr)
                    {
                        int idx2 = fbuf[i-1].ptr;
                        if (infos[idx2].size > 0)
                        {
                            infos[idx2].score = infos[idx2].len*infos[idx2].size /(1+infos[idx2].len+wsize*(1+infos[idx2].size));
                            myheap.UpdateKey(idx2, infos[idx2].score);
                        }
                    }
                    if (infos[idx].items.at(fbuf[i].off).isVoid == false)
                    {
                        infos[idx].size--;
                        infos[idx].items.at(fbuf[i].off).isVoid = true;
                        if ( infos[idx].size == 0)
                            myheap.deleteKey(idx);
                    }
                }

                idx = fbuf[cptr - 1].ptr;
                if (infos[idx].size > 0)
                {
                    infos[idx].score = infos[idx].len * infos[idx].size / (1 + infos[idx].len + wsize * (1 + infos[idx].size));
                    myheap.UpdateKey(idx, infos[idx].score);
                }
            }
        }

    }

    void finish3(int* refer, int* versionSizes, int id, int vs)
    {
        int start = 0;
        for ( int i = 0; i < vs; i++)
        {
            refer_ptrs[i] = &refer[start];
            start += versionSizes[i];
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

        for (int i = 0; i < vs; i++)
        {
            fprintf(fbase_frag, "%d\n", base_frag);
        }

        base_frag += select_ptr;
        fwrite(vbuf, sizeof(unsigned char), vbuf_ptr-vbuf, ffrag);

        for (int i = 0; i < ptr; i++)
        {
            infos[i].size = 0;
            infos[i].items.clear();
        }
    }

    void PushBlockInfo(vector<vector<unsigned> >& versions, int id, binfo* bis)
    {
        if (tpbuf == NULL) {
            tpbuf = new unsigned char[MAXSIZE];
        }

        int start = 0;
        for (int i = 0; i < vs; i++)
        {
            refer_ptrs2[i] = &tpbuf[start];
            start += versionSizes[i];
        }
        memset(tpbuf, 0, start);

        add_list_len = 0;
        int new_fid = 0;
        int total = 0;
        bis->total_dis = select_ptr;
        int total_post = 0;
        for (int i = 0; i < select_ptr; i++)
        {
            int myidx = selected_buf[i];
            total += infos2[myidx].items.size();
            total_post += infos2[myidx].len;

            for (vector<p>::iterator its = infos2[myidx].items.begin(); its != infos2[myidx].items.end(); its++)
            {
                int version = its->vid;
                int off = its->offset;
                int len = infos2[myidx].len;
                if (!its->isVoid)
                {
                    for (int j = off; j < off+len; j++)
                    {
                        if (refer_ptrs2[version][j] > 0)
                        {
                            FILE* ferror = fopen("ERROR", "w");
                            fprintf(ferror, "we got problems!%d, %d, %d\n", id, i, j); // thanks, "we got problems" really helps -YK
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

        for (int i = 0; i < vs; i++)
        {
            for (int j = 0; j < versionSizes[i]; j++){
                if (refer_ptrs2[i][j] < 0) {
                    FILE* ferror = fopen("ERRORS", "w");
                    fprintf(ferror, "at doc:%d, version:%d\tpos:%d\n", id, i, j);
                    fclose(ferror);
                    cerr << "See file ERRORS" << endl;
                    exit(0);
                }
            }
        }
    }

    /*
        Yan's version of cut, works on all versions at once using RePair
    */
    void cutAllVersions(vector<vector<unsigned> >& versions,
        unsigned* offsetsAllVersions, unsigned* versionPartitionSizes,
        float fragmentationCoefficient, unsigned minFragSize, unsigned method)
    {
        // This class is used as a wrapper around repair (See RepairPartitioningPrototype.h)
        RepairPartitioningPrototype prototype;

        // Run Repair Partitioning
        double score = prototype.runRepairPartitioning(
            versions,
            offsetsAllVersions,
            versionPartitionSizes,
            minFragSize,
            fragmentationCoefficient,
            method);

        // Set the variables that Jinru needs
        // Note: converting back to ints
        unsigned totalNumFrags(0);
        for (int i = 0; i < versions.size(); ++i)
        {
            // cerr << "Total num frags: " << totalNumFrags << endl;
            unsigned numFragsInVersion = versionPartitionSizes[i];
            if (numFragsInVersion > MAX_NUM_FRAGMENTS_PER_VERSION) {
                cerr << "numFragsInVersion " << i << ": " << numFragsInVersion << endl;
                cerr << "totalNumFrags " << totalNumFrags << endl;
                exit(1);
            }
            for (int j = 0; j < numFragsInVersion; ++j)
            {
                this->bound_buf[totalNumFrags + j] = (int) offsetsAllVersions[totalNumFrags + j];
            }
            totalNumFrags += numFragsInVersion;
        }
    }

    /*

        We need to set the same variables as Jinru's fragment (see general_partition.h)

        Example of versionSizes, the size of each version. This means that version 0 contains 50 words, version 1 contains 52 words, etc.
        versionSizes = [50, 52, 55, 49, 55]

        int vid: versionID (0, 1, 2, ...)
        int* refer: the contents of the current version (what I call wordIDs)
        int len: the length of the current version (wordIDs.size())
        int* mhbuf: hash stuff that I don't need
        int* hbuf: hash stuff that I don't need
        iv* root: passed as &trees[i] (inverted list but why is it called root?)
        int* w_size: window sizes
        int wlen: size of w_size

        Local var definitions:
            w_size: window sizes
            bound_buf: the partition boundary buffer
            total_level: the amount of iterations he's willing to optimize for, 
                aka the # of times he calls the cutting alg
            md5_buf: a buffer for storing fragment hashes?
            type p (posting?)
            infos: an array of finfo
            block_info2: an array of blocks
            block_num: Number of fragments

        Objectives
            Set bound_buf (same as offsetsAllVersions)
            Set root (object type: iv, see iv.h): Looks like inverted list!
            Call trees[v].complete();

    */
    int fragment(vector<vector<unsigned> >& versions, iv* trees, 
        float fragmentationCoefficient, unsigned minFragSize, unsigned method)
    {
        if (versions.size() == 0)
        {
            return 0;
        }
        unsigned hh, lh;
        int ins;
        int block_num = 0; // Replacing this with numFragsInVersion
        int error;
        int ep;

        int merge_size = 1;
        
        unsigned* versionPartitionSizes = new unsigned[versions.size()];
        unsigned* offsetsAllVersions = 
            new unsigned[versions.size() * MAX_NUM_FRAGMENTS_PER_VERSION];

        // Initialize to a known invalid value
        for (size_t i = 0; i < versions.size(); ++i)
        {
            versionPartitionSizes[i] = MAX_NUM_FRAGMENTS_PER_VERSION + 1;
            // TODO optional: init offsetsAllVersions to 0
        }

        // Sets this->bound_buf and versionPartitionSizes
        this->cutAllVersions(versions,
            offsetsAllVersions, versionPartitionSizes,
            fragmentationCoefficient, minFragSize, method);
        
        unsigned totalCountFragments(0);
        for (int v = 0; v < versions.size(); ++v)
        {
            unsigned numFragsInVersion = versionPartitionSizes[v];
            if (numFragsInVersion == 0)
            {
                cerr << "Error: Number of fragments in version " << v << " is 0." << endl;
                return 0;
            }

            iv* root = &trees[v];

            for (int j = 0; j < numFragsInVersion - 1; j++)
            {
                // set blocks (aka fragments) based on the offsets from cut algorithm
                int currOffset = bound_buf[totalCountFragments + j];
                int nextOffset = bound_buf[totalCountFragments + j + 1];
                block_info2[j].start = currOffset;
                block_info2[j].end = nextOffset;
            }

            // HA! Same thing as I do for the last fragment, use the version size!
            // Anyway I've modified this now that we're doing all versions at once
            block_info2[numFragsInVersion - 1].start = bound_buf[numFragsInVersion - 1];
            block_info2[numFragsInVersion - 1].end = versionSizes[v]; // length of the version in words

            // Jinru's frag optimization, right? -YK
            int counts = 0;
            while (counts < 1) // counts < total_level
            {
                for (int j = 0; j < numFragsInVersion; j++)
                {
                    for (int l = block_info2[j].start; l < block_info2[j].end; l++)
                    {
                        md5_buf[l - bound_buf[j]] = versions[v][l];
                    }

                    ep = block_info2[j].end;

                    md5_init(&state);
                    md5_append(&state, (const md5_byte_t *) md5_buf, sizeof(unsigned) * (ep - block_info2[j].start));
                    md5_finish(&state, digest);
                    hh = *((unsigned *)digest) ;
                    lh = *((unsigned *)digest + 1) ;
                    ins = lookupHash (lh, hh, h[0]);

                    // Didn't find hash for this fragment
                    if (ins == -1)
                    {
                        // p is a posting type
                        p p1;
                        p1.vid = v;
                        p1.offset = block_info2[j].start;
                        p1.isVoid = false;

                        insertHash(lh, hh, ptr, h[0]);
                        infos[ptr].fid = fID;
                        infos[ptr].size = 0;

                        frag_ptr fptr;
                        fptr.ptr = ptr;
                        fptr.off = infos[ptr].size;

                        p1.nptr = root->insert(fptr, block_info2[j].start, ep);

                        // what is nptr?
                        if (p1.nptr != -1)
                        {
                            infos[ptr].len = block_info2[j].end - block_info2[j].start;
                            if (infos[ptr].len < 0)
                            {
                                printf("we have an error!%d\tprev bound:%d\tnow bound:%d\t%d\n", block_num, bound_buf[j], bound_buf[j+1], j);
                                return -1;
                            }
                            // infos looks like an inverted list
                            infos[ptr].items.push_back(p1);
                            infos[ptr].size++;
                            

                            // Move to the next list?
                            ptr++;
                            if (ptr >= MAXDIS)
                            {
                                // This error message sucks. ptr looks like the number of inverted lists
                                printf("too many fragment !\n");
                                exit(0);
                            }
                            fID++;
                        }
                    }
                    else // has sharing block (or "we've seen this fragment before," as I would call it -YK)
                    {
                        frag_ptr fptr;
                        fptr.ptr = ins;
                        fptr.off = infos[ins].size;
                        p p2;
                        p2.vid = v;
                        p2.offset = block_info2[j].start;
                        p2.isVoid = false;

                        // Same as the call above, it will work once we set root properly (I sincerely hope -YK)
                        p2.nptr = root->insert(fptr, block_info2[j].start, ep);

                        if (p2.nptr != -1)
                        {
                            infos[ins].items.push_back(p2);
                            infos[ins].size++;
                        }
                    }
                }
                counts++;
            }
            totalCountFragments += numFragsInVersion;
            root->complete();
        }

        delete [] versionPartitionSizes;
        delete [] offsetsAllVersions;

        return totalCountFragments;
    }


    int init()
    {
        add_list_len = 0;
        flens_count = 0;
        block_info_ptr = 0;
        ptr = 0;
        fID = 0;

        if (h[0] != NULL)
        {
            destroyHash(h[0]);
        }

        h[0] = initHash();
        return 0;
    }

    // Thomas Wang's 32 bit Mix Function
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
        for (int i = 0; i< ptr; i++)
        {
            infos[i].size = 0;
            infos[i].items.clear();
        }
        ptr = 0;
        return 0;
    }

};

#endif /* PARTITION_H_ */
