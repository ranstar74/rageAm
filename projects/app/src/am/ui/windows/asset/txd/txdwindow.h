#pragma once

#include "am/asset/types/txd.h"
#include "am/graphics/texture/imagefit.h"
#include "am/ui/image.h"
#include "am/ui/styled/slwidgets.h"
#include "am/ui/windows/asset/assetwindow.h"
#include "helpers/format.h"

namespace rageam::ui::assetview
{
	static constexpr ConstString sm_FormatDisplay[] =
	{
		"Automatic",
		"RGBA - raw, alpha",
		"BC1 (DXT1) - diffuse / specular, no alpha",
		"BC2 (DXT3) - diffuse, alpha",
		"BC3 (DXT5) - diffuse, alpha",
		"BC4 (ATI1) - basic specular, 1 channel",
		"BC5 (BC5U) - normal",
		"BC7 - diffuse, alpha",
	};

	static constexpr ConstString sm_MipFilterDisplay[] = { "Box", "Triangle", "Kaiser" };
	static constexpr ConstString sm_ResizeFilterDisplay[] = { "Box", "Triangle", "Kaiser", "Mitchell" };

	// Supported DXGI formats

	static constexpr ConstString sm_FormatString[] = { "Auto", "RGBA", "BC1", "BC2", "BC3", "BC4", "BC5", "BC7" };
	static constexpr DXGI_FORMAT sm_FormatDirect3D[] =
	{
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_B8G8R8A8_UNORM,
		DXGI_FORMAT_BC1_UNORM,
		DXGI_FORMAT_BC2_UNORM,
		DXGI_FORMAT_BC3_UNORM,
		DXGI_FORMAT_BC4_UNORM,
		DXGI_FORMAT_BC5_UNORM,
		DXGI_FORMAT_BC7_UNORM,
	};

	static constexpr ConstString sm_QualityDisplay[] = { "Fastest", "Normal", "Production", "Highest", };

	static constexpr u32 sm_MaxSize[] = { 1 << 14, 1 << 13, 1 << 12, 1 << 11, 1 << 10, 1 << 9, 1 << 8, 1 << 7 };
	static constexpr ConstString sm_MaxSizeDisplay[] = { "16384", "8192", "4096", "2048", "1024", "512", "256", "128" };

	struct TextureOptionsVM
	{
		s32		MaxSizeIndex = 2;		// 4096
		s32		MipFilterIndex = 2;		// Kaiser
		s32		ResizeFilterIndex = 3;	// Mitchell
		s32		FormatIndex = 0;		// Auto
		s32		QualityIndex = 1;		// Normal
		s32		CurrentMip = 0;
		bool	GenerateMips = true;

		bool operator==(const TextureOptionsVM& other) const = default;
	};

	struct TextureVM
	{
		asset::Texture			Model;				// Copy of texture that we are editing
		TextureOptionsVM		Options;
		Image					Preview;
		texture::CompressedInfo PreviewInfo{};
		bool					ForceAllowRecompression = false; // For users that want to re-compress .dds for their own reasons

		TextureVM(const asset::Texture& model) : Model(model)
		{
			// Convert options from model to view model
			Options.MipFilterIndex = static_cast<int>(Model.Options.MipFilter);
			Options.ResizeFilterIndex = static_cast<int>(Model.Options.ResizeFilter);
			Options.QualityIndex = static_cast<int>(Model.Options.Quality);
			Options.GenerateMips = Model.Options.GenerateMips;

			// Linear search for max size index, map is overkill here
			Options.MaxSizeIndex = std::ranges::distance(
				sm_MaxSize, std::ranges::find(sm_MaxSize, Model.Options.MaxSize));
			// Same goes for format
			Options.FormatIndex = std::ranges::distance(
				sm_FormatDirect3D, std::ranges::find(sm_FormatDirect3D, Model.Options.Format));

			// We generate preview for every texture on loading just to have icons
			GeneratePreview();
		}

