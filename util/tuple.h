/*
 * tuple.h
 *
 *  Created on: Jan 11, 2010
 *      Author: jhe
 */

#ifndef TUPLE_H_
#define TUPLE_H_

#define MAX_LEN_NUMB	20
#define MAX_LEN_WORD	32
#define NB_CHECKPOINT	2000

#define MAX_POS			0xffffffff

typedef struct{
	unsigned docid;
	unsigned pos;
}post;

struct tuple
{
	unsigned word, doc_id, pos;
};

typedef struct
{
	long long offset;
	int freq, wid;
}index_item;

typedef struct
{
	char word[MAX_LEN_WORD+1];
	unsigned pos;
}p2;
#endif /* TUPLE_H_ */
