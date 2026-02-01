/// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "HeapInterface.h"

/**
 * @brief Pure C++ style heap (only using .h file), but using TArrays from UE.
 * @details Anyone using THeap must use the TIHeapItem template interface under HeapInterface.h.
 * A min-heap implementation for efficient priority queue operations in pathfinding and other algorithms.
 * @note 95% done with implementation, only testing remains.
 */
template <typename InType>
	requires HeapItemType<InType> /// concept, helping ensure all classes using THeap makes use of TIHeapItem
class GAMEJAM2026_API THeap
{
public:
	using T = InType; /// Alias - in regard to typename and UE naming standards

private:
	/// Storing pointers to handle both pure C++ classes and UObjects (pointers are great)
	TArray<T*> Items;
	int32 CurrentItemCount = 0;
	int32 MaxHeapSize = 0;

	/// Our own swap function, take indices (heap positions) and swap pointers there, and update the heap indices.
	void Swap(T* ItemA, T* ItemB)
	{
		int32 IndexA = ItemA->GetHeapIndex();
		int32 IndexB = ItemB->GetHeapIndex();

		Items[IndexA] = ItemB;
		Items[IndexB] = ItemA;

		ItemA->SetHeapIndex(IndexB);
		ItemB->SetHeapIndex(IndexA);
	}

	/// Binary Tree Sorting - Take the current node index (e.g. starts at 0) and look for 2 possible children (1, 2).
	/// E.g. with node index 13, it has possible children with indices 27 (left child) and 28 (right child).
	void SortDown(T* Item)
	{
		while (true)
		{
			int32 ChildIndexLeft = Item->GetHeapIndex() * 2 + 1;
			/// Account for the other half of the tree, then +1 (left child).
			int32 ChildIndexRight = ChildIndexLeft + 1; /// the right child comes after the left child.
			int32 HighestPriorityIndex = 0;

			//// Check if we have a left child, then right child (if no left child, we have no right child)
			if (ChildIndexLeft < CurrentItemCount)
			{
				HighestPriorityIndex = ChildIndexLeft;

				if (ChildIndexRight < CurrentItemCount && Items[ChildIndexLeft]->CompareWith(*Items[ChildIndexRight]) <
					0)
				{
					/// If the ChildIndexRight has a higher priority than ChildIndexLeft (lower cost), swap the two.
					HighestPriorityIndex = ChildIndexRight;
				}

				/// Compare the item with the child, returning 1, 0 or -1, returning positive if this node has higher priority (lower cost)
				if (Item->CompareWith(*Items[HighestPriorityIndex]) < 0)
				{
					Swap(Item, Items[HighestPriorityIndex]);
				}
				else return;
			}
			else return;
		}
	}

	void SortUp(T* Item)
	{
		int32 Index = Item->GetHeapIndex();
		int32 ParentIndex = (Index - 1) / 2;

		while (Index > 0)
		{
			T* Parent = Items[ParentIndex];

			/// Compare the item with the parent, returning 1, 0 or -1, returning positive if this node has higher priority (lower cost)
			if (Item->CompareWith(*Parent) > 0)
			{
				Swap(Item, Parent);
				Index = Item->GetHeapIndex();
				ParentIndex = (Index - 1) / 2;
			}
			else break;
		}
	}

public:
	/// Default constructor which sets the max heap size to a default of 1000.
	THeap() : MaxHeapSize(10000)
	{
		Items.SetNum(10000);
	}

	/// explicit keyword needed for single-argument constructors to remove ambiguity down-the-line
	explicit THeap(int32 InMaxHeapSize) : MaxHeapSize(InMaxHeapSize)
	{
		/// Using SetNum as the preferred and safe method when setting array size using TArrays.
		Items.SetNumZeroed(MaxHeapSize);
	}

	/// Initialize or resize the heap after construction - currently unused.
	void Initialize(const int32 MaxSize)
	{
		MaxHeapSize = MaxSize;
		Items.SetNumZeroed(MaxHeapSize);
		CurrentItemCount = 0;
	}

	/// Clear the heap without deallocating memory
	void Empty()
	{
		// Only touch valid, used items
		for (int32 i = 0; i < CurrentItemCount; i++)
		{
			if (Items[i])
			{
				Items[i]->SetHeapIndex(-1);
			}
		}
		CurrentItemCount = 0;
	}

	void ClearAll()
	{
		Empty();
		Items.Empty(); /// The important bit!
		CurrentItemCount = 0;
		MaxHeapSize = 0;
	}

	/// After figuring out max items in the heap, we go from 0 to the max size and use that as indices for each item
	void Add(T* Item)
	{
		checkf(Item != nullptr, TEXT("THeap cannot call Add() on a null pointer!"));
		check(CurrentItemCount < MaxHeapSize)
		
		checkf(Item != nullptr, TEXT("Add: Item is null!"));
		checkf(Item->GetHeapIndex() == -1,
			TEXT("Add: Tried to Add() item with heap index %d already!"),
			Item->GetHeapIndex());

		int32 Index = CurrentItemCount;

		Items[Index] = Item;
		Item->SetHeapIndex(Index);

		CurrentItemCount++;

		SortUp(Item);
	}

	/// Remove the first item in the heap and move the last item to the top of the heap
	T* RemoveFirst()
	{
		check(!IsEmpty());

		T* First = Items[0];

		CurrentItemCount--;
		Items[0] = Items[CurrentItemCount];

		if (Items[0])
		{
			Items[0]->SetHeapIndex(0);
			SortDown(Items[0]);
		}
		
		// Null out the old last slot to avoid ghost pointers
		Items[CurrentItemCount] = nullptr;
		
		First->SetHeapIndex(-1);
		return First;
	}

	void UpdateItem(T* Item)
	{
		checkf(Item != nullptr, TEXT("UpdateItem: Item is null!"));
		checkf(Item->GetHeapIndex() >= 0, 
			TEXT("UpdateItem: Item has invalid heap index %d"), 
			Item->GetHeapIndex());
		checkf(Item->GetHeapIndex() < CurrentItemCount,
			TEXT("UpdateItem: HeapIndex %d >= CurrentItemCount %d"),
			Item->GetHeapIndex(), CurrentItemCount);
		
		SortUp(Item);
	}

	bool Contains(T* Item) const
	{
		int32 Index = Item->GetHeapIndex();
		
		/// return true if index bigger or equal to 0, lower than CurrentItemCount and if index is current index in Items.
		return Index >= 0 && Index < CurrentItemCount && Items[Index] == Item;
	}


	/// Returns the current number of items in the THeap.
	/// Made to use the same API pattern as TArray (easier to use Num()).
	FORCEINLINE int32 Num() const { return CurrentItemCount; }
	/// Returns true if the THeap has 0 items.
	FORCEINLINE bool IsEmpty() const { return CurrentItemCount == 0; }
	/// Returns cuz MaxHeapSize.
	FORCEINLINE int32 GetMaxSize() const { return MaxHeapSize; }
	/// If the heap size needs to be expanded without reallocating the memory.
	FORCEINLINE void Reserve(int32 NewMax) { Items.Reserve(NewMax); }
};
