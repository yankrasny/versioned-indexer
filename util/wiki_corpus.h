#ifndef WIKI_CORPUS_H_
#define WIKI_CORPUS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXID 11400000
#define DIR "/data/jhe/wiki_pos2/"
#define MAXSIZE 130939073

typedef struct
{
    unsigned wid, vid, pos;
}posting;


typedef struct
{
    unsigned id, vs;
}item;

typedef struct
{
    int rstart, vstart, len;
}copys;

class wiki_corpus
{
public:
    wiki_corpus(char* fn)
    {
        idmap = new int[MAXID];		
        file_buf = new int[MAXSIZE];
        memset(idmap, 0, sizeof(int)*MAXID);
        FILE* f = fopen(fn, "rb");

        int nread = fread(&total, sizeof(unsigned), 1, f);

        items = new item[total];

        nread = fread(items, sizeof(item), total, f);

        fclose(f);

        for ( int i = 0; i < total; i++)
            idmap[items[i].id] = items[i].vs;

        ptr = -1;
        ndoc = 0;

    }

    ~wiki_corpus()
    {
        delete[] idmap;
        delete[] items;
    }

    inline int get_version(int id)
    {
        return idmap[id];
    }

    bool next()
    {
        ptr++;
        if ( ptr >= total )
            return false;

        return true;
    }

    item current()
    {
        return items[ptr];
    }

    inline int get_current_pos(int* wcounts, posting* post_buf)
    {
        return get_pos(items[ptr].id, items[ptr].vs, wcounts, post_buf);
    }

    /***********************************************************
     * read all the posting for one document into buffer
     * post_buf - readin buf
     * wcount - the #distinct word per version
     * id - the document id
     * vs - the number of versions
     * @return value is the total number of postings in the file
     ***********************************************************/
    int get_pos(int id, int vs, int* wcounts, posting* post_buf)
    {
        char fn[256];
        sprintf(fn, "%s%d", DIR, id);
        //printf("%s\n", fn);

        FILE* fss = fopen(fn, "r");
        if ( NULL == fss)
        {
            perror(fn);
            return -1;
        }

        memset(file_buf, 0, MAXSIZE);
        int nread = fread(file_buf, sizeof(int), MAXSIZE, fss);

        fclose(fss);

        int wcount,wid,freq;
        int ptr = 0;
        int post_ptr = 0;


        for ( int i = 0; i < vs; i++)
        {
            wcount = file_buf[ptr++];
            wcounts[i] = 0;

            if ( wcount == 0){
                continue;
            }

            for ( int j = 0; j < wcount; j++)
            {	
                wid = file_buf[ptr++];
                freq = file_buf[ptr++];

                for ( int m = 0; m < freq; m++, ptr++)
                {
                    post_buf[post_ptr].wid = wid;
                    post_buf[post_ptr].vid = ndoc; 
                    post_buf[post_ptr++].pos = file_buf[ptr];	
                    wcounts[i]++; //increase the number of posting in version i
                }			

            }
            ndoc++;
        }


        return post_ptr;

    }

private:
    item* items;	
    int* idmap;
    int ptr;
    int total;
    int* file_buf;
    int ndoc;
};

#endif

