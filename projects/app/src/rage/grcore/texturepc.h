// File: texturepc.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "texture.h"

namespace rage
{
	// Attempts to retrieve grcTexture pointer from resource private data
	grcTexture* GetTextureFromResourcePointer(ID3D11Resource* resource);

	struct grcTextureDX11_ExtraData
	{
		u32                      SyncType  : 2;
		u32                      LockFlags : 2;
		amComPtr<ID3D11Resource> StagingTexture;
		HANDLE                   Mutex;
	};

	enum ImageType : u8
	{
		IMAGE_TYPE_STANDARD,
		IMAGE_TUBE_CUBE,
		IMAGE_TYPE_DEPTH,
		IMAGE_TYPE_VOLUME,
	};

	class grcTexturePC : public grcTexture
	{
	protected:

		struct grcDX11InfoBits
		{
			u8 IsSRGB           : 1;
			u8 ReadWriteAccess  : 3;
			u8 OwnsBackingStore : 1; // TRUE: Virtual; FALSE: Physical
			u8 HasBeenDeleted   : 1;
			u8 Dynamic          : 1;
			u8 Dirty            : 1;
		};
		
		u32								   m_ExtraFlags;
		u32								   m_ExtraFlagsPadding;
		u16								   m_Width;
		u16								   m_Height;
		u16								   m_Depth;
		u16								   m_Stride;
		// In compiled resources stored as DX9, converted to DXGI on deserializing
		u32								   m_Format;
		ImageType						   m_ImageType;
		u8								   m_MipCount;
		u8								   m_CutMipLevels;
		grcDX11InfoBits					   m_InfoBits;
		grcTexturePC*					   m_Next;
		grcTexturePC*					   m_Previous;
		// Texture pixels, each mip map is stored next to each other without padding
		pVoid							   m_BackingStore;
		// Same as for m_CachedTexture, use DX11 type for simplicity
		amComPtr<ID3D11ShaderResourceView> m_ShaderResourceView;
		grcTextureDX11_ExtraData*		   m_ExtraData;

	public:
		grcTexturePC(u16 width, u16 height, u8 mipCount, u16 stride, DXGI_FORMAT fmt);
		grcTexturePC(const grcTexturePC& other) = default;
		grcTexturePC(const datResource& rsc);

		u16 GetWidth() const override { return m_Width; }
		u16 GetHeight() const override { return m_Height; }
		u16 GetDepth() const override { return m_Depth; }
		u8 GetMipMapCount() const override { return m_MipCount; }
		u8 GetArraySize() const override { return 1; }

		bool IsValid() const override;
		bool IsSRGB() const override { return m_InfoBits.IsSRGB; }

		void* GetTextureView() const override { return m_ShaderResourceView.Get(); }

		u32 GetImageFormat() const override;

		bool LockRect(int layer, int mipLevel, grcTextureLock& lock, grcLockFlags lockFlags) override;
		void UnlockRect(grcTextureLock& lock) override;

		atFixedBitSet<8, u8> FindUsedChannels() const override;

		virtual void DeviceLost() {}
		virtual void DeviceReset() {}

		virtual void CreateFromBackingStore(bool recreate) = 0;
		// Sets texture's resource name in DirectX debug layer
		virtual void SetPrivateData() = 0;
		virtual bool CopyTo(grcImage* image, bool invert) = 0;
	};

	class grcTextureDX11 : public grcTexturePC
	{
		static constexpr int GRC_TEXTURE_DX11_EXTRA_DATA_MAX = 512;

		// Staging texture store
		static grcTextureDX11_ExtraData sm_ExtraDatas[];
		static inline int sm_ExtraDatasCount = 0;

		struct CreateInternalInfo
		{
			D3D11_SUBRESOURCE_DATA* SubresourceData;
			u32						SubresourceCount;
		};

		grcDevice::Stereo_t m_StereoRTMode;

		static grcTextureDX11_ExtraData* AllocExtraData(grcsTextureSyncType syncType);
		static void FreeExtraData(grcTextureDX11_ExtraData* extraData);
		static bool DoesNeedStagingTexture(grcTextureCreateType createType);
		static bool UsesBackingStoreForLocks(grcTextureCreateType createType);
		static void GetDescUsageAndCPUAccessFlags(grcTextureCreateType createType, D3D11_USAGE& usage, u32& cpuAccessFlags);
		DXGI_FORMAT TranslateDX9ToDX11Format(u32 fmt, bool sRGB) const;
		u32 TranslateDX11ToDX9Format(DXGI_FORMAT fmt) const;

		void CreateInternal(
			CreateInternalInfo& createInfo, 
			grcTextureCreateType createType, 
			grcsTextureSyncType syncType, 
			grcBindFlags extraBindFlags, 
			bool isFromBackingStore);
		void DeleteResources();

	public:
		// 2D Texture; If storeData is true, pixel data will be copied, otherwise - used only for creating DX11 resource
		grcTextureDX11(u16 width, u16 height, u8 mipCount, DXGI_FORMAT fmt, pVoid data, bool storeData = false);
		grcTextureDX11(const grcTextureDX11& other);
		grcTextureDX11(const datResource& rsc);
		~grcTextureDX11() override;

		DXGI_FORMAT GetDXGIFormat() const { return static_cast<DXGI_FORMAT>(m_Format); }

		u32 GetStride(int mip) const;
		u32 GetRowCount(int mip) const;
		u32 GetSliceCount(int mip) const;
		u32 GetTotalMipSize(int mip) const;
		u32 CalculateMemoryForASingleLayer() const;
		u32 CalculateMemoryForAllLayers() const;

		u8 GetBitsPerPixel() const override;

		void CreateFromBackingStore(bool recreate = false) override;
		void SetPrivateData() override;
		bool CopyTo(grcImage* image, bool invert) override;

		bool LockRect(int layer, int mipLevel, grcTextureLock& lock, grcLockFlags lockFlags) override;
		void UnlockRect(grcTextureLock& lock) override;

		// NOTE: data might be NULL!
		pVoid GetBackingStore() const { return m_BackingStore; }
	};
}
