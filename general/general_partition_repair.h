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
#include <list>
#include <vector>
#include <cmath>
#include <algorithm>
#include "heap.h"
#include "winnow.h"
#include "md5.h"
#include "mhash.h"
#include "vb.h"

// Integration with Repair Partitioning code
#include "Repair-Partitioning/prototype/RepairPartitioningPrototype.h"

#define MAXDIS 20000000
#define MAX_NUM_WORDS_PER_DOC 200000

using namespace std;

// Doesn't seem to be used anywhere, will probably remove
struct node;
 
// I think this is a fragment application -YK
struct p
{
    int vid;
    int offset;
    int nptr;
    bool isVoid;
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

// Inverted list class
#include "iv.h"

// lowerpartition.h contains different cutting algorithms, 99% we don't need this
#include "lowerpartition.h"

struct PCompare : public binary_function<frag_ptr, frag_ptr, bool>
{
    inline bool operator() (const frag_ptr& f1, const frag_ptr& f2)
    {
        if (f1.ptr == f2.ptr)
            return f1.off < f2.off;
        else
            return f1.ptr < f2.ptr;
    }
} comparep;

struct posting
{
    unsigned wid; // wordID
    unsigned vid; // version ID?
    unsigned pos; // position? posting ID? wtf man...
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

    int ptr; // size of fragments???
    int fID;
    heap myheap;
    static const unsigned W_SET = 4;
    md5_state_t state;
    md5_byte_t digest[16];
    FILE* ffrag; // the file to write meta inforation
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
    finfo*             fragments;       // an array of finfo
    finfo*             fragments2;      // an array of finfo
    frag_ptr*          fbuf;            // an array of frag_ptr
    int**              refer_ptrs;      // versions (so each inner array is like wordIDs for one version)
    unsigned char**    refer_ptrs2;     // versions (so each inner array is like wordIDs for one version)
    unsigned*          id_trans;        // 
    unsigned*          selected_buf;    //
    unsigned char*     tpbuf;           // 

