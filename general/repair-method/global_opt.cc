/*

Global Optimization of param values using tradeoff tables for each document

For each document, we have produced tradeoff tables that look something like this:

paramValue numDistinctFrags numFragApps numPostings
1          100              240         1167
2          130              361         1002

Now assume we have a budget of fragment applications (we can obtain this budget by running the alg with a selected param value)

// what happens if the heap is empty before we get below the budget? (if the budget is a hard requirement, we fail it)
    // that just means the budget is too low

// what happens if the budget is hit but the heap still has entries? (we want this)
    // well at that point stop, you'll be choosing entries that maximize meta data and minimize index
        // in this case your meta budget allows you to do that, so you get to compress the index better

while the budget is not exceeded and the heap is not empty
    el = heap.pop()
    foreach (el.othersInDoc() as el2) // get all the entries that are no longer valid since we're choosing this one
        el2.removeFromHeap() // remove those entries from the heap
    currentRow[el.doc] = el.end // move the pointer to the destination of our element

In the end, currentRow should look like this:
[2,1,3,3,1,2,1,1,...]
This tells us which entry in the tradeoff table is being chosen for each document

We can write that array to disk and consume it in index_repair.cc

The challenge is the data structure with regard to othersInDoc. Each heap entry points to a few others, which is scary...

Author: Yan Krasny

*/

// TODO define and include TradeoffHeap
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <fstream>
#include <vector>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <set>
#include "tradeoff-heap/TradeoffHeapEntry.h"
#include "tradeoff-heap/TradeoffHeap.h"
using namespace std;

#define NUM_DOCS 240179

struct TradeoffRecord
{
    unsigned numFragApplications; // number of fragment applications
    unsigned numDistinctFrags; // number of distinct fragments
    unsigned numPostings; // total number of postings
    double paramValue; // number of levels we traverse in the repair tree (more = possibly smaller fragments)
    bool isValid; // there are some documents with empty tradeoff tables, use these as a placeholder but make sure the code using them know that

    TradeoffRecord() {
        numFragApplications = 0;
        numDistinctFrags = 0;
        numPostings = 0;
        paramValue = 0.0;
        isValid = false;
    }
};

class TradeoffOptimizer 
{
private:
    unsigned totalNumFragApps;
    unsigned fragAppBudget;
    vector<unsigned> currRowIndexes;
    vector<vector<TradeoffRecord> > rowsAllDocs;
    unsigned numDocs;
    TradeoffHeap heap;

    set<unsigned> alreadyChosen;
public:
    TradeoffOptimizer(unsigned numDocs, unsigned fragAppBudget, set<unsigned>& alreadyChosen)
        : numDocs(numDocs), fragAppBudget(fragAppBudget), alreadyChosen(alreadyChosen)
    {
        totalNumFragApps = 0;
        currRowIndexes = vector<unsigned>(numDocs); // Fill with zeros (every doc starts off with the first row active)
        rowsAllDocs = vector<vector<TradeoffRecord> >();
        heap = TradeoffHeap();
    }

    unsigned getTotalNumFragApps() const {
        return totalNumFragApps;
    }

    void getRowsForDoc(unsigned docId, vector<TradeoffRecord>& rowsOneDoc) {
        char fn[256];
        memset(fn, 0, 256);
        sprintf(fn, "test/tradeoff-%d", docId);

        ifstream ifs;
        ifs.open(fn, std::ifstream::in);

        TradeoffRecord row;

        double paramValue;
        unsigned numDistinctFrags;
        unsigned numFragApplications;
        unsigned numPostings;
        
        while (ifs >> paramValue >> numDistinctFrags >> numFragApplications >> numPostings) {
            row.paramValue = paramValue;
            row.numDistinctFrags = numDistinctFrags;
            row.numFragApplications = numFragApplications;
            row.numPostings = numPostings;
            row.isValid = true;

            rowsOneDoc.push_back(row);
        }
    }

