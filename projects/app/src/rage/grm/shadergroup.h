//
// File: shadergroup.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/dat/base.h"
#include "rage/grcore/effect/effect.h"
#include "rage/grcore/texture/texturedict.h"
#include "rage/paging/resource.h"

namespace rage
{
	/**
	 * \brief Although it's called 'Shader', the actual meaning is 'Material'.
	 */
	class grmShader : public grcInstanceData
	{
	public:
		using grcInstanceData::grcInstanceData;

		// TODO: Draw functions

		// Returns num of passes
		u16 BeginDraw(u32 drawType = 0, bool forceRestoreState = false, fxHandle_t techniqueHandle = 0) const
		{
			static u16(*fn)(const grmShader*, u32, bool, fxHandle_t) =
				gmAddress::Scan("8B 05 ?? ?? ?? ?? 4C 63 DA 83").To<decltype(fn)>();
			return fn(this, drawType, forceRestoreState, techniqueHandle);
		}

		void EndDraw() const
		{
			static void(*fn)(const grmShader*) =
				gmAddress::Scan("E8 ?? ?? ?? ?? 45 01 6E 24").GetCall().To<decltype(fn)>();
			return fn(this);
		}

		void BeginPass(u16 passIndex)
		{
			m_Effect.Ptr->BeginPass(passIndex, this);
		}

		void EndPass() const
		{
			m_Effect.Ptr->EndPass();
		}
	};

	/**
	 * \brief Material Group.
	 */
	class grmShaderGroup : public datBase
	{
		pgUPtr<grcTextureDictionary>  m_EmbedTextures;
		pgUPtrArray<grmShader, false> m_Shaders;

		u64		m_Unknown20;
		u16		m_Unknown28;
		u16		m_Unknown2A;
		u32		m_Unknown2C;
		// In rmcDrawable shader group is allocated with shaders using sysMemContainer,
		// for unknown reason (could be obfuscation) it is stored divided by 16
		// All allocations are multiples of 16 so division is done without remainder
		u16		m_ContainerBlockSize;
		bool	m_HasInstancedShader;
		u32		m_Unknown34;
		u64		m_Unknown38;

		void ScanForInstancedShaders();

	public:
		grmShaderGroup() = default;
		grmShaderGroup(const datResource& rsc);
		grmShaderGroup(const grmShaderGroup& other) { Copy(other); }

		void Snapshot();

		void Copy(const grmShaderGroup& other);

		grmShader* GetShader(u16 index) const { return m_Shaders[index].Get(); }
		u16 GetShaderCount() const { return m_Shaders.GetSize(); }

		// Shader has to be allocated on heap, memory lifetime will be maintained by this group
		void AddShader(grmShader* shader);
		// Note: After removing shader it cannot be accessed anymore!
		void RemoveShader(const grmShader* shader);
		void RemoveShaderAt(u16 index);

		grcTextureDictionary* GetEmbedTextureDictionary() const { return m_EmbedTextures.Get(); }
		void SetEmbedTextureDict(grcTextureDictionary* dict) { m_EmbedTextures = dict; }

		// Those two are used by rmcDrawable to re-allocate shader group in memory container

		// Allocates shader array and shaders
		void PreAllocateContainer(grmShaderGroup* containerInst) const;
		// Copies embed dictionary and clones shaders, those don't go into container
		void CopyToContainer(grmShaderGroup* containerInst) const;

		u32 GetContainerBlockSize() const { return static_cast<u32>(m_ContainerBlockSize) * 16; }
		void SetContainerBlockSize(u32 size) { m_ContainerBlockSize = static_cast<u16>(size / 16); }
	};
}
