#ifndef TRADEOFF_HEAP_ENTRY_H
#define TRADEOFF_HEAP_ENTRY_H

#include <algorithm>
#include <vector>
#include <iostream>
#include "TradeoffHeap.h"

class TradeoffHeapEntry {
	unsigned docId;
	unsigned startIdx;
	unsigned endIdx;
	double priority;
	
	std::vector<TradeoffHeapEntry*> relatedEntries;

	TradeoffHeap* myHeap;
	int index; // the object must know where it is, so it can be found in O(1) inside the heap

public:
	TradeoffHeapEntry(unsigned docId, unsigned startIdx, unsigned endIdx, double priority, TradeoffHeap* myHeap, int index) 
		: docId(docId), startIdx(startIdx), endIdx(endIdx), priority(priority), myHeap(myHeap), index(index)
	{
		relatedEntries = std::vector<TradeoffHeapEntry*>();
	}

	TradeoffHeapEntry(const TradeoffHeapEntry& rhs)
	{
		this->index = rhs.index;
		this->priority = rhs.priority;
		
		this->docId = rhs.docId;
		this->startIdx = rhs.startIdx;
		this->endIdx = rhs.endIdx;
		
		this->myHeap = rhs.myHeap;

		// TODO wtf do we do with relatedEntries??
		relatedEntries = std::vector<TradeoffHeapEntry*>();
		for (auto it = rhs.getRelatedEntries().begin(); it != rhs.getRelatedEntries().end(); ++it) {
			TradeoffHeapEntry* entry;
			relatedEntries.push_back(entry);
		}
	}

	// Assignment Operator, data is all initialized
	// Define assignment as copying key and priority
	// The heap is the same for everyone, just leave it
	// Don't take the index from rhs either, just take its values
	TradeoffHeapEntry& operator=(const TradeoffHeapEntry& rhs)
	{
		if (this != &rhs) {
			this->docId = rhs.docId;
			this->startIdx = rhs.startIdx;
			this->endIdx = rhs.endIdx;

			this->priority = rhs.priority;

			this->relatedEntries = std::vector<TradeoffHeapEntry*>();

			for (auto it = rhs.getRelatedEntries().begin(); it != rhs.getRelatedEntries().end(); ++it) {
				TradeoffHeapEntry* entry;
				relatedEntries.push_back(entry);
			}
			rhs.getRelatedEntries().clear();
		}
		return *this;
	}
	
	// Don't release the mem for myHeap because others are pointing to it
	// Just make sure this object no longer points to it
	~TradeoffHeapEntry()
	{
		myHeap = NULL;
		// std::cerr << "Destructor for TradeoffHeapEntry[key = " << key << "]" << std::endl;
	}

	std::vector<TradeoffHeapEntry*> getRelatedEntries() const
	{
		return relatedEntries;
	}

	unsigned getStartIdx() const {
		return startIdx;
	}

	unsigned getEndIdx() const {
		return endIdx;
	}

	int getIndex() const {
		return index;
	}

	void setIndex(int index)
	{
		this->index = index;
	}

	double getPriority() const {
		return priority;
	}

	unsigned getDocId() {
		return docId;
	}

	TradeoffHeap* const getMyHeap()
	{
		return myHeap;
	}
};

#endif