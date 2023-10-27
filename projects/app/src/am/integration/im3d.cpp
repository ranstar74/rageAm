#include "im3d.h"

#include "ImGuizmo.h"
#include "imgui_internal.h"
#include "am/graphics/shapetest.h"
#include "game/viewport.h"
#include "rage/grcore/device.h"

#define GUIZMO_MTX(mtx) ((float*)&mtx)

constexpr ImU32 bgColor = IM_COL32(0, 0, 0, 140);

static inline bool s_CenterNext = false;
static inline bool s_WorldGizmoMode = true;

static auto GetDrawList()
{
	return ImGui::GetForegroundDrawList();
}

bool Im3D::WorldToScreen(const ImVec3V& worldPos, ImVec2& screenPos)
{
	u32 width, height;
	rage::grcDevice::GetScreenSize(width, height);

	rage::Vec4V vec = worldPos.Transform4(CViewport::GetViewProjectionMatrix());

	// Check if position is behind of the screen
	bool isCulled = vec.Z() < 0.0f;

	// TODO: Replace on remap, add helper in CViewport?
	vec *= rage::S_HALF / vec.W();
	vec += rage::Vec4V(0.5f, 0.5f, 1.0f, 1.0f);
	vec.SetY(1.0f - vec.Y());
	vec *= rage::Vec4V(static_cast<float>(width), static_cast<float>(height), 1.0f, 1.0f);

	screenPos = ImVec2(vec.X(), vec.Y());
	return !isCulled;
}

void Im3D::CenterNext()
{
	s_CenterNext = true;
}

void Im3D::Text(const ImVec3V& pos, ConstString fmt, ...)
{
	bool centerNext = s_CenterNext;
	s_CenterNext = false;

	va_list args;
	va_start(args, fmt);
	const char* text, * text_end;
	ImFormatStringToTempBufferV(&text, &text_end, fmt, args);
	va_end(args);

	ImVec2 sPos;
	if (!WorldToScreen(pos, sPos))
		return;

	if (centerNext)
	{
		ImVec2 textSize = ImGui::CalcTextSize(text, text_end);
		sPos -= { textSize.x * 0.5f, textSize.y * 0.5f };
	}

	GetDrawList()->AddText(sPos, ImGui::GetColorU32(ImGuiCol_Text), text, text_end);
}

void Im3D::TextBg(const ImVec3V& pos, ConstString fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	TextBgV(pos, fmt, args);
	va_end(args);
}

void Im3D::TextBgV(const ImVec3V& pos, ConstString fmt, va_list args)
{
	bool centerNext = s_CenterNext;
	s_CenterNext = false;

	const char* text, * text_end;
	ImFormatStringToTempBufferV(&text, &text_end, fmt, args);

	ImVec2 sPos;
	if (!WorldToScreen(pos, sPos))
		return;

	ImVec2 textSize = ImGui::CalcTextSize(text, text_end);
	if (centerNext)
	{
		sPos -= { textSize.x * 0.5f, textSize.y * 0.5f };
	}

	// Black rectangle background for text
	ImVec2 bgFrom = sPos;
	ImVec2 bgTo = sPos + textSize;
	GetDrawList()->AddRectFilled(bgFrom, bgTo, bgColor);
	GetDrawList()->AddText(sPos, ImGui::GetColorU32(ImGuiCol_Text), text, text_end);
}

void Im3D::TextBgColored(const ImVec3V& pos, u32 col, ConstString fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	ImGui::PushStyleColor(ImGuiCol_Text, col);
	TextBgV(pos, fmt, args);
	ImGui::PopStyleColor();
	va_end(args);
}

void Im3D::SetGizmoUseWorld(bool world)
{
	s_WorldGizmoMode = world;
}

bool Im3D::GetGizmoUseWorld()
{
	return s_WorldGizmoMode;
}

bool Im3D::GizmoTrans(ImMat44V& mtx)
{
	return Manipulate(
		GUIZMO_MTX(CViewport::GetViewMatrix()),
		GUIZMO_MTX(CViewport::GetProjectionMatrix()),
		ImGuizmo::TRANSLATE,
		ImGuizmo::WORLD,
		GUIZMO_MTX(mtx));
}

