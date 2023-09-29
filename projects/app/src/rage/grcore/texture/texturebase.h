//
// File: texturebase.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "textureinfo.h"
#include "rage/paging/base.h"

#include "common/types.h"

namespace rage
{
	// This is a pure virtual base of rage::grcTexture,
	// this class doesn't exist in native implementation but we need it to 'inject'
	// it in game as native one. Most of functions are unknown, there's a lot null subs.
	// They're either debug only or platform specific, in any case, we don't need them.
	// So they're marked as private.
	// Can anyone suggest better way to do this?

	class grcTextureBase : public pgBase
	{
	private:
		virtual bool Function3() { return false; }
		virtual bool Function4() { return false; }

	public:
		virtual u16 GetWidth() const = 0;
		virtual u16 GetHeight() const = 0;
		virtual u16 GetDepth() const = 0;
		virtual u8 GetMipLevels() const = 0;

	private:
		virtual void Function9() {}

	public:
		virtual u8 GetBitsPerPixel() const = 0;

	private:
		virtual bool Function11() { return true; }
		virtual u32 Function12(u32& a1) { a1 = 0; return a1; }
		virtual bool Function13() { return false; }
		virtual bool Function14() { return false; }
		virtual bool Function15() { return false; }
		virtual bool Function16() { return false; }
		virtual grcTextureBase* Function17() { return this; }
		virtual grcTextureBase* Function18() { return this; }

	protected:
		virtual bool GetUnk19() = 0;

		virtual pVoid GetResource() = 0;
		virtual pVoid GetResource() const = 0;

	public:
		virtual void* GetResourceView() const = 0;

	private:
		virtual bool Function23() { return false; }

	protected:
		virtual u32 ConvertUnkFormat() { return 0; }

	public:
		virtual grcTextureInfo GetTextureInfo() = 0;

	private:
		virtual bool Function26() { return false; }
		virtual bool Function27() { return false; }
		virtual bool Function28() { return false; }
		virtual bool Function29() { return false; }
		virtual bool Function30() { return false; }
		virtual bool Function31() { return false; }
		virtual bool Function32() { return false; }
		virtual bool Function33() { return false; }
		virtual bool Function34() { return false; }
		virtual bool Function35() { return false; }
		virtual bool Function36() { return false; }
		virtual bool Function37() { return false; }
		virtual bool Function38() { return false; }

	protected:
		virtual u32 GetUnkFlags(u32& flags) = 0;

	private:
		virtual bool Function40() { return false; }
		virtual bool Function41() { return false; }
	};
}
