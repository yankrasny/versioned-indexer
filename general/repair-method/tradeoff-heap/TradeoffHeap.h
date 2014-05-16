#pragma once

#ifndef TRADEOFF_HEAP_H
#define TRADEOFF_HEAP_H

#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>
#include <assert.h>

/*
Goal: to be able to refer to a specific entry from an outside data structure, in O(1)
That's why we store the index in TradeoffHeapEntry
*/

class TradeoffHeapEntry;

class TradeoffHeap
{
	friend std::ostream& operator<<(std::ostream& os, const TradeoffHeap& rhs);

private:
	std::vector<TradeoffHeapEntry*> heap;
public:
	TradeoffHeap()
	{
		heap = std::vector<TradeoffHeapEntry*>();
	}

	int getSize() {
		return heap.size();
	}

	bool empty() const;

	void printHeap() const;

	TradeoffHeapEntry* getAtIndex(unsigned pos) const;

	TradeoffHeapEntry* getMax() const;

	TradeoffHeapEntry* getBack() const;

	TradeoffHeapEntry* extractMax();

	TradeoffHeapEntry* insert(TradeoffHeapEntry* entry);

	int deleteAtIndex(int pos);

	TradeoffHeapEntry* extractAtIndex(int pos);

	int heapifyUp(int pos);

	int heapifyDown(int pos);

	void cleanup();

	// Big 3
	// IndexedHeap(const IndexedHeap& rhs);

	// IndexedHeap& operator=(const IndexedHeap& rhs);

	// ~IndexedHeap();

	void checkValid();
};

class TradeoffHeapTest
{
private:
	void runTest(unsigned long long n);
public:
	TradeoffHeapTest(unsigned long long numElements);
};

#endif
