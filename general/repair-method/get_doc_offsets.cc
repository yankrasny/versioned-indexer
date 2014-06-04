#include <set>
#include <iterator>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <ctime>
#include <algorithm>
using namespace std;

#define MAXSIZE 81985059


void initAlreadyChosenFromFile(set<unsigned>& alreadyChosen) {
    ifstream fin;
    fin.open("docids.txt");
    istream_iterator<unsigned> data_begin(fin);
    istream_iterator<unsigned> data_end;
    alreadyChosen = set<unsigned>(data_begin, data_end);
    fin.close();
}


int main(int argc, char**argv)
{
    set<unsigned> alreadyChosen = set<unsigned>();
    initAlreadyChosenFromFile(alreadyChosen);

    int* fileDataBuffer = new int[MAXSIZE];

    FILE* fileWikiDocSizes = fopen("/data/jhe/wiki_access/numv", "rb");
    FILE* fileWikiVersionSizes = fopen("/data/jhe/wiki_access/word_size", "rb");
    FILE* fcomplete = fopen("/data/jhe/wiki_access/completeFile", "rb");

    int totalDocs;
    fread(&totalDocs, sizeof(unsigned), 1, fileWikiDocSizes);

    int* numVersionsPerDoc = new int[totalDocs]; // number of versions for each document
    fread(numVersionsPerDoc, sizeof(unsigned), totalDocs, fileWikiDocSizes);
    
    // I think this is the total number of versions in all docs
    unsigned totalNumVersions;
    fread(&totalNumVersions, sizeof(unsigned), 1, fileWikiVersionSizes);

    int* versionSizes = new int[totalNumVersions]; // versionSizes[i] is the length of version i (the number of word Ids)
    fread(versionSizes, sizeof(unsigned), totalNumVersions, fileWikiVersionSizes);


    FILE* myfile = fopen("docoffsets.txt", "wb");
    unsigned long currPosInFile;
    bool skipThisDoc;
    int numVersionsReadSoFar = 0;
    unsigned numSkipped = 0;

    for (unsigned i = 0; i < totalDocs; ++i) {
        unsigned totalWordsInDoc = 0;
        unsigned totalWordsInVersion = 0;

        for (int v = 0; v < numVersionsPerDoc[i]; v++) {
            totalWordsInVersion = versionSizes[numVersionsReadSoFar + v];
            totalWordsInDoc += totalWordsInVersion;
        }

        int len = fread(fileDataBuffer, sizeof(unsigned), totalWordsInDoc, fcomplete);
        if (len > MAXSIZE) {
            cerr << "Doc size is: " << len << endl;
            exit(1);
        }

        if (alreadyChosen.find(i) == alreadyChosen.end()) {
            skipThisDoc = true;
        } else {
            skipThisDoc = false;
        }

        if (!skipThisDoc) {
            // Write the docID and the offset to a file. Format is 2 unsigned ints in binary (no commas or newlines)
            currPosInFile = ftell(fcomplete);
            fwrite (&i, sizeof(unsigned), 1, myfile);
            fwrite (&currPosInFile, sizeof(unsigned long), 1, myfile);
        } else {
            // cerr << "Document " << i << ": skipped" << endl;
            ++numSkipped;
        }
        
        numVersionsReadSoFar += numVersionsPerDoc[i];

    }

    fclose(myfile);
}