#pragma once
#include "sceneingame.h"

#ifdef AM_INTEGRATED

#include "lighteditor.h"
#include "mateditor.h"
#include "am/ui/app.h"
#include "am/integration/gameentity.h"
#include "am/asset/types/hotdrawable.h"
#include "am/integration/components/drawablerender.h"

namespace rageam::integration
{
	// TODO: UI for archetype definition

	// Data shared between all components of the scene (scene graph, light editor, material editor etc.)
	struct ModelSceneContext
	{
		bool                        IsDrawableLoading;
		int                         EntityHandle;
		Mat44V                      EntityWorld;
		pVoid                       EntityPtr; // fwEntity
		AssetHotFlags               HotFlags;
		gtaDrawablePtr              Drawable;
		asset::DrawableAssetPtr     DrawableAsset;
		rage::grcTextureDictionary* MegaDictionary;
		asset::HotDrawable*         HotDrawable;
	};

	struct DrawableStats
	{
		Vec3S Dimensions; // Size of drawable on each axis
		u32   Bones;
		u32   Lods;
		u32   Models;
		u32   Geometries;
		u32   Vertices;
		u32   Triangles;
		u32   Lights;

		static DrawableStats ComputeFrom(gtaDrawable* drawable);
	};

	// static const Vec3V SCENE_ISOLATED_POS = { -1700, -6000, 310 };
	// static const Vec3V SCENE_POS = { -676, 167, 73.55f };
	// static const Vec3V SCENE_POS = { 285, -1580, 30 };
	// static const Vec3V SCENE_POS = { 0, 0, 5 };

	// TODO: Rename to SceneEditor
	class ModelScene : public SceneInGame
	{
		enum eSceneNodeAttr
		{
			SceneNodeAttr_None,
			SceneNodeAttr_Mesh,			// rage::grmModel
			SceneNodeAttr_Bone,			// rage::crBoneData
			SceneNodeAttr_Collision,	// rage::phBound
			SceneNodeAttr_Light,		// CLightAttr
		};

		static constexpr ConstString SceneNodeAttrDisplay[] =
		{
			"None", "Mesh", "Bone", "Collision", "Light"
		};

		asset::HotDrawablePtr	 m_HotDrawable;
		amPtr<CBaseArchetypeDef> m_ArchetypeDef;
		ModelSceneContext		 m_Context;
		// Graph View
		s32						 m_SelectedNodeIndex = -1;
		s32						 m_HoveredNodeIndex = -1;
		eSceneNodeAttr			 m_SelectedNodeAttr = SceneNodeAttr_None;
		eSceneNodeAttr			 m_JustSelectedNodeAttr = SceneNodeAttr_None; // In current frame
		DrawableStats			 m_DrawableStats;
		SmallList<string>		 m_CompilerMessages;
		double					 m_CompilerProgress = 0.0;
		std::mutex				 m_CompilerMutex;
		bool					 m_AssetTuneChanged = false;		 // Set by UI to post reload request on the end of frame
		bool					 m_ResetUIAfterCompiling = false; // We don't want to reset UI when just recompiling drawable
		file::Path				 m_LoadedName;

		auto GetDrawable()									const { return m_Context.Drawable.Get(); }
		auto GetDrawableMap()								const -> const asset::DrawableAssetMap&;
		auto GetMeshAttr(u16 nodeIndex)						const -> rage::grmModel*;
		auto GetBoneAttr(u16 nodeIndex)						const -> rage::crBoneData*;
		auto GetBoundAttr(u16 nodeIndex, u16 arrayIndex)	const -> rage::phBound*;
		auto GetBoundAttrCount(u16 nodeIndex)				const -> int;
		auto GetLightAttr(u16 nodeIndex)					const -> CLightAttr*;

		void CreateArchetypeDefAndSpawnGameEntity();
		// Spawns entity and resets scene state
		void OnDrawableCompiled();
		void UpdateHotDrawableAndContext();

		void DrawSceneGraphRecurse(const graphics::SceneNode* sceneNode);
		void DrawSceneGraph(const graphics::SceneNode* sceneNode);
		void DrawSkeletonGraph();

		void DrawNodePropertiesUI(u16 nodeIndex);
		void DrawDrawableUI();
		void OnRender() override;

		void SaveAssetConfig() const;

	public:
		ModelScene();

		ConstString GetName() const override { return "Editor"; }
		ConstString GetModelName() const override { return m_LoadedName; }
		bool HasMenu() const override { return true; }

		// Unloads currently loaded scene and destroys spawned drawable
		void Unload(bool keepHotDrawable = false);
		void LoadFromPath(ConstWString path) override;

		LightEditor		LightEditor;
		MaterialEditor	MaterialEditor;
	};
}

#endif
