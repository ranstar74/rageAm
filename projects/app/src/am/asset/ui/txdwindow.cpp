#include "txdwindow.h"

#include "am/asset/types/texpresets.h"
#include "am/graphics/render.h"
#include "am/ui/extensions.h"
#include "am/ui/font_icons/icons_am.h"
#include "am/ui/imglue.h"
#include "helpers/format.h"

bool rageam::ui::RenderCompressOptionsControls(graphics::ImageCompressorOptions& options, bool compressorAvailable, float itemWidth)
{
	bool needRecompress = false;

	SlGui::CategoryText("Compression Options");

	// Quality
	bool qualityAvailable = options.Format == graphics::BlockFormat_None;
	if (qualityAvailable) ImGui::BeginDisabled();
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::SliderFloat("Quality", &options.Quality, 0.0f, 1.0f);
	if (ImGui::IsItemDeactivated())
		needRecompress = true;
	if (qualityAvailable) ImGui::EndDisabled();

	// Compression format
	static constexpr ConstString s_BlockFormats[] =
	{
		"Raw",
		"BC1 (DXT1)",
		"BC2 (DXT3)",
		"BC3 (DXT5)",
		"BC4 (ATI1)",
		"BC5 (ATI2)",
		"BC7",
	};
	if (!compressorAvailable) ImGui::BeginDisabled();
	ImGui::SetNextItemWidth(itemWidth);
	if (ImGui::Combo("Format", (int*)&options.Format, s_BlockFormats, IM_ARRAYSIZE(s_BlockFormats)))
		needRecompress = true;
	if (!compressorAvailable) ImGui::EndDisabled();
	if (!compressorAvailable)
	{
		ImGui::SameLine();
		ImGui::HelpMarker("Image resolution is not power or 4, only raw format is available.");
	}

	// Mip Filter
	static constexpr ConstString s_ResizeFilters[] =
	{
		"Box", "Triangle", "CubicBSpline", "CatmullRom", "Mitchell"
	};
	ImGui::SetNextItemWidth(itemWidth);
	if (ImGui::Combo("Mip Filter", (int*)&options.MipFilter, s_ResizeFilters, IM_ARRAYSIZE(s_ResizeFilters)))
		needRecompress = true;

	if (ImGui::Checkbox("Generate Mips", &options.GenerateMipMaps))
		needRecompress = true;

	SlGui::CategoryText("Post Processing");

	// Max size
	static constexpr int s_MaxResolutions[] = { 0, 1 << 11, 1 << 10, 1 << 9, 1 << 8, 1 << 7, 1 << 6, 1 << 5 };
	static constexpr ConstString s_MaxResolutionStrings[] = { "-", "2048", "1024", "512", "256", "128", "64", "32" };
	// Linear search for max size index
	int maxResCurrIndex = std::ranges::distance(s_MaxResolutions, std::ranges::find(s_MaxResolutions, options.MaxResolution));
	bool addSubMaxSize = 
		ImGui::AddSubButtons<int>("MaxSizeAddSub", maxResCurrIndex, 0, IM_ARRAYSIZE(s_MaxResolutionStrings) - 1, false);
	ImGui::SameLine(0, 1);
	ImGui::SetNextItemWidth(itemWidth * 0.6f);
	if (ImGui::Combo("Max Size", &maxResCurrIndex, s_MaxResolutionStrings, IM_ARRAYSIZE(s_MaxResolutionStrings)) || addSubMaxSize)
	{
		options.MaxResolution = s_MaxResolutions[maxResCurrIndex];
		needRecompress = true;
	}

	ImGui::SetNextItemWidth(itemWidth);
	if (ImGui::SliderInt("Brightness", &options.Brightness, -100, 100))
		needRecompress = true;

	ImGui::SetNextItemWidth(itemWidth);
	if (ImGui::SliderInt("Contrast", &options.Contrast, -100, 100))
		needRecompress = true;

	// Cutout alpha
	if (ImGui::Checkbox("Cutout Alpha", &options.CutoutAlpha))
		needRecompress = true;
	ImGui::SetNextItemWidth(itemWidth);
	if (!options.CutoutAlpha) ImGui::BeginDisabled();
	ImGui::SliderInt("###CUTOUT", &options.CutoutAlphaThreshold, 0, 255);
	if (ImGui::IsItemDeactivated())
		needRecompress = true;
	if (!options.CutoutAlpha) ImGui::EndDisabled();

	// Test alpha coverage
	if (ImGui::Checkbox("Alpha Coverage", &options.AlphaTestCoverage))
		needRecompress = true;
	if (!options.AlphaTestCoverage) ImGui::BeginDisabled();
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::SliderInt("###ALPHA", &options.AlphaTestThreshold, 0, 255);
	if (ImGui::IsItemDeactivated())
		needRecompress = true;
	if (!options.AlphaTestCoverage) ImGui::EndDisabled();

	return needRecompress;
}

rageam::ui::TextureVM::AsyncImage::~AsyncImage()
{
	CancelAsyncLoading();
}

void rageam::ui::TextureVM::AsyncImage::LoadAsyncCompressed(const file::WPath& path, const graphics::ImageCompressorOptions& options)
{
	CancelAsyncLoading();

	LoadTask = BackgroundWorker::Run([this, options, path]
		{
			Timer timer = Timer::StartNew();

			IsLoading = true;

			graphics::CompressedImageInfo compressedInfo;
			graphics::ImagePtr image = graphics::ImageFactory::LoadFromPathAndCompress(
				path, options, &compressedInfo, &LoadToken);
			if (!image)
			{
				IsLoading = false;
				return false;
			}

			amComPtr<ID3D11ShaderResourceView> view;
			graphics::ImageDX11ResourceOptions resourceOptions = {};
			resourceOptions.CreateMips = false;
			resourceOptions.PadToPowerOfTwo = false;
			resourceOptions.AllowImageConversion = false;
			image->CreateDX11Resource(view, resourceOptions);

			timer.Stop();

			Mutex.lock();
			ViewPending = std::move(view);
			Image = std::move(image);
			CompressedInfo = compressedInfo;
			LastLoadTime = timer.GetElapsedMilliseconds();
			IsLoading = false;
			Mutex.unlock();

			return true;
		});
}

void rageam::ui::TextureVM::AsyncImage::LoadAsync(const file::WPath& path)
{
	LoadTask = BackgroundWorker::Run([this, path]
		{
			Timer timer = Timer::StartNew();

			IsLoading = true;

			graphics::ImagePtr image = graphics::ImageFactory::LoadFromPath(path);
			if (!image)
			{
				IsLoading = false;
				return false;
			}

			Vec2S uv2;
			amComPtr<ID3D11ShaderResourceView> view;
			graphics::ImageDX11ResourceOptions resourceOptions = {};
			resourceOptions.CreateMips = true;
			resourceOptions.PadToPowerOfTwo = true;
			resourceOptions.AllowImageConversion = true;
			resourceOptions.UpdateSourceImage = true;
			image->CreateDX11Resource(view, resourceOptions, &uv2);

			timer.Stop();

			Mutex.lock();
			ViewPending = std::move(view);
			Image = std::move(image);
			CompressedInfo = {};
			UV2 = ImVec2(uv2.X, uv2.Y);
			LastLoadTime = timer.GetElapsedMilliseconds();
			IsLoading = false;
			Mutex.unlock();

			return true;
		});
}

