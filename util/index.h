/*
 * index.h
 *
 *  Created on: Sep 3, 2009
 *      Author: jhe
 */

#ifndef INDEX_H_
#define INDEX_H_

typedef struct
{
	unsigned freq, off;
}lex;


class index_copy
{
public:
	index_copy(int restore, int type)
	{
		id_ptr = 0;
		copy_ptr = 0;
		total_copy = 0;
		lex_buf = new lex[LEX_SIZE];
		memset(lex_buf, 0, sizeof(lex)*LEX_SIZE);
		copy_buf = new copys[BUF_SIZE];
		
		this->type = type;
		sprintf(fn, "meta/copy_list_%d", type);
		sprintf(fn2, "meta/copy.idx_%d", type);
		
		if ( restore == 0 )
		{
			remove(fn);
			remove(fn2);
		}
		else
		{
			restores();
		}

		
	}

	~index_copy()
	{
		
	}

	int add_index(copys* copy, int len, int id)
	{
		lex_buf[id].freq = len;
		lex_buf[id].off = total_copy;

		//printf("add copy: rstart:%d\tvstart:%d\tlen:%d\n", copy[0].rstart, copy[0].vstart, copy[0].len);
		//copy_buf[copy_ptr].rstart = copy[0].rstart;
		//copy_buf[copy_ptr].vstart = copy[0].vstart;
		//copy_buf[copy_ptr++].len = copy[0].len;

		for ( int i = 0; i < len; i++, total_copy++)
		{
			copy_buf[copy_ptr].rstart = copy[i].rstart; //- copy[i-1].rstart;
			copy_buf[copy_ptr].vstart = copy[i].vstart;//-( copy[i-1].vstart + copy[i-1].len);
			copy_buf[copy_ptr++].len = copy[i].len;

			if ( copy_ptr >= BUF_SIZE )
			{
				flush();
			}

		}
		return 0;

	}


	void flush()
	{
		fout = fopen(fn, "a+");
		if ( copy_ptr > 0)
		{
			fwrite(copy_buf, sizeof(copys), copy_ptr, fout);
			copy_ptr = 0;
		}
		fclose(fout);
		FILE* idx = fopen(fn2, "w");
		fwrite(&total_copy, sizeof(unsigned), 1, idx);
		fwrite(lex_buf, sizeof(lex), LEX_SIZE, idx);
		fclose(idx);
	}

	void restores()
	{
		FILE* idx = fopen(fn2, "r");
		int nread = fread(&total_copy, sizeof(unsigned), 1, idx);
		nread = fread(lex_buf, sizeof(lex), LEX_SIZE, idx);
		fclose(idx);
	}


private:
	unsigned id_ptr;
	lex* lex_buf;
	copys* copy_buf;
	unsigned total_copy;
	int copy_ptr;

	const static int BUF_SIZE = 100000000;
	const static int LEX_SIZE = 85000000;

	FILE* fout;
	int type;
	char fn[64];
	char fn2[64];

};

#endif /* INDEX_H_ */
