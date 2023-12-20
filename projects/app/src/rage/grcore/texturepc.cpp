#include "texturepc.h"

#include "am/graphics/render/engine.h"
#include "am/graphics/image/image.h"
#include "am/graphics/dxgi_utils.h"

GUID TextureBackPointerGuid = { 0x637C4F8D, 0x7724, 0x436D, 0x0B2, 0x0D1, 0x49, 0x12, 0x5B, 0x3A, 0x2A, 0x25 };

rage::grcTextureDX11_ExtraData rage::grcTextureDX11::sm_ExtraDatas[GRC_TEXTURE_DX11_EXTRA_DATA_MAX] = {};

#define DXT1	0x31545844
#define DXT2	0x32545844
#define DXT3	0x33545844
#define DXT4	0x34545844
#define DXT5	0x35545844
#define ATI1	0x31495441
#define ATI2	0x32495441
#define RGBG	0x47424752
#define GRGB	0x42475247
#define G8R8	0x38523847
#define YUY2	0x32595559
#define R16		0x20363152
#define BC6		0x20364342
#define BC7		0x20374342
#define R8		0x20203852

rage::grcTexture* rage::GetTextureFromResourcePointer(ID3D11Resource* resource)
{
	grcTexture* tex;
	UINT size = sizeof(pVoid);
	if (resource->GetPrivateData(TextureBackPointerGuid, &size, &tex) == S_OK)
		return tex;
	return nullptr;
}

rage::grcTexturePC::grcTexturePC(u16 width, u16 height, u8 mipCount, u16 stride, DXGI_FORMAT fmt) : grcTexture(TEXTURE_NORMAL)
{
	m_Format = static_cast<u32>(fmt);

	m_ImageType = IMAGE_TYPE_STANDARD;

	m_Width = width;
	m_Height = height;
	m_Depth = 1;
	m_MipCount = mipCount;
	m_Stride = stride;
	m_LayerCount = 0;
	m_CutMipLevels = 0;

	m_ExtraFlags = 0;
	m_ExtraFlagsPadding = 0;
	m_ExtraData = nullptr;

	m_Next = nullptr;
	m_Previous = nullptr;

	m_BackingStore = nullptr;
	m_InfoBits = {};
}

// ReSharper disable once CppPossiblyUninitializedMember
rage::grcTexturePC::grcTexturePC(const datResource& rsc) : grcTexture(rsc)
{
	rsc.Fixup(m_BackingStore);
}

bool rage::grcTexturePC::IsValid() const
{
	// Native implementation does cache check, but we don't use cache device
	return true;
}

u32 rage::grcTexturePC::GetImageFormat() const
{
	AM_UNREACHABLE("grcTexturePC::GetImageFormat() -> Not implemented.");
}

bool rage::grcTexturePC::LockRect(int layer, int mipLevel, grcTextureLock& lock, grcLockType lockFlags)
{
	AM_UNREACHABLE("grcTexturePC::LockRect() -> Not implemented.");
}

void rage::grcTexturePC::UnlockRect(grcTextureLock& lock)
{
	AM_UNREACHABLE("grcTexturePC::UnlockRect() -> Not implemented.");
}

rage::atFixedBitSet<8, unsigned char> rage::grcTexturePC::FindUsedChannels() const
{
	AM_UNREACHABLE("grcTexturePC::FindUsedChannels() -> Not implemented.");
}

rage::grcTextureDX11_ExtraData* rage::grcTextureDX11::AllocExtraData(grcsTextureSyncType syncType)
{
	AM_ASSERT(sm_ExtraDatasCount < GRC_TEXTURE_DX11_EXTRA_DATA_MAX,
		"grcTextureDX11::AllocExtraData() -> Out of extra datas, possible leak?");

	grcTextureDX11_ExtraData* newExtraData = nullptr;
	for (grcTextureDX11_ExtraData& extraData : sm_ExtraDatas)
	{
		if (extraData.StagingTexture == nullptr)
		{
			newExtraData = &extraData;
			break;
		}
	}

	newExtraData->SyncType = syncType;
	if (syncType == grcsTextureSync_Mutex)
	{
		newExtraData->Mutex = CreateMutexA(NULL, FALSE, NULL); // rage::sysIpcCreateMutex
	}
	else if (syncType == grcsTextureSync_DirtySemaphore)
	{
		newExtraData->Mutex = CreateSemaphoreA(NULL, 1, INT16_MAX, NULL); // rage::sysIpcCreateSema
	}

	return newExtraData;
}

