#include "texturedx11.h"

#include "formats.h"
#include "am/types.h"
#include "am/file/fileutils.h"
#include "am/graphics/render/engine.h"
#include "rage/paging/compiler/compiler.h"

void rage::grcTextureDX11::GetUsageAndAccessFlags(u8 createFlags, D3D11_USAGE& usage, UINT& access)
{
	switch (createFlags)
	{
	case 1:
	case 2:
		usage = D3D11_USAGE_DEFAULT;
		access = 0;
		break;
	case 3:
	case 4:
	case 5:
		usage = D3D11_USAGE_DYNAMIC;
		access = (u8)D3D11_CPU_ACCESS_WRITE;
		break;
	default:
		usage = D3D11_USAGE_IMMUTABLE;
		access = 0;
	}
}

void rage::grcTextureDX11::ConvertFormatToDXGI()
{
	m_Format = D3D::LegacyTextureFormatToDXGI((u32)m_Format, m_Flags5F);
	if (m_Format == DXGI_FORMAT_UNKNOWN)
	{
		AM_WARNINGF("grcTextureDX11::ConvertFormatToDXGI() -> Unknown format %u, from importing texture %s...", m_Format, m_Name.GetCStr());
		m_Format = DXGI_FORMAT_R8G8B8A8_UINT;
	}
}

void rage::grcTextureDX11::ConvertFormatToLegacy()
{
	u32 fmt;

	switch (m_Format)
	{
	case DXGI_FORMAT_B8G8R8A8_UNORM:	fmt = 21;	break; // Also 63 but CodeWalker supports only 21
	case DXGI_FORMAT_BC1_UNORM:			fmt = DXT1; break;
	case DXGI_FORMAT_BC2_UNORM:			fmt = DXT3; break;
	case DXGI_FORMAT_BC3_UNORM:			fmt = DXT5; break;
	case DXGI_FORMAT_BC4_UNORM:			fmt = ATI1; break;
	case DXGI_FORMAT_BC5_UNORM:			fmt = ATI2; break;
	case DXGI_FORMAT_BC7_UNORM:			fmt = BC7;	break;

	default:							fmt = 0;
	}

	AM_ASSERT(fmt != 0, "grcTextureDX11::ConvertFormatToLegacy() -> Format %u is not supported, from exporting texture %s", m_Format, m_Name);

	m_Format = (DXGI_FORMAT)fmt;
}

bool rage::grcTextureDX11::ComputePitch(u8 mip, u32* pRowPitch, u32* pSlicePitch) const
{
	HRESULT result = D3D::ComputePitch(
		m_Format, m_Width, m_Height, mip, pRowPitch, pSlicePitch);
	return result == S_OK;
}

D3D11_TEXTURE3D_DESC rage::grcTextureDX11::GetDesc3D(u8 createFlags, u8 bindFlags) const
{
	D3D11_TEXTURE3D_DESC desk{};
	desk.Format = m_Format;
	desk.Width = m_Width;
	desk.Height = m_Height;
	desk.MipLevels = m_MipLevels;
	desk.BindFlags = bindFlags | D3D11_BIND_SHADER_RESOURCE;
	GetUsageAndAccessFlags(createFlags, desk.Usage, desk.CPUAccessFlags);
	return desk;
}

D3D11_TEXTURE2D_DESC rage::grcTextureDX11::GetDesc2D(u8 createFlags, u8 bindFlags) const
{
	D3D11_TEXTURE2D_DESC desk{};
	desk.Format = m_Format;
	desk.Width = m_Width;
	desk.Height = m_Height;
	desk.MipLevels = m_MipLevels;
	desk.BindFlags = bindFlags | D3D11_BIND_SHADER_RESOURCE;
	desk.ArraySize = m_LayerCount + 1;
	GetUsageAndAccessFlags(createFlags, desk.Usage, desk.CPUAccessFlags);

	desk.SampleDesc.Count = 1;
	desk.SampleDesc.Quality = 0;

	if (IsCubeMap())
		desk.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	return desk;
}

void rage::grcTextureDX11::GetViewDesc3D(D3D11_SHADER_RESOURCE_VIEW_DESC& viewDesk) const
{
	viewDesk.Format = m_Format;
	viewDesk.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	viewDesk.Texture3D.MipLevels = m_MipLevels;
}

