#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_DOCS 240179

typedef struct
{
    int total;
    int numDistinctFrags;
    int numPostings;
    float wsize;
} linfo;

int comparep(const void* a1, const void* a2)
{
    linfo* l1 = (linfo*)a1;
    linfo* l2 = (linfo*)a2;

    int ret =  l1->numPostings - l2->numPostings;
    int ret2 = l1->total - l2->total;

    if (ret2 == 0)
        return ret;
    else
        return ret2;
}

int main(int argc, char** argv)
{
    linfo* lbuf = new linfo[NUM_DOCS];
    int ptr = 0;
    unsigned* lens = new unsigned[NUM_DOCS];
    memset(lens, 0, NUM_DOCS * sizeof(unsigned));
    char fn[256];
    memset(fn, 0, 256);
    FILE* f2 = fopen("finfos", "wb");
    FILE* f3 = fopen("/data/jhe/wiki_access/numv", "rb");
    int size;
    fread(&size, sizeof(unsigned), 1, f3);
    unsigned* lens2 = new unsigned[size];
    fread(lens2, sizeof(unsigned), size, f3);
    fclose(f3);

    FILE* f4 = fopen("/data/jhe/wiki_access/word_size", "rb");
    unsigned wlens;
    fread(&wlens, sizeof(unsigned), 1, f4);
    unsigned* dlens = new unsigned[wlens];
    fread(dlens, sizeof(unsigned), wlens, f4);
    fclose(f4);
    int wsize = 0;
    FILE* fexcept = fopen("except", "w");

    for (int i = 0; i < atoi(argv[1]); i++)
    {
        memset(fn, 0, 256);
        sprintf(fn, "test/%d.2", i);
        FILE* f1 = fopen(fn, "r");
        ptr = 0;
        printf("processing:%d...", i);
        if (lens2[i] > 1)
        {
            bool change = false; 
            while (fscanf(f1, "%f\t%d\t%d\t%d\n", &lbuf[ptr].wsize, &lbuf[ptr].numDistinctFrags,  &lbuf[ptr].total, &lbuf[ptr].numPostings)>0)
            {
                if (ptr > 0 && lbuf[ptr].numPostings != lbuf[ptr-1].numPostings)
                {
                    change = true;
                }
                ptr++;
            }
            fclose(f1);
            memset(fn, 0, 256);
            sprintf(fn, "test/convert-%d", i);
            f1 = fopen(fn, "w");
            qsort(lbuf, ptr, sizeof(linfo), comparep);
            int ptr1 = 0;
            int ptr2 = 1;
            while (ptr2 < ptr)
            {
                while (ptr2 < ptr && lbuf[ptr2].numPostings >= lbuf[ptr1].numPostings)
                    ptr2++;

                if (ptr2 >= ptr)
                    break;

                ptr1++;
                lbuf[ptr1] = lbuf[ptr2];
                ptr2++;
            }
            ptr1++;
            
            for (int j = 0; j < ptr1; j++)
            {
                fprintf(f1, "%.1f\t%d\t%d\t%d\n", lbuf[j].wsize, lbuf[j].total, lbuf[j].numDistinctFrags, lbuf[j].numPostings);
            }
            
            fclose(f1);
            fwrite(lbuf, sizeof(linfo), ptr1, f2);
            lens[i] = ptr1;
            if (change == false)
            {
                fprintf(fexcept, "%d\t%d\n", i, ptr);
            }
        }
        else
        {
            fclose(f1);
            lens[i] = 1;
            lbuf[0].wsize = dlens[wsize];
            lbuf[0].total = 1;
            lbuf[0].numDistinctFrags  = 1;
            lbuf[0].numPostings = dlens[wsize];
            fwrite(lbuf, sizeof(linfo), 1, f2);
        }
        wsize += lens2[i];
        printf("done!\n");
    }

    fclose(fexcept);
    size = atoi(argv[1]);
    FILE* fsize = fopen("linfos", "wb");
    fwrite(&size,  sizeof(unsigned), 1, fsize);
    fwrite(lens, sizeof(unsigned), size , fsize);
    fclose(fsize);
    fclose(f2);
    return 0;
}