void rage::grcTextureDX11::FreeExtraData(grcTextureDX11_ExtraData* extraData)
{
	if (extraData->SyncType == grcsTextureSync_Mutex)
	{
		CloseHandle(extraData->Mutex); // sysIpcDeleteMutex
	}
	else if (extraData->SyncType == grcsTextureSync_DirtySemaphore)
	{
		CloseHandle(extraData->Mutex); // sysIpcDeleteSema
	}

	extraData->Mutex = nullptr;
	extraData->StagingTexture = nullptr;
}

bool rage::grcTextureDX11::DoesNeedStagingTexture(grcTextureCreateType createType)
{
	return createType == grcsTextureCreate_ReadWriteHasStaging;
}

bool rage::grcTextureDX11::UsesBackingStoreForLocks(grcTextureCreateType createType)
{
	return createType != grcsTextureCreate_WriteOnlyFromRTDynamic;
}

void rage::grcTextureDX11::GetDescUsageAndCPUAccessFlags(grcTextureCreateType createType, D3D11_USAGE& usage, u32& cpuAccessFlags)
{
	switch (createType)
	{
	case grcsTextureCreate_NeitherReadNorWrite:
		usage = D3D11_USAGE_IMMUTABLE;
		cpuAccessFlags = 0;
		return;

	case grcsTextureCreate_ReadWriteHasStaging:
	case grcsTextureCreate_Write:
		usage = D3D11_USAGE_DEFAULT;
		cpuAccessFlags = 0;
		return;

	case grcsTextureCreate_ReadWriteDynamic:
	case grcsTextureCreate_WriteDynamic:
	case grcsTextureCreate_WriteOnlyFromRTDynamic:
		usage = D3D11_USAGE_DYNAMIC;
		cpuAccessFlags = D3D11_CPU_ACCESS_WRITE;
		return;

	default: AM_UNREACHABLE("GetDescUsageAndCPUAccessFlags() -> Unknown create type '%i'", createType);
	}
}

DXGI_FORMAT rage::grcTextureDX11::TranslateDX9ToDX11Format(u32 fmt, bool sRGB) const
{
	// https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dformat

	if (sRGB)
	{
		switch (fmt)
		{
		case BC7:	return DXGI_FORMAT_BC7_UNORM_SRGB;
		case DXT1:	return DXGI_FORMAT_BC1_UNORM_SRGB;
		case DXT2:
		case DXT3:	return DXGI_FORMAT_BC2_UNORM_SRGB;
		case DXT4:
		case DXT5:	return DXGI_FORMAT_BC3_UNORM_SRGB;

		case 21:
		case 63:	return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case 32:
		case 33:	return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case 22:
		case 62:	return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
		case 75:
		case 77:
		case 79:
		case 83:	return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case 82:
		case 84:	return DXGI_FORMAT_D32_FLOAT;
		}

		return DXGI_FORMAT_UNKNOWN;
	}

	switch (fmt)
	{
	case R16:	return DXGI_FORMAT_R16_UNORM;
	case BC6:	return DXGI_FORMAT_BC6H_UF16;
	case BC7:	return DXGI_FORMAT_BC7_UNORM;
	case ATI1:	return DXGI_FORMAT_BC4_UNORM;
	case DXT1:	return DXGI_FORMAT_BC1_UNORM;
	case ATI2:	return DXGI_FORMAT_BC5_UNORM;
	case GRGB:	return DXGI_FORMAT_G8R8_G8B8_UNORM;
	case RGBG:	return DXGI_FORMAT_R8G8_B8G8_UNORM;
	case DXT2:
	case DXT3:	return DXGI_FORMAT_BC2_UNORM;
	case DXT4:
	case DXT5:	return DXGI_FORMAT_BC3_UNORM;
	case 60:
	case G8R8:	return DXGI_FORMAT_R8G8_UNORM;
	case 41:
	case 50:
	case R8:	return DXGI_FORMAT_R8_UNORM;

	case 23:	return DXGI_FORMAT_B5G6R5_UNORM;
	case 28:	return DXGI_FORMAT_A8_UNORM;
	case 36:	return DXGI_FORMAT_R16G16B16A16_UNORM;
	case 40:	return DXGI_FORMAT_R8G8_UINT;
	case 21:
	case 63:	return DXGI_FORMAT_B8G8R8A8_UNORM;
	case 22:
	case 62:	return DXGI_FORMAT_B8G8R8X8_UNORM;
	case 24:
	case 25:	return DXGI_FORMAT_B5G5R5A1_UNORM;
	case 31:
	case 67:
	case 35:	return DXGI_FORMAT_R10G10B10A2_UNORM;
	case 32:
	case 33:	return DXGI_FORMAT_R8G8B8A8_UNORM;
	case 34:
	case 64:	return DXGI_FORMAT_R16G16_UNORM;
	case 70:
	case 80:	return DXGI_FORMAT_D16_UNORM;
	case 71:
	case 82:
	case 84:	return DXGI_FORMAT_D32_FLOAT;
	case 75:
	case 77:
	case 79:
	case 83:	return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case 111:	return DXGI_FORMAT_R16_FLOAT;
	case 112:	return DXGI_FORMAT_R16G16_FLOAT;
	case 113:	return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case 114:	return DXGI_FORMAT_R32_FLOAT;
	case 115:	return DXGI_FORMAT_R32G32_FLOAT;
	case 116:	return DXGI_FORMAT_R32G32B32A32_FLOAT;
	}
	return DXGI_FORMAT_UNKNOWN;
}

