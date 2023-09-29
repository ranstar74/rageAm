//
// File: map.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/paging/datResource.h"

namespace rage
{
	// TODO: Use atArray as base

	// TODO: Is this atMap? or atSet?
	// Looks more like set

	template<typename T>
	class pgMap
	{
		struct Bucket
		{
			u32 HashKey;
			T* Value;
			Bucket* Next;
		};

		Bucket** m_Buckets;
		u16 m_Capacity; // 11 - closest prime number
		u16 m_Size;
	public:
		pgMap() {}

		pgMap(const datResource& rsc,
			void placeBucket(const datResource&, const Bucket*),
			T* placeValue(const datResource&, T*))
		{
			rsc.Fixup(m_Buckets);

			for (u16 i = 0; i < m_Capacity; i++)
			{
				rsc.Fixup(m_Buckets[i]);

				Bucket* nodeIt = m_Buckets[i];
				while (nodeIt)
				{
					rsc.Fixup(nodeIt->Next);

					if (placeBucket)
						placeBucket(rsc, nodeIt);
					if (placeValue)
						nodeIt->Value = placeValue(rsc, nodeIt->Value);

					nodeIt = nodeIt->Next;
				}
			}
		}

		u16 GetCapacity() const { return m_Capacity; }
		u16 GetSize() const { return m_Size; }

		T* Find(u32 hashKey)
		{
			u32 index = hashKey % m_Capacity;

			Bucket* bucket = m_Buckets[index];
			while (bucket->HashKey != hashKey)
				bucket = bucket->Next;

			if (!bucket) return nullptr;
			return bucket->Value;
		}
	};
}
