//
// File: drawable.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "lightattr.h"
#include "am/ui/extensions.h"
#include "rage/paging/template/array.h"
#include "rage/physics/bounds/boundbase.h"
#include "rage/rmc/drawable.h"

class gtaDrawable : public rage::rmcDrawable
{
	rage::pgArray<CLightAttr>		m_Lights;
	rage::pgUPtr<rage::pgArray<u8>> m_TintData;
	rage::phBoundPtr				m_Bound;

public:
	gtaDrawable() = default;
	// ReSharper disable once CppPossiblyUninitializedMember
	gtaDrawable(const rage::datResource& rsc) : rmcDrawable(rsc) {}
	gtaDrawable(const gtaDrawable& other) : rmcDrawable(other) {}
	~gtaDrawable() override {}

	u16 GetLightCount() const { return m_Lights.GetSize(); }
	CLightAttr* GetLight(u16 index) { return &m_Lights[index]; }
	CLightAttr& AddLight()
	{
		CLightAttr& light = m_Lights.Construct();
		light.FixupVft();
		return light;
	}
	rage::atArray<CLightAttr>& GetLightArray() { return *(rage::atArray<CLightAttr>*)&m_Lights; }

	void SetBound(const rage::phBoundPtr& bound) { m_Bound = bound; }
	const rage::phBoundPtr& GetBound() const { return m_Bound; }

	void Draw(const rage::Mat34V& mtx, rage::grcRenderMask mask, rage::eDrawableLod lod) override
	{
		rmcDrawable::Draw(mtx, mask, lod);
	}

	void DrawSkinned(const rage::Mat34V& mtx, u64 a3, rage::grcRenderMask mask, rage::eDrawableLod lod) override
	{
		rmcDrawable::DrawSkinned(mtx, a3, mask, lod);
	}
};

using gtaDrawablePtr = rage::pgCountedPtr<gtaDrawable>;
