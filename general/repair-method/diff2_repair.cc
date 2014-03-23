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
#include "general_partition_repair.h"
using namespace std;

GeneralPartitionAlgorithm* partitionAlgorithm;
int dothejob(vector<vector<unsigned> >& versions, int docId, const vector<double>& wsizes2)
{
    char fn[256];
    if (versions.size() != 1)
    {
        memset(fn, 0, 256);
        sprintf(fn, "/research/run/jhe/proximity/model/test/%d", docId);
        FILE* f = fopen(fn, "r");

        iv* invertedLists = new iv[versions.size()];
        partitionAlgorithm->init();

        // I think wsizes is window sizes, and total is the size of that array
        // TODO these vars should not be hardcoded, if time permits we'll change it
        float fragmentationCoefficient = 1.0;
        float minFragSize = 100;
        unsigned numLevelsDown = 5;
        int numFrags = partitionAlgorithm->fragment(versions, invertedLists, 
            fragmentationCoefficient, minFragSize, numLevelsDown);
        
        // printf("Finished partitioning!\n");

        // TODO rewrite selection based on your params, not wsizes (window sizes for minnowing alg)
        // build table for each document recording total number of block and total postings
        vector<TradeoffRecord> TradeoffTable;
        TradeoffRecord tradeoffRecord;
        memset(fn, 0, 256);
        sprintf(fn, "test/meta-vs-index-tradeoff-%d", docId);
        f = fopen(fn, "w");

        // Iterate over values of the window size param
        for (int i = 0; i < wsizes2.size(); i++)
        {
            double wsize = wsizes2[i];

            partitionAlgorithm->addFragmentsToHeap(wsize);

            partitionAlgorithm->selectGoodFragments(invertedLists, wsize);
            
            partitionAlgorithm->writeTradeoffRecord(versions, docId, &tradeoffRecord);

            tradeoffRecord.wsize = wsize;
            TradeoffTable.push_back(tradeoffRecord);
            fprintf(f, "%.2lf\t%d\t%d\t%d\n", wsize, tradeoffRecord.numDistinctFrags, tradeoffRecord.numFragApplications, tradeoffRecord.numPostings);
        }
        fclose(f);

        partitionAlgorithm->dump_frag();
        delete [] invertedLists;

        return numFrags;
    }
    else
    {
        memset(fn, 0, 256);
        sprintf(fn, "test/%d.2", docId);
        FILE* f = fopen(fn, "w");
        fprintf(f, "%.2lf\t%d\t%d\t%d\n", versions[0].size(), 1, 1, versions[0].size());
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
    partitionAlgorithm = new GeneralPartitionAlgorithm();
    
    // Doc Sizes means number of versions in each doc
    FILE* fileWikiDocSizes = fopen("/data/jhe/wiki_access/numv", "rb");
    FILE* fileWikiVersionSizes = fopen("/data/jhe/wiki_access/word_size", "rb");
    
    // FILE* fileWikiComplete = fopen("/data/jhe/wiki_access/completeFile", "rb");
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
    ifstream fin("../options");
    istream_iterator<double> data_begin(fin);
    istream_iterator<double> data_end;
    vector<double> wsizes2(data_begin, data_end);
    fin.close();

    int numVersionsReadSoFar = 0;

    if (argc > 1)
    {
        // use this to test a small number of docs
        docCount = atoi(argv[1]);
    }

    // Gets populated in the loop below
    // fragmentCounts[i] is the number of fragments in the partitioning doc i
    unsigned* fragmentCounts = new unsigned[docCount];
    auto versions = vector<vector<unsigned> >();
    unsigned totalWordsInDoc;
    unsigned totalWordsInVersion;
    bool skipThisDoc = false;
    unsigned numSkipped = 0;

    // In this loop, i is the docId
    for (size_t i = 0; i < docCount; ++i) // for each document -YK
    {
        totalWordsInDoc = 0;
        totalWordsInVersion = 0;
        currentWordID = 0; // This is a global used by repair, see repair-algorithm/Util.h

        // If you're confused about the line that reads into the vector, see: http://stackoverflow.com/questions/15143670/how-can-i-use-fread-on-a-binary-file-to-read-the-data-into-a-stdvector
        auto currentVersion = vector<unsigned>();
        for (size_t v = 0; v < numVersionsPerDoc[i]; ++v) // for each version in this doc
        {
            totalWordsInVersion = versionSizes[numVersionsReadSoFar + v];
            totalWordsInDoc += totalWordsInVersion;
            if (totalWordsInDoc > MAX_NUM_WORDS_PER_DOC)
            {
                skipThisDoc = true;
            } else {
                skipThisDoc = false;
            }

            // Read the contents of the current version into a vector<unsigned>
            currentVersion.resize(totalWordsInVersion);
            inputWikiComplete.read(reinterpret_cast<char*>(&currentVersion[0]), currentVersion.size() * sizeof(unsigned));
            versions.push_back(currentVersion);

            // Start with wordId 1 greater than the highest we've seen
            for (size_t j = 0; j < currentVersion.size(); ++j) {
                if (currentVersion[j] > currentWordID) {
                    currentWordID = currentVersion[j];
                }
            }
            ++currentWordID;
        }

        // cerr << "Starting ID for Repair: " << currentWordID << endl;

        if (!skipThisDoc) {
            // Run our partitioning algorithm with several different param values
            fragmentCounts[i] = dothejob(versions, i, wsizes2);
            cerr << "Document " << i << ": processed" << endl;
        } else {
            fragmentCounts[i] = 0;
            cerr << "Document " << i << ": skipped" << endl;
            ++numSkipped;
        }
        
        numVersionsReadSoFar += numVersionsPerDoc[i];

        for (size_t v = 0; v < numVersionsPerDoc[i]; ++v) {
            versions[v].clear();
        }
        versions.clear();
    }

    cerr << "Param Selection Complete" << endl;
    cerr << "Number of documents skipped: " << numSkipped << endl;

    delete [] numVersionsPerDoc;
    delete [] fragmentCounts;

    inputWikiComplete.close();
    fclose(fileWikiDocSizes);
    fclose(fileWikiVersionSizes);

    FILE* successFile = fopen("SUCCESS PARAM SELECTION!", "w");
    fclose(successFile);

    delete partitionAlgorithm;
    return 0;
}