void rage::grcTextureDX11::GetViewDesc2D(D3D11_SHADER_RESOURCE_VIEW_DESC& viewDesk) const
{
	viewDesk.Format = m_Format;
	u8 arraySize = GetArraySize();

	if (IsCubeMap())
	{
		viewDesk.Texture2D.MipLevels = m_MipLevels;
		if (arraySize <= 6)
		{
			viewDesk.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			return;
		}

		viewDesk.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
		viewDesk.Texture2DArray.ArraySize = arraySize / 6;
		return;
	}

	viewDesk.Texture2D.MipLevels = m_MipLevels;
	if (arraySize <= 1)
	{
		viewDesk.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		return;
	}

	viewDesk.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	viewDesk.Texture2DArray.ArraySize = arraySize;
}

HRESULT rage::grcTextureDX11::CreateFromData(const std::shared_ptr<D3D11_SUBRESOURCE_DATA[]> subData,
	u8 createFlags, u8 cacheFlags, D3D11_BIND_FLAG bindFlags)
{
	HRESULT result;

	// TODO: RAGE Cache Device?
	rageam::render::Engine* engine = rageam::render::Engine::GetInstance();
	ID3D11Device* device = engine->GetFactory();

	// Alternative versions of code below:

	// m_PcFlags2 &= 0xB1u;
	// v18 = createFlags >= 3u;
	// m_PcFlags2 |= 2 * ((32 * v18) | createFlags & 7);

	// if ( createFlags >= 3u )
	//     v9 = 32;
	// m_PcFlags2 = m_PcFlags2 & 0xB1 | (2 * (createFlags & 7 | v9));

	m_Flags5F &= 0xB1;
	m_Flags5F |= 2 * (createFlags & 7 | 32 * (createFlags >= 0x3));

	D3D11_SUBRESOURCE_DATA* pInitialData = subData.get();

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesk{};
	if (IsVolume())
	{
		const D3D11_TEXTURE3D_DESC desk = GetDesc3D(createFlags, bindFlags);
		if (desk.Usage != D3D11_USAGE_IMMUTABLE)
			pInitialData = nullptr;

		ID3D11Texture3D* pTexture;
		result = device->CreateTexture3D(&desk, pInitialData, &pTexture);
		m_Resource = pTexture;

		GetViewDesc3D(viewDesk);

		// TODO: Cache entry
	}
	else
	{
		const D3D11_TEXTURE2D_DESC desk = GetDesc2D(createFlags, bindFlags);
		if (desk.Usage == D3D11_USAGE_DYNAMIC)
			pInitialData = nullptr;

		ID3D11Texture2D* pTexture;
		result = device->CreateTexture2D(&desk, pInitialData, &pTexture);
		m_Resource = pTexture;

		GetViewDesc2D(viewDesk);

		// TODO: Cache entry
	}

	if (result == S_OK)
	{
		result = device->CreateShaderResourceView((ID3D11Resource*)m_Resource, &viewDesk, &m_ShaderView);
	}

	// Also can be seen as flag removal &= ~0x80u;
	m_Flags5F &= 0x7F;

	// TODO: There's some weird code...

	return result;
}

std::shared_ptr<D3D11_SUBRESOURCE_DATA[]> rage::grcTextureDX11::GetInitialData() const
{
	char* pTextureData = static_cast<char*>(m_PixelData);
	auto pSubData = std::make_shared<D3D11_SUBRESOURCE_DATA[]>(m_MipLevels);
	for (u8 i = 0; i < m_MipLevels; i++)
	{
		u32 rowPitch;
		u32 slicePitch;
		ComputePitch(i, &rowPitch, &slicePitch);

		D3D11_SUBRESOURCE_DATA& subData = pSubData[i];
		subData.pSysMem = pTextureData;
		subData.SysMemPitch = rowPitch;
		subData.SysMemSlicePitch = slicePitch;

		pTextureData += slicePitch;
	}
	return pSubData;
}

