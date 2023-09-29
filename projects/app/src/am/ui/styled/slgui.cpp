#include "slgui.h"

#include "nvtt.h"
#include "am/file/fileutils.h"
#include "am/file/iterator.h"
#include "am/graphics/texture/pngutils.h"
#include "am/system/datamgr.h"
#include "am/ui/color_hsl.h"
#include "am/ui/extensions.h"
#include "am/ui/font_icons/icons_awesome.h"
#include "misc/freetype/imgui_freetype.h"
#include "rage/atl/array.h"
#include "rage/atl/string.h"

SlGuiStyle::SlGuiStyle()
{
	ButtonRounding = 2;
}

SlGuiContext* SlGui::CreateContext()
{
	// Natively ImGui supports multiple context's but we don't need that

	SlGuiContext* ctx = IM_NEW(SlGuiContext)();
	ImGui::SetCurrentContext(ctx);
	ImGui::Initialize();
	return ctx;
}

SlGuiContext* SlGui::GetContext()
{
	return (SlGuiContext*)GImGui;
}

SlGuiStyle& SlGui::GetStyle()
{
	return GetContext()->SlStyle;
}

void SlGui::StyleColorsLight()
{
	SlGuiStyle& style = GetStyle();

	style.Colors[SlGuiCol_None] = ColorConvertU32ToGradient(0, 0);
	style.Colors[SlGuiCol_Button] = ColorConvertU32ToGradient(IM_COL32(231, 231, 231, 255), IM_COL32(245, 245, 245, 255));
	style.Colors[SlGuiCol_ButtonHovered] = ColorConvertU32ToGradient(IM_COL32(125, 187, 237, 255), IM_COL32(169, 220, 247, 255));
	style.Colors[SlGuiCol_ButtonPressed] = ColorConvertU32ToGradient(IM_COL32(130, 186, 223, 255), IM_COL32(148, 200, 230, 255));
	style.Colors[SlGuiCol_Gloss] = ColorConvertU32ToGradient(IM_COL32(252, 252, 252, 160), IM_COL32(252, 252, 252, 100));
	style.Colors[SlGuiCol_Border] = ColorConvertU32ToGradient(IM_COL32(92, 92, 92, 55), IM_COL32(97, 97, 97, 55));
	style.Colors[SlGuiCol_BorderHighlight] = ColorConvertU32ToGradient(IM_COL32(30, 44, 152, 100), IM_COL32(30, 44, 152, 55));
	style.Colors[SlGuiCol_Shadow] = ColorConvertU32ToGradient(IM_COL32(176, 176, 176, 255), IM_COL32(176, 176, 176, 255));
}