void rageam::ui::TextureVM::AsyncImage::CancelAsyncLoading()
{
	LoadToken.Canceled = true;
	if (LoadTask) LoadTask->Wait();
	LoadTask = nullptr;
}

ImTextureID rageam::ui::TextureVM::AsyncImage::GetTextureID()
{
	std::unique_lock lock(Mutex);
	if (ViewPending)
	{
		View = ViewPending;
		ViewPending = nullptr;
	}
	return View.Get();
}

rageam::ui::TextureVM::SharedState::SharedState()
{
	ID3D11Device* device = graphics::RenderGetDevice();
	ID3D11SamplerState* samplerState;

	// Nearest sampling for checker, we don't want it to look muddy when zooming
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = FLT_MAX;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	AM_ASSERT_STATUS(device->CreateSamplerState(&samplerDesc, &samplerState));
	CheckerSampler = amComPtr(samplerState);

	// Linear sampler by default
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	AM_ASSERT_STATUS(device->CreateSamplerState(&samplerDesc, &samplerState));
	DefaultSampler = amComPtr(samplerState);

	graphics::ImageFactory::CreateChecker_Opacity()->CreateDX11Resource(Checker);
}

ID3D11SamplerState* rageam::ui::TextureVM::SharedState::GetSamplerStateFor(int mipLevel, bool linear)
{
	// Default view, created by default
	if (linear && mipLevel == 0)
		return DefaultSampler.Get();

	amComPtr<ID3D11SamplerState>* states = linear ? LodLinearSamplers : LodPointSamplers;

	// Create sampler on request
	if (!states[mipLevel])
	{
		ID3D11Device* device = graphics::RenderGetDevice();
		ID3D11SamplerState* samplerState;

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.MinLOD = static_cast<float>(mipLevel);
		samplerDesc.MaxLOD = static_cast<float>(mipLevel);

		samplerDesc.Filter = linear ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;
		AM_ASSERT_STATUS(device->CreateSamplerState(&samplerDesc, &samplerState));
		states[mipLevel] = amComPtr(samplerState);
	}

	return states[mipLevel].Get();
}

ImVec2 rageam::ui::TextureVM::WorldToScreen(ImVec2 pos) const
{
	// Center
	pos -= m_HalfViewportSize;
	// To camera location
	pos.x += m_ImageView.X;
	pos.y += m_ImageView.Y;
	// Apply depth
	pos.x *= m_ImageView.Z;
	pos.y *= m_ImageView.Z;
	// Un-center
	pos += m_HalfViewportSize;
	return pos;
}

ImVec2 rageam::ui::TextureVM::ScreenToWorld(ImVec2 pos) const
{
	// Center
	pos -= m_HalfViewportSize;
	// Apply depth
	pos.x /= m_ImageView.Z;
	pos.y /= m_ImageView.Z;
	// To camera location
	pos.x -= m_ImageView.X;
	pos.y -= m_ImageView.Y;
	// Un-center
	pos += m_HalfViewportSize;
	return pos;
}

ID3D11SamplerState* s_OldSamplerState;
void rageam::ui::TextureVM::BackupSamplerCallback(const ImDrawList*, const ImDrawCmd*)
{
	ID3D11DeviceContext* context = graphics::RenderGetContext();
	context->PSGetSamplers(0, 1, &s_OldSamplerState);
}

void rageam::ui::TextureVM::RestoreSamplerCallback(const ImDrawList*, const ImDrawCmd*)
{
	ID3D11DeviceContext* context = graphics::RenderGetContext();
	context->PSSetSamplers(0, 1, &s_OldSamplerState);
}

void rageam::ui::TextureVM::SetSamplerCallback(const ImDrawList*, const ImDrawCmd* cmd)
{
	SamplerCallbackState* state = (SamplerCallbackState*)&cmd->UserCallbackData;
	ID3D11DeviceContext* context = graphics::RenderGetContext();
	ID3D11SamplerState* sampler = sm_SharedState->GetSamplerStateFor(state->LOD, state->IsLinear);
	context->PSSetSamplers(0, 1, &sampler);
}

void rageam::ui::TextureVM::SetCheckerSamplerCallback(const ImDrawList*, const ImDrawCmd*)
{
	ID3D11DeviceContext* context = graphics::RenderGetContext();
	ID3D11SamplerState* sampler = sm_SharedState->CheckerSampler.Get();
	context->PSSetSamplers(0, 1, &sampler);
}

void rageam::ui::TextureVM::SetCompressOptionsWithUndo(const graphics::ImageCompressorOptions& newCompressOptions)
{
	struct SetCompressOptionsState
	{
		graphics::ImageCompressorOptions	CompressOptions;
		bool								CompressOptionsMatchPreset;
	};

	SetCompressOptionsState undoState(m_CompressOptions, m_CompressOptionsMatchPreset);
	SetCompressOptionsState doState(newCompressOptions, false);

	UndoStack::GetCurrent()->Add(
		new UndoableStateEx<SetCompressOptionsState>(undoState, doState, [this](const SetCompressOptionsState& state)
			{
				m_CompressOptions = state.CompressOptions;
				m_CompressOptionsPending = state.CompressOptions;
				m_CompressOptionsMatchPreset = state.CompressOptionsMatchPreset;
				ReloadImage(true);
				m_Parent->NotifyHotDrawableThatConfigWasChanged();
			}));
}

rageam::ui::TextureVM::TextureVM(TxdWindow* parent, asset::TextureTune* tune)
{
	m_Parent = parent;
	m_Tune = tune;

	m_CompressOptions = tune->GetCustomOptionsOrFromPreset(&m_MatchedTexturePreset).CompressorOptions;
	m_CompressOptionsPending = m_CompressOptions;
	m_CompressOptionsMatchPreset = m_MatchedTexturePreset != nullptr;

	// Compressed is always loaded, RAW is on request...
	m_Compressed = std::make_unique<AsyncImage>();
	m_Compressed->LoadAsyncCompressed(m_Tune->GetFilePath(), m_CompressOptions);

	// Shared globally, destroyed with the system
	if (!sm_SharedState) sm_SharedState = std::make_unique<SharedState>();

	UpdateTextureName();
	ResetViewport();
}

bool rageam::ui::TextureVM::RenderListEntry(bool selected) const
{
	ImTextureID icon;
	int iconWidth, iconHeight;
	GetIcon(icon, iconWidth, iconHeight);

	SlRenamingSelectableState state = {};
	state.TextDisplay = m_TextureName;
	state.Selected = selected;
	state.IconScale = 2.0f;
	state.Icon = icon;
	state.IconBg = sm_SharedState->Checker.Get();
	state.IconBgUV2 = { 0.125f, 0.125f }; // Checker is pretty dense... way too dense for small icon
	state.IconWidth = static_cast<float>(iconWidth);
	state.IconHeight = static_cast<float>(iconHeight);
	state.IconPadding = false;
	state.SpanAllColumns = false;
	state.ScrollingText = true;

	if (m_IsMissing) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.15f);
	bool clicked = SlGui::RenamingSelectable(state);
	if (m_IsMissing) ImGui::PopStyleVar();

	// Draw glowing outline around missing items
	if (m_IsMissing)
	{
		ImRect nodeRect = GImGui->LastItemData.Rect;
		nodeRect.Expand(ImVec2(0, -1));
		ImGui::GetWindowDrawList()->AddRect(nodeRect.Min, nodeRect.Max, ImGui::GetMissingColor());
	}

	return clicked;
}

