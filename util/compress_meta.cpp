#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <map>

using namespace std;


int main(int argc, char** argv)
{

	int* buf = new int[102400];
	unsigned char* buf2 = new unsigned char[1000000];
	FILE* f = fopen("meta", "r");
	FILE* fout = fopen("meta_info", "wb");
	int id, vs;
	while (!feof(f))
	{
		fscanf(f, "%d\t%d\t", &id, &vs);
		for ( int i = 0; i < vs; i++){
			fscanf(f, "%d,", &buf[i]);
			//if ( i > 0 )
			//	buf[i]-=buf[i-1];
		}

		
		fscanf(f, "\n");
		//fwrite(&id, sizeof(unsigned), 1, fout);
		//fwrite(&vs, sizeof(unsigned), 1, fout);
		fwrite(buf, sizeof(unsigned), vs, fout);
	}

	fclose(f);
	fclose(fout);
	return 0;
}
