/*
 * =====================================================================================
 *
 *       Filename:  test.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  01/11/2012 07:49:43 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Jinru He (jhe), mikehe1117@gmail.com
 *        Company:  NYU-Poly
 *
 * =====================================================================================
 */

#include	<vector>
#include	<iterator>
#include	<fstream>
#include	<iostream>

using namespace std;

#include	<stdlib.h>

    int
main ( int argc, char *argv[] )
{
    ifstream fin("options");
    istream_iterator<int> data_begin(fin);
    istream_iterator<int> data_end;
    vector<int> vbuf(data_begin, data_end);

    cout << vbuf.size()<<endl;
    ostream_iterator<int> iout(cout, "\t");
    copy(vbuf.begin(), vbuf.end(), iout);
    cout <<endl;
    fin.close();
    return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
