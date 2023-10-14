//
// File: mateditor.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "rage/rmc/drawable.h"
#include "am/asset/types/drawable.h"
#include "rage/streaming/streaming.h"
#include "rage/file/watcher.h"

namespace rageam::integration
{
	/**
	 * \brief UI configuration for shaders, see data/ShaderUI.xml
	 */
	struct ShaderUIConfig
	{
		enum eUIWidgetType // Name must match XML file
		{
			Default = 0,
			SliderFloat = 1,
			SliderFloatLogarithmic = 2,
			ToggleFloat = 3,
			ColorRGB = 4,
		};

		struct UIVar
		{
			u32				NameHash;
			string			OverrideName;
			string			Description;
			eUIWidgetType	WidgetType = Default;
			union
			{
				struct { float Min, Max; }			SliderFloat;
				struct { float Enabled, Disabled; }	ToggleFloat;
			} WidgetParams;
		};

		HashSet<UIVar> Vars;

		// Loads config from data/ShaderUI.xml
		void Load();
		void CleanUp();

		UIVar* GetUIVarFor(const rage::grcEffectVar* varInfo) const;

		file::WPath GetFilePath() const { return DataManager::GetDataFolder() / L"ShaderUI.xml"; }
	};

	// Shader 'Kinds'
	enum ShaderCategories : u32
	{
		ST_Vehicle = 1 << 0,
		ST_Ped = 1 << 1,
		ST_Weapon = 1 << 2,
		ST_Terrain = 1 << 3,
		ST_Glass = 1 << 4,
		ST_Decal = 1 << 5,
		ST_Cloth = 1 << 6,
		ST_Cutout = 1 << 7,
		ST_Emissive = 1 << 8,
		ST_Parallax = 1 << 9,
		ST_Misc = 1 << 10,		// For shaders like 'minimap' that don't fit anywhere else
	};
	static constexpr u32 SHADER_CATEGORIES_COUNT = 11;

	// Texture types that shader supports
	enum ShaderMaps : u32
	{
		SM_Detail = 1 << 0,
		SM_Specular = 1 << 1,
		SM_Normal = 1 << 2,
		SM_Cubemap = 1 << 3,
		SM_Tint = 1 << 4,
	};
	static constexpr u32 SHADER_MAPS_COUNT = 5;

	static constexpr u32 SHADER_MAX_TOKENS = 16;
	struct ShaderPreset
	{
		struct FilterTag
		{
			u32	Categories;
			u32 Maps;
		};

		rage::grcInstanceData*	InstanceData;
		string					Name;
		string					FileName;
		u32						FileNameHash;
		FilterTag				Tag;
		std::string_view		Tokens[SHADER_MAX_TOKENS];
		u32						TokenCount;
	};

	/**
	 * \brief Origin of rageAm.
	 */
	class MaterialEditor
	{
		rage::rmcDrawable*					m_Drawable = nullptr;
		asset::DrawableAssetMap*			m_DrawableMap = nullptr;
		graphics::Scene*					m_Scene = nullptr;
		int									m_SelectedMaterialIndex = 0;
		ShaderUIConfig						m_UIConfig;
		rage::fiDirectoryWatcher			m_UIConfigWatcher;
		SmallList<ShaderPreset>				m_ShaderPresets;
		SmallList<u16>						m_SearchIndices;
		char								m_SearchText[64] = {};
		u32									m_SearchCategories= 0;
		u32									m_SearchMaps = 0;
		rage::strLocalIndex					m_SearchYtdSlot = rage::INVALID_STR_INDEX;
		rage::atArray<rage::grcTexture*>	m_SearchTextures;

		// Loads preload.list and assigns tags to shader presets
		void InitializePresetSearch();
		void ComputePresetFilterTagAndTokens(ShaderPreset& preset) const;

		rage::grmShader* GetSelectedMaterial() const;

		rage::grcTexture* TexturePicker(ConstString id, const rage::grcTexture* currentTexture);
		void HandleMaterialValueChange(u16 varIndex, rage::grcInstanceVar* var, const rage::grcEffectVar* varInfo) const;
		void HandleMaterialSelectionChanged();
		void SearchTexture(ImmutableString searchText);
		void DoFuzzySearch();
		void DrawShaderSearchListItem(u16 index);
		void DrawShaderSearchList();
		void DrawShaderSearchFilters();
		void DrawShaderSearch();
		void DrawMaterialList();
		void DrawMaterialVariables();
		void DrawMaterialOptions() const;

	public:
		MaterialEditor();

		void Render();
		void SetDrawable(rage::rmcDrawable* drawable);
		void SetMap(graphics::Scene* scene, asset::DrawableAssetMap* map);

		bool IsOpen = false;
	};
}

#endif
