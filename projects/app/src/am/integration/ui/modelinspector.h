//
// File: mateditor.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "am/integration/gameentity.h"
#include "game/modelinfo/basemodelinfo.h"
#include "am/ui/font_icons/icons_am.h"
#include "sceneingame.h"

#include <ImGui.h>

// TODO: Async model loading

namespace rageam::integration
{
	class ModelInspector : public SceneInGame
	{
		amPtr<CBaseArchetypeDef> m_ArchetypeDef;
		gtaDrawablePtr           m_Drawable;
		u32                      m_FileSize = 0;
		u32                      m_VirtualSize = 0;
		u32                      m_PhysicalSize = 0;
		u32                      m_ResourceSize = 0;
		ImGuiID                  m_SelectedRowID = 0;
		int                      m_SelectedMaterial = 0;

		ConstString FormatVec(const Vec3V& vec) const;

		bool IconNav() const { return IconButton(ICON_AM_BACKWARDS); }
		bool IconButton(ConstString name) const;
		void DisabledCheckBox(ConstString name, bool v) const;
		void PropertyValue(ConstString propertyName, ConstString valueFormat, ...);
		bool BeginCategory(ConstString name, bool defaultOpen = false, bool spanAllColumns = false);
		void EndCategory() const;
		bool BeginProperties() const;
		void EndProperties() const;

		ConstString FormatMaterialName(int index) const;

		void DrawGeometry(rage::grmModel* model, u16 index);
		void DrawLodGroup();
		void DrawShaderParams(const rage::grmShader* shader, rage::grcEffect* effect);
		void DrawMaterialGroup();
		void OnRender() override;
		
	public:
		ModelInspector() = default;

		ConstString GetName() const override { return "Inspector"; }
		ConstString GetModelName() const override { return m_Drawable ? m_Drawable->GetName() : nullptr; }
		bool Padding() const override { return true; }
		void LoadFromPath(ConstWString path) override;
	};
}

#endif
