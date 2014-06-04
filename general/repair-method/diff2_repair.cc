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

// TODO test
void initDocLocations(map<unsigned, unsigned long>& docLocations) {
    // Read the whole list of doc locations into this set
    FILE* fin = fopen("docoffsets.txt", "rb");
    unsigned docId;
    unsigned long offset;

    // TODO THIS DOESN'T TERMINATE

    while (!feof(fin)) {

        int res = fread(&docId, sizeof(unsigned), 1, fin);
        int res2 = fread(&offset, sizeof(unsigned long), 1, fin);

    // while (fscanf(fin, "%u%u", &docId, &offset) == 2) {
        docLocations.insert(std::pair<unsigned, unsigned long>(docId, offset));
    }
    fclose(fin);
}


GeneralPartitionAlgorithm* partitionAlgorithm;
unsigned totalNumFragApps = 0;
int dothejob(vector<vector<unsigned> >& versions, int docId, const vector<double>& paramArray)
{
    char fn[256];
    if (versions.size() != 1)
    {
        // memset(fn, 0, 256);
        // sprintf(fn, "/research/run/jhe/proximity/model/test/%d", docId);
        // FILE* f = fopen(fn, "r");

        // iv* invertedLists = new iv[versions.size()];
        
        partitionAlgorithm->initRepair(versions);

        // Make a copy to use after repair (TODO can we avoid duplicating the data like this?)
        auto versionsCopy = vector<vector<unsigned> >(versions);

        // Run the repair algorithm
        // This produces a set of associations that can be used to build repair trees
        partitionAlgorithm->getRepairTrees();

        /*
            Build the tradeoff table for each document
            The tradeoff is between meta data size and index size
            The more fragments we have, the more meta data
            The more "good" fragments we identify, the less redundant postings
            So write down how many index postings will be generated for a partitioning
            The global index optimization will use these figures to choose a good param value for each doc
        */
        vector<TradeoffRecord> TradeoffTable;
        TradeoffRecord tradeoffRecord;
        memset(fn, 0, 256);
        sprintf(fn, "test/tradeoff-%d", docId);
        FILE* f = fopen(fn, "w");

        double paramValue;
        unsigned numBaseFrags;
        unsigned numLevelsDown = 8;
        
        // Remember to clear all the data structures at end of each iteration
        // I did this but it took a lot of debugging, be careful -YK
        
        for (int i = 0; i < paramArray.size(); i++)
        {
            iv* invertedLists = new iv[versions.size()];

            partitionAlgorithm->init();
            
            paramValue = paramArray[docId];

            numBaseFrags = partitionAlgorithm->getBaseFragments(invertedLists, versionsCopy, numLevelsDown);

            partitionAlgorithm->addBaseFragmentsToHeap(paramValue);

            partitionAlgorithm->selectGoodFragments(invertedLists, paramValue);
            
            partitionAlgorithm->writeTradeoffRecord(versionsCopy, docId, &tradeoffRecord);

            tradeoffRecord.paramValue = paramValue;
            TradeoffTable.push_back(tradeoffRecord);

            totalNumFragApps += tradeoffRecord.numFragApplications;
            fprintf(f, "%.2f\t%d\t%d\t%d\n",
                paramValue,
                tradeoffRecord.numDistinctFrags,
                tradeoffRecord.numFragApplications,
                tradeoffRecord.numPostings);

            partitionAlgorithm->resetBeforeNextRun();

            delete [] invertedLists;
        }
        fclose(f);

        return numBaseFrags;
    }
    else
    {
        memset(fn, 0, 256);
        sprintf(fn, "test/%d.2", docId);
        FILE* f = fopen(fn, "w");
        fprintf(f, "%f\t%d\t%d\t%d\n", 0.0, 1, 1, (unsigned) versions[0].size());
        fclose(f);
        return 1;
    }
}

void initAlreadyChosen(set<unsigned>& alreadyChosen) {
    ifstream fin;
    fin.open("docids.txt");
    istream_iterator<unsigned> data_begin2(fin);
    istream_iterator<unsigned> data_end2;
    alreadyChosen = set<unsigned>(data_begin2, data_end2);
    fin.close();
}

// External Ids for repair
unsigned currentFragID = 0;
unsigned currentWordID = 0;
unsigned currentOffset = 0;

