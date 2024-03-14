//
// File: device.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "fvf.h"
#include "rage/math/mtxv.h"
#include "am/system/threadinfo.h"

namespace rage
{
	class grcVertexBuffer;
	class grcIndexBuffer;
	struct grcVertexElement;
	struct grcVertexDeclaration;

	enum grcDrawMode
	{
		grcDrawPointList,
		grcDrawLineList,
		grcDrawLineStrip,
		grcDrawTriangleList,
		grcDrawTriangleStrip,
		grcDrawCount,
	};

	enum grcUsage
	{
		grcUsageDefault,
		grcUsageImmutable,
		grcUsageDynamic,
		grcUsageStage,
		grcUsageCount
	};

	enum grcCPUAccess
	{
		grcCPUNoAccess = 0x0,
		grcCPUWrite    = 0x10000L,
		grcCPURead     = 0x20000L,
	};

	enum grcBindFlags_
	{
		grcBindNone            = 0x0,
		grcBindVertexBuffer    = 0x1,
		grcBindIndexBuffer     = 0x2,
		grcBindConstantBuffer  = 0x4,
		grcBindShaderResource  = 0x8,
		grcBindStreamOutput    = 0x10,
		grcBindRenderTarget    = 0x20,
		grcBindDepthStencil    = 0x40,
		grcBindUnorderedAccess = 0x80,
	};
	using grcBindFlags = int;

	struct grcPoint { int x, y; };
	struct grcRect { int x1, y1, x2, y2; };

	enum grcLockFlags_
	{
		grcsRead          = 1 << 0,
		grcsWrite         = 1 << 1,
		grcsDoNotWait     = 1 << 2, // Will return a failed lock if unsuccessful
		grcsDiscard       = 1 << 3, // Discard contents of locked region entire buffer
		grcsNoOverwrite   = 1 << 4, // Indicates that the region locked should not be in use by the GPU
		grcsNoDirty       = 1 << 5, // Prevents driver from marking the region as dirty
		grcsAllowVRAMLock = 1 << 6,

		grcsReadWrite = grcsRead | grcsWrite,
	};
	using grcLockFlags = int;

	class grcDevice
	{
		static inline grcVertexDeclaration* sm_CurrentVertexDeclaration = nullptr;

	public:
		// Builds vertex input layout from given elements.
		static grcVertexDeclaration* CreateVertexDeclaration(const grcVertexElement* elements, u32 elementCount);
		// Same as CreateVertexDeclaration but uses D3D11_INPUT_ELEMENT_DESC
		static grcVertexDeclaration* CreateVertexDeclaration(const grcVertexElementDesc* elementDescs, u32 elementCount);

		static void SetDrawMode();
		static void SetIndexBuffer(grcIndexBuffer* buffer);
		static void DrawIndexedPrimitive(
			grcDrawMode drawMode,
			grcVertexDeclarationPtr vtxDeclaration,
			grcVertexBuffer* vtxBuffer, grcIndexBuffer* idxBuffer, u32 idxCount);

		static void SetWorldMtx(const Mat34V& mtx);

		static void GetScreenSize(u32& width, u32& height);
		
		enum MSAAModeEnum
		{
			MSAA_None = 0,
			MSAA_NonMaskAble = 1,
			MSAA_2 = 2,
			MSAA_4 = 4,
			MSAA_8 = 8,
		};
		struct MSAAMode { MSAAModeEnum Mode; };

		static MSAAMode GetMSAA();

		enum class Stereo_t
		{
			MONO = 0x0,
			STEREO = 0x1,
			AUTO = 0x2,
		};

		static bool IsRenderThread() { return rageam::ThreadInfo::GetInstance()->IsRenderThread(); }
		static void LockContext() { }
		static void UnlockContext() { }
	};

	// Extra draw functions

	void grcDrawLine();
}