void rageam::ui::TextureVM::RenderMiscProperties()
{
	ConstString presetName = m_CompressOptionsMatchPreset ? m_MatchedTexturePreset->Name.GetCStr() : "-";
	ImGui::Text("Preset:");
	ImGui::SameLine(0, ImGui::GetFontSize() * 0.25f);
	ImGui::PushFont(ImFont_Medium);
	ImGui::Text(presetName);
	ImGui::PopFont();

	bool canResetOptions = !m_CompressOptionsMatchPreset;
	if (!canResetOptions) ImGui::BeginDisabled();
	if (ImGui::Button("Reset settings (to preset)"))
	{
		struct ResetCompressOptionsState
		{
			graphics::ImageCompressorOptions	CompressOptions;
			bool								CompressOptionsMatchPreset;
		};

		asset::TexturePresetPtr texPreset = m_Tune->MatchPreset();

		ResetCompressOptionsState undoState(m_CompressOptions, m_CompressOptionsMatchPreset);
		ResetCompressOptionsState doState(texPreset->Options.CompressorOptions, true);

		UndoStack::GetCurrent()->Add(
			new UndoableStateEx<ResetCompressOptionsState>(undoState, doState, [this](const ResetCompressOptionsState& state)
				{
					m_CompressOptions = state.CompressOptions;
					m_CompressOptionsPending = state.CompressOptions;
					m_CompressOptionsMatchPreset = state.CompressOptionsMatchPreset;
					ReloadImage(true);
					m_Parent->NotifyHotDrawableThatConfigWasChanged();
				}));
	}
	if (!canResetOptions) ImGui::EndDisabled();

	static constexpr ConstString COPY_SETTINGS_POPUP = "Copy settings###COPY_SETTINGS_POPUP";
	if (ImGui::Button("Copy settings to..."))
	{
		// Reset selection from previous copying
		for (u32 i = 0; i < m_Parent->m_TextureVMs.GetSize(); i++)
			m_Parent->m_TextureVMs[i].m_MarkedForCopying = false;

		ImGui::OpenPopup(COPY_SETTINGS_POPUP);
	}
	ImGui::SetNextWindowSize(ImVec2(0, ImGui::GetFrameHeight() * 12), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal(COPY_SETTINGS_POPUP))
	{
		ImGui::Text("Copy settings from '%s' to selected textures:", m_TextureName);

		// Batch selection / deselection
		bool selectAll = ImGui::Button("Select All");
		ImGui::SameLine();
		bool selectNone = ImGui::Button("Select None");
		if (selectAll || selectNone)
		{
			for (u32 i = 0; i < m_Parent->m_TextureVMs.GetSize(); i++)
			{
				// Skip current texture
				if (i == m_Parent->m_SelectedIndex)
					continue;

				// selectAll will be false if 'Select None' was pressed
				m_Parent->m_TextureVMs[i].m_MarkedForCopying = selectAll;
			}
		}

		// Texture list for picking (with checkboxes), we hold it in child for scrolling 
		ImVec2 childSize( // Leave space for buttons, they're horizontal, so it's frame height and padding
			0,
			ImGui::GetContentRegionAvail().y - (ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.y));
		// Pack items as dense as possible
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
		if (ImGui::BeginChild("COPY_TEX_LIST", childSize))
		{
			for (u32 i = 0; i < m_Parent->m_TextureVMs.GetSize(); i++)
			{
				// Don't skip source texture because it makes harder to navigate, display it as disabled instead
				bool disabled = i == m_Parent->m_SelectedIndex;

				TextureVM& textureVM = m_Parent->m_TextureVMs[i];

				// Image didn't finish initial loading, we must skip it because we don't know
				// if we can copy compressed format or not... see how OK button is handled below
				if (!textureVM.m_Compressed->Image)
					disabled = true;

				if (disabled) ImGui::BeginDisabled();
				ImGui::Checkbox(textureVM.m_TextureName, &textureVM.m_MarkedForCopying);
				if (disabled) ImGui::EndDisabled();
			}
		}
			ImGui::EndChild();
		ImGui::PopStyleVar(); // ItemSpacing

		// Bottom panel
		if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("OK"))
		{
			UndoStack* undo = UndoStack::GetCurrent();
			undo->BeginGroup();
			for (u32 i = 0; i < m_Parent->m_TextureVMs.GetSize(); i++)
			{
				TextureVM* textureVM = &m_Parent->m_TextureVMs[i];
				if (!textureVM->m_MarkedForCopying)
					continue;

				graphics::ImageCompressorOptions compressOptions = m_CompressOptions;
				// Image can't be compressed (resolution is not power of 4),
				// ->Image cannot be NULL though because it was filtered out in list (see above...)
				if (textureVM->m_Compressed->Image && !textureVM->m_Compressed->Image->CanBeCompressed())
					compressOptions.Format = graphics::BlockFormat_None;

				textureVM->SetCompressOptionsWithUndo(compressOptions);
			}
			undo->EndGroup();

			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup(); // COPY_SETTINGS_POPUP
	}

	// Display string for override preset...
	ConstString overridePreset = m_Tune->OverridePreset;
	if (String::IsNullOrEmpty(overridePreset))
		overridePreset = "-";

	ImGui::Text("Override Preset:");
	if (ImGui::BeginCombo("###TEX_OVERRIDE_PRESET", overridePreset))
	{
		// Reset option
		if (ImGui::Selectable("-"))
		{
			m_Tune->OverridePreset = "";
			ReloadPreset();
			ReloadImage();
			m_Parent->NotifyHotDrawableThatConfigWasChanged();
		}

		asset::TexturePresetStore* presetStore = asset::TexturePresetStore::GetInstance();
		asset::TexturePresetCollection& workspacePresets =
			presetStore->GetPresets(m_Tune->GetParent()->GetWorkspacePath());

		presetStore->Lock();
		for (const asset::TexturePresetPtr& preset : workspacePresets.Items)
		{
			if (ImGui::Selectable(preset->Name))
			{
				m_Tune->OverridePreset = preset->Name;
				SetPreset(preset);
				ReloadImage();
				m_Parent->NotifyHotDrawableThatConfigWasChanged();
			}
		}
		presetStore->Unlock();

		ImGui::EndCombo();
	}
}

