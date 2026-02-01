// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"

/**
 * @brief Pure C++ virtual template interface, designed to help handle different data types safely.
 * All classes intending to use THeap must implement TIHeapItem under HeapInterface.h.
 * @note 100% implemented.
 */
template<typename InType>
class GAMEJAM2026_API TIHeapItem
{
public:
	using T = InType; // Alias - in regard to typename and UE naming standards
	
	virtual ~TIHeapItem() = default;
	
	virtual int32 GetHeapIndex() const = 0;
	virtual void SetHeapIndex(int32 Index) = 0;

	// Classes need to implement their own sorting function to help compare which class instances should have a
	// higher priority. 
	virtual int32 CompareWith(const T& OtherNode) const = 0;
};

template<typename InType>
concept HeapItemType = std::is_base_of_v<TIHeapItem<InType>, InType>;

// Concept to ensure types used with THeap implements TIHeapItem, non-pointer and pointer types
// Retrieved from AI Assistant, Claude Sonnet 4.5.
/* template<typename InType>
concept HeapItemType = []() {
	if constexpr (std::is_pointer_v<InType>)
	{
		// For pointer types like FNavNodeInternal*, check if FNavNodeInternal implements TIHeapItem<FNavNodeInternal*>
		return std::is_base_of_v<TIHeapItem<InType>, std::remove_pointer_t<InType>>;
	}
	else
	{
		// For value types like FSpawnEntry, check if FSpawnEntry implements TIHeapItem<FSpawnEntry>
		return std::is_base_of_v<TIHeapItem<InType>, InType>;
	}
}();*/

// Concept to ensure types used with THeap implements TIHeapItem, supporting non-pointer and pointer types
// Retrieved from ChatGPT 5.
/*
template<typename InType>
concept HeapItemType = requires {
	[]<typename U>() {
		if constexpr (std::is_pointer_v<U>)
		{
			static_assert(std::is_base_of_v<TIHeapItem<std::remove_pointer_t<U>>, std::remove_pointer_t<U>>,
				"Pointer type does not point to a TIHeapItem subclass");
		}
		else
		{
			static_assert(std::is_base_of_v<TIHeapItem<InType>, InType>,
				"Type does not implement TIHeapItem");
		}
	}.template operator()<InType>();
};
*/