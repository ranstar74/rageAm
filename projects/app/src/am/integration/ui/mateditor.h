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
#include "rage/file/watcher.h"
#include "am/system/datamgr.h"

namespace rageam::integration
{
	struct ModelSceneContext;

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
		ST_Vehicle  = 1 << 0,
		ST_Ped      = 1 << 1,
		ST_Weapon   = 1 << 2,
		ST_Terrain  = 1 << 3,
		ST_Glass    = 1 << 4,
		ST_Decal    = 1 << 5,
		ST_Cloth    = 1 << 6,
		ST_Cutout   = 1 << 7,
		ST_Emissive = 1 << 8,
		ST_Parallax = 1 << 9,
		ST_Misc     = 1 << 10, // For shaders like 'minimap' that don't fit anywhere else
	};
	static constexpr u32 SHADER_CATEGORIES_COUNT = 11;

	// Texture types that shader supports
	enum ShaderMaps : u32
	{
		SM_Detail   = 1 << 0,
		SM_Specular = 1 << 1,
		SM_Normal   = 1 << 2,
		SM_Cubemap  = 1 << 3,
		SM_Tint     = 1 << 4,
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
		struct TextureSearch
		{
			char							DictName[128];
			rage::grcTextureDictionary*		Dict;
			SmallList<u16>					Textures;
		};

		union VarBlob
		{
			// rage::grcInstanceVar data is divided on 16 byte blocks,
			// maximum data size we can have is 4x4 mat (64 bytes)
			char Data[64];
			// We can't store grcTexture pointer because it won't be synced
			// with HotDrawable, instead we store texture name
			char TextureName[128];
		};

		ModelSceneContext*				m_Context;
		int								m_SelectedMaterialIndex = 0;
		ShaderUIConfig					m_UIConfig;
		rage::fiDirectoryWatcher		m_UIConfigWatcher;
		SmallList<ShaderPreset>			m_ShaderPresets;
		SmallList<u16>					m_PresetSearchIndices;
		char							m_PresetSearchText[64] = {};
		u32								m_PresetSearchCategories= 0;
		u32								m_PresetSearchMaps = 0;
		char							m_TextureSearchText[256] = {};
		SmallList<TextureSearch>		m_TextureSearchEntries;
		// Those two are used only in list texture picker mode
		int								m_DictionaryIndex = 0;
		int								m_TextureIndex = 0; // In TextureSearch::Textures
		// We store all material parameters input by user to preserve them on switching shader
		SmallList<HashSet<VarBlob>>		m_MaterialValues;
		double							m_TexturePickerOpenTime = 0.0f;
		// Shaders that are marked for copying settings to them
		SmallList<bool>					m_MarkedShaders;
		SmallList<bool>					m_IgnoredShaderVars;

		// Loads preload.list and assigns tags to shader presets
		void InitializePresetSearch();
		void ComputePresetFilterTagAndTokens(ShaderPreset& preset) const;

		rage::grmShaderGroup* GetShaderGroup() const;
		rage::grmShader*      GetSelectedMaterial() const;
		asset::MaterialTune*  GetSelectedMaterialTune() const;

		// Returns UI override for missing textures 
		ImTextureID GetTexID(const rage::grcTexture* tex) const;
		ImTextureID GetThumbnailTexID(const rage::grcTexture* tex) const;
		ImU32 GetTexLabelCol(const rage::grcTexture* tex) const;
		void ScrollingLabel(const ImVec2& pos, const ImRect& bb, const rage::grcTexture* texture) const;
		rage::grcTexture* TexturePicker_Grid(bool groupByDict, float iconScale);
		rage::grcTexture* TexturePicker_List(float iconScale);
		rage::grcTexture* TexturePicker(ConstString id, const rage::grcTexture* currentTexture);
		void DoTextureSearch();

		void HandleMaterialValueChange(u16 varIndex, rage::grcInstanceVar* var, const rage::grcEffectVar* varInfo);
		void HandleShaderChange();

		void DoFuzzySearch();
		void DrawShaderSearchListItem(u16 index);
		void DrawShaderSearchList();
		void DrawShaderSearchFilters();
		void DrawMaterialList();

		void DrawMaterialVariables();
		void DrawMaterialOptions() const;

		// Saves given variable value
		void StoreMaterialValue(u16 materialIndex, u16 varIndex);
		// Tries to retrieve saved variable value
		void RestoreMaterialValue(u16 materialIndex, u16 varIndex);

		void CopyMaterialSettingsButton();

	public:
		MaterialEditor(ModelSceneContext* sceneContext);

		void Render();

		// Resets state for new shader group
		void Reset();

		bool IsOpen = false;
	};
}

#endif
