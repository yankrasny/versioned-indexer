/*
 * index_repair.cc
 *
 * Author: Yan Krasny
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <fstream>
#include "general_partition_repair.h"
#include "../../util/indexer.h"
using namespace std;

indexer* myIndexer;
GeneralPartitionAlgorithm* partitionAlgorithm;
int dothejob(vector<vector<unsigned> >& versions, int docId, double paramValue, unsigned numLevelsDown = 10)
{
    iv* invertedLists = new iv[versions.size()];
    partitionAlgorithm->initRepair(versions);

    // Make a copy to use after repair
    auto versionsCopy = vector<vector<unsigned> >(versions);

    // Do repair and save the result in some form so that trees can be built for partitioning
    partitionAlgorithm->getRepairTrees();

    partitionAlgorithm->init();

    unsigned numBaseFrags = partitionAlgorithm->getBaseFragments(invertedLists, versionsCopy, numLevelsDown);
    partitionAlgorithm->addBaseFragmentsToHeap(paramValue);
    partitionAlgorithm->selectGoodFragments(invertedLists, paramValue);
    partitionAlgorithm->populatePostings(versions, docId);

    for (int i = 0; i < partitionAlgorithm->add_list_len; i++)
    {
        // partitionAlgorithm->add_list is a posting*
        // wid is wordID
        // pos is position
        // vid is versionID, and we're using version IDs as doc IDs

        // Well, it seems we need to do a bit more here

        /*
            Index postings are word, position version, right?
            Ok, so how does QP work? There must be a way to decide on a list of docs after running QP, not frags or versions.



        */



        myIndexer->insert_term(
            partitionAlgorithm->add_list[i].wid, 
            partitionAlgorithm->add_list[i].pos,
            partitionAlgorithm->add_list[i].vid);
    }
    
    delete [] invertedLists;
    return partitionAlgorithm->add_list_len;
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
    
    // FILE* fileWikiComplete = fopen("/data/jhe/wiki_access/completeFile", "rb");
    std::ifstream inputWikiComplete("/data/jhe/wiki_access/completeFile", std::ios::in | std::ifstream::binary);

    int totalDocs;
    fread(&totalDocs, sizeof(unsigned), 1, fileWikiDocSizes);

    int* numVersionsPerDoc = new int[totalDocs]; // number of versions for each document
    fread(numVersionsPerDoc, sizeof(unsigned), totalDocs, fileWikiDocSizes);
    
    // I think this is the total number of versions in all docs
    unsigned totalNumVersions;
    fread(&totalNumVersions, sizeof(unsigned), 1, fileWikiVersionSizes);

    int* versionSizes = new int[totalNumVersions]; // versionSizes[i] is the length of version i (the number of word Ids)
    fread(versionSizes, sizeof(unsigned), totalNumVersions, fileWikiVersionSizes);

    int numVersionsReadSoFar = 0;

    // if (argc > 1)
    // {
    //     // use this to test a small number of docs
    //     totalDocs = atoi(argv[1]);
    // }

    unsigned numLevelsDown = 8;

    // Gets populated in the loop below
    // fragmentCounts[i] is the number of fragments in the partitioning doc i
    unsigned* fragmentCounts = new unsigned[totalDocs];
    auto versions = vector<vector<unsigned> >();
    unsigned totalWordsInDoc;
    unsigned totalWordsInVersion;
    bool skipThisDoc = false;
    unsigned numSkipped = 0;


    auto alreadyChosen = set<unsigned>();
    initAlreadyChosen(alreadyChosen);

    // in case we can't read the files or something, just use a value somewhere in the middle
    double defaultParamValue = 1.0;


    // See ../../util/indexer.h
    unsigned maxTuplesPerBlock = 10000;
    unsigned typeLabel = numLevelsDown;
    unsigned sizeLabel = alreadyChosen.size();
    myIndexer = new indexer(maxTuplesPerBlock, typeLabel, sizeLabel);

    
    // if (alreadyChosen.find(874) == alreadyChosen.end()) {
    //     cerr << "Could not find 874, but it should be there..." << endl;
    //     cerr << *(alreadyChosen.find(874));
    // }
    // exit(0);

    // In this loop, i is the docId
    for (int i = 0; i < totalDocs; ++i) // for each document -YK
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

        if (alreadyChosen.find(i) == alreadyChosen.end()) {
            skipThisDoc  = true;
        }

        if (!skipThisDoc) {

            char fn[256];
            memset(fn, 0, 256);
            sprintf(fn, "test/convert-%d", i);

            double paramValue;
            ifstream ifs;
            ifs.open(fn, std::ifstream::in);
            if (ifs && (ifs >> paramValue)) {
                // reading the param worked
            } else {
                // reading the param didn't work so use the default
                printf("Using default value for doc %d...\n", i);
                paramValue = defaultParamValue;
            }

            fragmentCounts[i] = dothejob(versions, i, paramValue, numLevelsDown);
            cerr << "Document " << i << ": processed" << endl;
        } else {
            fragmentCounts[i] = 0;
            // cerr << "Document " << i << ": skipped" << endl;
            ++numSkipped;
        }
        
        numVersionsReadSoFar += numVersionsPerDoc[i];

        for (size_t v = 0; v < numVersionsPerDoc[i]; ++v) {
            versions[v].clear();
        }
        versions.clear();
    }

    // Write index to disk
    myIndexer->dump();

    cerr << "Indexing Complete" << endl;
    cerr << "Number of documents skipped: " << numSkipped << endl;

    final = clock() - init;

    double timeInSeconds = (double)final / ((double)CLOCKS_PER_SEC);
    int totalDocsProcessed = totalDocs - numSkipped;
    double docsPerSecond = (double)totalDocsProcessed / timeInSeconds;

    cerr << "Total time: " << timeInSeconds << endl;
    cerr << "Docs per second: " << docsPerSecond << endl;

    delete [] numVersionsPerDoc;
    delete [] fragmentCounts;

    inputWikiComplete.close();
    fclose(fileWikiDocSizes);
    fclose(fileWikiVersionSizes);

    FILE* successFile = fopen("SUCCESS INDEX!", "w");
    fclose(successFile);

    // delete partitionAlgorithm;
    return 0;
}