//
// File: fvf.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "format.h"
#include "common/types.h"
#include "rage/atl/fixedarray.h"

#include <d3d11.h>

namespace rage
{
	// This all is quite over-engineered because game still ends up using reflected vertex format from the shader

	enum grcVertexSemantic
	{
		POSITION = 0,
		POSITIONT = 1,
		NORMAL = 2,
		BINORMAL = 3,
		TANGENT = 4,
		TEXCOORD = 5,
		BLENDWEIGHT = 6,
		BLENDINDICES = 7,
		COLOR = 8,
	};

	constexpr ConstString VertexSemanticName[] =
	{
		"POSITION",
		"POSITIONT",
		"NORMAL",
		"BINORMAL",
		"TANGENT",
		"TEXCOORD",
		"BLENDWEIGHT",
		"BLENDINDICES",
		"COLOR",
	};

	grcVertexSemantic GetVertexSemanticFromName(ConstString semanticName);

	struct grcVertexElement
	{
		u32					InputSlot;		// For multiple buffers, we just use default one - 0
		grcVertexSemantic	Semantic;
		u32					SemanticIndex;	// In TexCoord0, TexCoord1 - Indices are 0 and 1
		u32					DataSize;
		grcFormat			Format;
		u32					Unknown14;
		u32					Unknown18;
	};

	static constexpr u32 TEXCOORD_MAX = 8;	// 8 texcoord attributes
	static constexpr u32 TANGENT_MAX = 2;	// 2 tangent attributes
	static constexpr u32 COLOR_MAX = 2;		// 2 color attributes

	/**
	 * \brief Even more flexible than DirectX one! Flexible-Vertex-Format.
	 */
	struct grcFvf
	{
		// RAGE's flexible vertex format holds information about elements (POSITION, NORMAL, COLOR)
		// packed in 32 bits, very similar to D3D9 tech:
		// 
		// Format = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX0
		// 
		// But to make it to work on any graphics API we have to implement our own decoding of fvf that will
		// convert it to D3D11_INPUT_ELEMENT_DESC[] or similar
		// 
		// Decoding (unpacking) is very simple and done via iterating through all channel bits in grcFvf::Channels
		// and checking whether they're set to 1 (used) or 0 (unused).
		// 
		// Default channels bits are following: (originally they're declared in grcore/fvfchannels.h according to .dcl files in GTA IV)
		// 
		// CHANNEL NAME     CHANNEL BIT    FORMAT SHIFT
		//   POSITION       1 << 0         0
		//   BLENDWEIGHT    1 << 1         4
		//   BLENDINDICES   1 << 2         8
		//   NORMAL         1 << 3         12
		//   COLOR0         1 << 4         16
		//   COLOR1         1 << 5         20
		//   TEXCOORD0      1 << 6         24
		//   TEXCOORD1      1 << 7         28
		//   TEXCOORD2      1 << 8         32
		//   TEXCOORD3      1 << 9         36
		//   TEXCOORD4      1 << 10        40
		//   TEXCOORD5      1 << 11        44
		//   TEXCOORD6      1 << 12        48
		//   TEXCOORD7      1 << 13        52
		//   TANGENT0       1 << 14        56
		//   TANGENT1       1 << 15        60
		//   BINORMAL0      1 << 16        64
		//   BINORMAL1      1 << 17        68
		// 
		// Default channels are unpacked in: grcFvf::UnpackDefault function,
		// there's also alternative channel layout but it's used very rare.
		// 
		// As explained above, corresponding channel bits set to 1 in grcFvf::Channels define whether
		// channel is used or not.
		// 
		// After unpacking all input elements will be in ordered from lowest (least significant) to the highest
		// (most significant) bit.
		// 
		// Additionally, grcFormat (kind of DXGI_FORMAT) for every channel is defined by grcFvf::ChannelFormats
		// grcFormat has 16 items total, this fits into 1 byte (0-15).
		//	Note: BINORMAL0 & BINORMAL1 formats cannot be stored in ChannelFormats because they exceed 64 bits.
		//	those are apparently unused by any existing asset but channels still parsed.
		//
		// So ChannelFormats can be seen as array of formats where corresponding channel format is mapped by its bit multiplied by 4
		// As an example in order to retrieve format for TEXCOORD with semantic index 0:
		//	u8 bit = 1 << 6; // Channel bit in table above
		//	grcFormat format = (ChannelFormats >> (bit * 4)) 0xF;

