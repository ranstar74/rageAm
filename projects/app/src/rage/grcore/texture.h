//
// File: texture.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/atl/conststring.h"
#include "rage/paging/base.h"
#include "rage/paging/place.h"
#include "rage/atl/bitset.h"
#include "device.h"

#include <d3d11.h>

namespace rage
{
	class grcImage;
	class grcEffect;
	struct grcEffectVar;

	static constexpr ConstString TEXTURE_DEFAULT_NAME = "none";

	struct grcTextureLock
	{
		int   MipLevel;
		pVoid Base;
		int   Pitch;
		int   BitsPerPixel;
		int   Width;
		int   Height;
		int   RawFormat;
		int   Layer;
	};

	enum grcTextureCreateType
	{
		grcsTextureCreate_NeitherReadNorWrite,
		grcsTextureCreate_ReadWriteHasStaging,
		grcsTextureCreate_Write,
		grcsTextureCreate_ReadWriteDynamic,
		grcsTextureCreate_WriteDynamic,
		grcsTextureCreate_WriteOnlyFromRTDynamic,
	};

	enum grcsTextureSyncType
	{
		grcsTextureSync_Mutex,
		grcsTextureSync_DirtySemaphore,
		grcsTextureSync_CopyData,
		grcsTextureSync_None,
	};

	enum eTextureSwizzle
	{
		TEXTURE_SWIZZLE_R,
		TEXTURE_SWIZZLE_G,
		TEXTURE_SWIZZLE_B,
		TEXTURE_SWIZZLE_A,
		TEXTURE_SWIZZLE_0,
		TEXTURE_SWIZZLE_1,
	};

	enum eTextureConversionFlags
	{
		TEXTURE_CONVERSION_FLAG_PROCESSED         = 0x80,
		TEXTURE_CONVERSION_FLAG_SKIPPED           = 0x40,
		TEXTURE_CONVERSION_FLAG_FAILED_PROCESSING = 0x20,
		TEXTURE_CONVERSION_FLAG_INVALID_METADATA  = 0x10,
		TEXTURE_CONVERSION_FLAG_OPTIMISED_DXT     = 0x8,
		TEXTURE_CONVERSION_FLAG_NO_PROCESSING     = 0x60,
		TEXTURE_CONVERSION_FLAG_MASK              = 0xF8,
	};

	enum eTextureType
	{
		TEXTURE_NORMAL,
		TEXTURE_RENDER_TARGET,
		TEXTURE_REFERENCE,
	};

	enum eTextureTemplateType
	{
		TEXTURE_TEMPLATE_TYPE_UNKNOWN,
		TEXTURE_TEMPLATE_TYPE_DEFAULT,
		TEXTURE_TEMPLATE_TYPE_TERRAIN,
		TEXTURE_TEMPLATE_TYPE_CLOUDDENSITY,
		TEXTURE_TEMPLATE_TYPE_CLOUDNORMAL,
		TEXTURE_TEMPLATE_TYPE_CABL5,
		TEXTURE_TEMPLATE_TYPE_FENC6,
		TEXTURE_TEMPLATE_TYPE_ENVEFF,
		TEXTURE_TEMPLATE_TYPE_SCRIPT,
		TEXTURE_TEMPLATE_TYPE_WATERFLOW,
		TEXTURE_TEMPLATE_TYPE_WATERFOAM,
		TEXTURE_TEMPLATE_TYPE_WATERFOG,
		TEXTURE_TEMPLATE_TYPE_WATEROCEAN,
		TEXTURE_TEMPLATE_TYPE_WATER,
		TEXTURE_TEMPLATE_TYPE_FOAMOPACITY,
		TEXTURE_TEMPLATE_TYPE_FOAM,
		TEXTURE_TEMPLATE_TYPE_DIFFUSEMIPSHARPEN,
		TEXTURE_TEMPLATE_TYPE_DIFFUSEDETAIL,
		TEXTURE_TEMPLATE_TYPE_DIFFUSEDARK,
		TEXTURE_TEMPLATE_TYPE_DIFFUSEALPHAOPAQUE,
		TEXTURE_TEMPLATE_TYPE_DIFFUSE,
		TEXTURE_TEMPLATE_TYPE_DETAIL,
		TEXTURE_TEMPLATE_TYPE_NORMAL,
		TEXTURE_TEMPLATE_TYPE_SPECULAR,
		TEXTURE_TEMPLATE_TYPE_EMISSIVE,
		TEXTURE_TEMPLATE_TYPE_TINTPALETTE,
		TEXTURE_TEMPLATE_TYPE_SKIPPROCESSING,
		TEXTURE_TEMPLATE_TYPE_DONOTOPTIMIZE,
		TEXTURE_TEMPLATE_TYPE_TEST,
		TEXTURE_TEMPLATE_TYPE_COUNT,
		TEXTURE_TEMPLATE_TYPE_MASK            = 0x1F,
		TEXTURE_TEMPLATE_TYPE_FLAG_NOT_HALF   = 0x20,
		TEXTURE_TEMPLATE_TYPE_FLAG_HD_SPLIT   = 0x40,
		TEXTURE_TEMPLATE_TYPE_FLAG_FULL       = 0x80,
		TEXTURE_TEMPLATE_TYPE_FLAG_MAPS_HALF  = 0x100,
		TEXTURE_TEMPLATE_TYPE_FLAG_MASK       = 0x1E0,
		TEXTURE_TEMPLATE_TYPE_VERSION_MASK    = 0xE00,
		TEXTURE_TEMPLATE_TYPE_VERSION_SHIFT   = 0x9,
		TEXTURE_TEMPLATE_TYPE_VERSION_CURRENT = 0x200,
	};

