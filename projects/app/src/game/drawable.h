#pragma once

#include "lightattr.h"
#include "am/graphics/render/context.h"
#include "am/ui/extensions.h"
#include "rage/framework/pool.h"
#include "rage/paging/template/array.h"
#include "rage/physics/inst.h"
#include "rage/physics/bounds/boundbase.h"
#include "rage/rmc/drawable.h"

class gtaDrawable : public rage::rmcDrawable
{
public: // TEST
	rage::pgArray<CLightAttr>	m_Lights;
	u64							m_UnknownC0;
	rage::phBoundPtr			m_Bound;

public:
	gtaDrawable() = default;

	// ReSharper disable once CppPossiblyUninitializedMember
	gtaDrawable(const rage::datResource& rsc) : rmcDrawable(rsc)
	{

	}

	gtaDrawable(const gtaDrawable& other) : rage::rmcDrawable(other)
	{

	}

	~gtaDrawable() override
	{

	}

	void SetBound(rage::phBound* bound) { m_Bound = bound; }
	rage::phBound* GetBound() const { return m_Bound.Get(); }

	void DrawDebug(const rage::Mat44V& mtx)
	{
		GRenderContext->DebugRender.RenderDrawable(this, mtx);
	}

	void Draw(const rage::Mat44V& mtx, rage::grcDrawBucketMask mask, rage::eDrawableLod lod) override
	{
		rmcDrawable::Draw(mtx, mask, lod);
		DrawDebug(mtx);
	}

	void DrawSkinned(const rage::Mat44V& mtx, u64 a3, rage::grcDrawBucketMask mask, rage::eDrawableLod lod) override
	{
		rmcDrawable::DrawSkinned(mtx, a3, mask, lod);
		DrawDebug(mtx);
	}
};