u32 rage::grcTextureDX11::TranslateDX11ToDX9Format(DXGI_FORMAT fmt) const
{
	switch (fmt)
	{
	case DXGI_FORMAT_BC7_UNORM_SRGB:		return BC7;
	case DXGI_FORMAT_BC1_UNORM_SRGB:		return DXT1;
	case DXGI_FORMAT_BC2_UNORM_SRGB:		return DXT3;	// DXT2
	case DXGI_FORMAT_BC3_UNORM_SRGB:		return DXT5;	// DXT4
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:	return 21;		// 63
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:	return 32;		// 33
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:	return 22;		// 62
	case DXGI_FORMAT_D24_UNORM_S8_UINT:		return 75;		// 77 79 83

	case DXGI_FORMAT_BC1_UNORM:				return DXT1;
	case DXGI_FORMAT_BC2_UNORM:				return DXT3;	// DXT2
	case DXGI_FORMAT_BC3_UNORM:				return DXT5;	// DXT4
	case DXGI_FORMAT_BC4_UNORM:				return ATI1;
	case DXGI_FORMAT_BC5_UNORM:				return ATI2;
	case DXGI_FORMAT_BC6H_UF16:				return BC6;
	case DXGI_FORMAT_BC7_UNORM:				return BC7;
	case DXGI_FORMAT_R16_UNORM:				return R16;
	case DXGI_FORMAT_G8R8_G8B8_UNORM:		return GRGB;
	case DXGI_FORMAT_R8G8_B8G8_UNORM:		return RGBG;
	case DXGI_FORMAT_R8G8_UNORM:			return G8R8;	// 60
	case DXGI_FORMAT_R8_UNORM:				return R8;		// 41 50
	case DXGI_FORMAT_B5G6R5_UNORM:			return 23;
	case DXGI_FORMAT_A8_UNORM:				return 28;
	case DXGI_FORMAT_R16G16B16A16_UNORM:	return 36;
	case DXGI_FORMAT_R8G8_UINT:				return 40;
	case DXGI_FORMAT_B8G8R8A8_UNORM:		return 21;		// 63
	case DXGI_FORMAT_B8G8R8X8_UNORM:		return 22;		// 62
	case DXGI_FORMAT_B5G5R5A1_UNORM:		return 24;		// 25
	case DXGI_FORMAT_R10G10B10A2_UNORM:		return 31;		// 35 67
	case DXGI_FORMAT_R8G8B8A8_UNORM:		return 32;		// 33
	case DXGI_FORMAT_R16G16_UNORM:			return 34;		// 64
	case DXGI_FORMAT_D16_UNORM:				return 70;		// 80
	case DXGI_FORMAT_D32_FLOAT:				return 71;		// 82 84
	case DXGI_FORMAT_R16_FLOAT:				return 111;
	case DXGI_FORMAT_R16G16_FLOAT:			return 112;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:	return 113;
	case DXGI_FORMAT_R32_FLOAT:				return 114;
	case DXGI_FORMAT_R32G32_FLOAT:			return 115;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:	return 116;

	default:
		AM_UNREACHABLE("grcTextureDX11::TranslateDX11ToDX9Format() -> DXGI Format '%s' is not supported.",
			rageam::Enum::GetName(fmt));
	}
}

