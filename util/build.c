#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXI 240179
typedef struct
{
	unsigned id, vs;
}item;


int main(int argc, char** argv)
{
	item* its = (item*)malloc(sizeof(item)*MAXI);
	int ptr =0;
	int id, vs;
	FILE* fs = fopen("url.idx3", "r");

	char line[1024];
	

	while (!feof(fs))
	{
		memset(line, 0, 1024);
		fgets(line, 1024, fs);

		char* a = strtok(line, "\t\n");
		if ( NULL == a)
			continue;

		id = atoi(a);
		char* b = strtok(NULL, "\t\n");
		if ( NULL == b)
			continue;

		char* c = strtok(NULL, "\t\n");
		if ( NULL == c)
			continue;
		vs = atoi(c);

		its[ptr].id = id;
		its[ptr++].vs = vs;
		 
	}
 
	fclose(fs);

	FILE* fout = fopen("url.idx", "wb");
	fwrite(&ptr, sizeof(unsigned), 1, fout);
	fwrite(its, sizeof(item), ptr, fout);
	fclose(fout);

	return 0;
}
