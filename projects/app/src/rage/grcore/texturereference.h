//
// File: texture.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "texture.h"
#include "rage/paging/ref.h"

namespace rage
{
	/**
	 * \brief Used in compiled resources to reference external texture.
	 */
	class grcTextureReference : public grcTexture
	{
		pgPtr<grcTexture> m_Reference;

	public:
		grcTextureReference(ConstString name, const pgPtr<grcTexture>& ref);
		grcTextureReference(const datResource& rsc);

		const grcTexture* GetReference() const override { return m_Reference ? m_Reference.Get() : nullptr; }
		grcTexture*		  GetReference() override	    { return m_Reference ? m_Reference.Get() : nullptr; }
		void			  SetReference(const pgPtr<grcTexture>& ref);

		const void* GetTexturePtr() const override { return GetReference() ? GetReference()->GetTexturePtr() : nullptr; }
		void*		GetTexturePtr() override	   { return GetReference() ? GetReference()->GetTexturePtr() : nullptr; }
		void* GetTextureView() const override	   { return GetReference() ? GetReference()->GetTextureView() : nullptr; }

		u16  GetWidth() const override			{ return GetReference() ? GetReference()->GetWidth() : 0; }
		u16  GetHeight() const override			{ return GetReference() ? GetReference()->GetHeight() : 0; }
		u16  GetDepth() const override			{ return GetReference() ? GetReference()->GetDepth() : 0; }
		u8   GetMipMapCount() const override	{ return GetReference() ? GetReference()->GetMipMapCount() : 0; }
		u8   GetArraySize() const override		{ return GetReference() ? GetReference()->GetArraySize() : 0; }
		u8   GetBitsPerPixel() const override	{ return GetReference() ? GetReference()->GetBitsPerPixel() : 0; }
		u32  GetImageFormat() const override	{ return GetReference() ? GetReference()->GetImageFormat() : 0; }
		bool IsSRGB() const override			{ return GetReference() ? GetReference()->IsSRGB() : false; }
		bool IsValid() const override			{ return GetReference() ? GetReference()->IsValid() : false; }
		void Download() const override			{ if (GetReference()) GetReference()->Download(); }
		int  GetDownloadSize() const override	{ return GetReference() ? GetReference()->GetDownloadSize() : 0; }
		bool Copy(const grcImage* img) override { return GetReference() ? GetReference()->Copy(img) : false; }

		bool LockRect(int layer, int mipLevel, grcTextureLock& lock, grcLockFlags lockFlags) override
		{
			return GetReference() ? GetReference()->LockRect(layer, mipLevel, lock, lockFlags) : false;
		}
		void UnlockRect(grcTextureLock& lock) override { if (GetReference()) GetReference()->UnlockRect(lock); }

		atFixedBitSet<8, u8> FindUsedChannels() const override { return {}; }
	};
}
