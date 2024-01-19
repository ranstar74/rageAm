//
// File: archetype.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"

namespace rage
{
	class rmcDrawable;
	class fwEntity;

	/**
	 * \brief Provides a way to apply entity instance data to drawable on rendering.
	 */
	class fwCustomShaderEffect
	{
		u32         m_Size : 16;
		u32         m_Type : 4;
		mutable u32 m_SharedDataInfo[2]; // TODO: dlSharedDataInfo

	public:
		fwCustomShaderEffect() = default;
		virtual ~fwCustomShaderEffect() = default;

		virtual void DeleteInstance() { delete this; }
		virtual void Update(fwEntity*) {}
		virtual void SetShaderVariables(rmcDrawable* drawable) = 0;		// BeforeDraw
		virtual void AddToDrawList(u32 modelIndex, bool execute) = 0;	// BeforeAddToDrawList
		virtual void AddToDrawListAfterDraw() {}						// AfterAddToDrawList
		virtual bool AddKnownRefs() { return false; }					// Add known refs on a newly-copied object
		virtual void RemoveKnownRefs() {}								// Remove known refs on a temporary copy that's about to die

		u32   Size() const { return m_Size; }
		u8    GetType() const { return m_Type; }
		auto& GetSharedDataOffset() const { return m_SharedDataInfo; }

		static constexpr int CSE_MAX_TYPES = 16;
	};

	class fwCustomShaderEffectBaseType
	{
	protected:
		u8 m_Type = 0;

	public:
		fwCustomShaderEffectBaseType() = default;
		virtual ~fwCustomShaderEffectBaseType() = default;
		virtual void RemoveRef() = 0;

		u8 GetType() const { return m_Type; }
	};
}