    /*
        Use currRowIndexes to point to the current active row in each doc
        So we can access the current active row in document i as: 
        rowsAllDocs[i][currRowIndexes[i]]
    */
    TradeoffRecord getCurrentRecord(unsigned docId) {
        TradeoffRecord rec;
        if (docId >= 0 &&
            docId < rowsAllDocs.size() &&
            docId < currRowIndexes.size())
        {

            char fn[256];
            memset(fn, 0, 256);
            sprintf(fn, "test/tradeoff-%d", docId);
            FILE* f = fopen(fn, "r");
            if (f == NULL) {
                // fclose(f);
                return rec;
            }
            fclose(f);
            rec = rowsAllDocs[docId][currRowIndexes[docId]];
        }
        return rec;
    }

    void fillHeap() {
        // Read the tradeoff tables for all the docs and fill the heap with entries based on those
        for (unsigned i = 0; i < numDocs; ++i) {

            // Every doc starts off with the first row active
            currRowIndexes.push_back(0);

            // Save all the records to avoid doing extra file i/o
            auto rowsOneDoc = vector<TradeoffRecord>();
            getRowsForDoc(i, rowsOneDoc);
            rowsAllDocs.push_back(rowsOneDoc);          

            // Use the list of chosen docs to get the same random sample as used in diff2_repair            
            if (alreadyChosen.find(i) == alreadyChosen.end() || rowsOneDoc.size() < 1) {
                continue;
            }

            TradeoffRecord currRecord = getCurrentRecord(i);
            assert(currRecord.isValid);
            
            totalNumFragApps += currRecord.numFragApplications;

            // rowsAllDocs[i] are the tradeoff rows for document i
            // rowsAllDocs[i][j] is one tradeoff row

            auto relatedEntries = vector<TradeoffHeapEntry*>();
            
            TradeoffHeapEntry* entry;

            for (int j = 0; j < rowsOneDoc.size() - 1; ++j) {
                for (int k = j + 1; k < rowsOneDoc.size(); ++k) {
                    // pay close attention to the indexes here
                    // row[k] should have higher meta size and lower idx size than row[j]
                    int postingDiff = rowsOneDoc[j].numPostings - rowsOneDoc[k].numPostings;
                    int metaDiff = rowsOneDoc[k].numFragApplications - rowsOneDoc[j].numFragApplications;

                    if (postingDiff <= 0 || metaDiff <= 0) {
                        // the record doesn't follow the normal pattern (meta should go up, idx should go down)
                        // so we don't want to add this entry to the heap
                        continue;
                    }
                    
                    unsigned docId = i;
                    unsigned startIdx = j;
                    unsigned endIdx = k;
                    
                    double priority = static_cast<double>(postingDiff) / static_cast<double>(metaDiff); // the ratio of improvement in size of index to cost in size of meta data

                    auto entry = new TradeoffHeapEntry(docId, startIdx, endIdx, priority, &heap, 0);
                    
                    relatedEntries.push_back(entry);

                    // Add all the other entries in this document to the list of related if they really are related
                    for (auto it = relatedEntries.begin(); it != relatedEntries.end(); ++it) {
                        TradeoffHeapEntry* otherEntry = (*it);
                        // related is defined as: if I get chosen, all of these get removed
                        // to understand this, draw the diagram of the files with the arrows
                        if (entry != otherEntry && entry->getEndIdx() >= otherEntry->getStartIdx()) {
                            entry->getRelatedEntries().push_back(otherEntry);
                        }
                    }

                    assert(entry->getDocId() < NUM_DOCS);
                    heap.insert(entry);
                }
            }
        }
    }

