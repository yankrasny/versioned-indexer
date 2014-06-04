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
#include "../Repair-Partitioning/prototype/RepairPartitioningPrototype.h"
using namespace std;

#define MAX_NUM_BASE_FRAGMENTS 200000
#define MAX_NUM_FRAGMENTS 20000000
#define MAX_NUM_WORDS_PER_DOC 1000000


// This is a row in the tradeoff table (metadata vs index size)
struct TradeoffRecord
{
    int numFragApplications; // number of fragment applications
    int numDistinctFrags; // number of distinct fragments
    int numPostings; // total number of postings
    unsigned paramValue; // number of levels we traverse in the repair tree (more = possibly smaller fragments)
};

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
    int fid; // the fragment id
    int length; // length of the fragment in words
    int numApplications; // i think this is the number of applications, but why do you need that if we can get it from the vector?
    double score;
    vector<FragmentApplication> applications;
};

// Inverted list class
#include "iv_repair.h"

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
    unsigned pos; // position in version
};

class GeneralPartitionAlgorithm
{
private:

    int fID; // the current fragment ID, gets incremented every time we create a fragment
    
    heap myheap; // the heap we use to rank fragments

    RepairAlgorithm repairAlg;
    
    // md5 vars
    static const unsigned W_SET = 4;
    md5_state_t state;
    md5_byte_t digest[16];
    hStruct* h[W_SET];

    FILE* ffrag; // the file to write meta inforation
    FILE* fbase_frag; // TODO not yet sure what this file is for
    
    unsigned* maxid; // TODO
    unsigned* currid; // TODO current ID of what? is this an array?

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

        currid[0] = 0;
        for (int i = 1; i < size; ++i)
        {
            currid[i] = currid[i-1] + maxid[i-1];
        }
    }