void SlGui::StyleColorsDark()
{
	SlGuiStyle& style = GetStyle();

	style.Colors[SlGuiCol_None] = ColorConvertU32ToGradient(0, 0);

	//style.Colors[SlGuiCol_Bg] = ColorConvertU32ToGradient(IM_COL32(36, 41, 48, 255), IM_COL32(36, 40, 47, 255));
	style.Colors[SlGuiCol_Bg] = ColorConvertU32ToGradient(IM_COL32(30, 34, 40, 255), IM_COL32(36, 40, 47, 255));
	style.Colors[SlGuiCol_Bg2] = ColorConvertU32ToGradient(IM_COL32(45, 51, 60, 255), IM_COL32(45, 51, 60, 255));

	style.Colors[SlGuiCol_DockspaceBg] = ColorConvertU32ToGradient(IM_COL32(40, 44, 49, 255), IM_COL32(24, 27, 30, 255));

	style.Colors[SlGuiCol_ToolbarBg] = ColorConvertU32ToGradient(IM_COL32(30, 36, 48, 255), IM_COL32(29, 41, 57, 255));

	style.Colors[SlGuiCol_Gloss] = ColorConvertU32ToGradient(IM_COL32(252, 252, 252, 130), IM_COL32(252, 252, 252, 70));
	style.Colors[SlGuiCol_GlossBg] = ColorConvertU32ToGradient(IM_COL32(252, 252, 252, 1/*100*/), IM_COL32(252, 252, 252, 0/*4*//*35*/));
	style.Colors[SlGuiCol_GlossBg2] = ColorConvertU32ToGradient(IM_COL32(252, 252, 252, 0/*35*/), IM_COL32(252, 252, 252, 0/*5*//*100*/));

	style.Colors[SlGuiCol_Border] = ColorConvertU32ToGradient(IM_COL32(92, 92, 92, 55), IM_COL32(97, 97, 97, 55));
	style.Colors[SlGuiCol_BorderHighlight] = ColorConvertU32ToGradient(IM_COL32(30, 44, 152, 100), IM_COL32(30, 44, 152, 55));

	//style.Colors[SlGuiCol_Shadow] = ColorConvertU32ToGradient(IM_COL32(176, 176, 176, 255), IM_COL32(176, 176, 176, 255));
	style.Colors[SlGuiCol_Shadow] = ColorConvertU32ToGradient(IM_COL32(30, 30, 30, 255), IM_COL32(30, 30, 30, 255));

	style.Colors[SlGuiCol_Button] = ColorConvertU32ToGradient(IM_COL32(9, 14, 18, 255), IM_COL32(39, 40, 50, 255));
	style.Colors[SlGuiCol_ButtonHovered] = ColorConvertU32ToGradient(IM_COL32(9, 14, 18, 255), IM_COL32(24, 113, 169, 255));
	style.Colors[SlGuiCol_ButtonPressed] = ColorConvertU32ToGradient(IM_COL32(9, 14, 18, 255), IM_COL32(4, 59, 98, 255));

	style.Colors[SlGuiCol_Node] = ColorConvertU32ToGradient(IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
	style.Colors[SlGuiCol_NodeHovered] = ColorConvertU32ToGradient(IM_COL32(40, 88, 133, 125), IM_COL32(35, 77, 117, 125));
	style.Colors[SlGuiCol_NodePressed] = ColorConvertU32ToGradient(IM_COL32(25, 55, 84, 125), IM_COL32(27, 59, 89, 125));
	style.Colors[SlGuiCol_NodeBorderHighlight] = ColorConvertU32ToGradient(IM_COL32(25, 57, 87, 115), IM_COL32(14, 40, 63, 80));

	//style.Colors[SlGuiCol_Node] = ColorConvertU32ToGradient(IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
	//style.Colors[SlGuiCol_NodeHovered] = ColorConvertU32ToGradient(IM_COL32(50, 58, 75, 255), IM_COL32(50, 58, 75, 255));
	//style.Colors[SlGuiCol_NodePressed] = ColorConvertU32ToGradient(IM_COL32(62, 78, 105, 255), IM_COL32(62, 78, 105, 255));
	//style.Colors[SlGuiCol_NodeBorderHighlight] = ColorConvertU32ToGradient(IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));

	//style.Colors[SlGuiCol_List] = ColorConvertU32ToGradient(IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
	//style.Colors[SlGuiCol_ListHovered] = ColorConvertU32ToGradient(IM_COL32(79, 131, 104, 125), IM_COL32(92, 146, 92, 125));
	//style.Colors[SlGuiCol_ListPressed] = ColorConvertU32ToGradient(IM_COL32(37, 101, 133, 125), IM_COL32(78, 151, 70, 125));
	//style.Colors[SlGuiCol_ListBorderHighlight] = ColorConvertU32ToGradient(IM_COL32(45, 91, 50, 100), IM_COL32(92, 188, 118, 100));

	style.Colors[SlGuiCol_List] = ColorConvertU32ToGradient(IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
	style.Colors[SlGuiCol_ListHovered] = ColorConvertU32ToGradient(IM_COL32(40, 88, 133, 125), IM_COL32(35, 77, 117, 125));
	style.Colors[SlGuiCol_ListPressed] = ColorConvertU32ToGradient(IM_COL32(53, 111, 166, 125), IM_COL32(39, 86, 129, 125));
	//style.Colors[SlGuiCol_ListPressed] = ColorConvertU32ToGradient(IM_COL32(25, 55, 84, 125), IM_COL32(27, 59, 89, 125));
	style.Colors[SlGuiCol_ListBorderHighlight] = ColorConvertU32ToGradient(IM_COL32(25, 57, 87, 115), IM_COL32(14, 40, 63, 80));

	//style.Colors[SlGuiCol_TableHeader] = ColorConvertU32ToGradient(IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
	//style.Colors[SlGuiCol_TableHeaderHovered] = ColorConvertU32ToGradient(IM_COL32(79, 131, 104, 125), IM_COL32(92, 146, 92, 125));
	//style.Colors[SlGuiCol_TableHeaderPressed] = ColorConvertU32ToGradient(IM_COL32(37, 101, 133, 125), IM_COL32(78, 151, 70, 125));
	//style.Colors[SlGuiCol_TableHeaderBorderHighlight] = ColorConvertU32ToGradient(IM_COL32(45, 91, 50, 100), IM_COL32(92, 188, 118, 100));

	style.Colors[SlGuiCol_TableHeader] = ColorConvertU32ToGradient(IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
	style.Colors[SlGuiCol_TableHeaderHovered] = ColorConvertU32ToGradient(IM_COL32(40, 88, 133, 125), IM_COL32(35, 77, 117, 125));
	style.Colors[SlGuiCol_TableHeaderPressed] = ColorConvertU32ToGradient(IM_COL32(25, 55, 84, 125), IM_COL32(27, 59, 89, 125));
	style.Colors[SlGuiCol_TableHeaderBorderHighlight] = ColorConvertU32ToGradient(IM_COL32(25, 57, 87, 115), IM_COL32(14, 40, 63, 80));
}

void SlGui::AddCustomIcons()
{
	ImGuiIO& io = ImGui::GetIO();

	ImFont* customIconFont = io.Fonts->Fonts[SlFont_Regular];

	const auto& fontIcons = rageam::DataManager::GetFontIconsFolder();
	rageam::file::Iterator iconIterator(fontIcons / L"*.png");
	rageam::file::FindData findData;

	// Cached file names to not ping file system twice
	rage::atArray<rageam::file::WPath> imageFiles;

	// 1: Layout pass - allocate atlas for every texture
	ImWchar customIconBegin = 0xF900;
	rage::atArray<int> iconIds;
	while (iconIterator.Next())
	{
		iconIterator.GetCurrent(findData);
		imageFiles.Add(findData.Path);

		auto [iconWidth, iconHeight] = rageam::texture::GetPNGResolution(findData.Path);

		ImWchar iconId = customIconBegin + iconIds.GetSize(); // Unicode ID

		iconIds.Add(io.Fonts->AddCustomRectFontGlyph(
			customIconFont, iconId, (int)iconWidth, (int)iconHeight, (float)iconWidth));
	}

	// 2: Finish layout and build atlas
	io.Fonts->Build();

	// 3: Copy texture pixel data to pixel rectangle on atlas
	unsigned char* tex_pixels = nullptr;
	int tex_width, tex_height;
	io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_width, &tex_height);

	u16 iconId = 0;
	for (rageam::file::WPath& path : imageFiles)
	{
		nvtt::Surface iconSurface;
		if (!iconSurface.load(PATH_TO_UTF8(path)))
			continue;

		const float* r = iconSurface.channel(nvtt::Red);
		const float* g = iconSurface.channel(nvtt::Green);
		const float* b = iconSurface.channel(nvtt::Blue);
		const float* a = iconSurface.channel(nvtt::Alpha);

		const ImFontAtlasCustomRect* rect = io.Fonts->GetCustomRectByIndex(iconIds[iconId++]);

		// Write pixels to atlas region from png icon
		for (int y = 0; y < iconSurface.height(); y++)
		{
			ImU32* p = reinterpret_cast<ImU32*>(tex_pixels) + (rect->Y + y) * tex_width + rect->X; // NOLINT(bugprone-implicit-widening-of-multiplication-result)
			for (int x = 0; x < iconSurface.width(); x++)
			{
				//ImU32 col = IM_COL32(
				//	(int)(*r++ * 255),
				//	(int)(*g++ * 255),
				//	(int)(*b++ * 255),
				//	(int)(*a++ * 255));

				// See https://learn.microsoft.com/en-us/VisualStudio/extensibility/ux-guidelines/images-and-icons-for-visual-studio?view=vs-2022
				// 'Color inversion for dark themes'

				// https://stackoverflow.com/questions/36778989/vs2015-icon-guide-color-inversion

				ImU32 bg = IM_COL32(0, 0, 0, 255);
				ImU32 halo = IM_COL32(246, 246, 246, 255);
				float haloH, haloS, haloL;
				ImGui::ColorConvertRGBtoHSL(246, 246, 246, haloH, haloS, haloL);

				int colR = (int)(*r++ * 255);
				int colG = (int)(*g++ * 255);
				int colB = (int)(*b++ * 255);
				int  colA = (int)(*a++ * 255);
				float colH, colS, colL;
				ImGui::ColorConvertRGBtoHSL(colR, colG, colB, colH, colS, colL);

				float bgH, bgS, bgL;
				ImGui::ColorConvertRGBtoHSL(43, 43, 43, bgH, bgS, bgL);
				if (bgL < 0.5f)
				{
					haloL = 1.0f - haloL;
					colL = 1.0f - colL;
				}

				if (colL < haloL)
					colL = bgL * colL / haloL;
				else
					colL = (1.0f - bgL) * (colL - 1.0f) / (1.0f - haloL) + 1.0f;

				ImGui::ColorConvertHSLtoRGB(colH, colS, colL, colR, colG, colB);
				*p++ = IM_COL32(colR, colG, colB, colA);
			}
		}
	}
}

void SlGui::LoadFonts()
{
	ImGuiIO& io = ImGui::GetIO();

	io.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
	io.Fonts->FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint;

	float fontSize = 16.0f;
	float iconSize = 15.0f;

	ImFontConfig regularConfig{};
	regularConfig.RasterizerMultiply = 1.1f;
	regularConfig.OversampleH = 2;
	regularConfig.OversampleV = 1;

	ImFontConfig mediumConfig{};
	mediumConfig.RasterizerMultiply = 0.9f;
	mediumConfig.OversampleH = 2;
	mediumConfig.OversampleV = 1;

	ImFontConfig iconConfig{};
	iconConfig.RasterizerMultiply = 1.0f;
	iconConfig.MergeMode = true;
	iconConfig.OversampleH = 2;
	iconConfig.GlyphMinAdvanceX = 13.0f;
	iconConfig.GlyphOffset = ImVec2(0, 1);
	iconConfig.GlyphExtraSpacing = ImVec2(6, 0);
	iconConfig.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;

	static constexpr ImWchar iconRange[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

	// Load fonts from "data/fonts"

	const auto& fonts = rageam::DataManager::GetFontsFolder();

	// TODO: We support only latin & cyrillic for now
	const auto fontRange = ImGui::GetIO().Fonts->GetGlyphRangesCyrillic();

	// ---- Regular ----
	io.Fonts->AddFontFromFileTTF(PATH_TO_UTF8(fonts / L"Regular.ttf"), fontSize, &regularConfig, fontRange);
	// Merged with regular, note: merged fonts wont not added in IO.Fonts vector
	io.Fonts->AddFontFromFileTTF(PATH_TO_UTF8(fonts / L"Font Awesome 6.ttf"), iconSize, &iconConfig, iconRange);

	// ---- Medium ----
	io.Fonts->AddFontFromFileTTF(PATH_TO_UTF8(fonts / L"SemiBold.ttf"), fontSize, &mediumConfig, fontRange);

	// ---- Small ----
	io.Fonts->AddFontFromFileTTF(PATH_TO_UTF8(fonts / L"Regular.ttf"), fontSize - 2.0f, &regularConfig, fontRange);

	// ---- Custom Icons ----
	AddCustomIcons();
}

ImVec4 SlGui::StorageGetVec4(ImGuiID id)
{
	ImGui::PushOverrideID(id);

	ImGuiID x = ImGui::GetID("x");
	ImGuiID y = ImGui::GetID("y");
	ImGuiID z = ImGui::GetID("z");
	ImGuiID w = ImGui::GetID("w");

	ImVec4 result;
	ImGuiStorage* st = ImGui::GetStateStorage();
	result.x = st->GetFloat(x);
	result.y = st->GetFloat(y);
	result.z = st->GetFloat(z);
	result.w = st->GetFloat(w);

	ImGui::PopID();

	return result;
}

void SlGui::StorageSetVec4(ImGuiID id, const ImVec4& vec)
{
	ImGui::PushOverrideID(id);

	ImGuiID x = ImGui::GetID("x");
	ImGuiID y = ImGui::GetID("y");
	ImGuiID z = ImGui::GetID("z");
	ImGuiID w = ImGui::GetID("w");

	ImGuiStorage* st = ImGui::GetStateStorage();
	st->SetFloat(x, vec.x);
	st->SetFloat(y, vec.y);
	st->SetFloat(z, vec.z);
	st->SetFloat(w, vec.w);

	ImGui::PopID();
}

SlGradient SlGui::StorageGetGradient(ImGuiID id)
{
	ImGui::PushOverrideID(id);

	ImVec4 start = StorageGetVec4(ImGui::GetID("Start"));
	ImVec4 end = StorageGetVec4(ImGui::GetID("End"));

	ImGui::PopID();

	return { start, end };
}

void SlGui::StorageSetGradient(ImGuiID id, const SlGradient& col)
{
	ImGui::PushOverrideID(id);

	StorageSetVec4(ImGui::GetID("Start"), col.Start);
	StorageSetVec4(ImGui::GetID("End"), col.End);

	ImGui::PopID();
}

SlGradient SlGui::GetColorGradient(SlGuiCol col)
{
	SlGuiStyle& slStyle = GetStyle();
	ImGuiStyle& imStyle = ImGui::GetStyle();

	SlGradient gradient = slStyle.Colors[col];
	gradient.Start.w *= imStyle.Alpha;
	gradient.End.w *= imStyle.Alpha;

	return gradient;
}

SlGradient SlGui::GetColorAnimated(ConstString strId, SlGuiCol col, float time)
{
	float t = ImGui::GetIO().DeltaTime * 15 * (1.0f / time);

	ImGuiID id = ImGui::GetID(strId);
	ImGui::PushOverrideID(id);

	SlGradient current = StorageGetGradient(id);
	current = ImLerp(current, GetColorGradient(col), t);
	StorageSetGradient(id, current);

	ImGui::PopID();

	return current;
}

void SlGui::PushFont(SlFont font)
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::PushFont(io.Fonts->Fonts[font]);
}

void SlGui::PopFont()
{
	ImGui::PopFont();
}

void SlGui::ShadeVerts(const ImRect& bb, const SlGradient& col, int vtx0, int vtx1, float bias, float shift, ImGuiAxis axis)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGuiStyle& style = ImGui::GetStyle();

	ImVec2 p0 = bb.Min;
	ImVec2 p1 = axis == ImGuiAxis_X ? bb.GetTR() : bb.GetBL();
	p1 = ImLerp(p0, p1, bias);

	float shiftDist = abs(p1.y - p0.y) * shift;

	p0.y += shiftDist;
	p1.y += shiftDist;

	ImU32 col1 = ImGui::ColorConvertFloat4ToU32(col.Start);
	ImU32 col2 = ImGui::ColorConvertFloat4ToU32(col.End);

	ImGui::ShadeVertsLinearColorGradient(
		window->DrawList, vtx0, vtx1, p0, p1, col1, col2);
}

void SlGui::RenderFrame(const ImRect& bb, const SlGradient& col, float bias, float shift, ImGuiAxis axis)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	int vtx0 = window->DrawList->VtxBuffer.Size;
	window->DrawList->AddRectFilled(bb.Min, bb.Max, IM_COL32_WHITE, GImGui->Style.FrameRounding);
	int vtx1 = window->DrawList->VtxBuffer.Size;

	ShadeVerts(bb, col, vtx0, vtx1, bias, shift, axis);
}

void SlGui::RenderBorder(const ImRect& bb, const SlGradient& col, float bias, float shift, ImGuiAxis axis)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGuiStyle& style = GImGui->Style;

	int vtx0 = window->DrawList->VtxBuffer.Size;
	window->DrawList->AddRect(bb.Min, bb.Max, IM_COL32_WHITE, style.FrameRounding, 0, style.FrameBorderSize);
	int vtx1 = window->DrawList->VtxBuffer.Size;

	ShadeVerts(bb, col, vtx0, vtx1, bias, shift, axis);
}

void SlGui::ShadeItem(SlGuiCol col)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	const ImRect& rect = window->ClipRect;

	RenderFrame(rect, GetColorGradient(col));
}

bool SlGui::BeginPadded(ConstString name, const ImVec2& padding)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
	bool opened = ImGui::BeginChild(name, ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding);
	ImGui::PopStyleVar();
	return opened;
}

void SlGui::EndPadded()
{
	ImGui::EndChild();
}