    /*
        The heap optimization algorithm
        fragAppBudget should come from running the whole thing with set params
        perhaps choose x ST numFragApps is very large
    */
    void runHeapSelectionAlg() {
        while (!heap.empty() && totalNumFragApps < fragAppBudget) {
            TradeoffHeapEntry* entry = heap.getMax();
            unsigned currDocId = entry->getDocId();
            assert(currDocId <= NUM_DOCS);

            // We're about to use a different record, and we'll add those fragApps below
            TradeoffRecord recordBeforeMove = getCurrentRecord(currDocId);
            totalNumFragApps -= recordBeforeMove.numFragApplications;

            // erase all the now invalid heap entries
            vector<TradeoffHeapEntry*> related = entry->getRelatedEntries();
            for (auto it = related.begin(); it != related.end(); ++it) {
                TradeoffHeapEntry* otherEntry = (*it);
                int idxInHeap = otherEntry->getIndex();
                heap.deleteAtIndex(idxInHeap);
            }

            // move the pointer down to where this entry ends
            currRowIndexes[currDocId] = entry->getEndIdx();

            // Now that we've moved the pointer, the number of frag apps has changed
            TradeoffRecord recordAfterMove = getCurrentRecord(currDocId);
            totalNumFragApps += recordAfterMove.numFragApplications;
            heap.deleteAtIndex(entry->getIndex());
        }
    }

    void writeOutput(bool debug = false) {
        char fn[256];

        // Write to a file that will be consumed by the indexing script
        TradeoffRecord rec;
        int totalPostings = 0;
        for (unsigned i = 0; i < currRowIndexes.size(); i++) {

            if (alreadyChosen.find(i) == alreadyChosen.end()) {
                continue;
            }

            TradeoffRecord rec = getCurrentRecord(i);

            if (rec.isValid) {

                totalPostings += rec.numPostings;

                memset(fn, 0, 256);
                sprintf(fn, "test/convert-%d", i);
                FILE* f = fopen(fn, "w");
                fprintf(f, "%.2f", rec.paramValue);
                fclose(f);

                if (debug) {
                    cerr << "Doc " << i << " is at index: " << currRowIndexes[i] << " with paramValue " << rec.paramValue << endl;  
                }
            }
        }

        if (debug) {
            cerr << "Total frag apps: " << getTotalNumFragApps() << endl;
            cerr << "Total index postings: " << totalPostings << endl;
        }
    }
};

unsigned getFragAppBudget(unsigned defaultBudget = 10000)
{
    char fn[256];
    memset(fn, 0, 256);
    sprintf(fn, "numFragApps");

    ifstream ifs;
    ifs.open(fn, std::ifstream::in);
    if (!ifs) {
        return defaultBudget;
    }

    unsigned fragAppBudget;
    if (!(ifs >> fragAppBudget)) {
        return defaultBudget;
    }
    return fragAppBudget;
}


void initAlreadyChosen(set<unsigned>& alreadyChosen) {
    ifstream fin;
    fin.open("docids.txt");
    istream_iterator<unsigned> data_begin2(fin);
    istream_iterator<unsigned> data_end2;
    alreadyChosen = set<unsigned>(data_begin2, data_end2);
    fin.close();
}

int main(int argc, char** argv)
{
    clock_t init, final;
    init = clock();


    auto alreadyChosen = set<unsigned>();
    initAlreadyChosen(alreadyChosen);


    int numDocs = NUM_DOCS;
    // if (argc > 1)
    // {
    //     // use this to test a small number of docs
    //     numDocs = atoi(argv[1]);
    // }

    // Get the budget from the file (the diff2_repair script should have written it there)
    unsigned fragAppBudget = getFragAppBudget();
    if (argc > 1)
    {
        // optionally override the budget on command line
        fragAppBudget = atoi(argv[2]);
    }

    TradeoffOptimizer optimizer = TradeoffOptimizer(numDocs, fragAppBudget, alreadyChosen);
    optimizer.fillHeap();
    optimizer.runHeapSelectionAlg();
    optimizer.writeOutput(true);

    final = clock() - init;
    double timeInSeconds = (double)final / ((double)CLOCKS_PER_SEC);
    cerr << "Total time: " << timeInSeconds << endl;
}