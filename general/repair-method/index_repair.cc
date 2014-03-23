/*
 * index_repair.cc
 *
 * Author: Yan Krasny
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <fstream>
#include "general_partition_repair.h"
#include "../../util/indexer.h"
using namespace std;

indexer* myIndexer;
GeneralPartitionAlgorithm* partitionAlgorithm;
int dothejob(vector<vector<unsigned> >& versions, int docId, float wsize)
{
    // TODO read your chosen variables
    // A good way to write this is to just hardcode some
    // Test the indexer, then write the code that reads params

    iv* invertedLists = new iv[versions.size()];
    partitionAlgorithm->init();

    float fragmentationCoefficient = 1.0;
    float minFragSize = 100;
    unsigned numLevelsDown = 5;
    int numFrags = partitionAlgorithm->fragment(versions, invertedLists, 
        fragmentationCoefficient, minFragSize, numLevelsDown);
    
    // printf("Finished partitioning!\n");

    // TOKEEP
    partitionAlgorithm->addFragmentsToHeap(wsize);
    partitionAlgorithm->selectGoodFragments(invertedLists, wsize);
    partitionAlgorithm->populatePostings(versions, docId);
    for (int i = 0; i < partitionAlgorithm->add_list_len; i++)
    {
        // partitionAlgorithm->add_list is a posting*
        // wid is wordID
        // pos is position (most likely)
        // vid is versionID, and we're using version IDs as doc IDs
        myIndexer->insert_term(partitionAlgorithm->add_list[i].wid, 
            partitionAlgorithm->add_list[i].pos, partitionAlgorithm->add_list[i].vid);
    }
    
    delete [] invertedLists;
    return partitionAlgorithm->add_list_len;
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

    // See ../../util/indexer.h
    unsigned maxTuplesPerBlock = 400000000;
    unsigned typeLabel = 50;
    unsigned sizeLabel = 50;
    myIndexer = new indexer(maxTuplesPerBlock, typeLabel, sizeLabel);

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
            // TODO change wsize to your params
            fragmentCounts[i] = dothejob(versions, i, 5.0);
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

    // Write index to disk
    myIndexer->dump();

    cerr << "Indexing Complete" << endl;
    cerr << "Number of documents skipped: " << numSkipped << endl;

    delete [] numVersionsPerDoc;
    delete [] fragmentCounts;

    inputWikiComplete.close();
    fclose(fileWikiDocSizes);
    fclose(fileWikiVersionSizes);

    FILE* successFile = fopen("SUCCESS INDEX!", "w");
    fclose(successFile);

    delete partitionAlgorithm;
    return 0;
}