void rage::grcTextureDX11::CreateInternal(
	CreateInternalInfo& createInfo, grcTextureCreateType createType,
	grcsTextureSyncType syncType, grcBindFlag extraBindFlags, bool isFromBackingStore)
{
	ID3D11Device* device = rageam::render::GetDevice();

	m_InfoBits.Dirty = false;
	m_InfoBits.HasBeenDeleted = false;
	m_InfoBits.ReadWriteAccess = createType;
	m_InfoBits.Dynamic =
		createType == grcsTextureCreate_ReadWriteDynamic ||
		createType == grcsTextureCreate_WriteDynamic ||
		createType == grcsTextureCreate_WriteOnlyFromRTDynamic;

	extraBindFlags |= D3D11_BIND_SHADER_RESOURCE;

	D3D11_USAGE usage;
	u32 cpuAccessFlags;
	GetDescUsageAndCPUAccessFlags(createType, usage, cpuAccessFlags);

	D3D11_SUBRESOURCE_DATA* initialData = createInfo.SubresourceData;
	if (usage != D3D11_USAGE_IMMUTABLE)
		initialData = nullptr;

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = GetDXGIFormat();

	if (m_ImageType == IMAGE_TYPE_VOLUME)
	{
		D3D11_TEXTURE3D_DESC desc = {};
		desc.Width = m_Width;
		desc.Height = m_Height;
		desc.Depth = m_Depth;
		desc.MipLevels = m_MipCount;
		desc.BindFlags = extraBindFlags;
		desc.Usage = usage;
		desc.CPUAccessFlags = cpuAccessFlags;
		desc.Format = GetDXGIFormat();

		ID3D11Texture3D* tex;
		AM_ASSERT_D3D(device->CreateTexture3D(&desc, initialData, &tex));
		m_CachedTexture = amComPtr<ID3D11Resource>(tex);

		if (DoesNeedStagingTexture(createType))
		{
			m_ExtraData = AllocExtraData(syncType);

			desc.BindFlags = 0;
			desc.Usage = D3D11_USAGE_STAGING;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

			ID3D11Texture3D* texStaging;
			AM_ASSERT_D3D(device->CreateTexture3D(&desc, nullptr, &texStaging));
			m_ExtraData->StagingTexture = amComPtr<ID3D11Resource>(texStaging);
		}

		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		viewDesc.Texture3D.MipLevels = m_MipCount;
	}
	else
	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.ArraySize = m_LayerCount + 1;
		desc.Width = m_Width;
		desc.Height = m_Height;
		desc.MipLevels = m_MipCount;
		desc.BindFlags = extraBindFlags;
		desc.Usage = usage;
		desc.CPUAccessFlags = cpuAccessFlags;
		desc.Format = GetDXGIFormat();
		desc.SampleDesc.Count = 1;

		if (m_ImageType == IMAGE_TUBE_CUBE)
			desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		ID3D11Texture2D* tex;
		AM_ASSERT_D3D(device->CreateTexture2D(&desc, initialData, &tex));
		m_CachedTexture = amComPtr<ID3D11Resource>(tex);

		if (DoesNeedStagingTexture(createType))
		{
			m_ExtraData = AllocExtraData(syncType);

			desc.BindFlags = 0;
			desc.Usage = D3D11_USAGE_STAGING;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

			if (m_StereoRTMode == grcDevice::Stereo_t::STEREO)
			{
				desc.Width = m_Width * 2;
				desc.Height = m_Height + 1;
			}

			ID3D11Texture2D* texStaging;
			AM_ASSERT_D3D(device->CreateTexture2D(&desc, createInfo.SubresourceData, &texStaging));
			m_ExtraData->StagingTexture = amComPtr<ID3D11Resource>(texStaging);
		}

		if (m_ImageType == IMAGE_TYPE_STANDARD)
		{
			if (desc.ArraySize == 1)
			{
				viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				viewDesc.Texture2D.MipLevels = m_MipCount;
			}
			else
			{
				viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				viewDesc.Texture2DArray.MipLevels = m_MipCount;
			}
		}
		else if (m_ImageType == IMAGE_TUBE_CUBE)
		{
			if (desc.ArraySize <= 6)
			{
				viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				viewDesc.TextureCube.MipLevels = m_MipCount;
			}
			else
			{
				viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
				viewDesc.TextureCubeArray.MipLevels = m_MipCount;
				viewDesc.TextureCubeArray.NumCubes = desc.ArraySize / 6;
			}
		}
	}

	ID3D11ShaderResourceView* shaderResourceView;
	AM_ASSERT_D3D(device->CreateShaderResourceView(m_CachedTexture.Get(), &viewDesc, &shaderResourceView));
	m_ShaderResourceView = amComPtr(shaderResourceView);

	if (UsesBackingStoreForLocks(createType) && !isFromBackingStore)
	{
		u32 totalMemory = CalculateMemoryForAllLayers();

		m_BackingStore = GetAllocator(ALLOC_TYPE_VIRTUAL)->Allocate(totalMemory);
		m_InfoBits.OwnsBackingStore = true;

		if (createInfo.SubresourceData) // TODO: Verify if this code is right
		{
			char* pixelData = static_cast<char*>(m_BackingStore);
			for (int i = 0; i < createInfo.SubresourceCount; i++)
			{
				D3D11_SUBRESOURCE_DATA& subData = createInfo.SubresourceData[i];

				u32 sliceCount = GetSliceCount(i);
				u32 mipSize = subData.SysMemSlicePitch * sliceCount;
				memcpy(pixelData, subData.pSysMem, mipSize);
				pixelData += mipSize;
			}
		}
	}

	SetPrivateData();
}