void rageam::ui::TextureVM::RenderImageProperties()
{
	// Make sure that image is not modified when we're using it....
	amUPtr<AsyncImage>& im = m_DisplayCompressed ? m_Compressed : m_Raw;
	im->Mutex.lock();

	float fontSize = ImGui::GetFontSize();
	float itemWidth = fontSize * 7.0f;

	bool uiDisabled = im->CompressedInfo.IsSourceCompressed && !m_CompressOptions.AllowRecompress;
	bool needRecompress = false;

	if (im->CompressedInfo.IsSourceCompressed)
	{
		ImGui::HelpMarker(
			"The texture is already block compressed and re-compressing will not make the quality any better, it will only get worse.\n"
			"DDS format is preserved.",
			"Why disabled");

		ImGui::SameLine();
		if (ImGui::Checkbox("Allow", &m_CompressOptionsPending.AllowRecompress))
			needRecompress = true;
	}

	if (uiDisabled) ImGui::BeginDisabled();

	ImGui::Checkbox("Display Compressed", &m_DisplayCompressed);

	// Advanced view enables mip (LOD) selection and point sampler
	ImGui::Checkbox("Advanced Preview", &m_AdvancedView);
	if (!m_AdvancedView) ImGui::BeginDisabled();
	{
		int mipCount = 1;
		if (im->Image)
			mipCount = im->Image->GetInfo().MipCount;

		if (mipCount == 1) ImGui::BeginDisabled();
		ImGui::SetNextItemWidth(itemWidth);
		ImGui::SliderInt("Mip Level", &m_MipLevel, 0, mipCount - 1);
		if (mipCount == 1) ImGui::EndDisabled();

		ImGui::Checkbox("Point Sampler", &m_PointSampler);
	}
	if (!m_AdvancedView) ImGui::EndDisabled();

	bool canCompress = m_Compressed->Image && m_Compressed->Image->CanBeCompressed();
	if (RenderCompressOptionsControls(m_CompressOptionsPending, canCompress, itemWidth))
		needRecompress = true;

	if (uiDisabled) ImGui::EndDisabled();

	im->Mutex.unlock();
	if (needRecompress)
	{
		SetCompressOptionsWithUndo(m_CompressOptionsPending);
	}

	SlGui::CategoryText("Misc");
	RenderMiscProperties();
}

void rageam::ui::TextureVM::RenderHoveredPixel(const graphics::ImagePtr& image) const
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImDrawList* dl = window->DrawList;

	// We allow NULL image during loading (only on first load actually, because we don't have anything to display yet...)
	graphics::ColorU32 pixelColor = graphics::COLOR_BLACK;
	int pixelX = 0;
	int pixelY = 0;
	if (image)
	{
		// We clamp mip level because if we switch from raw/compressed, they may have different number of mips...
		int mipLevel = ImClamp<int>(m_MipLevel, 0, image->GetMipCount() - 1);
		int width = image->GetWidth();
		int height = image->GetHeight();
		int mipWidth = width >> mipLevel;
		int mipHeight = height >> mipLevel;

		// Get exact selected pixel without fraction, at selected LOD
		pixelX = static_cast<int>(floorf(m_HoveredCoors.x * static_cast<float>(mipWidth)));
		pixelY = static_cast<int>(floorf(m_HoveredCoors.y * static_cast<float>(mipHeight)));
		// There might be float precision issues, make sure we don't go outside boundaries
		if (pixelX >= width) pixelX = width - 1;
		if (pixelY >= height) pixelY = height - 1;
		// Now we can get hovering pixel color
		pixelColor = image->GetPixel(pixelX, pixelY, mipLevel);
	}

	// Hovered info horizontal table:
	// Color | RGBA | Coors
	if (ImGui::BeginTable("TEX_HOVERED_INFO_TABLE", 3))
	{
		// Setup fixed width for columns, for 'worst-case' layout scenarios, preventing visual noise
		ImGui::TableSetupColumn("TEX_COL_COLOR", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("TEX_COL_RGBA", ImGuiTableColumnFlags_WidthFixed,
			ImGui::CalcTextSize("RGBA(255, 255, 255, 255)").x);
		ImGui::TableSetupColumn("TEX_COL_COORS", ImGuiTableColumnFlags_WidthFixed,
			ImGui::CalcTextSize("###16384, 16384").x);

		// Column: Colored rectangle
		ImGui::TableNextColumn();
		{
			float frameHeight = ImGui::GetFrameHeight();
			ImGui::Dummy(ImVec2(frameHeight, frameHeight));
			ImRect& rect = GImGui->LastItemData.Rect;
			dl->AddRectFilled(rect.Min, rect.Max, pixelColor);
			dl->AddRect(rect.Min, rect.Max, ImGui::GetColorU32(ImGuiCol_Border));
		}

		// Column: RGBA
		ImGui::TableNextColumn();
		ImGui::Text("RGBA(%i %i %i %i)", pixelColor.R, pixelColor.G, pixelColor.B, pixelColor.A);

		// Column: Coors
		ImGui::TableNextColumn();
		ImGui::PushFont(ImFont_Regular); ImGui::Text(ICON_AM_SEARCH_CONTRACT); ImGui::SameLine(); ImGui::PopFont();
		ImGui::Text("%i, %i", pixelX, pixelY);

		ImGui::EndTable();
	}
}

void rageam::ui::TextureVM::RenderImageInfo(const graphics::ImagePtr& image) const
{
	graphics::ImageInfo imageInfo = image->GetInfo();

	// Image information
	ConstString formatName = graphics::ImagePixelFormatToName[imageInfo.PixelFormat];
	u32 totalSize = ImageComputeTotalSizeWithMips(
		imageInfo.Width, imageInfo.Height, imageInfo.MipCount, imageInfo.PixelFormat);
	ImGui::Text("%ix%i %i Mip(s) %s %s",
		imageInfo.Width, imageInfo.Height, imageInfo.MipCount, formatName, FormatSize(totalSize));

	// See RenderHoveredPixel for clamping reasoning
	int mipLevel = ImClamp<int>(m_MipLevel, 0, image->GetMipCount() - 1);

	// Mip information
	int mipWidth = imageInfo.Width >> mipLevel;
	int mipHeight = imageInfo.Height >> mipLevel;
	if (m_AdvancedView)
	{
		u32 mipSize = ImageComputeSlicePitch(mipWidth, mipHeight, imageInfo.PixelFormat);
		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		ImGui::Text("Mip %i %ix%i %s", mipLevel, mipWidth, mipHeight, FormatSize(mipSize));
	}
}

void rageam::ui::TextureVM::RenderLoadingProgress(const amUPtr<AsyncImage>& im) const
{
	double percent = 0;
	// Block processing not started yet, avoid division by zero
	if (im->LoadToken.TotalBlockLines > 0)
	{
		percent = static_cast<double>(im->LoadToken.ProcessedBlockLines) / im->LoadToken.TotalBlockLines * 100.0;
	}
	ImGui::Text("Loading %.02f%%", percent);
}

