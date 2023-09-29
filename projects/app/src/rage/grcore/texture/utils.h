//
// File: utils.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "DirectXTex.h"
#include "rage/grcore/texture/formats.h"
#include "common/types.h"
#include "helpers/ranges.h"
#include "helpers/align.h"

namespace D3D
{
	inline bool IsCompressed(DXGI_FORMAT fmt)
	{
		return DirectX::IsCompressed(fmt);
	}

	inline u32 BitsPerPixel(DXGI_FORMAT fmt)
	{
		return static_cast<u32>(DirectX::BitsPerPixel(fmt));
	}

	/**
	 * \brief Converts texture dimension to mip level.
	 * \n For e.g. 512 on 2nd mip level is 512 >> 2 = 512 / 4 = 128;
	 * \n NOTE: If result value is 0, (for e.g. 512 on mip 8 is 0), 1 returned.
	 * \param value Width / Height / Depth of the texture.
	 * \param mip Mip level.
	 */
	inline u32 MipSize(u32 value, u32 mip)
	{
		value >>= mip;
		return value > 0 ? value : 1;
	}

	/**
	 * \brief Computes row and slice pitches.
	 * \n - Row pitch is distance (in bytes) one line of texture occupies (Line is pixels from '0' to 'Width').
	 * \n - Row slice pitch is distance (in bytes) of one depth level. (Row Pitch * Height).
	 * \remark Out params can be NULL if not required.
	 */
	inline HRESULT ComputePitch(DXGI_FORMAT fmt, u32 width, u32 height, u32 mip, u32* pRowPitch, u32* pSlicePitch)
	{
		size_t rowPitch, slicePitch;
		width = MipSize(width, mip);
		height = MipSize(height, mip);
		HRESULT result = DirectX::ComputePitch(fmt, width, height, rowPitch, slicePitch);
		if (pRowPitch) *pRowPitch = static_cast<u32>(rowPitch);
		if (pSlicePitch) *pSlicePitch = static_cast<u32>(slicePitch);
		return result;
	}

	/**
	 * \brief Gets texture size (in bytes) at given mip level.
	 */
	inline HRESULT GetTextureMipSize(DXGI_FORMAT fmt, u32 width, u32 height, u32 depth, u32 mip, u32& size)
	{
		u32 slicePitch;

		HRESULT result = ComputePitch(fmt, width, height, mip, nullptr, &slicePitch);
		if (result != S_OK) return result;

		depth = MipSize(depth, mip);
		size = depth * slicePitch;
		return S_OK;
	}

	/**
	 * \brief Gets texture size in bytes.
	 */
	inline HRESULT GetTextureSize(DXGI_FORMAT fmt, u32 width, u32 height, u32 depth, u32 mipLevels, u32& size)
	{
		u32 sum = 0;
		for (u32 i = 0; i < mipLevels; i++)
		{
			u32 mipSize;
			HRESULT result = GetTextureMipSize(fmt, width, height, depth, i, mipSize);
			if (result != S_OK) return result;
			sum += mipSize;
		}
		size = sum;
		return S_OK;
	}

	/**
	 * \brief Gets maximum number of mip levels for given resolution.
	 * \n If size is not power of two, 0 is returned.
	 */
	inline u8 GetAvailableMipLevels(u32 width, u32 height)
	{
		if (!IS_POWER_OF_TWO(width) || !IS_POWER_OF_TWO(height))
			return 1;

		if (MIN(width, height) <= 4)
			return 1;

		// -1 to make 4x4 be minimum possible mip level ( as required by d3d )
		return BitScanR32(MIN(width, height)) - 1;
	}

	/**
	 * \brief Ensures that given number of mip levels does not exceed
	 * available number of mip levels for given resolution.
	 */
	inline u32 ClampMipLevels(u32 width, u32 height, u8 mipLevels)
	{
		u8 available = GetAvailableMipLevels(width, height);
		return MIN(available, mipLevels);
	}

	/**
	 * \brief Allocates and constructs array of initial texture data levels from raw pixel data and texture description.
	 * \remark Array has to be de-allocated manually.
	 */
	inline D3D11_SUBRESOURCE_DATA* CreateSubData(char* pixelData, const D3D11_TEXTURE2D_DESC& desc)
	{
		D3D11_SUBRESOURCE_DATA* subData = new D3D11_SUBRESOURCE_DATA[desc.MipLevels];
		for (u8 i = 0; i < desc.MipLevels; i++)
		{
			u32 rowPitch;
			u32 slicePitch;
			ComputePitch(desc.Format, desc.Width, desc.Height, i, &rowPitch, &slicePitch);

			D3D11_SUBRESOURCE_DATA& data = subData[i];
			data.pSysMem = pixelData;
			data.SysMemPitch = rowPitch;
			data.SysMemSlicePitch = slicePitch;

			pixelData += slicePitch;
		}
		return subData;
	}

