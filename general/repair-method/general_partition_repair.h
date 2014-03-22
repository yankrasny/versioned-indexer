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
#include "../heap.h"
#include "../md5.h"
#include "../mhash.h"
#include "../vb.h"

// Integration with Repair Partitioning code
#include "../Repair-Partitioning/prototype/RepairPartitioningPrototype.h"

#define MAX_NUM_BASE_FRAGMENTS 200000
#define MAX_NUM_FRAGMENTS 20000000
#define MAX_NUM_WORDS_PER_DOC 500000

using namespace std;

struct FragmentApplication
{
    int vid;
    int offsetInVersion;
    int nodeId; // does this refer to nID in iv.h?
    bool isVoid;
};

// Looks like my FragInfo -YK
struct FragmentInfo
{
    // the fragment id
    int fid;

    // length of the fragment in words
    int length; 
    
    // i think this is the number of applications, but why do you need that if we can get it from the vector?
    int numApplications;

    double score;
    vector<FragmentApplication> applications;
};

// Inverted list class
#include "iv_repair.h"

struct BaseFragment
{
    int start;
    int end;
};

struct PCompare : public binary_function<FragIndexes, FragIndexes, bool>
{
    inline bool operator() (const FragIndexes& f1, const FragIndexes& f2)
    {
        if (f1.indexInFragArray == f2.indexInFragArray)
            return f1.indexInApplicationArray < f2.indexInApplicationArray;
        else
            return f1.indexInFragArray < f2.indexInFragArray;
    }
} comparep;

struct posting
{
    unsigned wid; // word ID
    unsigned vid; // version ID
    unsigned pos; // position? wtf man...
};

class GeneralPartitionAlgorithm
{
private:

    int fID; // the current fragment ID, gets incremented every time we create a fragment
    
    heap myheap; // the heap we use to rank fragments
    
    // md5 vars
    static const unsigned W_SET = 4;
    md5_state_t state;
    md5_byte_t digest[16];
    hStruct* h[W_SET];

    FILE* ffrag; // the file to write meta inforation
    FILE* fbase_frag; // TODO not yet sure what this file is for
    
    unsigned* maxid; // TODO
    unsigned* currid; // TODO current ID of what? is this an array?
    // int* bound_buf; // the partition boundary buffer

    

    // int total_level; // TODO where is this initialized?

    int load_global()
    {
        FILE* f = fopen("./../maxid", "rb");
        int size;
        fread(&size, sizeof(unsigned), 1, f);
        maxid = new unsigned[size];
        currid = new unsigned[size];
        fread(maxid, sizeof(unsigned), size, f);
        fclose(f);

        // unsigned overallMax = 0;

        currid[0] = 0;
        for (int i = 1; i < size; ++i)
        {
            // if (maxid[i] > overallMax) {
            //     overallMax = maxid[i];
            // }
            currid[i] = currid[i-1] + maxid[i-1];
        }

        // cerr << overallMax << endl;
        // exit(1);

    }

public:


    vector<BaseFragment> baseFragments;

    // I think these are the indexes in the heap of good fragments
    // That makes no sense, why would you store heap indexes?
    unsigned* selectedFragIndexes; 
    
    FragIndexes* fbuf; // TODO what is a FragIndexes?
    unsigned char* varbyteBuffer;
    unsigned* md5_buf; // TODO
    posting* add_list;

    FragmentInfo* fragments; // an array of FragmentInfo, I think this is the general fragment pool
    int fragments_count; // size of fragments

    FragmentInfo* fragments2; // an array of FragmentInfo, I think this is a subset of fragments (really uncertain about that)

    
    int block_info_ptr;
    int add_list_len;
    unsigned* flens;
    unsigned flens_count;
    unsigned base_frag;
    

