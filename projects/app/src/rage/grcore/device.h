//
// File: device.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "fvf.h"

#include <d3d11.h>
#include <xmmintrin.h>

#include "rage/math/mtxv.h"

namespace rage
{
	class grcVertexBuffer;
	class grcIndexBuffer;
	struct grcVertexElement;
	struct grcVertexDeclaration;

	enum grcDrawMode
	{
		GRC_DRAW_POINTLIST,
		GRC_DRAW_LINELIST,
		GRC_DRAW_LINESTRIP,
		GRC_DRAW_TRIANGLELIST,
		GRC_DRAW_TRIANGLESTRIP,
		GRC_DRAW_INDEXED,
	};

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
			MSAA_None = 0x0,
			MSAA_NonMaskAble = 0x1,
			MSAA_2 = 0x2,
			MSAA_4 = 0x4,
			MSAA_8 = 0x8,
			MSAA_2xMS = 0x2,
			MSAA_Centered4xMS = 0x4,
			MSAA_Rotated4xMS = 0x8,
		};
		struct MSAAMode { MSAAModeEnum Mode; };

		enum class Stereo_t
		{
			MONO = 0x0,
			STEREO = 0x1,
			AUTO = 0x2,
		};
	};

	// Extra draw functions

	void grcDrawLine();
}
