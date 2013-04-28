/*
 * =====================================================================================
 *
 *       Filename:  check_size.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  01/11/2012 10:31:33 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Jinru He (jhe), mikehe1117@gmail.com
 *        Company:  NYU-Poly
 *
 * =====================================================================================
 */
#include	<cstdlib>
#include	<cstring>
#include	<cstdio>
#include	<vector>
#include	<map>
#include	<fstream>
#include	<iostream>
#include	<iterator>

using namespace std;

struct item
{
    float a;
    int b, c, d;
};

istream& operator >>(istream& in, item& its)
{
    in >> its.a >> its.b>>its.c>>its.d;
    return in;
}

int main(int argc, char** argv)
{
    FILE* f = fopen("/data/jhe/wiki_access/numv", "rb");
    int size = 0;
    fread(&size, sizeof(unsigned), 1, f);
    unsigned* buf = new unsigned[size];
    fread(buf, sizeof(unsigned), size, f);
    fclose(f);

    char fn[256];
    vector<item> i1, i2;
    ofstream fout("rets");
    for ( int i = 0; i < 1000; ++i)
    {
        memset(fn, 0, 256);
        sprintf(fn, "../MultiLevelGeneral/test/%d.3", i);
        ifstream fin(fn);
        if (fin.fail())
            continue;
        istream_iterator<item> data_begin(fin);
        istream_iterator<item> data_end;
        i1.assign(data_begin, data_end);
        fin.close();
        memset(fn, 0, 256);
        sprintf(fn, "test/%d.3", i);
        ifstream fin2(fn);
        istream_iterator<item> data_begin2(fin2);
        istream_iterator<item> data_end2;
        i2.assign(data_begin2, data_end2);
        fin2.close();

        int last1 = i1.size()-1;
        int last2 = i2.size()-1;

        if (i1[0].b < i2[0].b)
        {
            fout <<i<<'\t'<<buf[i]<<'\t'<< i1[0].b <<'\t'<<i1[0].d<<'\t'<<i2[0].b<<'\t'<<i2[0].d<<endl;

        }
    }
    fout.close();
    return 0;
}