		void SetOptionsWithUndo(const TextureOptionsVM& newOptions)
		{
			UndoStack::GetCurrent()->Add(new UndoableState(Options, newOptions, [this]
				{
					// Set options from texture view model to model
					Model.Options.MaxSize = sm_MaxSize[Options.MaxSizeIndex];
					Model.Options.MipFilter = static_cast<texture::MipFilter>(Options.MipFilterIndex);
					Model.Options.ResizeFilter = static_cast<texture::ResizeFilter>(Options.ResizeFilterIndex);
					Model.Options.Format = sm_FormatDirect3D[Options.FormatIndex];
					Model.Options.Quality = static_cast<texture::CompressorQuality>(Options.QualityIndex);
					Model.Options.GenerateMips = Options.GenerateMips;

					// Generate preview for new settings
					GeneratePreview();
				}));
		}

		void GeneratePreview()
		{
			file::WPath path = Model.GetFullPath();

			texture::CompressionOptions compressOptions;
			Model.Options.GetCompressionOptions(compressOptions);

			Preview.LoadAsync(path, compressOptions, PreviewInfo);
		}
	};
	using TextureViewModels = rage::atArray<TextureVM>;

	/**
	 * \brief UI Editor for Texture Dictionary.
	 */
	class TxdWindow : public AssetWindow
	{
		TextureViewModels	m_TextureVms;
		int					m_SelectedIndex = -1;

		void RenderProperties()
		{
			// TODO: Move this into TextureVM

			TextureVM& textureVm = m_TextureVms[m_SelectedIndex];
			TextureOptionsVM newOptions = textureVm.Options;
			Image& preview = textureVm.Preview;
			const texture::Compressor& compressor = preview.GetSurface().GetCompressor();

			SlGui::CategoryText("Compression");
			{
				bool edited = false;
				bool noCompression = textureVm.Model.IsPreCompressed && !textureVm.ForceAllowRecompression;

				if (noCompression)
				{
					ImGui::HelpMarker(
						"The texture is already compressed (based on .dds extension) and re-compressing will not make the quality any better, it will only get worse.",
						"Why disabled");

					// TODO: We need to pass options into asset compiler
					//ImGui::Checkbox("I know what i'm doing.", &textureVm.ForceAllowRecompression);

					ImGui::BeginDisabled();
				}

				if (SlGui::Combo("Max Size", &newOptions.MaxSizeIndex, sm_MaxSizeDisplay, IM_ARRAYSIZE(sm_MaxSizeDisplay)))
					edited = true;

				if (SlGui::Combo("Quality", &newOptions.QualityIndex, sm_QualityDisplay, IM_ARRAYSIZE(sm_QualityDisplay)))
					edited = true;

				if (compressor.SupportsBlockCompression())
				{
					if (SlGui::Combo("Format", &newOptions.FormatIndex, sm_FormatDisplay, IM_ARRAYSIZE(sm_FormatDisplay)))
						edited = true;
				}
				else // Force RGBA for textures that don't support block compression (resolution is not multiple of 4)
				{
					ImGui::HelpMarker(
						"Block compression (DXT / BC) is only supported on textures with a resolution that is a multiple of 4.",
						"Why disabled");

					newOptions.FormatIndex = 1; // RGBA
					ImGui::BeginDisabled();
					SlGui::Combo("Format", &newOptions.FormatIndex, sm_FormatDisplay, IM_ARRAYSIZE(sm_FormatDisplay));
					ImGui::EndDisabled();
				}

				if (SlGui::Combo("Resize Filter", &newOptions.ResizeFilterIndex, sm_ResizeFilterDisplay, IM_ARRAYSIZE(sm_ResizeFilterDisplay)))
					edited = true;

				if (SlGui::Combo("Mip Filter", &newOptions.MipFilterIndex, sm_MipFilterDisplay, IM_ARRAYSIZE(sm_MipFilterDisplay)))
					edited = true;

				if (SlGui::Checkbox("Mip Maps", &newOptions.GenerateMips))
					edited = true;

				if (noCompression) ImGui::EndDisabled();

				if (edited && textureVm.Options != newOptions)
				{
					textureVm.SetOptionsWithUndo(newOptions);
				}
			}

			if (newOptions.GenerateMips)
			{
				SlGui::CategoryText("Preview");
				{
					// TODO: Not functioning properly, need fix
					int mipMin = 0;
					int mipMax = textureVm.Preview
						.GetSurface()
						.GetCompressor()
						.GetAvailableMipCount();

					mipMax -= 1; // If there's 1 mip map (max - 1) we have range (0 - 0)

					ImGui::SliderInt("Current Mip", &newOptions.CurrentMip, mipMin, mipMax);
				}
			}
		}