int main(int argc, char**argv)
{
    clock_t init, final;
    init = clock();

    partitionAlgorithm = new GeneralPartitionAlgorithm();
    
    // Doc Sizes means number of versions in each doc
    FILE* fileWikiDocSizes = fopen("/data/jhe/wiki_access/numv", "rb");
    FILE* fileWikiVersionSizes = fopen("/data/jhe/wiki_access/word_size", "rb");
    FILE* fileWikiComplete = fopen("/data/jhe/wiki_access/completeFile", "rb");

    int totalDocs;
    fread(&totalDocs, sizeof(unsigned), 1, fileWikiDocSizes);

    int* numVersionsPerDoc = new int[totalDocs]; // number of versions for each document
    fread(numVersionsPerDoc, sizeof(unsigned), totalDocs, fileWikiDocSizes);
    
    // I think this is the total number of versions in all docs
    unsigned totalNumVersions;
    fread(&totalNumVersions, sizeof(unsigned), 1, fileWikiVersionSizes);

    int* versionSizes = new int[totalNumVersions]; // versionSizes[docId] is the length of version i (the number of word Ids)
    fread(versionSizes, sizeof(unsigned), totalNumVersions, fileWikiVersionSizes);

    bool single = false;
    if (argc > 1)
    {
        // use this to test a small number of docs
        single = true;
    }

    // The file contains one unsigned per line
    ifstream fin;
    if (single) {
        fin.open("paramSingle.txt");
    } else {
        fin.open("paramList.txt");
    }
    istream_iterator<double> data_begin(fin);
    istream_iterator<double> data_end;
    vector<double> paramArray(data_begin, data_end);
    fin.close();

    int numVersionsReadSoFar = 0;

    auto versions = vector<vector<unsigned> >();
    unsigned totalWordsInDoc;
    unsigned totalWordsInVersion;
    
    auto docLocations = map<unsigned, unsigned long>();
    initDocLocations(docLocations);

    // This is an ordered set, expect it to go in sorted order
    for (auto it = docLocations.begin(); it != docLocations.end(); ++it) {
        // Read the docID and the offset from the file
        unsigned docId = (*it).first;
        unsigned long docOffset = (*it).second;

        // We have the doc id and offset, seek to the offset and read that doc
        fseek(fileWikiComplete, docOffset, 0);

        // Time to read it
        auto currentVersion = vector<unsigned>();

        for (size_t v = 0; v < numVersionsPerDoc[docId]; ++v) // for each version in this doc
        {
            totalWordsInVersion = versionSizes[numVersionsReadSoFar + v];
            totalWordsInDoc += totalWordsInVersion;

            // Read the contents of the current version into a vector<unsigned>
            currentVersion.resize(totalWordsInVersion);
            // inputWikiComplete.read(reinterpret_cast<char*>(&currentVersion[0]), currentVersion.size() * sizeof(unsigned));
            fread(reinterpret_cast<char*>(&currentVersion[0]), sizeof(unsigned), currentVersion.size(), fileWikiComplete);

            versions.push_back(currentVersion);

            // Start with wordId 1 greater than the highest we've seen
            for (size_t j = 0; j < currentVersion.size(); ++j) {
                if (currentVersion[j] > currentWordID) {
                    currentWordID = currentVersion[j];
                }
            }
            ++currentWordID;
        }

        dothejob(versions, docId, paramArray);
        
        cerr << "Document " << docId << ": processed" << endl;
        
        numVersionsReadSoFar += numVersionsPerDoc[docId];

        for (size_t v = 0; v < numVersionsPerDoc[docId]; ++v) {
            versions[v].clear();
        }
        versions.clear();
    }

    cerr << "Param Selection Complete" << endl;
    // cerr << "Number of documents skipped: " << numSkipped << endl;

    delete [] numVersionsPerDoc;
    // delete [] fragmentCounts;

    // inputWikiComplete.close();
    fclose(fileWikiComplete);
    fclose(fileWikiDocSizes);
    fclose(fileWikiVersionSizes);

    FILE* successFile = fopen("SUCCESS PARAM SELECTION!", "w");
    fclose(successFile);

    final = clock() - init;

    double timeInSeconds = (double)final / ((double)CLOCKS_PER_SEC);
    int totalDocsProcessed = totalDocs;
    double docsPerSecond = (double)totalDocsProcessed / timeInSeconds;

    cerr << "Total time: " << timeInSeconds << endl;
    cerr << "Docs per second: " << docsPerSecond << endl;

    if (single) {
        char fn[256];
        memset(fn, 0, 256);
        sprintf(fn, "numFragApps");

        FILE* fOut = fopen(fn, "w");
        if (fOut) {
            fprintf(fOut, "%u", totalNumFragApps);
        }
        fclose(fOut);
    }

    delete partitionAlgorithm;
    return 0;
}
