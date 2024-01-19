//
// File: archetype.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/atl/hashstring.h"
#include "rage/math/vecv.h"
#include "rage/paging/ref.h"
#include "rage/physics/archetype.h"

namespace rage
{
	struct fwDynamicArchetypeComponent
	{
		Vec4V              AiAvoidancePoint;
		atHashString       ExpressionName;
		pgPtr<phArchetype> Physics;
		s16                ClipDictionarySlot;
		s16                BlendShapeFileSlot;
		s16                ExpressionDictionarySlot;
		u16                PhysicsRefCount;
		s8                 PoseMatcherFileSlot;
		s8                 PoseMatcherProneSlot;
		s16                CreatureMetadataSlot;
		u16                AutoStartAnim        : 1;
		u16                HasPhysicsInDrawable : 1;

		fwDynamicArchetypeComponent();

		void SetClipDictionary(const atHashString& cdName)
		{
			// TODO: ...
		}

		// TODO: This class should use pool allocator
	};
}