void rageam::ui::TextureVM::RenderImageViewport()
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImDrawList* dl = window->DrawList;

	// Raw is not used often and loaded on request...
	if (!m_DisplayCompressed && !m_Raw)
	{
		m_Raw = std::make_unique<AsyncImage>();
		m_Raw->LoadAsync(m_Tune->GetFilePath());
	}

	// Make sure that image is not modified when we're using it....
	amUPtr<AsyncImage>& im = m_DisplayCompressed ? m_Compressed : m_Raw;
	std::unique_lock lock(im->Mutex);

	// Toolbar
	if (ImGui::Button(ICON_AM_AUTO_SIZE_OPTIMIZE" Reset View"))
	{
		ResetViewport();
	}
	ImGui::SameLine();
	bool canCancel = im->IsLoading;
	if (!canCancel) ImGui::BeginDisabled();
	if (ImGui::Button(ICON_AM_CANCEL_BUILD" Cancel"))
	{
		im->CancelAsyncLoading();
	}
	if (!canCancel) ImGui::EndDisabled();

	// Draw image and viewport info (size, mip, format, hovered coors/color...)
	ImGui::PushFont(ImFont_Small);
	if (!im->IsLoading && im->View)
	{
		RenderImageInfo(im->Image);
		RenderHoveredPixel(im->Image);
	}
	else // Image is being processed
	{
		// This replaces RenderImageInfo in branch where image is loaded
		RenderLoadingProgress(im);
		RenderHoveredPixel(im->Image);
	}
	ImGui::PopFont();

	// Compute viewport and image extents...
	int imWidth = 0;
	int imHeight = 0;
	if (im->Image)
	{
		imWidth = im->Image->GetWidth();
		imHeight = im->Image->GetHeight();
	}
	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	ImVec2 viewportMin = window->DC.CursorPos;
	ImVec2 viewportMax = viewportMin + viewportSize - GImGui->Style.FramePadding;
	ImRect viewportBb(viewportMin, viewportMax);
	viewportSize = viewportBb.GetSize(); // We have to adjust it because padding was added in viewportMax
	m_HalfViewportSize = viewportSize * 0.5f;
	// Canvas is rect within viewport where we fit our image without stretching
	int canvasWidth, canvasHeight;
	ImageScaleResolution(
		imWidth, imHeight,
		static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y),
		canvasWidth, canvasHeight,
		graphics::ResolutionScalingMode_Fit);
	ImVec2 canvasSize(static_cast<float>(canvasWidth), static_cast<float>(canvasHeight));
	// For centering
	ImVec2 canvasMin(
		(viewportSize.x - static_cast<float>(canvasWidth)) * 0.5f,
		(viewportSize.y - static_cast<float>(canvasHeight)) * 0.5f);
	canvasMin += viewportMin;

	// Transform image coors to screen space
	ImVec2 imageMin(0, 0);
	ImVec2 imageMax = canvasSize;
	imageMin = WorldToScreen(imageMin);
	imageMax = WorldToScreen(imageMax);
	imageMin += canvasMin;
	imageMax += canvasMin;

	// Add size of viewport
	ImGui::ItemSize(viewportBb);
	ImGui::ItemAdd(viewportBb, ImGui::GetID("TEX_VIEWPORT"));

	// Base background + border
	dl->AddRectFilled(viewportMin, viewportMax, 0x30303030);
	dl->AddRect(viewportMin - ImVec2(1, 1), viewportMax + ImVec2(1, 1), ImGui::GetColorU32(ImGuiCol_Border));

	dl->PushClipRect(viewportMin, viewportMax, true);
	{
		dl->AddCallback(BackupSamplerCallback, NULL);

		// Alpha checker
		dl->AddCallback(SetCheckerSamplerCallback, NULL);
		dl->AddImage(sm_SharedState->Checker.Get(), imageMin, imageMax);
		// Texture
		SamplerCallbackState samplerCallbackState;
		if (m_AdvancedView)
		{
			samplerCallbackState.IsLinear = m_PointSampler == false;
			samplerCallbackState.LOD = m_MipLevel;
		}
		else
		{
			samplerCallbackState.IsLinear = true;
			samplerCallbackState.LOD = 0;
		}
		// We interpret callback state as address
		dl->AddCallback(SetSamplerCallback, *(char**)&samplerCallbackState);
		dl->AddImage(im->GetTextureID(), imageMin, imageMax, ImVec2(0, 0), im->UV2);

		dl->AddCallback(RestoreSamplerCallback, NULL);
	}
	dl->PopClipRect();

	// Viewport controls
	static ImVec2 s_PrevMousePos;
	static bool s_Panning = false;
	bool viewportActive = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsWindowHovered();
	bool viewportHovered = viewportActive && ImGui::IsMouseHoveringRect(viewportMin, viewportMax);
	if (!m_IsMissing && (s_Panning || viewportHovered)) // We disable controls for missing textures...
	{
		// Calculate normalized coordinates of mouse hovered over image
		if (im)
		{
			// Picking pixel outside the image works but feels weird...
			if (ImGui::IsMouseHoveringRect(imageMin, imageMax))
			{
				ImVec2 mouseRelative = ImGui::GetMousePos() - canvasMin;
				mouseRelative = ScreenToWorld(mouseRelative);
				mouseRelative /= canvasSize;
				mouseRelative.x = ImClamp(mouseRelative.x, 0.0f, 1.0f);
				mouseRelative.y = ImClamp(mouseRelative.y, 0.0f, 1.0f);
				m_HoveredCoors = mouseRelative;
			}
		}
		else
		{
			// Image is not loaded... reset hovered coors,
			// we don't know exact image size (although we can get it from meta)
			m_HoveredCoors = {};
		}

		ImVec2 mousePos = ImGui::GetMousePos();

		// Panning: Start
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			s_Panning = true;
			s_PrevMousePos = mousePos;
			m_ImageViewOld = m_ImageView;
		}
		// Panning: Process
		if (s_Panning)
		{
			// Compute delta and apply panning
			ImVec2 mouseDelta = mousePos - s_PrevMousePos;
			mouseDelta /= m_ImageView.Z; // Match pixel distance at zoom level
			m_ImageView.X += mouseDelta.x;
			m_ImageView.Y += mouseDelta.y;
			s_PrevMousePos = mousePos;
		}
		// Panning: Confirm
		if (s_Panning && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			s_Panning = false;
		}
		// Panning: Cancel
		if (s_Panning && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		{
			s_Panning = false;
			m_ImageView = m_ImageViewOld;
		}

		// Zooming
		float scroll = ImGui::GetIO().MouseWheel;
		if (scroll != 0.0f)
		{
			float zoomFactor = m_ImageView.Z / 5.0f;
			m_ImageView.Z += scroll * zoomFactor;
			m_ImageView.Z = ImClamp(m_ImageView.Z, 0.025f, 50.0f);
		}
	}
}

void rageam::ui::TextureVM::ResetViewport()
{
	m_ImageView = Vec3V(0, 0, 1);
}

void rageam::ui::TextureVM::GetIcon(ImTextureID& id, int& width, int& height) const
{
	std::unique_lock lock(m_Compressed->Mutex);

	id = m_Compressed->GetTextureID();

	// Image was not loaded yet or failed...
	if (!id)
	{
		width = 1; height = 1;
		return;
	}
	width = m_Compressed->Image->GetWidth();
	height = m_Compressed->Image->GetHeight();
}

void rageam::ui::TextureVM::UpdateTune() const
{
	if (m_CompressOptionsMatchPreset)
	{
		m_Tune->Options.Reset();
	}
	else
	{
		asset::TextureOptions newOptions;
		newOptions.CompressorOptions = m_CompressOptions;
		m_Tune->Options = newOptions;
	}
}

void rageam::ui::TextureVM::ReloadImage(bool onlyCompressed) const
{
	file::WPath path = m_Tune->GetFilePath();
	m_Compressed->LoadAsyncCompressed(path, m_CompressOptions);
	if (!onlyCompressed && m_Raw) m_Raw->LoadAsync(path);
}

void rageam::ui::TextureVM::UpdateTextureName()
{
	// This is not validated name (TxdAsset::GetValidatedTextureName) but will work fine for editor
	m_TextureName = PATH_TO_UTF8(file::WPath(m_Tune->GetFilePath()).GetFileNameWithoutExtension());
}

