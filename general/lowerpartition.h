/*
 * =====================================================================================
 *
 *       Filename:  lowerpartition.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  01/10/2012 01:39:54 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Jinru He (jhe), mikehe1117@gmail.com
 *        Company:  NYU-Poly
 *
 * =====================================================================================
 */
#include	<cstdio>
#include	<cstdlib>
#include	<cmath>
#include	<algorithm>
#include	<fstream>
#include	<iostream>

using namespace std;

#ifndef  B
#define	B 10			/*  */
#endif
#define MAXLENGTH 2905680

struct block
{
    int start;
    int end;
};

struct disblock
{
    int diff;
    int pos;
};

struct DisblockFunctor
{
    inline bool operator()(const disblock& d1, const disblock& d2)
    {
        return abs(d2.diff) < abs(d1.diff);
    }
}compareD;

struct DisblockFunctor2
{
    inline bool operator()(const disblock& d1, const disblock& d2)
    {
        return d1.pos < d2.pos;
    }
}compareD2;

class CutDocument2
{
private:
    int baselayer;
    disblock* dbuf;
    unsigned char* sbuf;
    int threshold;
    int maxblock_threshold;
    int* start2;
public:
    CutDocument2(int para, int max_block, int pLayer)
    {
        baselayer = pLayer;
        dbuf = new disblock[MAXLENGTH];
        sbuf = new unsigned char[MAXLENGTH];
        start2 = new int[MAXLENGTH];
        threshold = para;
        maxblock_threshold = max_block;
    }
    CutDocument2(int para, int max_block)
    {
        dbuf = new disblock[MAXLENGTH];
        sbuf = new unsigned char[MAXLENGTH];
        start2 = new int[MAXLENGTH];
        threshold = para;
        maxblock_threshold = max_block;
    }

    void setBaseLayer(int pLayer)
    {
        baselayer = pLayer;
    }

    bool isOK(int len, int pos)
    {
        int start = pos-B>0?pos-B:0;
        int end = pos+B>len?len:pos+B;
        for ( int i = start; i < end; ++i)
            if (sbuf[i] == 1)
                return false;

        return true;
    }

    int cut_hash(int* mhbuf,int*hbuf, int startpos, int len, int wsize, int* start)
    {
        int num = 0;
        for ( int i = startpos+B; i < startpos+len-B; i++)
        {
            if (mhbuf[i]>= wsize && hbuf[i]>1) 
            {
                start[num++] = i;
            }
        }
        return num;
    }

    int cut(int* mhbuf, int* hbuf, int len, int wsize, int* start)
    {
        int i;
        for ( i = 0; i < len - B; ++i)
        {
            dbuf[i].diff = mhbuf[i] - mhbuf[i+1];
            dbuf[i].pos = i;
            sbuf[i] = 0;
        }

        for ( int j = i; j < len; ++j)
            sbuf[i] = 0;

        sort(dbuf, dbuf+i, compareD);
        start2[0] = 0;
        int num = 1;
        for ( int j = 0; j < i; ++j)
        {
            if ( abs(dbuf[j].diff) <= threshold)
                break;
            if ( dbuf[j].diff > 0)
            {
                int pos = dbuf[j].pos + B;
                if (isOK(len, pos))
                {
                    start2[num++] = dbuf[j].pos+B;
                    sbuf[pos] = 1;
                }
            }
            else
            {
                int pos = dbuf[j].pos +1;
                if (isOK(len, pos))
                {
                    start2[num++] = dbuf[j].pos+1;
                    sbuf[pos]=1;
                }
            }
        }

        sort(start2, start2 + num);
        start[0] = start2[0];
        int ptr = 1;
        for ( i = 1; i < num; ++i)
        {
            if (start2[i] - start2[i-1] > maxblock_threshold)
            {
                int lens = cut_hash(hbuf, mhbuf, start2[i-1], start2[i]-start2[i-1], B, &start[ptr]);
                ptr+=lens;
            }
            start[ptr++] = start2[i];
        }

        if (len - start2[i-1] > maxblock_threshold)
        {
            int lens = cut_hash(hbuf, mhbuf, start2[i-1], len-start2[i-1], B, &start[ptr]);
            ptr += lens;
        }
        return ptr;
    }