    // int total_level; // TODO where is this initialized?


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
            fragments[i].size = 0;
            fragments[i].items.clear();
        }
        ptr = 0;
        return 0;
    }


    // Why not just put this code in the constructor?
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


    // What is this doing here?
    char fn[256];
    unsigned char* vbuf;


    partitions(int para = 0, int maxblock = 0, int blayer = 0, int pTotal = 0) : myheap(MAXDIS)
    {
        selected_buf = new unsigned[MAXDIS];
        fbuf = new frag_ptr[5000000];
        refer_ptrs = new int*[20000];
        refer_ptrs2 = new unsigned char*[20000];
        vbuf = new unsigned char[50000000];
        bound_buf = new int[5000000];
        md5_buf = new unsigned[5000000];
        add_list = new posting[1000000];
        fragments = new finfo[5000000];
        fragments2 = new finfo[5000000];
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

    ~partitions()
    {
        delete [] selected_buf;

        // TODO garbage collect all the other buffers

        delete[] fbuf;
        fclose(ffrag);
        fclose(fbase_frag);
    }

    void completeCount(double wsize)
    {
        myheap.init();
        for (int i = 0; i < ptr; i++)
        {
            fragments[i].size = fragments[i].items.size();
        }

        for (int i = 0; i < ptr; i++)
        {
            for (vector<p>::iterator its = fragments[i].items.begin(); its != fragments[i].items.end(); its++)
            {
                its->isVoid = false;
            }

            fragments[i].score = static_cast<double>(fragments[i].len * fragments[i].size) / (1 + fragments[i].len + wsize * (1 + fragments[i].size)); 
            hpost ite;
            ite.ptr = i;
            ite.score = fragments[i].score;
            myheap.push(ite);
        }
    }

    int compact(frag_ptr* fi, int len)
    {
        int ptr1 = 0;
        int ptr2 = 1;
        while (ptr2 < len)
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
    void select(iv* currentInvertedList, double wsize)
    {
        select_ptr = 0;
        while (myheap.size() > 0)
        {
            hpost po = myheap.top();
            selected_buf[select_ptr++] = po.ptr;
            myheap.pop();
            int idx = po.ptr;
            int cptr = 0;
            for (vector<p>::iterator its = fragments[idx].items.begin(); its != fragments[idx].items.end(); its++)
            {
                if ( its->isVoid == false)
                {
                    fragments2[idx].items.push_back(*its);
                    int len = currentInvertedList[its->vid].find_conflict(its->nptr, fragments[idx].items, po.ptr, &fbuf[cptr]);
                    cptr += len;
                }
            }
            fragments2[idx].size = fragments2[idx].items.size();
            fragments2[idx].len = fragments[idx].len;

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
                        if (fragments[idx2].size > 0)
                        {
                            fragments[idx2].score = fragments[idx2].len*fragments[idx2].size /(1+fragments[idx2].len+wsize*(1+fragments[idx2].size));
                            myheap.UpdateKey(idx2, fragments[idx2].score);
                        }
                    }
                    if (fragments[idx].items.at(fbuf[i].off).isVoid == false)
                    {
                        fragments[idx].size--;
                        fragments[idx].items.at(fbuf[i].off).isVoid = true;
                        if (fragments[idx].size == 0)
                            myheap.deleteKey(idx);
                    }
                }

                idx = fbuf[cptr - 1].ptr;
                if (fragments[idx].size > 0)
                {
                    fragments[idx].score = fragments[idx].len * fragments[idx].size / (1 + fragments[idx].len + wsize * (1 + fragments[idx].size));
                    myheap.UpdateKey(idx, fragments[idx].score);
                }
            }
        }
    }

    /*

        Add a description
        Ask questions, and eventually you'll get it

        Description:
            Essentially the point here is to populate add_list with postings
            I think this function has another goal, something to do with vbuf
                There's an fwrite of that varbyte content, need to understand that better

        Notes:        
            refer = buf, so that's docContent
            refer_ptrs is like my versions variable

            What's the relationship between fragments and fragments2?

    */
    void finish3(vector<vector<unsigned> >& versions, int docId)
    {
        add_list_len = 0;
        int new_fid = 0;
        unsigned char* vbuf_ptr = vbuf;

        // Iterate over selected_buf (TODO what's selected_buf?)
        for (int i = 0; i < select_ptr; i++)
        {
            int myidx = selected_buf[i];
            p po = fragments2[myidx].items.at(0);
            int version = po.vid;
            int off = po.offset;
            int len = fragments2[myidx].len;

            // Iterate over ... TODO
            for (int j = off; j < off + len; j++)
            {
                // add_list looks like the output variable here
                add_list[add_list_len].vid = i + base_frag;
                add_list[add_list_len].pos = j - off;
                add_list[add_list_len].wid = versions[version][j];
                ++add_list_len;
            }

            // TODO What does this code do? We're encoding something and then not putting it anywhere...
            VBYTE_ENCODE(vbuf_ptr, len);
            int size = fragments2[myidx].size; //items.size();
            VBYTE_ENCODE(vbuf_ptr, size);
            for (vector<p>::iterator it = fragments2[myidx].items.begin(); it != fragments2[myidx].items.end(); it++)
            {
                int a = (*it).vid + currid[docId];
                int b = (*it).offset;
                VBYTE_ENCODE(vbuf_ptr, a);
                VBYTE_ENCODE(vbuf_ptr, b);
            }

            fragments2[myidx].items.clear();
        }

        // writing one int for each version, line delimited
        // wtf? base_frag doesnn't change, we're just writing the same thing over and over...
        for (int i = 0; i < versions.size(); i++)
        {
            fprintf(fbase_frag, "%d\n", base_frag);
        }

        // Was this line meant to be inside the loop?
        base_frag += select_ptr;

        // ok, now we're writing the varbyte encoded stuff somewhere, might be important
        fwrite(vbuf, sizeof(unsigned char), vbuf_ptr - vbuf, ffrag);

        // clear the vector<p> in each fragment
        for (int i = 0; i < ptr; i++)
        {
            fragments[i].size = 0;
            fragments[i].items.clear();
        }
    }

    void PushBlockInfo(vector<vector<unsigned> >& versions, int id, binfo* bis)
    {
        if (tpbuf == NULL) {
            tpbuf = new unsigned char[MAXSIZE];
        }

        int start = 0;
        for (int i = 0; i < versions.size(); i++)
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
            total += fragments2[myidx].items.size();
            total_post += fragments2[myidx].len;

            for (vector<p>::iterator its = fragments2[myidx].items.begin(); its != fragments2[myidx].items.end(); its++)
            {
                int version = its->vid;
                int off = its->offset;
                int len = fragments2[myidx].len;
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
            fragments2[myidx].items.clear();
        }

        bis->total = total;
        bis->post = total_post;

        for (int i = 0; i < versions.size(); i++)
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
        iv* currentInvertedList: passed as &invertedLists[i]
        int* w_size: window sizes
        int wlen: size of w_size

        Local var definitions:
            w_size: window sizes
            bound_buf: the partition boundary buffer
            total_level: the amount of iterations he's willing to optimize for, 
                aka the # of times he calls the cutting alg
            md5_buf: a buffer for storing fragment hashes?
            type p (posting?)
            fragments: an array of finfo
            block_info2: an array of blocks
            block_num: Number of fragments

        Objectives
            Set bound_buf (same as offsetsAllVersions)
            Set currentInvertedList (object type: iv, see iv.h): Looks like inverted list!
            Call invertedLists[v].complete();

    */
    int fragment(vector<vector<unsigned> >& versions, iv* invertedLists, 
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

            iv* currentInvertedList = &invertedLists[v];

            for (int j = 0; j < numFragsInVersion - 1; j++)
            {
                // set blocks (aka fragments) based on the offsets from cut algorithm
                int currOffset = bound_buf[totalCountFragments + j];
                int nextOffset = bound_buf[totalCountFragments + j + 1];
                block_info2[j].start = currOffset;
                block_info2[j].end = nextOffset;
            }

            // Last fragment ends with the version size TODO check this
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
                        fragments[ptr].fid = fID;
                        fragments[ptr].size = 0;

                        frag_ptr fptr;
                        fptr.ptr = ptr;
                        fptr.off = fragments[ptr].size;

                        p1.nptr = currentInvertedList->insert(fptr, block_info2[j].start, ep);

                        // what is nptr?
                        if (p1.nptr != -1)
                        {
                            fragments[ptr].len = block_info2[j].end - block_info2[j].start;
                            if (fragments[ptr].len < 0)
                            {
                                printf("we have an error!%d\tprev bound:%d\tnow bound:%d\t%d\n", block_num, bound_buf[j], bound_buf[j+1], j);
                                return -1;
                            }
                            // fragments looks like an inverted list
                            fragments[ptr].items.push_back(p1);
                            fragments[ptr].size++;
                            

                            // Move to the next list?
                            ptr++;
                            if (ptr >= MAXDIS)
                            {
                                // This error message sucks. ptr looks like the number of inverted lists
                                printf("Too many fragments!\n");
                                exit(0);
                            }
                            fID++;
                        }
                    }
                    else // has sharing block (or "we've seen this fragment before," as I would call it -YK)
                    {
                        frag_ptr fptr;
                        fptr.ptr = ins;
                        fptr.off = fragments[ins].size;
                        p p2;
                        p2.vid = v;
                        p2.offset = block_info2[j].start;
                        p2.isVoid = false;

                        // Same as the call above, it will work once we set currentInvertedList properly (I sincerely hope -YK)
                        p2.nptr = currentInvertedList->insert(fptr, block_info2[j].start, ep);

                        if (p2.nptr != -1)
                        {
                            fragments[ins].items.push_back(p2);
                            fragments[ins].size++;
                        }
                    }
                }
                counts++;
            }
            totalCountFragments += numFragsInVersion;
            currentInvertedList->complete();
        }

        delete [] versionPartitionSizes;
        delete [] offsetsAllVersions;

        return totalCountFragments;
    }


};

#endif /* PARTITION_H_ */