void rage::grcTextureDX11::DeleteResources()
{
	if (m_ExtraData)
	{
		FreeExtraData(m_ExtraData);
		m_ExtraData = nullptr;
	}

	m_CachedTexture = nullptr;
	m_ShaderResourceView = nullptr;

	// Revert cut mip maps
	m_MipCount += m_CutMipLevels;
	m_Width <<= m_CutMipLevels;
	m_Height <<= m_CutMipLevels;
	m_Stride <<= m_CutMipLevels;
	if (m_ImageType == IMAGE_TYPE_VOLUME)
		m_Depth <<= m_CutMipLevels;

	if (m_InfoBits.OwnsBackingStore)
		GetAllocator(ALLOC_TYPE_VIRTUAL)->Free(m_BackingStore);
	else
		GetAllocator(ALLOC_TYPE_PHYSICAL)->Free(m_BackingStore);
	m_BackingStore = nullptr;
}

rage::grcTextureDX11::grcTextureDX11(u16 width, u16 height, u8 mipCount, DXGI_FORMAT fmt, pVoid data, bool storeData) :
	grcTexturePC(width, height, mipCount, 0, fmt)
{
	m_Stride = GetStride(0);

	if (!IsResourceCompiling())
	{
		if (storeData)
		{
			u32 totalSize = CalculateMemoryForAllLayers();
			m_BackingStore = GetAllocator(ALLOC_TYPE_PHYSICAL)->Allocate(totalSize);
			memcpy(m_BackingStore, data, totalSize);
			m_InfoBits.OwnsBackingStore = true;
		}
		else
		{
			m_BackingStore = data;
			m_InfoBits.OwnsBackingStore = false;
		}

		grcTextureDX11::CreateFromBackingStore();
	}
}

