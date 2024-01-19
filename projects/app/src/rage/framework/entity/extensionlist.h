//
// File: extensionlist.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/atl/bitset.h"
#include "rage/dat/base.h"

namespace rage
{
	class fwArchetype;
	class fwEntity;

	struct fwExtensionDef : datBase
	{
		atHashValue Name;
	};

	using fwExtensionDefPtr = pgUPtr<fwExtensionDef>;
	using fwExtensionDefs = pgArray<fwExtensionDefPtr>;

	/**
	 * \brief A way of extending a class.
	 */
	class fwExtension
	{
	public:
		virtual ~fwExtension()
		{
		}

		virtual void InitEntityExtensionFromDefinition(const fwExtensionDef* definition, fwEntity* entity)
		{
		}

		virtual void InitArchetypeExtensionFromDefinition(const fwExtensionDef* definition, fwArchetype* archetype)
		{
		}
	};

	/**
	 * \brief List for storing multiple entity extensions.
	 */
	class fwExtensionList
	{
		// Maximum number of extensions that can be looked up quickly through a bit check
		static constexpr int MAX_BIT_MANAGED_EXTENSIONS = 32;

		struct LinkedListNode
		{
			fwExtension*    m_Data;
			LinkedListNode* m_Next;
		};

		LinkedListNode*                                m_Head = nullptr;
		atFixedBitSet<MAX_BIT_MANAGED_EXTENSIONS, u32> m_AvailableExtensions;

	public:
		fwExtensionList() = default;

		void DestroyAll() {}
	};
}