	class grcTexture : public pgBase
	{
		friend class grcTextureReference;

		static constexpr u8  TYPE_MASK = 0x3;
		static constexpr u8  CONVERSION_FLAGS_MASK = 0xF8;
		static constexpr u32 PHYSICAL_SIZE_MASK = 0x7FFFF80;

	protected:

		// Unused (leftover cellGcmTexture PS3 type)
		struct
		{
			u8  Format;
			u8  Mipmap;
			u8  Dimension;
			u8  Cubemap;
			u32 Remap;
			u16 Width;
			u16 Height;
			u16 Depth;
			u8  Location;
			u8	Padding;
			u32 Pitch;
			u32 Offset;
		}						 m_Texture;
		atConstString			 m_Name;
		u16						 m_RefCount;
		u8						 m_ResourceTypeAndConversionFlags;
		u8						 m_LayerCount;
		// Native type is void* but since in PC version there is only DX11
		// we use smart pointer, for simplicity
		amComPtr<ID3D11Resource> m_CachedTexture;
		u32						 m_PhysicalSizeAndTemplateType;
		u32						 m_HandleIndex;

	public:
		grcTexture(eTextureType type, ConstString name = TEXTURE_DEFAULT_NAME);
		grcTexture(const grcTexture& other);
		grcTexture(const datResource& rsc);

		IMPLEMENT_REF_COUNTER(grcTexture);

		static grcTexture* Place(const datResource& rsc, grcTexture* that);
		static grcTexture* Snapshot(pgSnapshotAllocator* allocator, grcTexture* from);

		// Keeps pixel data (backing store) pointer after deserializing resource,
		// can be used to export to DDS or back to #TD
		// NOTE: Resource must be built with keeping physical memory segment!
		static inline thread_local bool tl_KeepResourcePixelData = false;

		eTextureType GetResourceType() const { return eTextureType(m_ResourceTypeAndConversionFlags & TYPE_MASK); }
		int  GetConversionFlags() const { return m_ResourceTypeAndConversionFlags & CONVERSION_FLAGS_MASK; }
		void SetConversionFlags(int flags)
		{
			m_ResourceTypeAndConversionFlags &= ~CONVERSION_FLAGS_MASK;
			m_ResourceTypeAndConversionFlags |= flags;
		}
		int GetTemplateType() const
		{
			// Template type + flags are stored in 2 parts: 0x7F and 0xF800'0000; Version is not stored
			return (int)(m_PhysicalSizeAndTemplateType & 0x7F) | (int)((m_PhysicalSizeAndTemplateType >> 27 & 0x1F) << 7);
		}
		void SetTemplateTypename(int templateTypeAndFlags)
		{
			m_PhysicalSizeAndTemplateType &= PHYSICAL_SIZE_MASK;
			m_PhysicalSizeAndTemplateType |= templateTypeAndFlags & 0x7F;
			m_PhysicalSizeAndTemplateType |= (templateTypeAndFlags >> 7 & 0x1F) << 27;
		}