void rage::grcTextureDX11::Init()
{
	if (!m_PixelData)
		return;

	m_MipLevels = D3D::ClampMipLevels(m_Width, m_Height, m_MipLevels);

	std::shared_ptr<D3D11_SUBRESOURCE_DATA[]> pSubData = GetInitialData();

	u8 createFlags = 0; // Immutable?
	if (m_Flags48 != 0)
	{
		createFlags = 1; // Create cache entry?
		m_PixelData = nullptr;
	}

	CreateFromData(pSubData, createFlags, 0, D3D11_BIND_SHADER_RESOURCE);
}

rage::grcTextureDX11::grcTextureDX11()
{
	m_PixelData = nullptr;
	m_ShaderView = nullptr;
	m_Unknown80 = 0;
	m_Unknown88 = 0;
}

rage::grcTextureDX11::grcTextureDX11(u16 width, u16 height, u16 depth, u8 mipLevels, DXGI_FORMAT fmt, pVoid pixelData, bool keepData) : grcTexturePC(width, height, depth, mipLevels)
{
	m_ShaderView = nullptr;
	m_Unknown80 = 0;
	m_Unknown88 = 0;

	m_Format = fmt;
	m_PixelData = pixelData;

	Init();

	if (!keepData)
		m_PixelData = nullptr;
}

rage::grcTextureDX11::grcTextureDX11(const datResource& rsc) : grcTexturePC(rsc)
{
	rsc.Fixup(m_PixelData);

	ConvertFormatToDXGI();
	Init();

	// Texture data is in physical memory segment, it will be removed after resource placement
	m_PixelData = nullptr;
}

rage::grcTextureDX11::grcTextureDX11(const grcTextureDX11& other) : grcTexturePC(other)
{
	m_Format = other.m_Format;
	m_Unknown80 = 0;
	m_Unknown88 = 0;

	if (IsResourceCompiling())
	{
		pgSnapshotAllocator* snapshotAllocator = pgRscCompiler::GetPhysicalAllocator();
		u32 dataSize = 0;
		for (u8 i = 0; i < m_MipLevels; i++)
		{
			u32 rowPitch;
			u32 slicePitch;
			ComputePitch(i, &rowPitch, &slicePitch);

			// TODO: Calculate it somewhere else
			if (i == 0)
				m_Stride = rowPitch;

			dataSize += slicePitch;
		}

		ConvertFormatToLegacy();

		snapshotAllocator->AllocateRef(m_PixelData, dataSize);
		memcpy(m_PixelData, other.m_PixelData, dataSize);
	}
	else
	{
		m_PixelData = nullptr;
		m_ShaderView = other.m_ShaderView;
		m_Resource = other.m_Resource;

		ID3D11Resource* resource = (ID3D11Resource*)m_Resource;
		if (resource) resource->AddRef();
	}
}

rage::grcTextureDX11::~grcTextureDX11()
{
	ID3D11Resource* resource = (ID3D11Resource*)m_Resource;
	if (resource) resource->Release();

	delete m_PixelData;
}

ID3D11ShaderResourceView* rage::grcTextureDX11::GetShaderResourceView() const
{
	return m_ShaderView.Get();
}

void rage::grcTextureDX11::ExportTextureTo(ConstWString outDir, bool allowOverwrite) const
{
	rageam::render::Engine* render = rageam::render::Engine::GetInstance();

	DirectX::ScratchImage image;
	ID3D11Resource* pResource = (ID3D11Resource*)m_Resource;
	HRESULT hr = CaptureTexture(
		render->GetFactory(),
		render->GetDeviceContext(), pResource, image);

	rageam::wstring textureName = String::ToWideTemp(m_Name);
	rageam::file::WPath texturePath;
	bool isNameFine = true;
	do
	{
		texturePath = outDir;
		texturePath /= textureName;
		texturePath += L".dds";

		if (!allowOverwrite && IsFileExists(texturePath))
		{
			isNameFine = false;
			textureName += L" (copy)";
		}
		else
		{
			isNameFine = true;
		}
	} while (!isNameFine);

	if (SUCCEEDED(hr))
	{
		hr = SaveToDDSFile(
			image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::DDS_FLAGS_NONE, texturePath);

		if (FAILED(hr))
		{
			AM_ERRF(L"grcTextureDX11::ExportTextureTo() -> Failed to export texture %ls", textureName.GetCStr());
		}
	}
}