	/**
	 * \brief Converts legacy (D3DFORMAT?) grc texture format to DXGI.
	 */
	inline DXGI_FORMAT LegacyTextureFormatToDXGI(u32 fmt, u8 flags)
	{
		if (flags != 0)
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

	inline ConstString DxgiFormatToString(DXGI_FORMAT e)
	{
		switch (e)
		{
		case DXGI_FORMAT_UNKNOWN: return "DXGI_FORMAT_UNKNOWN";
		case DXGI_FORMAT_R32G32B32A32_TYPELESS: return "DXGI_FORMAT_R32G32B32A32_TYPELESS";
		case DXGI_FORMAT_R32G32B32A32_FLOAT: return "DXGI_FORMAT_R32G32B32A32_FLOAT";
		case DXGI_FORMAT_R32G32B32A32_UINT: return "DXGI_FORMAT_R32G32B32A32_UINT";
		case DXGI_FORMAT_R32G32B32A32_SINT: return "DXGI_FORMAT_R32G32B32A32_SINT";
		case DXGI_FORMAT_R32G32B32_TYPELESS: return "DXGI_FORMAT_R32G32B32_TYPELESS";
		case DXGI_FORMAT_R32G32B32_FLOAT: return "DXGI_FORMAT_R32G32B32_FLOAT";
		case DXGI_FORMAT_R32G32B32_UINT: return "DXGI_FORMAT_R32G32B32_UINT";
		case DXGI_FORMAT_R32G32B32_SINT: return "DXGI_FORMAT_R32G32B32_SINT";
		case DXGI_FORMAT_R16G16B16A16_TYPELESS: return "DXGI_FORMAT_R16G16B16A16_TYPELESS";
		case DXGI_FORMAT_R16G16B16A16_FLOAT: return "DXGI_FORMAT_R16G16B16A16_FLOAT";
		case DXGI_FORMAT_R16G16B16A16_UNORM: return "DXGI_FORMAT_R16G16B16A16_UNORM";
		case DXGI_FORMAT_R16G16B16A16_UINT: return "DXGI_FORMAT_R16G16B16A16_UINT";
		case DXGI_FORMAT_R16G16B16A16_SNORM: return "DXGI_FORMAT_R16G16B16A16_SNORM";
		case DXGI_FORMAT_R16G16B16A16_SINT: return "DXGI_FORMAT_R16G16B16A16_SINT";
		case DXGI_FORMAT_R32G32_TYPELESS: return "DXGI_FORMAT_R32G32_TYPELESS";
		case DXGI_FORMAT_R32G32_FLOAT: return "DXGI_FORMAT_R32G32_FLOAT";
		case DXGI_FORMAT_R32G32_UINT: return "DXGI_FORMAT_R32G32_UINT";
		case DXGI_FORMAT_R32G32_SINT: return "DXGI_FORMAT_R32G32_SINT";
		case DXGI_FORMAT_R32G8X24_TYPELESS: return "DXGI_FORMAT_R32G8X24_TYPELESS";
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return "DXGI_FORMAT_D32_FLOAT_S8X24_UINT";
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS: return "DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS";
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT: return "DXGI_FORMAT_X32_TYPELESS_G8X24_UINT";
		case DXGI_FORMAT_R10G10B10A2_TYPELESS: return "DXGI_FORMAT_R10G10B10A2_TYPELESS";
		case DXGI_FORMAT_R10G10B10A2_UNORM: return "DXGI_FORMAT_R10G10B10A2_UNORM";
		case DXGI_FORMAT_R10G10B10A2_UINT: return "DXGI_FORMAT_R10G10B10A2_UINT";
		case DXGI_FORMAT_R11G11B10_FLOAT: return "DXGI_FORMAT_R11G11B10_FLOAT";
		case DXGI_FORMAT_R8G8B8A8_TYPELESS: return "DXGI_FORMAT_R8G8B8A8_TYPELESS";
		case DXGI_FORMAT_R8G8B8A8_UNORM: return "DXGI_FORMAT_R8G8B8A8_UNORM";
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return "DXGI_FORMAT_R8G8B8A8_UNORM_SRGB";
		case DXGI_FORMAT_R8G8B8A8_UINT: return "DXGI_FORMAT_R8G8B8A8_UINT";
		case DXGI_FORMAT_R8G8B8A8_SNORM: return "DXGI_FORMAT_R8G8B8A8_SNORM";
		case DXGI_FORMAT_R8G8B8A8_SINT: return "DXGI_FORMAT_R8G8B8A8_SINT";
		case DXGI_FORMAT_R16G16_TYPELESS: return "DXGI_FORMAT_R16G16_TYPELESS";
		case DXGI_FORMAT_R16G16_FLOAT: return "DXGI_FORMAT_R16G16_FLOAT";
		case DXGI_FORMAT_R16G16_UNORM: return "DXGI_FORMAT_R16G16_UNORM";
		case DXGI_FORMAT_R16G16_UINT: return "DXGI_FORMAT_R16G16_UINT";
		case DXGI_FORMAT_R16G16_SNORM: return "DXGI_FORMAT_R16G16_SNORM";
		case DXGI_FORMAT_R16G16_SINT: return "DXGI_FORMAT_R16G16_SINT";
		case DXGI_FORMAT_R32_TYPELESS: return "DXGI_FORMAT_R32_TYPELESS";
		case DXGI_FORMAT_D32_FLOAT: return "DXGI_FORMAT_D32_FLOAT";
		case DXGI_FORMAT_R32_FLOAT: return "DXGI_FORMAT_R32_FLOAT";
		case DXGI_FORMAT_R32_UINT: return "DXGI_FORMAT_R32_UINT";
		case DXGI_FORMAT_R32_SINT: return "DXGI_FORMAT_R32_SINT";
		case DXGI_FORMAT_R24G8_TYPELESS: return "DXGI_FORMAT_R24G8_TYPELESS";
		case DXGI_FORMAT_D24_UNORM_S8_UINT: return "DXGI_FORMAT_D24_UNORM_S8_UINT";
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS: return "DXGI_FORMAT_R24_UNORM_X8_TYPELESS";
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT: return "DXGI_FORMAT_X24_TYPELESS_G8_UINT";
		case DXGI_FORMAT_R8G8_TYPELESS: return "DXGI_FORMAT_R8G8_TYPELESS";
		case DXGI_FORMAT_R8G8_UNORM: return "DXGI_FORMAT_R8G8_UNORM";
		case DXGI_FORMAT_R8G8_UINT: return "DXGI_FORMAT_R8G8_UINT";
		case DXGI_FORMAT_R8G8_SNORM: return "DXGI_FORMAT_R8G8_SNORM";
		case DXGI_FORMAT_R8G8_SINT: return "DXGI_FORMAT_R8G8_SINT";
		case DXGI_FORMAT_R16_TYPELESS: return "DXGI_FORMAT_R16_TYPELESS";
		case DXGI_FORMAT_R16_FLOAT: return "DXGI_FORMAT_R16_FLOAT";
		case DXGI_FORMAT_D16_UNORM: return "DXGI_FORMAT_D16_UNORM";
		case DXGI_FORMAT_R16_UNORM: return "DXGI_FORMAT_R16_UNORM";
		case DXGI_FORMAT_R16_UINT: return "DXGI_FORMAT_R16_UINT";
		case DXGI_FORMAT_R16_SNORM: return "DXGI_FORMAT_R16_SNORM";
		case DXGI_FORMAT_R16_SINT: return "DXGI_FORMAT_R16_SINT";
		case DXGI_FORMAT_R8_TYPELESS: return "DXGI_FORMAT_R8_TYPELESS";
		case DXGI_FORMAT_R8_UNORM: return "DXGI_FORMAT_R8_UNORM";
		case DXGI_FORMAT_R8_UINT: return "DXGI_FORMAT_R8_UINT";
		case DXGI_FORMAT_R8_SNORM: return "DXGI_FORMAT_R8_SNORM";
		case DXGI_FORMAT_R8_SINT: return "DXGI_FORMAT_R8_SINT";
		case DXGI_FORMAT_A8_UNORM: return "DXGI_FORMAT_A8_UNORM";
		case DXGI_FORMAT_R1_UNORM: return "DXGI_FORMAT_R1_UNORM";
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP: return "DXGI_FORMAT_R9G9B9E5_SHAREDEXP";
		case DXGI_FORMAT_R8G8_B8G8_UNORM: return "DXGI_FORMAT_R8G8_B8G8_UNORM";
		case DXGI_FORMAT_G8R8_G8B8_UNORM: return "DXGI_FORMAT_G8R8_G8B8_UNORM";
		case DXGI_FORMAT_BC1_TYPELESS: return "DXGI_FORMAT_BC1_TYPELESS";
		case DXGI_FORMAT_BC1_UNORM: return "DXGI_FORMAT_BC1_UNORM";
		case DXGI_FORMAT_BC1_UNORM_SRGB: return "DXGI_FORMAT_BC1_UNORM_SRGB";
		case DXGI_FORMAT_BC2_TYPELESS: return "DXGI_FORMAT_BC2_TYPELESS";
		case DXGI_FORMAT_BC2_UNORM: return "DXGI_FORMAT_BC2_UNORM";
		case DXGI_FORMAT_BC2_UNORM_SRGB: return "DXGI_FORMAT_BC2_UNORM_SRGB";
		case DXGI_FORMAT_BC3_TYPELESS: return "DXGI_FORMAT_BC3_TYPELESS";
		case DXGI_FORMAT_BC3_UNORM: return "DXGI_FORMAT_BC3_UNORM";
		case DXGI_FORMAT_BC3_UNORM_SRGB: return "DXGI_FORMAT_BC3_UNORM_SRGB";
		case DXGI_FORMAT_BC4_TYPELESS: return "DXGI_FORMAT_BC4_TYPELESS";
		case DXGI_FORMAT_BC4_UNORM: return "DXGI_FORMAT_BC4_UNORM";
		case DXGI_FORMAT_BC4_SNORM: return "DXGI_FORMAT_BC4_SNORM";
		case DXGI_FORMAT_BC5_TYPELESS: return "DXGI_FORMAT_BC5_TYPELESS";
		case DXGI_FORMAT_BC5_UNORM: return "DXGI_FORMAT_BC5_UNORM";
		case DXGI_FORMAT_BC5_SNORM: return "DXGI_FORMAT_BC5_SNORM";
		case DXGI_FORMAT_B5G6R5_UNORM: return "DXGI_FORMAT_B5G6R5_UNORM";
		case DXGI_FORMAT_B5G5R5A1_UNORM: return "DXGI_FORMAT_B5G5R5A1_UNORM";
		case DXGI_FORMAT_B8G8R8A8_UNORM: return "DXGI_FORMAT_B8G8R8A8_UNORM";
		case DXGI_FORMAT_B8G8R8X8_UNORM: return "DXGI_FORMAT_B8G8R8X8_UNORM";
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return "DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM";
		case DXGI_FORMAT_B8G8R8A8_TYPELESS: return "DXGI_FORMAT_B8G8R8A8_TYPELESS";
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return "DXGI_FORMAT_B8G8R8A8_UNORM_SRGB";
		case DXGI_FORMAT_B8G8R8X8_TYPELESS: return "DXGI_FORMAT_B8G8R8X8_TYPELESS";
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return "DXGI_FORMAT_B8G8R8X8_UNORM_SRGB";
		case DXGI_FORMAT_BC6H_TYPELESS: return "DXGI_FORMAT_BC6H_TYPELESS";
		case DXGI_FORMAT_BC6H_UF16: return "DXGI_FORMAT_BC6H_UF16";
		case DXGI_FORMAT_BC6H_SF16: return "DXGI_FORMAT_BC6H_SF16";
		case DXGI_FORMAT_BC7_TYPELESS: return "DXGI_FORMAT_BC7_TYPELESS";
		case DXGI_FORMAT_BC7_UNORM: return "DXGI_FORMAT_BC7_UNORM";
		case DXGI_FORMAT_BC7_UNORM_SRGB: return "DXGI_FORMAT_BC7_UNORM_SRGB";
		case DXGI_FORMAT_AYUV: return "DXGI_FORMAT_AYUV";
		case DXGI_FORMAT_Y410: return "DXGI_FORMAT_Y410";
		case DXGI_FORMAT_Y416: return "DXGI_FORMAT_Y416";
		case DXGI_FORMAT_NV12: return "DXGI_FORMAT_NV12";
		case DXGI_FORMAT_P010: return "DXGI_FORMAT_P010";
		case DXGI_FORMAT_P016: return "DXGI_FORMAT_P016";
		case DXGI_FORMAT_420_OPAQUE: return "DXGI_FORMAT_420_OPAQUE";
		case DXGI_FORMAT_YUY2: return "DXGI_FORMAT_YUY2";
		case DXGI_FORMAT_Y210: return "DXGI_FORMAT_Y210";
		case DXGI_FORMAT_Y216: return "DXGI_FORMAT_Y216";
		case DXGI_FORMAT_NV11: return "DXGI_FORMAT_NV11";
		case DXGI_FORMAT_AI44: return "DXGI_FORMAT_AI44";
		case DXGI_FORMAT_IA44: return "DXGI_FORMAT_IA44";
		case DXGI_FORMAT_P8: return "DXGI_FORMAT_P8";
		case DXGI_FORMAT_A8P8: return "DXGI_FORMAT_A8P8";
		case DXGI_FORMAT_B4G4R4A4_UNORM: return "DXGI_FORMAT_B4G4R4A4_UNORM";
		case DXGI_FORMAT_P208: return "DXGI_FORMAT_P208";
		case DXGI_FORMAT_V208: return "DXGI_FORMAT_V208";
		case DXGI_FORMAT_V408: return "DXGI_FORMAT_V408";
		case DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE: return "DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE";
		case DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE: return "DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE";
		case DXGI_FORMAT_FORCE_UINT: return "DXGI_FORMAT_FORCE_UINT";
		default: return "unknown";
		}
	}
}
