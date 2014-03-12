/*
 * diff2_repair.cc
 *
 *  Created on: May 11, 2013
 *  Adapted from diff2.cc by Jinru He    
 *  Author: Yan Krasny
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <iterator>
#include <vector>
#include <fstream>

// I believe this is the maximum size of a document in the whole data set -YK
#define MAXSIZE 81985059

struct binfo
{
    int total;
    int total_dis;
    int post;
    float wsize;
};

struct CompareFunctor
{
    bool operator()(const binfo& l1, const binfo& l2)
    {
        if (l1.total == l2.total)
        {
            return l1.post < l2.post;
        }
        else
        {
            return l1.total < l2.total;
        }
    }
} compareb;

#include "general_partition_repair.h"

partitions<CutDocument2>* partitionAlgorithm;
int dothejob(vector<vector<unsigned> >& versions, int docId, int numVersions, const vector<double>& wsizes2)
{
    char fn[256];
    if (numVersions != 1)
    {
        memset(fn, 0, 256);
        sprintf(fn, "/research/run/jhe/proximity/model/test/%d", docId);
        FILE* f = fopen(fn, "r");

        iv* invertedLists = new iv[numVersions];

        partitionAlgorithm->init();

        // I think wsizes is window sizes, and total is the size of that array
        // TODO these vars should not be hardcoded, if time permits we'll change it
        float fragmentationCoefficient = 1.0;
        float minFragSize = 100;
        unsigned numLevelsDown = 5;
        int numFrags = partitionAlgorithm->fragment(versions, invertedLists, 
            fragmentationCoefficient, minFragSize, numLevelsDown);
        
        if (numFrags == -1)
        {
            printf("Warning: doc %d, too large, skipping...\n", docId);
        }
        else
        {
            printf("Finished sorting!\n");

            // TODO rewrite selection based on your params, not wsizes (window sizes for minnowing alg)
            // build table for each document recording total number of block and total postings
            vector<binfo> lbuf;
            binfo binf;
            memset(fn, 0, 256);
            sprintf(fn, "test/%d.2", docId);
            f = fopen(fn, "w");
            int length = wsizes2.size();
            for (int i = 0; i < length; i++)
            {
                double wsize = wsizes2[i];

                partitionAlgorithm->completeCount(wsize);

                partitionAlgorithm->select(invertedLists, wsize);

                // partitionAlgorithm->PushBlockInfo(documentContent, versionSizes, docId, numVersions, &binf);
                partitionAlgorithm->PushBlockInfo(versions, docId, &binf);

                binf.wsize = wsize;
                lbuf.push_back(binf);
                fprintf(f, "%.2lf\t%d\t%d\t%d\n", wsize, binf.total_dis, binf.total, binf.post);
            }
            fclose(f);
        }
        partitionAlgorithm->dump_frag();
        delete [] invertedLists;

        return numFrags;
    }
    else
    {
        memset(fn, 0, 256);
        sprintf(fn, "test/%d.2", docId);
        FILE* f = fopen(fn, "w");
        fprintf(f, "%.2lf\t%d\t%d\t%d\n", versionSizes[0], 1, 1, versionSizes[0]);
        fclose(f);
        return 1;
    }
}


// External Ids for repair
unsigned currentFragID = 0;
unsigned currentWordID = 0;
unsigned currentOffset = 0;

int main(int argc, char**argv)
{
    partitionAlgorithm = new partitions<CutDocument2>();
    // int* documentContent = new int[MAXSIZE]; // the contents of one document at a time (the fread below seems to overwrite for each doc)

    FILE* fileWikiComplete = fopen("/data/jhe/wiki_access/completeFile", "rb");
    FILE* fileWikiDocSizes = fopen("/data/jhe/wiki_access/numv", "rb");
    FILE* fileWikiVersionSizes = fopen("/data/jhe/wiki_access/word_size", "rb");

    std::ifstream inputWikiComplete("/data/jhe/wiki_access/completeFile", std::ios::in | std::ifstream::binary);

    int docCount;
    fread(&docCount, sizeof(unsigned), 1, fileWikiDocSizes);

    int* numVersionsPerDoc = new int[docCount]; // number of versions for each document
    fread(numVersionsPerDoc, sizeof(unsigned), docCount, fileWikiDocSizes);
    
    // I think this is the total number of versions in all docs
    unsigned totalNumVersions;
    fread(&totalNumVersions, sizeof(unsigned), 1, fileWikiVersionSizes);

    int* versionSizes = new int[totalNumVersions]; // versionSizes[i] is the length of version i (the number of word Ids)
    fread(versionSizes, sizeof(unsigned), totalNumVersions, fileWikiVersionSizes);

    // TODO use a similar approach (have different values in a text file) with your variables
    ifstream fin("options");
    istream_iterator<double> data_begin(fin);
    istream_iterator<double> data_end;
    vector<double> wsizes2(data_begin, data_end);
    fin.close();

    int numVersionsReadSoFar = 0;

    // use this to test a small number of docs
    docCount = 100;

    // Gets populated in the loop below
    // fragmentCounts[i] is the number of fragments in the partitioning doc i
    unsigned* fragmentCounts = new unsigned[docCount];
    int currentDoc = 0;
    auto versions = vector<vector<unsigned> >();

    for (size_t i = 0; i < docCount; ++i) // for each document -YK
    {
        // Trying to do it this way: http://stackoverflow.com/questions/15143670/how-can-i-use-fread-on-a-binary-file-to-read-the-data-into-a-stdvector
        auto currentVersion = vector<unsigned>();
        for (size_t v = 0; v < numVersionsPerDoc[i]; ++v) { // for each version in this doc

            // TODO add the threshold for total num words here:
            // if (current > MAX_NUM_WORDS_PER_DOC)
            // {
            //     return current;
            // }

            // Read the correct number of words into currentVersion
            currentVersion.resize(versionSizes[numVersionsReadSoFar + v]);
            inputWikiComplete.read(reinterpret_cast<char*>(&currentVersion[0]), currentVersion.size() * sizeof(unsigned));
            versions.push_back(currentVersion);
        }

        // // Get the total number of words to read for this doc
        // int totalNumWordsInDoc = 0; // the number of words in all versions of this doc
        // for (int j = 0; j < numVersionsPerDoc[i]; j++)
        // {
        //     totalNumWordsInDoc += versionSizes[numVersionsReadSoFar + j];
        // }
        // // Now read the doc contents from the wiki file
        // fread(documentContent, sizeof(unsigned), totalNumWordsInDoc, fileWikiComplete);

        // Run our partitioning algorithm with several different param values
        fragmentCounts[currentDoc] = dothejob(versions, i, numVersionsPerDoc[i], wsizes2);
        
        currentWordID = 0; // This is a global used by repair, see repair-algorithm/Util.h
        currentDoc++;        
        numVersionsReadSoFar += numVersionsPerDoc[i];

        for (size_t v = 0; v < numVersionsPerDoc[i]; ++v)
        {
            versions[v].clear();
        }
        versions.clear();

        printf("Complete: %d\n", i);
    }

    // delete [] documentContent;
    delete [] numVersionsPerDoc;
    delete [] fragmentCounts;

    fclose(fileWikiComplete);
    fclose(fileWikiDocSizes);
    fclose(fileWikiVersionSizes);

    FILE* successFile = fopen("SUCCESS!", "w");
    fclose(successFile);

    delete partitionAlgorithm;
    return 0;
}