bool Im3D::Gizmo(ImMat44V& mtx, ImMat44V& delta, ImGuizmo::OPERATION op)
{
	return Manipulate(
		GUIZMO_MTX(CViewport::GetViewMatrix()),
		GUIZMO_MTX(CViewport::GetProjectionMatrix()),
		op,
		s_WorldGizmoMode ? ImGuizmo::WORLD : ImGuizmo::LOCAL,
		GUIZMO_MTX(mtx),
		GUIZMO_MTX(delta));
}

bool Im3D::GizmoTrans(ImVec3V& translation)
{
	rage::Mat44V mtx(DirectX::XMMatrixTranslationFromVector(translation));
	bool moved = GizmoTrans(mtx);
	mtx.Decompose(&translation, nullptr, nullptr);
	return moved;
}

bool Im3D::GizmoTrans(ImVec3& translation)
{
	ImVec3V translationV = translation;
	bool moved = GizmoTrans(translationV);
	translation = translationV;
	return moved;
}

bool Im3D::GizmoBehaviour(bool& isDragging, const ImVec3V& planePos, const ImVec3V& planeNormal, ImVec3V& moveDelta, ImVec3V& startPos, ImVec3V& dragPos)
{
	static bool s_IsDragging = false;
	static ImVec2 s_DragStartPos;

	isDragging = false;

	int mouse = ImGuiMouseButton_Left;
	bool mouseDown = ImGui::IsMouseDown(mouse);

	if (!s_IsDragging && !mouseDown)
		return false;

	ImVec2 mousePos = ImGui::GetMousePos();

	if (ImGui::IsMouseClicked(mouse, false))
	{
		s_IsDragging = true;
		s_DragStartPos = mousePos;
	}

	// Get world mouse rays for where mouse position was at start of dragging & now
	rage::Vec3V mouseRayPos, mouseRayDir;
	CViewport::GetWorldRayFromScreen(mouseRayPos, mouseRayDir, mousePos.x, mousePos.y);
	rage::Vec3V startMouseRayPos, startMouseRayDir;
	CViewport::GetWorldRayFromScreen(startMouseRayPos, startMouseRayDir, s_DragStartPos.x, s_DragStartPos.y);

	// Cast rays on to dragging plane
	rage::Vec3V mouseOnPlane;
	rage::Vec3V startMouseOnPlane;
	rageam::graphics::ShapeTest::RayIntersectsPlane(mouseRayPos, mouseRayDir, planePos, planeNormal, mouseOnPlane);
	rageam::graphics::ShapeTest::RayIntersectsPlane(startMouseRayPos, startMouseRayDir, planePos, planeNormal, startMouseOnPlane);

	// Im3D::Text(mouseOnPlane, "#1");
	// Im3D::Text(startMouseOnPlane, "#2");

	dragPos = mouseOnPlane;
	startPos = startMouseOnPlane;
	moveDelta = mouseOnPlane - startMouseOnPlane;
	isDragging = s_IsDragging;

	if (!mouseDown) // Finished dragging
	{
		s_IsDragging = false;
		isDragging = false;
		return true;
	}
	return false; // Still dragging
}

bool Im3D::Begin(const ImVec3V& startPos, bool background)
{
	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_AlwaysAutoResize;

	ImVec2 sPos;
	bool isCulled = !WorldToScreen(startPos, sPos);

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowSize(io.DisplaySize);
	ImGui::SetNextWindowPos(sPos);

	ImGui::PushStyleColor(ImGuiCol_WindowBg, background ? bgColor : 0);
	ImGui::PushStyleColor(ImGuiCol_Border, 0);
	ImGui::PushStyleColor(ImGuiCol_WindowShadow, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	ImGui::Begin("Im3D", 0, flags);
	return !isCulled;
}

void Im3D::End()
{
	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(3); // WindowBg, Border, WindowShadow
}

bool Im3D::IsViewportHovered()
{
	return GImGui->HoveredWindow == nullptr; // TODO: Add gizmo here
}

bool Im3D::IsViewportFocused()
{
	return GImGui->ActiveIdWindow == nullptr; // TODO: Add gizmo here
}
