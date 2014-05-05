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
int dothejob(vector<vector<unsigned> >& versions, int docId, const vector<unsigned>& numLevelsDownArray)
{
    char fn[256];
    if (versions.size() != 1)
    {
        memset(fn, 0, 256);
        sprintf(fn, "/research/run/jhe/proximity/model/test/%d", docId);
        FILE* f = fopen(fn, "r");

        iv* invertedLists = new iv[versions.size()];
        
        partitionAlgorithm->init(versions);

        // Make a copy to use after repair (TODO can we avoid duplicating the data like this?)
        auto versionsCopy = vector<vector<unsigned> >(versions);

        // Run the repair algorithm
        // This produces a set of associations that can be used to build repair trees
        partitionAlgorithm->getRepairTrees();

        /*
            Build the tradeoff table for each document
            The tradeoff is between meta data size and index size
            The more fragments we have, the more meta data
            We also write down how many index postings will be generated for a partitioning
            The global index optimization will use these figures to choose a good param value for each doc
        */
        vector<TradeoffRecord> TradeoffTable;
        TradeoffRecord tradeoffRecord;
        memset(fn, 0, 256);
        sprintf(fn, "test/tradeoff-%d", docId);
        f = fopen(fn, "w");

        unsigned numLevelsDown;
        unsigned numBaseFrags;
        for (int i = 0; i < numLevelsDownArray.size(); i++)
        {
            numLevelsDown = numLevelsDownArray[i];

            numBaseFrags = partitionAlgorithm->getBaseFragments(invertedLists, versionsCopy, numLevelsDown);

            partitionAlgorithm->addBaseFragmentsToHeap(numLevelsDown);

            partitionAlgorithm->selectGoodFragments(invertedLists, numLevelsDown);
            
            partitionAlgorithm->writeTradeoffRecord(versionsCopy, docId, &tradeoffRecord);

            tradeoffRecord.numLevelsDown = numLevelsDown;
            TradeoffTable.push_back(tradeoffRecord);
            fprintf(f, "%d\t%d\t%d\t%d\n", numLevelsDown, tradeoffRecord.numDistinctFrags, 
                tradeoffRecord.numFragApplications, tradeoffRecord.numPostings);
        }
        fclose(f);

        partitionAlgorithm->dump_frag();
        delete [] invertedLists;

        return numBaseFrags;
    }
    else
    {
        memset(fn, 0, 256);
        sprintf(fn, "test/%d.2", docId);
        FILE* f = fopen(fn, "w");
        // TODO is numLevelsDown the right thing to put there?
        fprintf(f, "%d\t%d\t%d\t%d\n", 0, 1, 1, (unsigned) versions[0].size());
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
    // ifstream fin("../options");
    // istream_iterator<double> data_begin(fin);
    // istream_iterator<double> data_end;
    // vector<double> wsizes2(data_begin, data_end);
    // fin.close();

    auto numLevelsDownArray = vector<unsigned>();
    numLevelsDownArray.push_back(3);
    numLevelsDownArray.push_back(4);
    numLevelsDownArray.push_back(5);

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
            fragmentCounts[i] = dothejob(versions, i, numLevelsDownArray);
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