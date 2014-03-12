/*
 * count_docs.cc
 *
 *  Created on: March 9, 2014
 *  Author: Yan Krasny
 */

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
using namespace std;

int main(int argc, char**argv)
{
    FILE* fnumv = fopen("/data/jhe/wiki_access/numv", "rb");
    int doccount; // number of documents -yan
    fread(&doccount, sizeof(unsigned), 1, fnumv);
    cerr << "Num docs: " << doccount << endl;
}