		void RenderTextureList()
		{
			for (u16 i = 0; i < m_TextureVms.GetSize(); i++)
			{
				TextureVM& textureVm = m_TextureVms[i];

				Image& icon = textureVm.Preview;

				SlRenamingSelectableState state = {};
				state.TextDisplay = textureVm.Model.Name;
				state.Selected = i == m_SelectedIndex;
				state.Icon = icon.GetID();
				state.IconWidth = icon.GetWidth();
				state.IconHeight = icon.GetHeight();
				state.IconScale = 2.0f;

				if (SlGui::RenamingSelectable(state))
				{
					m_SelectedIndex = i;
				}
			}
		}

		void RenderPreview()
		{
			TextureVM& textureVm = m_TextureVms[m_SelectedIndex];
			Image& preview = textureVm.Preview;
			texture::CompressedInfo& info = textureVm.PreviewInfo;

			bool isLoading = preview.IsLoading();

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::BeginChild("ImagePreview", ImVec2(0, 0), false, ImGuiWindowFlags_MenuBar);
			ImGui::PopStyleVar();
			SlGui::ShadeItem(SlGuiCol_Bg);

			ImGui::BeginMenuBar();
			{
				SlGui::ShadeItem(SlGuiCol_Bg2);

				// Display statistics on compressed image

				SlGui::PushFont(SlFont_Medium);
				if (isLoading)
				{
					// 'Loading' indicator
					int type = (int)(ImGui::GetTime() * 5) % 3;
					ImGui::Text(type == 0 ? "." : type == 1 ? ".." : "...");
				}
				else
				{
					char sizeBuffer[16];
					FormatBytes(sizeBuffer, 16, info.Size);

					ConstString formatDisplay = D3D::DxgiFormatToString(info.Format);
					formatDisplay += sizeof "DXGI_FORMAT_" - 1 /* Null-Terminator */;

					// 512x512 1 Mip(s) RGBA 32 bit 4.6 MB
					char infoBuffer[64];
					sprintf_s(infoBuffer, 64, "%ux%u %u Mip(s) %s %u bit %s",
						info.Width, info.Height,
						info.MipCount,
						formatDisplay,
						D3D::BitsPerPixel(info.Format),
						sizeBuffer);

					ImGui::Text(infoBuffer);
				}
				ImGui::PopFont();
			}
			ImGui::EndMenuBar();

			// Stretch & center image on remaining space
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImRect& workRect = window->WorkRect;
			float availableX = workRect.GetWidth();
			float availableY = workRect.GetHeight();

			auto [pWidth, pHeight] =
				Resize(info.Width, info.Height, availableX, availableY, texture::ScalingMode_Fit);

			ImGui::SetCursorPosX((availableX - pWidth) / 2.0f); // Center image
			preview.Render(pWidth, pHeight);

			// Mouse coordinates in image space for zoom (we have to do it before drawing tooltip)
			ImGuiIO& io = ImGui::GetIO();
			ImGuiContext& ctx = *GImGui;
			float zoomPosX = io.MousePos.x - ctx.LastItemData.Rect.Min.x;
			float zoomPosY = io.MousePos.y - ctx.LastItemData.Rect.Min.y;

			// Zoom Region
			if (ImGui::BeginItemTooltip())
			{
				constexpr float zrSize = 256.0f;
				constexpr float zrRatio = 80.0f; // How we zoom in

				// Zoom tooltip rect
				float zrMinX = ImMax(zoomPosX - zrSize * 0.5f, 0.0f);
				float zrMinY = ImMax(zoomPosY - zrSize * 0.5f, 0.0f);
				float zrMaxX = zrMinX + zrSize;
				float zrMaxY = zrMinY + zrSize;

				// Zoomed picture rect
				ImRect zrRect(zrMinX, zrMinY, zrMaxX, zrMaxY);
				zrRect.Expand(-zrRatio);

				// Tex Coords
				float u1 = zrRect.Min.x / pWidth;
				float v1 = zrRect.Min.y / pHeight;
				float u2 = zrRect.Max.x / pWidth;
				float v2 = zrRect.Max.y / pHeight;

				// Render
				ImGui::Text("X: %.0f Y: %.0f", io.MousePos.x, ctx.LastItemData.Rect.Min.x);
				ImGui::Image(preview.GetID(), ImVec2(zrSize, zrSize), ImVec2(u1, v1), ImVec2(u2, v2));

				ImGui::EndTooltip();
			}

			ImGui::EndChild(); // ImagePreview
		}

