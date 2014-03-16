/*
 * heap.h
 *
 *  Created on: May 12, 2011
 *      Author: jhe
 */

#ifndef HEAP_H_
#define HEAP_H_


#endif /* HEAP_H_ */

typedef struct
{
	int ptr;
	double score;
} hpost;

class heap
{
private:
	int* key_pos;
	hpost* Elements;
	int Size;
	void update_key(const hpost& X, int idx){
		if ( idx > Size)
		{
			return;
		}
		int i = idx;
		int Child;

		if (X.score > Elements[idx].score ){
			//move up
			for (; Elements[i/2].score < X.score; i/=2){
				key_pos[Elements[i/2].ptr] = i;
				Elements[i] = Elements[i/2];

			}
			key_pos[X.ptr] = i;
			Elements[i] = X;
		}

		else if (X.score < Elements[idx].score )
		{
			//move down
			for (; i*2 <= Size; i = Child ){
					Child = i*2;
					if ( Child != Size && Elements[Child].score < Elements[Child+1].score)
						Child++;
					if ( X.score < Elements[Child].score){
						int idx = Elements[Child].ptr;
						key_pos[idx] = i;
						Elements[i] = Elements[Child];
					}
					else
						break;

			}
			key_pos[X.ptr] = i;
			Elements[i] = X;
		}
	}
public:
	void init(int maxNum)
	{
		key_pos = new int[maxNum];
		Elements = new hpost[maxNum];
		Size = 0;
		Elements[0].score = 999999;
		Elements[0].ptr = 0;
	}
	
	void init()
	{
		Size = 0;
		Elements[0].score = 999999;
		Elements[0].ptr = 0;
	}

	heap(int maxNUM)
	{
		key_pos = new int[maxNUM];
		Elements = new hpost[maxNUM];
		Size = 0;
		Elements[0].score = 999999;
		Elements[0].ptr = 0;
	}

	bool IsEmpty()
	{
		return Size == 0;
	}
	~heap()
	{

	}

	int size()
	{
		return Size;
	}

	void push(const hpost& p)
	{
		int i;
		int id;
		for ( i = ++Size; Elements[i/2].score < p.score; i/=2){
			int idx = Elements[i/2].ptr;
			key_pos[idx] = i;
			Elements[i] = Elements[i/2];
		}
	    Elements[ i ] = p;
		id = p.ptr;
		key_pos[id] = i;
	}

	void pop()
	{
		int i, Child;
		int id;

		if (IsEmpty()) {
			printf("Heap is already empty!\n");
			exit(0);
		}
		id = Elements[1].ptr;
		key_pos[id] = -1;
		hpost& LastElement = Elements[Size--];
		for (i = 1; i * 2 <= Size; i = Child) {
			Child = i * 2;

			if ( Child != Size && Elements[Child].score < Elements[Child+1].score)
				Child++;
			if ( Elements[Child].score > LastElement.score){
				Elements[ i ] = Elements[ Child ];
				id = Elements[i].ptr;
				key_pos[id] = i;
			}
			else
				break;
		}

		Elements[ i ] = LastElement;
		id = Elements[i].ptr;
		key_pos[id] = i;
		return;
	}

	void UpdateKey(int key, double value)
	{
		int idx = key_pos[key];
		if ( idx != -1){
			hpost p;
			p.ptr = key;
			p.score = value;
			update_key(p, idx);
		}
	}

	void deleteKey(int key)
	{
		double score = Elements[1].score+1;
		UpdateKey(key, score);
		pop();
	}

	hpost& top()
	{
		return Elements[1];
	}
};