void rageam::ui::TextureVM::SetIsMissing(bool missing)
{
	m_IsMissing = missing;

	// Most likely image was modified, if it was not - image cache will handle this in no time
	if (!m_IsMissing)
	{
		ReloadImage();
	}
}

void rageam::ui::TextureVM::ReloadPreset()
{
	asset::TexturePresetStore* presetStore = asset::TexturePresetStore::GetInstance();

	SetPreset(presetStore->MatchPreset(*m_Tune));
}

void rageam::ui::TextureVM::SetPreset(const asset::TexturePresetPtr& preset)
{
	m_MatchedTexturePreset = preset;
	if (m_CompressOptionsMatchPreset)
	{
		m_CompressOptions = m_MatchedTexturePreset->Options.CompressorOptions;
		m_CompressOptionsPending = m_CompressOptions;
	}
}

void rageam::ui::TextureVM::ShutdownClass()
{
	sm_SharedState = nullptr;
}

void rageam::ui::TexturePresetWizard::SnapshotPresets()
{
	m_Entries.Clear();

	asset::TexturePresetStore* presetStore = asset::TexturePresetStore::GetInstance();
	asset::TexturePresetCollection& presetCollection = presetStore->GetPresets(m_WorkspacePath);

	presetStore->Lock();
	for (asset::TexturePresetPtr& preset : presetCollection.Items)
	{
		PresetEntry entry = {};
		entry.Preset = *preset; // Copy
		String::Copy(entry.Name, PresetEntry::NAME_MAX, preset->Name);
		String::WideToUtf8(entry.Argument, PresetEntry::ARG_MAX, preset->RuleArgument);

		m_Entries.Emplace(std::move(entry));
		m_AliveEntriesCount++;
	}
	presetStore->Unlock();

	m_PresetCollection = &presetCollection;
	m_SelectedIndex = m_AliveEntriesCount > 0 ? 0 : -1;
}

void rageam::ui::TexturePresetWizard::RenderPresetList()
{
	for (int i = 0; i < m_Entries.GetSize(); i++)
	{
		PresetEntry& entry = m_Entries[i];

		if (entry.IsDeleted)
			continue;

		ConstString id = ImGui::FormatTemp("%s###TEX_PRESET_%i", entry.Name, i);

		bool selected = m_SelectedIndex == i;
		if (ImGui::Selectable(id, selected, ImGuiSelectableFlags_DontClosePopups))
		{
			m_SelectedIndex = i;
		}
	}
}

void rageam::ui::TexturePresetWizard::RenderPresetUI()
{
	if (m_SelectedIndex == -1)
		return;

	float availableWidth = ImGui::GetFrameHeight() * 9.0f;

	PresetEntry& entry = m_Entries[m_SelectedIndex];

	// Name
	ImGui::SetNextItemWidth(availableWidth);
	ImGui::InputText("Name", entry.Name, PresetEntry::NAME_MAX);

	// Rule
	static constexpr ConstString s_RuleNames[] // TexturePresetRule
	{
		"Default", "Contains", "Begins", "Ends"
	};
	ImGui::SetNextItemWidth(availableWidth);
	ImGui::Combo("Rule", (int*)&entry.Preset.Rule, s_RuleNames, IM_ARRAYSIZE(s_RuleNames));

	// Argument
	bool argAvailable = entry.Preset.Rule != asset::TexturePresetRule::Default;
	if (!argAvailable) ImGui::BeginDisabled();
	ImGui::SetNextItemWidth(availableWidth);
	ImGui::InputText("Argument", entry.Argument, PresetEntry::ARG_MAX);
	if (!argAvailable) ImGui::EndDisabled();

	RenderCompressOptionsControls(entry.Preset.Options.CompressorOptions, true, availableWidth);
}

void rageam::ui::TexturePresetWizard::ResetSelectionToFirstAlive()
{
	if (m_AliveEntriesCount == 0)
	{
		m_SelectedIndex = -1;
		return;
	}

	for (int i = 0; i < m_Entries.GetSize(); i++)
	{
		PresetEntry& entry = m_Entries[i];
		if (!entry.IsDeleted)
		{
			m_SelectedIndex = i;
			return;
		}
	}
}

void rageam::ui::TexturePresetWizard::DoSave() const
{
	// TODO: Ideally we do this if user really changed anything

	asset::TexturePresetStore* presetsStore = asset::TexturePresetStore::GetInstance();
	presetsStore->Lock();
	// Fastest way is to re-add all presets
	m_PresetCollection->Items.Clear();
	for (const PresetEntry& entry : m_Entries)
	{
		if (entry.IsDeleted)
			continue;

		asset::TexturePresetPtr preset = std::make_shared<asset::TexturePreset>(entry.Preset);
		preset->Name = entry.Name;
		if (preset->Rule != asset::TexturePresetRule::Default)
			preset->RuleArgument = String::ToWideTemp(entry.Argument);

		m_PresetCollection->Items.Emplace(std::move(preset));
	}
	m_PresetCollection->SaveTo(presetsStore->GetPresetsPathFromWorkspace(m_WorkspacePath));
	m_PresetCollection->Seal();
	presetsStore->MarkPresetsMatchFile(m_WorkspacePath);
	presetsStore->Unlock();
	presetsStore->NotifyPresetsWereModified(m_WorkspacePath);
}

bool rageam::ui::TexturePresetWizard::ValidateEntries()
{
	m_ValidationErrors.Clear();
	HashSet<ConstString> names;
	const PresetEntry* defaultRaw = nullptr;
	const PresetEntry* defaultCompressed = nullptr;
	for (const PresetEntry& entry : m_Entries)
	{
		if (entry.IsDeleted)
			continue;

		// Make sure name is not empty or occurs more than one time
		if (String::IsNullOrEmpty(entry.Name))
			m_ValidationErrors.Add(ImGui::FormatTemp("Preset name is not specified"));
		else if (names.Contains(entry.Name))
			m_ValidationErrors.Add(ImGui::FormatTemp("Multiple occurrences of '%s'", entry.Name));
		else
			names.Insert(entry.Name);

		// Make sure there's only one raw / compressed default presets
		if (entry.Preset.Rule == asset::TexturePresetRule::Default)
		{
			if (entry.Preset.Options.CompressorOptions.Format == graphics::BlockFormat_None)
			{
				if (defaultRaw)
					m_ValidationErrors.Add(ImGui::FormatTemp(
						"Multiple default presets without compression (%s, %s)", defaultRaw->Name, entry.Name));
				else
					defaultRaw = &entry;
			}
			else
			{
				if (defaultCompressed)
					m_ValidationErrors.Add(ImGui::FormatTemp(
						"Multiple default presets with compression (%s, %s)", defaultCompressed->Name, entry.Name));
				else
					defaultCompressed = &entry;
			}
		}
		// For Contains/Begins/Ends rules we have to ensure that argument is not empty
		else
		{
			if (String::IsNullOrEmpty(entry.Argument))
				m_ValidationErrors.Add(ImGui::FormatTemp(
					"Preset '%s' with rule '%s' has no argument", entry.Name, Enum::GetName(entry.Preset.Rule)));
		}

		if (m_ValidationErrors.Any())
			return false;
	}
	return true;
}