    int cut3(block* binfo, int len, int count)
    {
        int interval = (count<baselayer?1:2);
        int ptr1 = 0;
        int ptr2 = interval;
        while ( ptr2 < len)
        {
            binfo[ptr1].end = binfo[ptr2].end;
            ptr1++;
            if ( ptr2+interval >= len)
                break;

            binfo[ptr1].start = binfo[ptr2].start;
            ptr2 += interval;
        }

        return ptr1;
    }
};

class CutDocument4
{
private:
    int baselayer;
    disblock* dbuf;
    int*      start2;
    int threshold;
    int maxblock_threshold;
    unsigned char* sbuf;
public:
    CutDocument4(int para, int maxblock, int pLayer)
    {
        baselayer = pLayer;
        dbuf = new disblock[MAXLENGTH];
        start2 = new int[MAXLENGTH];
        sbuf = new unsigned char[MAXLENGTH];
        threshold = para;
        maxblock_threshold = maxblock;
    }
    CutDocument4(int para, int maxblock)
    {
        dbuf = new disblock[MAXLENGTH];
        start2 = new int[MAXLENGTH];
        sbuf = new unsigned char[MAXLENGTH];
        threshold = para;
        maxblock_threshold = maxblock;
    }

    bool isOK(int len, int pos)
    {
        int start = pos-B>0?pos-B:0;
        int end = pos+B>len?len:pos+B;
        for ( int i = start; i < end; ++i)
            if (sbuf[i] == 1)
                return false;
        return true;
    }

    int cut_hash(int* mhbuf,int*hbuf, int startpos, int len, int wsize, int* start)
    {
        int num = 0;
        for ( int i = startpos+B; i < startpos+len-B; i++)
        {
            if (mhbuf[i]>= wsize && hbuf[i]>1) 
            {
                start[num++] = i;
            }
        }
        return num;
    }

    int cut(int* mhbuf, int* hbuf, int len, int wsize, int* start)
    {
        int i;
        for ( i = 0; i < len - B; ++i)
        {
            int minf = mhbuf[i]<mhbuf[i+1]?mhbuf[i]:mhbuf[i+1];
            dbuf[i].diff = static_cast<float>(mhbuf[i] - mhbuf[i+1])/minf;
            dbuf[i].pos = i;
            sbuf[i] = 0;
        }

        for ( int j = i; j < len; ++j)
            sbuf[i] = 0;

        sort(dbuf, dbuf+i, compareD);
        start2[0] = 0;
        int num = 1;
        for ( int j = 0; j < i; ++j)
        {
            if ( abs(dbuf[j].diff) <= threshold)
                break;
            if ( dbuf[j].diff > 0)
            {
                int pos = dbuf[j].pos + B;
                if (isOK(len, pos))
                {
                    start2[num++] = dbuf[j].pos+B;
                    sbuf[pos] = 1;
                }
            }
            else
            {
                int pos = dbuf[j].pos +1;
                if (isOK(len, pos))
                {
                    start2[num++] = dbuf[j].pos+1;
                    sbuf[pos]=1;
                }
            }
        }

        sort(start2, start2 + num);
        start[0] = start2[0];
        int ptr = 1;
        for ( i = 1; i < num; ++i)
        {
            if (start2[i] - start2[i-1] > maxblock_threshold)
            {
                int lens = cut_hash(hbuf, mhbuf, start2[i-1], start2[i]-start2[i-1], B, &start[ptr]);
                ptr+=lens;
            }
            start[ptr++] = start2[i];
        }

        if (len - start2[i-1] > maxblock_threshold)
        {
            int lens = cut_hash(hbuf, mhbuf, start2[i-1], len-start2[i-1], B, &start[ptr]);
            ptr += lens;
        }
        return ptr;
    }

    int cut3(block* binfo, int len, int count)
    {
        int interval = (count<baselayer?1:2);
        int ptr1 = 0;
        int ptr2 = interval;
        while ( ptr2 < len)
        {
            binfo[ptr1].end = binfo[ptr2].end;
            ptr1++;
            if ( ptr2+interval >= len)
                break;

            binfo[ptr1].start = binfo[ptr2].start;
            ptr2 += interval;
        }

        return ptr1;
    }
};

