//
// File: example.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/paging/place.h"
#include "rage/paging/template/array.h"
#include "rage/crypto/joaat.h"
#include "rage/paging/base.h"

namespace rage
{
	/**
	 * \brief This is an example class derived from pgBase made for testing purposes.
	 */
	class pgExample : public pgBase
	{
		//atRef<pgExample> m_Parent;
		pgArray<ImmutableString> m_Items;
		u32 m_RefCount;
	public:
		pgExample(/*rage::atRef<pgExample> parent = nullptr*/)
		{
			m_Items.Add("GTA IV");
			m_Items.Add("GTA V");
			m_Items.Add("GTA VI");

			//m_Parent = parent;
			m_RefCount = 0;
		}

		// TODO: Macro to disable warning
		pgExample(const datResource& rsc)
			: m_Items(rsc)
		{

		}

		pgExample(pgExample&& other) noexcept
		{
			std::swap(m_Items, other.m_Items);
			std::swap(m_RefCount, other.m_RefCount);
		}

		pgExample(const pgExample& other) :
			pgBase(other), m_Items(other.m_Items) // All items copied to datAllocator in pgArray 
		{
			// Copy constructor is used by resource compiler to re-allocate resource on datAllocator,
			// which is equivalent to making snapshot of resource state

			m_RefCount = 0;

			// We need to copy reference count too because we're doing full snapshot of resource
			// Additionally see how rage::pgArray handles allocation.
			if (pgRscCompiler::GetCurrent())
			{
				m_RefCount = other.m_RefCount;
			}
		}

		void DisplayItems() const
		{
			AM_TRACEF("pgExample::DisplayItems() ->");
			for (ImmutableString item : m_Items)
				AM_TRACEF(" - %s", item);
		}

		IMPLEMENT_PLACE_INLINE(pgExample);
		IMPLEMENT_REF_COUNTER(pgExample);
	};
}