    GeneralPartitionAlgorithm(int para = 0, int maxblock = 0, int blayer = 0, int pTotal = 0) : myheap(MAX_NUM_FRAGMENTS)
    {
        baseFragments = vector<BaseFragment>();

        selectedFragIndexes = new unsigned[MAX_NUM_FRAGMENTS];
        fbuf = new FragIndexes[5000000];        
        varbyteBuffer = new unsigned char[50000000];
        md5_buf = new unsigned[5000000];
        add_list = new posting[1000000];
        fragments = new FragmentInfo[5000000];
        fragments2 = new FragmentInfo[5000000];
        flens = new unsigned[5000000];
        
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

    ~GeneralPartitionAlgorithm()
    {
        delete [] selectedFragIndexes;
        delete [] fbuf;
        delete [] varbyteBuffer;
        delete [] md5_buf;
        delete [] add_list;
        delete [] fragments;
        delete [] fragments2;
        delete [] flens;

        fclose(ffrag);
        fclose(fbase_frag);
    }


    int dump_frag()
    {
        for (int i = 0; i < fragments_count; i++)
        {
            fragments[i].numApplications = 0;
            fragments[i].applications.clear();
        }
        fragments_count = 0;
        return 0;
    }


    // Why not just put this code in the constructor?
    int init()
    {
        add_list_len = 0;
        flens_count = 0;
        block_info_ptr = 0;
        fragments_count = 0;
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


    /*
    
        Scores the fragments and adds them to a heap
        The heap entries have score and ptr, which I think means indexInHeap

        This should be called addFragmentsToHeap

    */
    void addFragmentsToHeap(double wsize)
    {
        myheap.init();
        for (int i = 0; i < fragments_count; i++)
        {
            fragments[i].numApplications = fragments[i].applications.size();
        }

        for (int i = 0; i < fragments_count; i++)
        {
            for (auto it = fragments[i].applications.begin(); it != fragments[i].applications.end(); it++)
            {
                it->isVoid = false;
            }

            // TODO replace wsize with one of your params
            fragments[i].score = static_cast<double>(fragments[i].length * fragments[i].numApplications) / (1 + fragments[i].length + wsize * (1 + fragments[i].numApplications)); 
            hpost heapEntry;
            heapEntry.ptr = i;
            heapEntry.score = fragments[i].score;
            myheap.push(heapEntry);
        }
    }

    int compact(FragIndexes* fragIndexes, int len)
    {
        int ptr1 = 0;
        int ptr2 = 1;
        while (ptr2 < len)
        {
            while (ptr2 < len && fragIndexes[ptr2] == fragIndexes[ptr1])
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
                fragIndexes[ptr1] = fragIndexes[ptr2];
                ptr2++;
            }
        }
        return ptr1 + 1;
    }

    /*
        Using the heap of fragments, select the best ones and add them to selectedFragIndexes
    */
    int numSelectedFrags;
    void selectGoodFragments(iv* currentInvertedList, double wsize)
    {
        numSelectedFrags = 0;
        while (myheap.size() > 0)
        {
            hpost heapEntry = myheap.top();
            selectedFragIndexes[numSelectedFrags++] = heapEntry.ptr;
            myheap.pop();

            /*
                TODO finish explanation
                iterate over the current fragment's applications
                if the application is valid, then add it to fragments2?
            */
            int idx = heapEntry.ptr;
            int cptr = 0;
            for (auto it = fragments[idx].applications.begin(); it != fragments[idx].applications.end(); ++it)
            {
                if (it->isVoid == false)
                {
                    fragments2[idx].applications.push_back(*it);
                    int len = currentInvertedList[it->vid].find_conflict(it->nodeId, fragments[idx].applications, heapEntry.ptr, &fbuf[cptr]);
                    cptr += len;
                }
            }
            fragments2[idx].numApplications = fragments2[idx].applications.size();
            fragments2[idx].length = fragments[idx].length;

            /*
                It looks like we're handling fragment conflicts here
                If there are some conflicts, then go through them and update scores and other vars for the fragments involved
            */
            if (cptr > 0)
            {
                sort(fbuf, fbuf + cptr, comparep);
                cptr = compact(fbuf, cptr);
                for (int i = 0; i < cptr; i++)
                {
                    int idx = fbuf[i].indexInFragArray;
                    if (i > 0 && fbuf[i].indexInFragArray != fbuf[i-1].indexInFragArray)
                    {
                        int idx2 = fbuf[i-1].indexInFragArray;
                        if (fragments[idx2].numApplications > 0)
                        {
                            fragments[idx2].score = fragments[idx2].length * fragments[idx2].numApplications / (1 + fragments[idx2].length + wsize * (1 + fragments[idx2].numApplications));
                            myheap.UpdateKey(idx2, fragments[idx2].score);
                        }
                    }
                    if (fragments[idx].applications.at(fbuf[i].indexInApplicationArray).isVoid == false)
                    {
                        fragments[idx].numApplications--;
                        fragments[idx].applications.at(fbuf[i].indexInApplicationArray).isVoid = true;
                        if (fragments[idx].numApplications == 0)
                            myheap.deleteKey(idx);
                    }
                }

                idx = fbuf[cptr - 1].indexInFragArray;
                if (fragments[idx].numApplications > 0)
                {
                    fragments[idx].score = fragments[idx].length * fragments[idx].numApplications / (1 + fragments[idx].length + wsize * (1 + fragments[idx].numApplications));
                    myheap.UpdateKey(idx, fragments[idx].score);
                }
            }
        }
    }

    /*

        TODO finish description

        Description:
            Essentially the point here is to populate add_list with postings
            I think this function has another goal, something to do with varbyteBuffer
                There's an fwrite of that varbyte content, need to understand that better

        Notes:

            What's the relationship between fragments and fragments2?
                Is it anything to do with overlapping fragments?

    */
    void populatePostings(vector<vector<unsigned> >& versions, int docId)
    {
        add_list_len = 0;
        unsigned char* varbyteBufferPtr = varbyteBuffer;

        // Iterate over selectedFragIndexes
        for (int i = 0; i < numSelectedFrags; i++)
        {
            int myidx = selectedFragIndexes[i];
            FragmentApplication currFragApp = fragments2[myidx].applications.at(0);
            int version = currFragApp.vid;
            int off = currFragApp.offsetInVersion;
            int len = fragments2[myidx].length;
            int size = fragments2[myidx].numApplications;

            // Iterate over ... TODO
            for (int j = off; j < off + len; j++)
            {
                // add_list looks like the output variable here
                add_list[add_list_len].vid = i + base_frag;
                add_list[add_list_len].pos = j - off;
                add_list[add_list_len].wid = versions[version][j];
                ++add_list_len;
            }

            /*
                TODO What does this code do? We're encoding something and then
                putting it into a file called frag_%d.info (see the end of this function)

            */
            VBYTE_ENCODE(varbyteBufferPtr, len);            
            VBYTE_ENCODE(varbyteBufferPtr, size);

            for (auto it = fragments2[myidx].applications.begin(); it != fragments2[myidx].applications.end(); it++)
            {
                int a = (*it).vid + currid[docId];
                int b = (*it).offsetInVersion;
                VBYTE_ENCODE(varbyteBufferPtr, a);
                VBYTE_ENCODE(varbyteBufferPtr, b);
            }

            fragments2[myidx].applications.clear();
        }

        // writing one int for each version, line delimited
        // wtf? base_frag doesn't change, we're just writing the same thing over and over...
        for (int i = 0; i < versions.size(); i++)
        {
            fprintf(fbase_frag, "%d\n", base_frag);
        }

        // Was this line meant to be inside the loop?
        base_frag += numSelectedFrags;

        // ok, now we're writing the varbyte encoded stuff somewhere, might be important
        fwrite(varbyteBuffer, sizeof(unsigned char), varbyteBufferPtr - varbyteBuffer, ffrag);

        // clear the vector<p> in each fragment
        for (int i = 0; i < fragments_count; i++)
        {
            fragments[i].numApplications = 0;
            fragments[i].applications.clear();
        }
    }

    /*
        I think this function populates one TradeoffRecord
        This function populates the Tradeoff Table (between meta data size and index size)
        Check diff2_repair for the definition of TradeoffRecord
    */
    void writeTradeoffRecord(vector<vector<unsigned> >& versions, int docId, TradeoffRecord* tradeoffRecord)
    {
        add_list_len = 0;
        int totalNumApplications = 0;
        int totalNumPostings = 0;

        // Each iteration of this loop processes one fragment
        for (int i = 0; i < numSelectedFrags; i++)
        {
            int myidx = selectedFragIndexes[i];
            totalNumApplications += fragments2[myidx].applications.size();
            totalNumPostings += fragments2[myidx].length;

            // Each iteration of this loop processes a fragment application
            // TODO I have no idea what this is supposed to do, it looks like nonsense
            // for (auto it = fragments2[myidx].applications.begin(); it != fragments2[myidx].applications.end(); it++)
            // {
            //     int version = it->vid;
            //     int off = it->offset;
            //     int len = fragments2[myidx].length;
            //     if (!it->isVoid)
            //     {
            //         for (int j = off; j < off + len; j++)
            //         {
            //             if (versions[version][j] > 0)
            //             {
            //                 FILE* ferror = fopen("ERROR", "w");
            //                 fprintf(ferror, "Error... docId: %d, i: %d, j: %d\n", docId, i, j);
            //                 fclose(ferror);
            //                 exit(0);
            //             }
            //             versions[version][j] = 1;
            //         }
            //     }
            // }
            // fragments2[myidx].applications.clear();
        }

        tradeoffRecord->numDistinctFrags = numSelectedFrags;
        tradeoffRecord->numFragApplications = totalNumApplications;
        tradeoffRecord->numPostings = totalNumPostings;

        // The following is debug code, doesn't affect the output
        // for (int i = 0; i < versions.size(); i++)
        // {
        //     for (int j = 0; j < versions[i].size(); j++)
        //     {
        //         if (versions[i][j] < 0)
        //         {
        //             FILE* ferror = fopen("ERRORS", "w");
        //             fprintf(ferror, "Error at doc: %d, version: %d, position: %d\n", docId, i, j);
        //             fclose(ferror);
        //             cerr << "See file ERRORS" << endl;
        //             exit(0);
        //         }
        //     }
        // }
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
            unsigned numFragsInVersion = versionPartitionSizes[i];
            if (numFragsInVersion > MAX_NUM_FRAGMENTS_PER_VERSION)
            {
                cerr << "numFragsInVersion " << i << ": " << numFragsInVersion << endl;
                cerr << "totalNumFrags " << totalNumFrags << endl;
                exit(1);
            }
            // TODO remove this senseless duplication
            // for (int j = 0; j < numFragsInVersion; ++j)
            // {
            //     this->bound_buf[totalNumFrags + j] = (int) offsetsAllVersions[totalNumFrags + j];
            // }
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
            fragments: an array of FragmentInfo
            baseFragments: an array of blocks
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
        int fragIndex;
        int endCurrentFrag;

        unsigned* versionPartitionSizes = new unsigned[versions.size()];
        unsigned* offsetsAllVersions = new unsigned[versions.size() * MAX_NUM_FRAGMENTS_PER_VERSION];

        // Initialize to a known invalid value
        for (size_t i = 0; i < versions.size(); ++i)
        {
            versionPartitionSizes[i] = MAX_NUM_FRAGMENTS_PER_VERSION + 1;
            // TODO optional: init offsetsAllVersions to 0
        }

        this->cutAllVersions(versions,
            offsetsAllVersions, versionPartitionSizes,
            fragmentationCoefficient, minFragSize, method);
        
        unsigned totalCountFragments(0);
        unsigned totalCountOffsets(0);
        for (int v = 0; v < versions.size(); ++v)
        {
            assert(versionPartitionSizes[v] > 1);
            unsigned numFragsInVersion = versionPartitionSizes[v] - 1;            
            iv* currentInvertedList = &invertedLists[v];
            for (int j = 0; j < numFragsInVersion; ++j)
            {
                BaseFragment currFrag;
                currFrag.start = (int)offsetsAllVersions[totalCountOffsets + j];
                currFrag.end = (int)offsetsAllVersions[totalCountOffsets + j + 1];
                baseFragments.push_back(currFrag);
            }

            /* At this point, baseFragments contains the fragments from repair partitioning */

            // Jinru's frag optimization, right? -YK
            int counts = 0;
            while (counts < 1) // counts < total_level
            {
                // TODO check these bounds for j
                for (int j = 0; j < numFragsInVersion; j++)
                {
                    for (int k = baseFragments[j].start; k < baseFragments[j].end; k++)
                    {
                        md5_buf[k - baseFragments[j].start] = versions[v][k];
                    }

                    endCurrentFrag = baseFragments[j].end;

                    md5_init(&state);
                    md5_append(&state, (const md5_byte_t *) md5_buf, sizeof(unsigned) * (endCurrentFrag - baseFragments[j].start));
                    md5_finish(&state, digest);
                    hh = *((unsigned *)digest) ;
                    lh = *((unsigned *)digest + 1) ;
                    fragIndex = lookupHash(lh, hh, h[0]);

                    // First time we've seen this fragment
                    if (fragIndex == -1)
                    {
                        FragmentApplication p1;
                        p1.vid = v;
                        p1.offsetInVersion = baseFragments[j].start;
                        p1.isVoid = false;

                        insertHash(lh, hh, fragments_count, h[0]);
                        fragments[fragments_count].fid = fID;
                        fragments[fragments_count].numApplications = 0;

                        FragIndexes fptr;
                        fptr.indexInFragArray = fragments_count;
                        fptr.indexInApplicationArray = fragments[fragments_count].numApplications;

                        p1.nodeId = currentInvertedList->insert(fptr, baseFragments[j].start, endCurrentFrag);

                        // what is nodeId?
                        if (p1.nodeId != -1)
                        {
                            fragments[fragments_count].length = baseFragments[j].end - baseFragments[j].start;
                            if (fragments[fragments_count].length < 0)
                            {
                                printf("Error... block number: %d, prev bound: %d, current bound: %d\n", j, offsetsAllVersions[j], offsetsAllVersions[j + 1]);
                                return -1;
                            }
                            fragments[fragments_count].applications.push_back(p1);
                            fragments[fragments_count].numApplications++;
                            fragments_count++;
                            if (fragments_count >= MAX_NUM_FRAGMENTS)
                            {
                                // This error message sucks. fragments_count looks like the number of inverted lists
                                printf("Too many fragments!\n");
                                exit(0);
                            }
                            fID++;
                        }
                    }
                    else // We've seen this fragment before
                    {
                        FragIndexes fptr;
                        fptr.indexInFragArray = fragIndex;
                        fptr.indexInApplicationArray = fragments[fragIndex].numApplications;
                        
                        FragmentApplication p2;
                        p2.vid = v;
                        p2.offsetInVersion = baseFragments[j].start;
                        p2.isVoid = false;

                        // Same as the call above, it will work once we set currentInvertedList properly (I sincerely hope -YK)
                        p2.nodeId = currentInvertedList->insert(fptr, baseFragments[j].start, endCurrentFrag);

                        if (p2.nodeId != -1)
                        {
                            fragments[fragIndex].applications.push_back(p2);
                            fragments[fragIndex].numApplications++;
                        }
                    }
                }
                counts++;
            }
            totalCountFragments += numFragsInVersion;
            totalCountOffsets += versionPartitionSizes[v];
            currentInvertedList->complete();
        }

        delete [] versionPartitionSizes;
        delete [] offsetsAllVersions;

        return totalCountFragments;
    }

};

#endif /* PARTITION_H_ */