rage::grcTextureDX11::grcTextureDX11(const grcTextureDX11& other) : grcTexturePC(other)
{
	m_StereoRTMode = other.m_StereoRTMode;

	if (IsResourceCompiling())
	{
		// Transfer pixel data on physical segment
		pgSnapshotAllocator* snapshotAllocator = pgRscCompiler::GetPhysicalAllocator();
		u32 dataSize = CalculateMemoryForAllLayers();
		snapshotAllocator->AllocateRef(m_BackingStore, dataSize);
		memcpy(m_BackingStore, other.m_BackingStore, dataSize);

		m_InfoBits.OwnsBackingStore = false;
		m_Format = TranslateDX11ToDX9Format(GetDXGIFormat());
	}
}

rage::grcTextureDX11::grcTextureDX11(const datResource& rsc) : grcTexturePC(rsc)
{
	u32 formatDX9 = m_Format;
	m_Format = static_cast<u32>(TranslateDX9ToDX11Format(m_Format, m_InfoBits.IsSRGB));
	AM_ASSERT(m_Format != DXGI_FORMAT_UNKNOWN, "grcTextureDX11() -> Unknown DX9 format '%u' in resource file.", formatDX9);

	grcTextureDX11::CreateFromBackingStore(false);
	grcTextureDX11::SetPrivateData();
}

rage::grcTextureDX11::~grcTextureDX11()
{
	DeleteResources();
}

u32 rage::grcTextureDX11::GetStride(int mip) const
{
	u32 width = MAX(GetWidth() >> mip, 1);

	DXGI_FORMAT format = GetDXGIFormat();
	if (!rageam::graphics::DXGI::IsCompressed(format))
	{
		return width * rageam::graphics::DXGI::BytesPerPixel(format);
	}

	u32 totalNumBlocks = (width + 3) / 4; // Rounded up block count

	// BC1/BC4 encoded block size is 128 bits (8 bytes)
	if (rageam::graphics::DXGI::IsBC1(format) || rageam::graphics::DXGI::IsBC4(format))
		return totalNumBlocks * 8;

	// Other BC formats block size is 256 bits(16 bytes)
	// BC2, BC3, BC5, BC7
	return totalNumBlocks * 16;
}

u32 rage::grcTextureDX11::GetRowCount(int mip) const
{
	u32 mipHeight = GetHeight() >> mip;
	if (rageam::graphics::DXGI::IsCompressed(GetDXGIFormat()))
	{
		mipHeight = (mipHeight + 3) / 4; // Rounded up block count
	}
	return MAX(mipHeight, 1);
}

u32 rage::grcTextureDX11::GetSliceCount(int mip) const
{
	return MAX(m_Depth >> mip, 1);
}

u32 rage::grcTextureDX11::GetTotalMipSize(int mip) const
{
	return GetStride(mip) * GetRowCount(mip) * GetSliceCount(mip);
}

u32 rage::grcTextureDX11::CalculateMemoryForASingleLayer() const
{
	u32 size = 0;
	for (int i = 0; i < m_MipCount; i++)
	{
		size += GetTotalMipSize(i);
	}
	return size;
}

u32 rage::grcTextureDX11::CalculateMemoryForAllLayers() const
{
	return CalculateMemoryForASingleLayer() * (m_LayerCount + 1);
}

u8 rage::grcTextureDX11::GetBitsPerPixel() const
{
	return rageam::graphics::DXGI::BitsPerPixel(GetDXGIFormat());
}

