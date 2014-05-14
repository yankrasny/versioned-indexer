/*

This is the global optimization using the tradeoff tables

We decide on parameters for each document based on some constraints on meta data size

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_DOCS 240179

// This is a row in the tradeoff table (metadata vs index size)
struct TradeoffRecord
{
    int numFragApplications; // number of fragment applications
    int numDistinctFrags; // number of distinct fragments
    int numPostings; // total number of postings
    unsigned numLevelsDown; // number of levels we traverse in the repair tree (more = possibly smaller fragments)
};

int comparep(const void* a1, const void* a2)
{
    TradeoffRecord* t1 = (TradeoffRecord*)a1;
    TradeoffRecord* t2 = (TradeoffRecord*)a2;

    int ret =  t1->numPostings - t2->numPostings;
    int ret2 = t1->numFragApplications - t2->numFragApplications;

    if (ret2 == 0)
        return ret;
    else
        return ret2;
}

int main(int argc, char** argv)
{
    TradeoffRecord* tradeoffs = new TradeoffRecord[NUM_DOCS];
    
    int ptr = 0;
    unsigned* lens = new unsigned[NUM_DOCS];
    memset(lens, 0, NUM_DOCS * sizeof(unsigned));
    
    char fn[256];
    memset(fn, 0, 256);
    
    FILE* f2 = fopen("finfos", "wb");
    
    FILE* fileWikiDocSizes = fopen("/data/jhe/wiki_access/numv", "rb");


    int docCount;
    fread(&docCount, sizeof(unsigned), 1, fileWikiDocSizes);

    unsigned* versionsPerDoc = new unsigned[docCount]; // number of versions for each document
    fread(versionsPerDoc, sizeof(unsigned), docCount, fileWikiDocSizes);

    fclose(fileWikiDocSizes);



    FILE* fileWikiVersionSizes = fopen("/data/jhe/wiki_access/word_size", "rb");
    unsigned totalNumVersions;
    fread(&totalNumVersions, sizeof(unsigned), 1, fileWikiVersionSizes);

    // versionSizes[i] is the length of version i (the number of word Ids)
    unsigned* versionSizes = new unsigned[totalNumVersions];
    fread(versionSizes, sizeof(unsigned), totalNumVersions, fileWikiVersionSizes);
    fclose(fileWikiVersionSizes);
    

    if (argc > 1)
    {
        // use this to test a small number of docs
        docCount = atoi(argv[1]);
    }
    unsigned numLevelsDown = 0;
    FILE* fexcept = fopen("except", "w");
    for (int i = 0; i < docCount; i++)
    {
        memset(fn, 0, 256);
        sprintf(fn, "test/tradeoff-%d", i);
        FILE* currFile = fopen(fn, "r");
        if (!currFile) {
            printf("skipping:%d...\n", i);    
            continue;
        }
        ptr = 0;
        printf("processing:%d...", i);
        if (versionsPerDoc[i] > 1)
        {
            bool change = false;
            while (fscanf(currFile, "%d\t%d\t%d\t%d\n", 
                &tradeoffs[ptr].numLevelsDown, 
                &tradeoffs[ptr].numDistinctFrags,
                &tradeoffs[ptr].numFragApplications,
                &tradeoffs[ptr].numPostings) > 0)
            {
                if (ptr > 0 && tradeoffs[ptr].numPostings != tradeoffs[ptr-1].numPostings)
                {
                    change = true;
                }
                ptr++;
            }

            fclose(currFile);

            memset(fn, 0, 256);
            sprintf(fn, "test/convert-%d", i);
            currFile = fopen(fn, "w");

            qsort(tradeoffs, ptr, sizeof(TradeoffRecord), comparep);

            int ptr1 = 0;
            int ptr2 = 1;
            while (ptr2 < ptr)
            {
                while (ptr2 < ptr && tradeoffs[ptr2].numPostings >= tradeoffs[ptr1].numPostings)
                    ptr2++;

                if (ptr2 >= ptr)
                    break;

                ptr1++;
                tradeoffs[ptr1] = tradeoffs[ptr2];
                ptr2++;
            }
            // ptr1++;
            
            // Why are we looping here, and what is ptr1?
            // for (int j = 0; j < ptr1; j++)
            // {
            //     fprintf(currFile, "%d\t%d\t%d\t%d\n", 
            //         tradeoffs[j].numLevelsDown, 
            //         tradeoffs[j].numFragApplications, 
            //         tradeoffs[j].numDistinctFrags, 
            //         tradeoffs[j].numPostings);
            // }
            
            fprintf(currFile, "%d\t%d\t%d\t%d\n", 
                tradeoffs[ptr1].numLevelsDown, 
                tradeoffs[ptr1].numDistinctFrags, 
                tradeoffs[ptr1].numFragApplications, 
                tradeoffs[ptr1].numPostings);


            fclose(currFile);
            fwrite(tradeoffs, sizeof(TradeoffRecord), ptr1, f2);
            lens[i] = ptr1;
            if (change == false)
            {
                fprintf(fexcept, "%d\t%d\n", i, ptr);
            }
        }
        else
        {
            fclose(currFile);
            lens[i] = 1;
            tradeoffs[0].numLevelsDown = versionSizes[numLevelsDown]; // TODO i don't think the rhs is right
            tradeoffs[0].numFragApplications = 1;
            tradeoffs[0].numDistinctFrags  = 1;
            tradeoffs[0].numPostings = versionSizes[numLevelsDown]; // TODO i don't think the rhs is right
            fwrite(tradeoffs, sizeof(TradeoffRecord), 1, f2);
        }
        numLevelsDown += versionsPerDoc[i];
        printf("done!\n");
    }

    fclose(fexcept);
    docCount = atoi(argv[1]);
    FILE* fsize = fopen("TradeoffRecords", "wb");
    fwrite(&docCount, sizeof(unsigned), 1, fsize);
    fwrite(lens, sizeof(unsigned), docCount , fsize);
    fclose(fsize);
    fclose(f2);
    return 0;
}