		// NOTE: This is used as debug metrics but modded resources don't account this variable,
		// it is either set to invalid value or just zero
		u32  GetPhysicalSize() const { return m_PhysicalSizeAndTemplateType & PHYSICAL_SIZE_MASK; }
		void SetPhysicalSize(u32 size)
		{
			m_PhysicalSizeAndTemplateType &= ~PHYSICAL_SIZE_MASK;
			m_PhysicalSizeAndTemplateType |= size & PHYSICAL_SIZE_MASK;
		}

		ConstString GetName() const
		{
			if (m_Name) return m_Name;
			return TEXTURE_DEFAULT_NAME; // Native textures often have name set to NULL
		}
		void SetName(ConstString name) { m_Name = name; }

		u32  GetHandleIndex() const override { return m_HandleIndex; }
		void SetHandleIndex(u32 index) override { m_HandleIndex = index; }

#if APP_BUILD_2699_16_RELEASE_NO_OPT
		ConstString GetDebugName() override { return GetName(); }
#endif

		// Ancient PS2 leftover, ensures that texture is loaded
		virtual void Download() const { }
		virtual int  GetDownloadSize() const { return 0; }

		virtual u16 GetWidth() const = 0;
		virtual u16 GetHeight() const = 0;
		virtual u16 GetDepth() const = 0;
		virtual u8  GetMipMapCount() const = 0;
		virtual u8  GetArraySize() const { return 1; }
		virtual u8  GetBitsPerPixel() const = 0;

		virtual bool IsValid() const { return true; }

		virtual grcDevice::MSAAMode GetMSAAMode(int* msaaMode) const { return { grcDevice::MSAA_None }; }

		virtual bool IsGammaEnabled() const { return false; }
		virtual void SetGammaEnabled(bool on) {}

		virtual int  GetTextureSignedMask() const { return 0; }
		virtual void SetTextureSignedMask(int mask) {}

		virtual const grcTexture* GetReference() const { return this; }
		virtual grcTexture*       GetReference() { return this; }

		virtual bool IsSRGB() const = 0;

		virtual const void* GetTexturePtr() const { return m_CachedTexture.Get(); }
		virtual void*       GetTexturePtr() { return m_CachedTexture.Get(); }
		virtual void*       GetTextureView() const = 0;

		virtual void UpdateGPUCopy() {}

		virtual u32 GetImageFormat() const = 0;

		virtual bool LockRect(int layer, int mipLevel, grcTextureLock& lock, grcLockFlags lockFlags) = 0;
		virtual void UnlockRect(grcTextureLock& lock) = 0;

		virtual void SwizzleTexture2D(grcTextureLock& lock) {}
		virtual void UnswizzleTexture2D(grcTextureLock& lock) {}
		virtual void Resize(int newWidth, int newHeight) {}
		virtual void SetEffectInfo(grcEffect* effect, grcEffectVar* effectVar) {}
		virtual void AddWidgets(pVoid bank) {}
		virtual bool Save() { return true; }

		virtual bool Copy(const pVoid src, u32 width, u32 height) { return true; }
		virtual bool Copy(const grcTexture* src, int dstSliceIndex, int dstMipIndex, int srcSliceIndex, int srcMipIndex) { return true; }
		virtual bool Copy(const grcImage* src) { return true; }
		virtual bool Copy2D(const pVoid src, const grcPoint& srcDim, const grcRect& dstRect, grcTextureLock& lock, int mipLevel) { return true; }
		virtual bool Copy2D(const pVoid src, int format, u32 width, u32 height, u32 mipCount) { return true; }

		virtual void Tile(int n) {}

		virtual atFixedBitSet<8, u8> FindUsedChannels() const = 0;

		virtual void SetTextureSwizzle(eTextureSwizzle r, eTextureSwizzle g, eTextureSwizzle b, eTextureSwizzle a, bool) {}
		virtual void GetTextureSizzle(eTextureSwizzle& r, eTextureSwizzle& g, eTextureSwizzle& b, eTextureSwizzle& a)
		{
			r = TEXTURE_SWIZZLE_R;
			g = TEXTURE_SWIZZLE_G;
			b = TEXTURE_SWIZZLE_B;
			a = TEXTURE_SWIZZLE_A;
		}
	};
}