void rage::grcTextureDX11::CreateFromBackingStore(bool recreate)
{
	m_StereoRTMode = grcDevice::Stereo_t::AUTO;

	char* pixelData = static_cast<char*>(m_BackingStore);
	grcBindFlag bindFlags = grcBindNone;
	grcTextureCreateType createType = grcsTextureCreate_NeitherReadNorWrite;

	// NOTE: Native function selects first mip map based on graphical settings, we don't do this
	if (!recreate) // Texture was already scaled down before...
	{
		m_CutMipLevels = 0; // rage::grcTexturePC::GetMipLevelScaleQuality

		if (m_CutMipLevels > 0)
		{
			for (int i = 0; i < m_CutMipLevels; i++)
			{
				pixelData += GetTotalMipSize(m_CutMipLevels);
			}

			m_MipCount -= m_CutMipLevels;
			m_Width >>= m_CutMipLevels;
			m_Height >>= m_CutMipLevels;
			m_Stride >>= m_CutMipLevels;
			if (m_ImageType == IMAGE_TYPE_VOLUME)
				m_Depth >>= m_CutMipLevels;
		}
	}

	// R* texture compressor creates all mip maps (up to 1x1) for block compressed formats,
	// DX11 does not allow such textures because lowest compressible pixel block size is 4x4
	if (rageam::graphics::DXGI::IsCompressed(GetDXGIFormat()))
	{
		int maxMipCount = rageam::graphics::ImageComputeMaxMipCount(m_Width, m_Height);
		m_MipCount = MIN(m_MipCount, maxMipCount);
	}

	D3D11_SUBRESOURCE_DATA resourceData[rageam::graphics::IMAGE_MAX_MIP_MAPS];
	for (int i = 0; i < m_MipCount; i++)
	{
		u32 stride = GetStride(i);
		u32 rowCount = GetRowCount(i);
		u32 sliceCount = GetSliceCount(i);

		resourceData[i].pSysMem = pixelData;
		resourceData[i].SysMemPitch = stride;
		resourceData[i].SysMemSlicePitch = stride * rowCount;

		pixelData += static_cast<size_t>(stride * rowCount * sliceCount);
	}

	if (m_Name)
	{
		atHashValue prefixHash = atStringViewHash(m_Name, 9);
		// Create RT for scaleform placeholder
		if (prefixHash == atStringHash("script_rt"))
		{
			DXGI_FORMAT newFormat = GetDXGIFormat();
			// Can't create render target with compressed pixel format
			if (rageam::graphics::DXGI::IsCompressed(newFormat))
				m_Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
			newFormat = rageam::graphics::DXGI::ToTypeless(newFormat);
			newFormat = rageam::graphics::DXGI::ToTexture(newFormat);
			m_Format = static_cast<u32>(newFormat);

			bindFlags = grcBindRenderTarget;
			createType = grcsTextureCreate_ReadWriteHasStaging;
		}
		else if (!m_ExtraFlags)
		{
			if (prefixHash == atStringHash("pedmugsho"))
			{
				// Such run-time generated textures are compressed using very fast (and terrible) DXT1 encoder
				if (rageam::graphics::DXGI::IsBC1(GetDXGIFormat()))
				{
					createType = grcsTextureCreate_Write;
				}
			}
			else if (prefixHash == atStringHash("mp_crewpa") && atStringHash(m_Name) == atStringHash("mp_crewpalette"))
			{
				createType = grcsTextureCreate_Write;
			}
		}
	}

	bool isFromBackingStore = true;
	if (m_ExtraFlags)
	{
		createType = grcsTextureCreate_ReadWriteHasStaging;
		isFromBackingStore = false;
		m_BackingStore = nullptr;
	}

	CreateInternalInfo createInternalInfo;
	createInternalInfo.SubresourceData = resourceData;
	createInternalInfo.SubresourceCount = m_MipCount;
	CreateInternal(createInternalInfo, createType, grcsTextureSync_Mutex, bindFlags, isFromBackingStore);

	if (!m_InfoBits.OwnsBackingStore && !tl_KeepResourcePixelData)
	{
		m_BackingStore = nullptr;
	}
}

void rage::grcTextureDX11::SetPrivateData()
{
	ConstString name = GetName();
	if (m_CachedTexture)
	{
		AM_ASSERT_D3D(m_CachedTexture->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(name), name));
		AM_ASSERT_D3D(m_CachedTexture->SetPrivateData(TextureBackPointerGuid, sizeof(pVoid), this));
	}

	if (m_ShaderResourceView)
	{
		char buffer[256];
		size_t len = sprintf_s(buffer, 256, "%s - Resource", name) + 1;
		AM_ASSERT_D3D(m_ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, len, name));
	}

	if (m_ExtraData && m_ExtraData->StagingTexture)
	{
		char buffer[256];
		size_t len = sprintf_s(buffer, 256, "%s - Staging", name) + 1;
		AM_ASSERT_D3D(m_ExtraData->StagingTexture->SetPrivateData(WKPDID_D3DDebugObjectName, len, name));
	}
}

bool rage::grcTextureDX11::CopyTo(grcImage* image, bool invert)
{
	AM_UNREACHABLE("grcTextureDX11::CopyTo() -> Not implemented.");
}