public:

    // I think these are the indexes in the heap of good fragments
    // That makes no sense, why would you store heap indexes?
    unsigned* selectedFragIndexes; 
    
    FragIndexes* fbuf; // TODO what is a FragIndexes?
    unsigned char* varbyteBuffer;
    unsigned* md5_buf;
    posting* add_list;

    FragmentInfo* fragments; // an array of FragmentInfo, I think this is the general fragment pool
    int fragments_count; // size of fragments

    FragmentInfo* selectedFragments; // an array of FragmentInfo, I think this is a subset of fragments (really uncertain about that)

    int block_info_ptr;
    int add_list_len;
    unsigned* flens;
    unsigned flens_count;
    unsigned base_frag;
    

    /*
        Make sure the allocations here and in init() make sense
            What I mean is, each document needs to do repair again, so
            Does repair clean up after itself properly? Is it enough to just call the constructor again like we do in init()?       
    */
    GeneralPartitionAlgorithm() : myheap(MAX_NUM_FRAGMENTS)
    {
        this->resetCounts();

        char fn[256];
        sprintf(fn, "frag_0.info");
        ffrag = fopen(fn, "wb");
        
        memset(fn, 0, 256);
        
        sprintf(fn, "base_frag_0");
        fbase_frag = fopen(fn, "w");
        load_global();

        selectedFragIndexes = new unsigned[MAX_NUM_FRAGMENTS];
        fbuf = new FragIndexes[5000000];
        varbyteBuffer = new unsigned char[50000000];
        md5_buf = new unsigned[5000000];
        add_list = new posting[1000000];
        fragments = new FragmentInfo[5000000];
        selectedFragments = new FragmentInfo[5000000];
        flens = new unsigned[5000000];
    }

    ~GeneralPartitionAlgorithm()
    {
        delete [] selectedFragIndexes;
        delete [] fbuf;
        delete [] varbyteBuffer;
        delete [] md5_buf;
        delete [] add_list;
        delete [] fragments;
        delete [] selectedFragments;
        delete [] flens;

        fclose(ffrag);
        fclose(fbase_frag);
    }

    void resetBeforeNextRun()
    {
        this->clearFrags();
        // this->resetCounts();
    }

    void clearFrags()
    {
        for (int i = 0; i < fragments_count; i++)
        {
            fragments[i].numApplications = 0;
            fragments[i].applications.clear();
        }

        // TODO selectedFragments?
    }

    void resetCounts()
    {
        base_frag = 0;
        add_list_len = 0;
        flens_count = 0;
        block_info_ptr = 0;
        fragments_count = 0;
        fID = 0;
    }

    void initRepair(vector<vector<unsigned> >& versions)
    {
        this->repairAlg = RepairAlgorithm(versions);
    }

    int init()
    {
        this->resetCounts();

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

    */
    void addBaseFragmentsToHeap(double x)
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

            double fragLength = static_cast<double>(fragments[i].length);
            double numFragApps = static_cast<double>(fragments[i].numApplications);
            double coverage = fragLength * numFragApps;
            double indexCost = 1.0;
            double z = 1.0;
            double metaCost = 1.0 + z * numFragApps;

            fragments[i].score = coverage / (indexCost + x * metaCost);

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
    void selectGoodFragments(iv* currentInvertedList, double x)
    {
        numSelectedFrags = 0;
        hpost heapEntry;
        while (!myheap.IsEmpty())
        {
            heapEntry = myheap.top();
            selectedFragIndexes[numSelectedFrags++] = heapEntry.ptr;
            myheap.pop();

            /*
                iterate over the current fragment's applications
                if the application has conflicts, record them
            */
            int idx = heapEntry.ptr;
            int cptr = 0;
            for (auto it = fragments[idx].applications.begin(); it != fragments[idx].applications.end(); ++it)
            {
                if (it->isVoid == false)
                {
                    // Add the frag application to the list for the current selected fragment object
                    selectedFragments[idx].applications.push_back(*it);
                    int len = currentInvertedList[it->vid].find_conflict(it->nodeId, fragments[idx].applications, heapEntry.ptr, &fbuf[cptr]);
                    cptr += len;
                }
            }

            selectedFragments[idx].numApplications = selectedFragments[idx].applications.size();
            selectedFragments[idx].length = fragments[idx].length;

            /*
                Handle fragment conflicts here
                If the current frag application conflicts with other frags
                    then go through them and update scores and other vars for the fragments involved

                My guesses for var names:
                    cptr is numConflicts
                    fbuf contains conflicting fragment applications
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
                            double fragLength = static_cast<double>(fragments[idx2].length);
                            double numFragApps = static_cast<double>(fragments[idx2].numApplications);
                            double coverage = fragLength * numFragApps;
                            double indexCost = 1.0;
                            double z = 1.0;
                            double metaCost = 1.0 + z * numFragApps;

                            fragments[idx2].score = coverage / (indexCost + x * metaCost);

                            myheap.UpdateKey(idx2, fragments[idx2].score);
                        }
                    }
                    if (fragments[idx].applications.at(fbuf[i].indexInApplicationArray).isVoid == false)
                    {
                        fragments[idx].numApplications--;
                        fragments[idx].applications.at(fbuf[i].indexInApplicationArray).isVoid = true;
                        if (fragments[idx].numApplications == 0)
                        {
                            myheap.deleteKey(idx);
                        }
                    }
                }

                idx = fbuf[cptr - 1].indexInFragArray;
                if (fragments[idx].numApplications > 0)
                {
                    double fragLength = static_cast<double>(fragments[idx].length);
                    double numFragApps = static_cast<double>(fragments[idx].numApplications);
                    double coverage = fragLength * numFragApps;
                    double indexCost = 1.0;
                    double z = 1.0;
                    double metaCost = 1.0 + z * numFragApps;

                    fragments[idx].score = coverage / (indexCost + x * metaCost);

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

    */
    void populatePostings(const vector<vector<unsigned> >& versions, int docId)
    {
        add_list_len = 0;
        unsigned char* varbyteBufferPtr = varbyteBuffer;

        // Iterate over selectedFragIndexes
        for (int i = 0; i < numSelectedFrags; i++)
        {
            int myidx = selectedFragIndexes[i];

            // Grab the first frag application, we can generate all the 
            // postings we need without looking at the other ones
            FragmentApplication currFragApp = selectedFragments[myidx].applications.at(0);
            int version = currFragApp.vid;
            int offsetInVersion = currFragApp.offsetInVersion;
            int len = selectedFragments[myidx].length;
            int numApplications = selectedFragments[myidx].numApplications;

            // Iterate over the indexes in the current frag app
            for (int j = offsetInVersion; j < offsetInVersion + len; j++)
            {
                add_list[add_list_len].vid = i + base_frag;
                add_list[add_list_len].pos = j - offsetInVersion;
                add_list[add_list_len].wid = versions[version][j];
                ++add_list_len;
            }

            /*
                TODO What does this code do? We're encoding something and then
                putting it into a file called frag_%d.info (see the end of this function)

                Well, maybe... just maybe this is the meta data being written and compressed. Who the hell knows?
            */

            VBYTE_ENCODE(varbyteBufferPtr, len);            
            VBYTE_ENCODE(varbyteBufferPtr, numApplications);

            for (auto it = selectedFragments[myidx].applications.begin(); it != selectedFragments[myidx].applications.end(); it++)
            {
                int a = (*it).vid + currid[docId];
                int b = (*it).offsetInVersion;
                VBYTE_ENCODE(varbyteBufferPtr, a);
                VBYTE_ENCODE(varbyteBufferPtr, b);
            }

            selectedFragments[myidx].applications.clear();
        }

        // writing one int for each version, line delimited
        // wtf? base_frag doesn't change, we're just writing the same thing over and over...
        for (int i = 0; i < versions.size(); i++)
        {
            fprintf(fbase_frag, "%d\n", base_frag);
            base_frag += numSelectedFrags;
        }

        // ok, now we're writing the varbyte encoded stuff somewhere, might be important
        fwrite(varbyteBuffer, sizeof(unsigned char), varbyteBufferPtr - varbyteBuffer, ffrag);

        // clear the vector of applications for each fragment
        for (int i = 0; i < fragments_count; i++)
        {
            fragments[i].numApplications = 0;
            fragments[i].applications.clear();
        }
    }

    /*
        This function populates the Tradeoff Table (between meta data size and index size)
        Check diff2_repair for the definition of TradeoffRecord
    */
    void writeTradeoffRecord(const vector<vector<unsigned> >& versions, int docId, TradeoffRecord* tradeoffRecord)
    {
        add_list_len = 0;
        int totalNumApplications = 0;
        int totalNumPostings = 0;

        // Each iteration of this loop processes one fragment
        for (int i = 0; i < numSelectedFrags; i++)
        {
            int myidx = selectedFragIndexes[i];
            totalNumApplications += selectedFragments[myidx].numApplications;
            totalNumPostings += selectedFragments[myidx].length;

            // Each iteration of this loop processes a fragment application
            // TODO I have no idea what this is supposed to do, it looks like some kind of error checking but mostly nonsense
            // for (auto it = selectedFragments[myidx].applications.begin(); it != selectedFragments[myidx].applications.end(); it++)
            // {
            //     int version = it->vid;
            //     int off = it->offset;
            //     int len = selectedFragments[myidx].length;
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
            selectedFragments[myidx].applications.clear();
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

    void getRepairTrees()
    {
        this->repairAlg.doRepair();
    }

    void checkBaseFragments(const BaseFragmentsAllVersions& baseFragmentsAllVersions)
    {
        // Check for max number of frags
        BaseFragment frag;
        BaseFragmentList baseFragList;
        for (auto it = baseFragmentsAllVersions.begin(); it != baseFragmentsAllVersions.end(); ++it) {
            baseFragList = (*it);
            unsigned v = baseFragList.getVersionNum();
            if (baseFragList.size() > MAX_NUM_FRAGMENTS_PER_VERSION)
            {
                cerr << "Version " << v << ", number of candidate frags: " << baseFragList.size() << endl;
                exit(1);
            }
        }
    }

    /*
        1) Partition the repair trees and get base fragments
        2) Add unique fragments to a list (will add to heap in another function)
      
        Note: This gets called a few times with different values of paramValue
        The higher the value of paramValue, the more fragments we generate
    */
    int getBaseFragments(iv* invertedLists, const vector<vector<unsigned> >& versions, unsigned numLevelsDown)
    {
        unsigned hh, lh;
        int fragIndex;
        int startCurrentFrag;
        int endCurrentFrag;

        BaseFragmentsAllVersions baseFragmentsAllVersions;

        // Partition the repair tree
        this->repairAlg.getBaseFragments(baseFragmentsAllVersions, numLevelsDown);

        this->checkBaseFragments(baseFragmentsAllVersions);

        // Hash the fragments in all versions and add the unique ones to a list
        BaseFragmentList baseFragmentsForVersion;
        unsigned v;
        for (auto it = baseFragmentsAllVersions.begin(); it != baseFragmentsAllVersions.end(); ++it) {

            auto baseFragObj = (*it);
            auto baseFragmentsForVersion = baseFragObj.getBaseFragments();
            v = baseFragObj.getVersionNum();

            // TODO rename this? I think it's not an inverted list
            iv* currentInvertedList = &invertedLists[v];

            unsigned j = 0;
            // j iterates over all the base fragments
            for (auto it2 = baseFragmentsForVersion.begin(); it2 != baseFragmentsForVersion.end(); ++it2)
            {
                startCurrentFrag = (*it2).start;
                endCurrentFrag = (*it2).end;
                
                // k iterates over the current base fragment
                for (int k = startCurrentFrag; k < endCurrentFrag; k++)
                {
                    md5_buf[k - startCurrentFrag] = versions[v][k];
                }

                md5_init(&state);
                md5_append(&state, (const md5_byte_t *) md5_buf, sizeof(unsigned) * (endCurrentFrag - startCurrentFrag));
                md5_finish(&state, digest);
                hh = *((unsigned *)digest) ;
                lh = *((unsigned *)digest + 1) ;
                fragIndex = lookupHash(lh, hh, h[0]);

                if (fragIndex == -1) // First time we've seen this fragment
                {
                    FragmentApplication p1;
                    p1.vid = v;
                    p1.offsetInVersion = startCurrentFrag;
                    p1.isVoid = false;

                    insertHash(lh, hh, fragments_count, h[0]);
                    fragments[fragments_count].fid = fID;
                    fragments[fragments_count].numApplications = 0;

                    FragIndexes fptr;
                    fptr.indexInFragArray = fragments_count;
                    fptr.indexInApplicationArray = fragments[fragments_count].numApplications;

                    p1.nodeId = currentInvertedList->insert(fptr, startCurrentFrag, endCurrentFrag);

                    // what is nodeId?
                    if (p1.nodeId != -1)
                    {
                        fragments[fragments_count].length = endCurrentFrag - startCurrentFrag;
                        if (fragments[fragments_count].length < 0)
                        {
                            printf("Error... block number: %d, prev bound: %d, current bound: %d\n", j, 
                                startCurrentFrag, endCurrentFrag);
                            return -1;
                        }
                        fragments[fragments_count].applications.push_back(p1);
                        fragments[fragments_count].numApplications++;
                        fragments_count++;
                        if (fragments_count >= MAX_NUM_FRAGMENTS)
                        {
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
                    p2.offsetInVersion = startCurrentFrag;
                    p2.isVoid = false;

                    // Same as the call above, it will work once we set currentInvertedList properly (I sincerely hope -YK)
                    p2.nodeId = currentInvertedList->insert(fptr, startCurrentFrag, endCurrentFrag);

                    if (p2.nodeId != -1)
                    {
                        fragments[fragIndex].applications.push_back(p2);
                        fragments[fragIndex].numApplications++;
                    }
                }
                ++j;
            }
            currentInvertedList->complete();
        }
        return fragments_count;
    }

};

#endif /* PARTITION_H_ */