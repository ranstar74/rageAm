//
// File: mateditor.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/ui/app.h"
#include "am/integration/gameentity.h"
#include "game/modelinfo/basemodelinfo.h"
#include "am/ui/font_icons/icons_am.h"

#include <ImGui.h>

// TODO: Async model loading

namespace rageam::integration
{
	class ModelInspector : public ui::App
	{
		ComponentOwner<GameEntity> m_GameEntity;
		amPtr<CBaseArchetypeDef>   m_ArchetypeDef;
		gtaDrawablePtr			   m_Drawable;
		u32						   m_FileSize = 0;
		u32						   m_VirtualSize = 0;
		u32						   m_PhysicalSize = 0;
		u32						   m_ResourceSize = 0;
		ImGuiID					   m_SelectedRowID = 0;
		int						   m_MaterialIndexToSelect = -1;

		ConstString FormatVec(const Vec3V& vec) const;

		bool IconNav() const { return IconButton(ICON_AM_BACKWARDS); }
		bool IconButton(ConstString name) const;
		void DisabledCheckBox(ConstString name, bool v) const;
		void PropertyValue(ConstString propertyName, ConstString valueFormat, ...);
		bool BeginCategory(ConstString name, bool defaultOpen = false);
		void EndCategory() const;

		void DrawGeometry(rage::grmModel* model, u16 index);
		void DrawLodGroup();
		void DrawShaderParams(const rage::grmShader* shader, rage::grcEffect* effect);
		void DrawMaterialGroup();
		void OnRender() override;

	public:
		ModelInspector();

		void LoadFromPath(ConstWString path);
	};
}