rageam::ui::TexturePresetWizard::TexturePresetWizard(ConstWString workspacePath)
{
	m_WorkspacePath = workspacePath;
	SnapshotPresets();
}

bool rageam::ui::TexturePresetWizard::Render(const graphics::ImageCompressorOptions* selectedOptions)
{
	UndoStack::Push(m_Undo);

	// MenuItem uses Selectable and by default it closes currently opened popup on press...
	ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);
	if (ImGui::BeginMenuBar())
	{
		// TODO: Need UNDO here

		if (ImGui::MenuItem(ICON_AM_ADD_TEXT_FILE" New"))
		{
			PresetEntry newEntry = {};
			String::Copy(newEntry.Name, PresetEntry::NAME_MAX, "New Preset");
			m_Entries.Emplace(std::move(newEntry));
			m_SelectedIndex = m_Entries.GetSize() - 1;
			m_AliveEntriesCount++;
			if (m_SelectedIndex == -1)
				ResetSelectionToFirstAlive();
		}

		// We can only create new if there's no presets
		bool canDeleteOrClone = m_SelectedIndex != -1;
		if (!canDeleteOrClone) ImGui::BeginDisabled();

		if (ImGui::MenuItem(ICON_AM_CANCEL" Delete"))
		{
			m_Entries[m_SelectedIndex].IsDeleted = true;
			m_AliveEntriesCount--;
			ResetSelectionToFirstAlive();
		}

		if (ImGui::MenuItem(ICON_AM_DOCUMENT_SOURCE" Clone"))
		{
			m_Entries.Add(m_Entries[m_SelectedIndex]);
			m_AliveEntriesCount++;
		}
		ImGui::ToolTip("Duplicates selected preset.");

		if (!selectedOptions) ImGui::BeginDisabled();
		if (ImGui::MenuItem(ICON_AM_COPY" Copy From Texture"))
		{
			m_Entries[m_SelectedIndex].Preset.Options.CompressorOptions = *selectedOptions;
		}
		if (!selectedOptions) ImGui::EndDisabled();
		ImGui::ToolTip("Copy settings from selected texture in TXD editor.");

		if (!canDeleteOrClone) ImGui::EndDisabled();

		ImGui::EndMenuBar();
	}
	ImGui::PopItemFlag();

	// Leave space for buttons on bottom
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec2 childSize = ImGui::GetContentRegionAvail();
	childSize.y -= ImGui::GetFrameHeight() + style.FramePadding.y + style.ItemSpacing.y;

	if (ImGui::BeginChild("TEX_PRESET_LAYOUT", childSize, ImGuiChildFlags_Border))
	{
		if (m_AliveEntriesCount == 0)
		{
			ImGui::BeginDisabled();
			ImGui::TextCentered("No presets...",
				ImGuiTextCenteredFlags_Horizontal | ImGuiTextCenteredFlags_Vertical);
			ImGui::EndDisabled();
		}
		else
		{
			if (ImGui::BeginTable("TEX_WIZARD_LAYOUT", 2, ImGuiTableFlags_Resizable))
			{
				ImGui::TableNextColumn();
				RenderPresetList();

				ImGui::TableNextColumn();
				RenderPresetUI();

				ImGui::EndTable();
			}
		}
	}
	ImGui::EndChild();

	static constexpr ConstString NOT_VALIDATED_POPUP = "Errors found###NOT_VALIDATED_POPUP";
	bool saved = false;
	if (ImGui::Button("Save"))
	{
		if (!ValidateEntries())
		{
			ImGui::OpenPopup(NOT_VALIDATED_POPUP);
		}
		else
		{
			DoSave();
			saved = true;
			ImGui::CloseCurrentPopup();
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape))
		ImGui::CloseCurrentPopup();

	ImGui::HandleUndoHotkeys();
	UndoStack::Pop();

	// Show validation errors
	if (ImGui::BeginPopupModal(NOT_VALIDATED_POPUP, 0, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Validation failed:");
		for (const string& error : m_ValidationErrors)
		{
			ImGui::BulletText(error.GetCStr());
		}
		if (ImGui::Button("Close"))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	return saved;
}

void rageam::ui::TxdWindow::RenderTextureList()
{
	for (u16 i = 0; i < m_TextureVMs.GetSize(); i++)
	{
		TextureVM& textureVM = m_TextureVMs[i];

		if (textureVM.RenderListEntry(i == m_SelectedIndex))
		{
			m_SelectedIndex = i;
		}
	}
}

void rageam::ui::TxdWindow::LoadTexturesFromAsset()
{
	asset::Textures& textureTunes = GetAsset()->GetTextureTunes();

	m_TextureVMs.Clear();
	m_TextureVMs.Reserve(textureTunes.GetSize());

	for (asset::TextureTune& tune : textureTunes)
	{
		m_TextureVMs.Construct(this, &tune);
	}

	m_SelectedIndex = textureTunes.Any() ? 0 : -1;
}

void rageam::ui::TxdWindow::OnFileChanged(const file::DirectoryChange& change)
{
	const asset::TxdAssetPtr& txd = GetAsset();
	asset::TextureTune* tune = txd->TryFindTuneFromPath(change.Path);

	// This is case when image was renamed before and now renamed back, we must look up
	// by NewPath (which is the old name)
	if (!tune && change.Action == file::ChangeAction_Renamed)
	{
		tune = txd->TryFindTuneFromPath(change.NewPath);
		if (tune)
		{
			TextureVM& textureVM = GetVMFromTune(*tune);
			textureVM.SetIsMissing(false);
		}
		return;
	}

	if (!tune)
	{
		// Add a new texture
		if (change.Action == file::ChangeAction_Added && asset::TxdAsset::IsSupportedImageFile(change.Path))
		{
			m_TextureVMs.Construct(this, &txd->AddTune(change.Path));
			return;
		}

		// Temporary file was created / tune file changed, whatever, we don't care
		return;
	}

	TextureVM& textureVM = GetVMFromTune(*tune);

	if (change.Action == file::ChangeAction_Modified)
	{
		textureVM.ReloadImage();
		return;
	}

	if (change.Action == file::ChangeAction_Renamed)
	{
		textureVM.SetIsMissing(true);
		return;
	}

	if (change.Action == file::ChangeAction_Removed)
	{
		textureVM.SetIsMissing(true);
		return;
	}

	// In this case texture was removed before and now re-added
	if (change.Action == file::ChangeAction_Added)
	{
		textureVM.SetIsMissing(false);
		return;
	}
}

void rageam::ui::TxdWindow::OnRender()
{
	AssetWindow::OnRender();

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImVec2 viewportStart;

	if (ImGui::BeginTable("TEX_TABLE", 3, ImGuiTableFlags_Resizable))
	{
		ImGui::TableSetupColumn("TEX_COL_LIST");
		ImGui::TableSetupColumn("TEX_COL_VIEWPORT");
		ImGui::TableSetupColumn("TEX_COL_PROPERTIES");

		// Column: Texture list
		ImGui::TableNextColumn();
		{
			if (ImGui::BeginChild("TEX_COL_LIST_CHILD"))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
				RenderTextureList();
				ImGui::PopStyleVar(); // ItemSpacing
			}
			ImGui::EndChild();
		}

		if (m_SelectedIndex != -1)
		{
			TextureVM& textureVM = m_TextureVMs[m_SelectedIndex];
			if (textureVM.IsMissing()) ImGui::BeginDisabled();

			// Column: Texture properties
			ImGui::TableNextColumn();
			{
				viewportStart = window->DC.CursorPos;
				textureVM.RenderImageViewport();
			}

			// Column: Texture properties
			ImGui::TableNextColumn();
			{
				if (ImGui::BeginChild("TEX_COL_VIEWPORT_PROPERTIES_CHILD"))
				{
					textureVM.RenderImageProperties();
				}
				ImGui::EndChild();
			}

			if (textureVM.IsMissing()) ImGui::EndDisabled();
		}

		ImGui::EndTable();
	}

	if (m_SelectedIndex != -1)
	{
		// We display overlay for missing textures
		TextureVM& textureVM = m_TextureVMs[m_SelectedIndex];
		ImVec2 propertiesEnd = window->WorkRect.Max;
		if (textureVM.IsMissing())
		{
			ConstString missingText = "Texture file is missing";
			ImVec2 missingTextSize = ImGui::CalcTextSize(missingText);
			ImVec2 texPos(
				IM_GET_CENTER(viewportStart.x, propertiesEnd.x - viewportStart.x, missingTextSize.x),
				IM_GET_CENTER(viewportStart.y, propertiesEnd.y - viewportStart.y, missingTextSize.y));
			ImU32 bgColor = ImGui::GetColorU32(ImGuiCol_WindowBg, 0.9f);

			ImGui::PushFont(ImFont_Medium);
			window->DrawList->AddRectFilled(viewportStart, propertiesEnd, bgColor);
			window->DrawList->AddText(texPos, ImGui::GetColorU32(ImGuiCol_Text), missingText);
			ImGui::PopFont();
		}
	}
}

