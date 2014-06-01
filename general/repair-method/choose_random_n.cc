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

void initAlreadyChosen(set<unsigned>& alreadyChosen, unsigned numDocs, unsigned maxDocId) {
    // fill the set with numDocs random unsigned numbers from 0 to maxDocId
    for (size_t i = 0; i < numDocs; i++) {
        int theRand;
        int range = maxDocId;
        theRand = rand() % range;
        while (alreadyChosen.find(theRand) != alreadyChosen.end()) {
            theRand = rand() % range;
        }
        alreadyChosen.insert(theRand);
    }
}


int main(int argc, char**argv)
{
	unsigned totalDocs = 240179;
    unsigned docCount = totalDocs;
    if (argc > 1)
    {
        // use this to test a small number of docs
        docCount = atoi(argv[1]);
    }
    srand (time(NULL));
    set<unsigned> alreadyChosen = set<unsigned>();
    initAlreadyChosen(alreadyChosen, docCount, totalDocs);

    ofstream myfile;
	myfile.open ("docids.txt");

    for (auto it = alreadyChosen.begin(); it != alreadyChosen.end(); it++) {
		myfile << (*it) << "\n";
	}
	myfile.close();
}