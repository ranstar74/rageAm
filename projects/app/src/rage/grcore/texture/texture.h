//
// File: texture.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "texturebase.h"
#include "rage/atl/conststring.h"
#include "rage/paging/resource.h"
#include "rage/paging/compiler/compiler.h"
#include "rage/paging/compiler/snapshotallocator.h"

namespace rage
{
	enum eTextureType
	{
		TEXTURE_DEFAULT,	// grcTextureDX11
		TEXTURE_CUSTOM,		// pVoid
		TEXTURE_REFERENCE,	// grcTextureReference
	};

	class grcTexture : public grcTextureBase
	{
	protected:
		u64				m_Unknown10;
		u64				m_Unknown18;
		u64				m_Unknown20;
		atConstString	m_Name;
		u16				m_RefCount;
		// eTextureType in low 2 bits and remaining are unknown flags
		u8				m_ResourceType;
		u8				m_LayerCount;
		pVoid			m_Resource;
		u32				m_Unknown40;
		u32				m_HandleIndex; // Unused in release build
	public:
		grcTexture()
		{
			m_Unknown10 = 0;
			m_Unknown18 = 0;
			m_Unknown20 = 0;

			m_RefCount = 1;
			m_LayerCount = 0;
			m_Resource = nullptr;

			m_ResourceType = 0 | 128; // grcTextureDX11

			m_Unknown40 = 0x20005a56; // TODO: Figure this out
			m_HandleIndex = 0;
		}

		grcTexture(const grcTexture& other) : grcTextureBase(other)
		{
			m_Unknown10 = other.m_Unknown10;
			m_Unknown18 = other.m_Unknown18;
			m_Unknown20 = other.m_Unknown20;
			m_Unknown40 = other.m_Unknown40;
			m_HandleIndex = other.m_HandleIndex;

			m_Name = other.m_Name;
			m_Resource = other.m_Resource;
			m_ResourceType = other.m_ResourceType;
			m_LayerCount = other.m_LayerCount;

			if (pgRscCompiler::GetCurrent())
			{
				m_RefCount = other.m_RefCount;
			}
			else
			{
				m_RefCount = 1;
			}
		}

		// ReSharper disable once CppPossiblyUninitializedMember
		grcTexture(const datResource& rsc)
		{

		}

		u32 GetHandleIndex() const override { return m_HandleIndex; }
		void SetHandleIndex(u32 value) override { m_HandleIndex = value; }

		pVoid GetResource() override { return m_Resource; }
		pVoid GetResource() const override { return m_Resource; }

		ConstString GetName() const
		{
			ConstString name = m_Name;
			if (!name) name = "None";
			return name;
		}
		void SetName(const char* name) { m_Name = name; }
	};
	static_assert(sizeof(grcTexture) == 0x48);
}