		void OnRender() override
		{
			// Remove huge gap between the columns
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(1, 0));
			bool tableOpened = ImGui::BeginTable("TxdAppTable", 3, ImGuiTableFlags_Resizable);
			ImGui::PopStyleVar();
			if (!tableOpened)
				return;

			ImGui::TableSetupColumn("Texture List");
			ImGui::TableSetupColumn("Preview");
			ImGui::TableSetupColumn("Properties");

			// Texture List
			ImGui::TableNextColumn();
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
				//if (ImGui::BeginChild("TxdAppTable_TextureList"))
				//{
				//	SlGui::ShadeItem(SlGuiCol_Bg);

				//	RenderTextureList();
				//	ImGui::EndChild();
				//}
				if (SlGui::BeginPadded("TxdAppTable_TextureList", ImVec2(4, 4)))
				{
					SlGui::ShadeItem(SlGuiCol_Bg);
					RenderTextureList();
					SlGui::EndPadded();
				}
				ImGui::PopStyleVar(); // ItemSpacing
			}

			// Texture Preview
			ImGui::TableNextColumn();
			if (m_SelectedIndex != -1)
			{
				RenderPreview();
			}

			// Texture Properties
			ImGui::TableNextColumn();
			if (m_SelectedIndex != -1)
			{
				if (SlGui::BeginPadded("Properties", ImVec2(10, 10)))
					RenderProperties();
				SlGui::EndPadded();
			}

			ImGui::EndTable();
		}
	public:
		TxdWindow(const asset::AssetPtr& asset) : AssetWindow(asset)
		{
			amPtr<asset::TxdAsset> txd = std::static_pointer_cast<asset::TxdAsset>(asset);

			auto& textures = txd->GetTextures();

			// Create view model for every texture
			m_TextureVms.Reserve(textures.GetNumUsedSlots());
			for (const asset::Texture& texture : textures)
			{
				m_TextureVms.Construct(texture);
			}

			if (textures.Any())
				m_SelectedIndex = 0;
		}

		void SaveChanges() override
		{
			amPtr<asset::TxdAsset> txd = std::static_pointer_cast<asset::TxdAsset>(GetAsset());

			auto& textures = txd->GetTextures();

			// Replace old texture configs with new ones
			for (const TextureVM& vm : m_TextureVms)
			{
				textures.Insert(vm.Model);
			}
		}

		ImVec2 GetDefaultSize() override { return { 740, 350 }; }
	};
}