void rageam::ui::TxdWindow::OnAssetMenuRender()
{
	static constexpr ConstString MANAGE_PRESETS_POPUP = "Texture Preset Wizard###TEX_PRESET_MANAGER_POPUP";
	static constexpr ConstString NO_IN_WORKSPACE_POPUP = "Not available###TEX_NOT_IN_WORKSPACE";
	if (ImGui::MenuItem(ICON_AM_EDIT_SMART_TAG" Manage Presets"))
	{
		ConstWString workspacePath = GetAsset()->GetWorkspacePath();
		if (String::IsNullOrEmpty(workspacePath))
		{
			ImGui::OpenPopup(NO_IN_WORKSPACE_POPUP);
		}
		else
		{
			m_TexturePresetWizard = std::make_unique<TexturePresetWizard>(workspacePath);
			ImGui::OpenPopup(MANAGE_PRESETS_POPUP);
		}
	}
	if (ImGui::BeginPopupModal(NO_IN_WORKSPACE_POPUP))
	{
		ImGui::Text("Can't manage presets outside workspace, TXD must be placed in a folder with '.ws' extension.");
		if (ImGui::Button("Close"))
			ImGui::CloseCurrentPopup();
	}
	if (ImGui::BeginPopupModal(MANAGE_PRESETS_POPUP, 0, ImGuiWindowFlags_MenuBar))
	{
		// Pass options for selected texture (if any) for 'copy from texture' option
		const graphics::ImageCompressorOptions* selectedTexOptions = nullptr;
		if (m_SelectedIndex != -1)
			selectedTexOptions = &m_TextureVMs[m_SelectedIndex].GetCompressorOptions();

		if (m_TexturePresetWizard->Render(selectedTexOptions))
		{
			// Presets were changed, reload all textures...
			for (TextureVM& textureVM : m_TextureVMs)
			{
				textureVM.ReloadPreset();
				textureVM.ReloadImage();
				NotifyHotDrawableThatConfigWasChanged();
			}
		}

		ImGui::EndPopup();
	}
	else
	{
		m_TexturePresetWizard = nullptr;
	}
}

void rageam::ui::TxdWindow::NotifyHotDrawableThatConfigWasChanged()
{
	asset::TxdAssetPtr asset = GetAsset();
	asset::Textures&   tunes = asset->GetTextureTunes();

	// Before saving we have to apply changes, and then un-apply them,
	// so we clone list with all tunes
	List<Nullable<asset::TextureOptions>> oldOptions;
	oldOptions.Reserve(tunes.GetSize());
	for (asset::TextureTune& tune : tunes)
		oldOptions.Add(tune.Options);

	SaveChanges();
	(void) asset->SaveConfig(true);

	// And now we can roll back old tunes back
	for (u32 i = 0; i < oldOptions.GetSize(); i++)
	{
		tunes[i].Options = std::move(oldOptions[i]);
	}
}

rageam::ui::TextureVM& rageam::ui::TxdWindow::GetVMFromTune(const asset::TextureTune& tune)
{
	for (u32 i = 0; i < m_TextureVMs.GetSize(); i++)
	{
		TextureVM& textureVM = m_TextureVMs[i];
		if (textureVM.GetTune()->GetHashKey() == tune.GetHashKey())
		{
			return textureVM;
		}
	}
	AM_UNREACHABLE(L"GetVMFromTune() -> There's no view model with key for '%ls'", tune.GetFilePath());
}

rageam::ui::TextureVM& rageam::ui::TxdWindow::GetVMFromHashKey(u32 hashKey)
{
	for (u32 i = 0; i < m_TextureVMs.GetSize(); i++)
	{
		TextureVM& textureVM = m_TextureVMs[i];
		if (textureVM.GetTune()->GetHashKey() == hashKey)
		{
			return textureVM;
		}
	}
	AM_UNREACHABLE("GetVMFromHashKey() -> There's no tune with key '%x'", hashKey);
}

rageam::ui::TxdWindow::TxdWindow(const asset::AssetPtr& asset) : AssetWindow(asset)
{
	LoadTexturesFromAsset();
}

rageam::ui::TxdWindow::~TxdWindow()
{
	// Remove temporary config, this will force hot drawable to reload
	// txd with regular config, applying new changes (or not, if changes weren't saved)
	file::WPath tempConfigPath = GetAsset()->GetConfigPath(true);
	DeleteFileW(tempConfigPath);
}

void rageam::ui::TxdWindow::SaveChanges()
{
	for (TextureVM& vm : m_TextureVMs)
	{
		vm.UpdateTune();
	}
}

void rageam::ui::TxdWindow::Reset()
{
	GetAsset()->GetTextureTunes().Clear();
	m_SelectedIndex = -1;
	m_TextureVMs.Clear();
}

void rageam::ui::TxdWindow::OnRefresh()
{
	// Attempt to preserve selected texture
	u32 selectedNameHash = m_SelectedIndex != -1 ?
		m_TextureVMs[m_SelectedIndex].GetTune()->GetHashKey() : 0;

	LoadTexturesFromAsset();

	for (u32 i = 0; i < m_TextureVMs.GetSize(); i++)
	{
		if (m_TextureVMs[i].GetTune()->GetHashKey() == selectedNameHash)
		{
			m_SelectedIndex = i;
			break;
		}
	}
}

rageam::asset::TxdAssetPtr rageam::ui::TxdWindow::GetAsset()
{
	return std::static_pointer_cast<asset::TxdAsset>(AssetWindow::GetAsset());
}