		static constexpr int MAX_CHANNELS = 16; // Although format allows up to 32 (one per 32 bits), only 16 are allowed
		static constexpr u64 FORMAT_MASK = 0xF;

		u32		Channels;
		u16		Stride;
		bool	UseAlternativeChannels;
		u8		ChannelCount;
		u64		ChannelFormats;

		// ----- Decoding Utils -----

		void UnpackChannels(
			grcVertexElement* elements, u32& elementsStartIndex,
			u8 channelBit, u8 semanticCount,
			grcVertexSemantic semantic,
			grcFormat overrideFormat = GRC_FORMAT_INVALID) const;

		u32 UnpackAlternative(grcVertexElement* elements) const;
		u32 UnpackDefault(grcVertexElement* elements) const;
		// Gets vertex input elements from channel bits
		u32 Unpack(grcVertexElement* elements) const;

		// ----- Encoding Utils -----

		void SetChannel(u8 channelBit, bool used);
		void SetFormat(u8 channelBit, grcFormat fmt);
	};

	using grcVertexElementDesc = D3D11_INPUT_ELEMENT_DESC;

	/**
	 * \brief Holds device-specific (D3D11 in our case) vertex input layout.
	 */
	struct grcVertexDeclaration
	{
		static constexpr u32 MAX_CACHED_DECLS = 50;

		// Cached declaration (by vertex format)
		struct DeclPair
		{
			u32 HashKey;
			grcVertexDeclaration* Declaration;

			static bool SortCompare(const DeclPair& lhs, const DeclPair& rhs);
			static bool SearchCompare(const DeclPair& lhs, const u32& rhs);
		};
		static atFixedArray<DeclPair, MAX_CACHED_DECLS> sm_CachedDecls;

		u32 ElementCount;
		u32 RefCount;
		u32 Stride;
		grcVertexElementDesc Elements[1];

		~grcVertexDeclaration();

		// Computes element offset from beginning of the vertex in bytes
		u32 GetElementOffset(u32 index) const;

		void EncodeFvf(grcFvf& format) const;

		static void CleanUpCache(); // Must be called on shutdown
		static grcVertexDeclaration* CreateFromFvf(const grcFvf& format);
		static grcVertexDeclaration* BuildCachedDeclarator(const grcVertexElement* elements, u32 elementCount);
		static grcVertexDeclaration* Allocate(u32 elementCount);
	private:
		// Only ::Allocate is allowed to construct one! To prevent dumb mistakes.
		grcVertexDeclaration() = default;
	};
	using grcVertexDeclarationPtr = grcVertexDeclaration*;

	class grcEffect;

	/**
	 * \brief Helper for caching FVF of vertex declaration.
	 */
	struct grcVertexFormatInfo
	{
		grcVertexDeclarationPtr Decl;
		grcFvf Fvf;

		grcVertexFormatInfo() = default;

		grcVertexFormatInfo(grcVertexDeclarationPtr decl)
		{
			Decl = decl;
			Decl->EncodeFvf(Fvf);
		}

		grcVertexFormatInfo(grcVertexDeclarationPtr decl, grcFvf fvf)
		{
			Decl = decl;
			Fvf = fvf;
		}

		// Fills vertex declaration using 'draw' technique vertex shader
		void FromEffect(grcEffect* effect, bool skinned);
	};
}
