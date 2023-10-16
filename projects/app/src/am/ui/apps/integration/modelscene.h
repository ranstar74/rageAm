#pragma once

#ifdef AM_INTEGRATED

#include "lighteditor.h"
#include "mateditor.h"
#include "am/types.h"
#include "am/integration/shvthread.h"
#include "am/integration/updatecomponent.h"
#include "am/integration/components/camera.h"
#include "am/integration/hooks/streaming.h"
#include "am/task/worker.h"
#include "am/ui/app.h"
#include "game/archetype.h"
#include "rage/file/watcher.h"
#include "rage/math/vecv.h"
#include "scripthook/shvnatives.h"
#include "am/asset/workspace.h"

#include <mutex>

namespace rageam
{
	class BackgroundTask;

	// State that we are querying from UI thread every tick, everything packed
	// in one struct to prevent too much mutex use
	struct ModelViewerState
	{
		bool			IsLoading;
		bool			IsEntitySpawned;
		rage::Mat44V	EntityWorld;
		u32				EntityHandle;
		pVoid			EntityPtr;
	};

	// Data shared between all components of the scene (scene graph, light editor, material editor etc)
	struct ModelSceneContext
	{
		ModelViewerState		ViewerState;			// State of spawned entity
		asset::WorkspacePtr		Workspace;				// Workspace of drawable asset, may be NULL
		asset::DrawableAssetPtr DrawableAsset;			// Drawable source asset
		gtaDrawable*			Drawable;				// Compiled drawable from the asset
		s32						JustChangedTXD = -1;	// Index of TXD in workspace that was edited in TXD Editor
		SmallList<rage::grcTextureDictionaryPtr> TXDs;	// Workspace TXDs 

		auto GetDrawableMap() const { return DrawableAsset ? &DrawableAsset->CompiledDrawableMap : nullptr; }
		auto GetScene() const { return DrawableAsset ? DrawableAsset->GetScene().get() : nullptr; }

		// Compiles ALL workspace texture dictionaries, we need this really only for material editor
		// TODO: We may want lazy loading here
		void CompileWorkspaceTXDs();
	};

	/**
	 * \brief Handles entity spawning in the game world
	 * \n In the future we can extend this to interface class to support standalone model viewer
	 */
	class ModelViewer : public integration::IUpdateComponent
	{
		using ModelInfo = CBaseModelInfo*;
		static constexpr u32 RAGEAM_HASH = Hash("RAGEAM_TESTBED_ARCHETYPE");

		struct LoadRequest
		{
			wstring	Path;
		};

		rage::Vec3V					m_EntityPos;
		u64							m_Entity = 0; // fwEntity*
		SHV::Object					m_EntityHandle = 0;
		rage::strLocalIndex			m_DrawableSlot = rage::INVALID_STR_INDEX;
		rage::strLocalIndex			m_DictSlot = rage::INVALID_STR_INDEX;
		ModelInfo					m_Archetype = nullptr;
		ModelInfo					m_ArchetypeOld = nullptr;
		amUniquePtr<gtaDrawable>	m_Drawable;
		amUniquePtr<gtaDrawable>	m_DrawableRequest;
		std::recursive_mutex		m_Mutex;
		amUniquePtr<LoadRequest>	m_LoadRequest;
		bool						m_IsLoaded = false;
		bool						m_HasModelRequest = false;
		bool						m_CleanUpRequested = false;
		rage::fiDirectoryWatcher	m_FileWatcher;
		amPtr<BackgroundTask>		m_LoadTask;
		amPtr<asset::DrawableAsset> m_DrawableAsset; // Exists only during loading

		void CreateEntity(const rage::Vec3V& coors);
		void DeleteEntity();
		void LoadAndCompileDrawableAsync(ConstWString path);
		void RegisterArchetype();
		void UnregisterArchetype();
		void FinalizeOldArchetype();
		void RegisterDrawable();
		void UnregisterDrawable();
		void RequestReload();
		void DeleteDrawable();

	public:
		bool OnAbort() override;
		void OnEarlyUpdate() override;
		void OnLateUpdate() override;

		void SetupFor(ConstWString path, const rage::Vec3V& coors);
		void CleanUp();

		void SetEntityPos(const rage::Vec3V& pos);

		void GetState(ModelViewerState& state);

		std::function<void(asset::DrawableAssetPtr&, gtaDrawable*)> LoadCallback;
	};

	class ModelSceneUI : public ui::App
	{
		using CameraOwner = integration::ComponentOwner<integration::ICameraComponent>;
		using ModelSceneOwner = integration::ComponentOwner<ModelViewer>;

		static inline const rage::Vec3V DEFAULT_ISOLATED_POS = { -1700, -6000, 310 };
		static inline const rage::Vec3V DEFAULT_POS = { -676, 167, 73.55f };
		static constexpr u32 RAGEAM_HASH = Hash("RAGEAM_TESTBED_ARCHETYPE");

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

		//file::WPath					m_AssetPath; // User/Desktop/rageAm.idr
		rage::Vec3V					m_ScenePosition = DEFAULT_POS;
		rage::Vector3				m_Dimensions;
		u32							m_NumLods = 0;
		u32							m_NumModels = 0;
		u32							m_NumGeometries = 0;
		u32							m_VertexCount = 0;
		u32							m_TriCount = 0;
		u32							m_LightCount = 0;
		ModelSceneOwner				m_ModelViewer;
		CameraOwner					m_Camera;
		bool						m_IsolatedSceneActive = false;
		bool						m_CameraEnabled = false;
		bool						m_UseOrbitCamera = true;
		std::mutex					m_Mutex;

		// Graph View
		s32							m_SelectedNodeIndex = -1;
		eSceneNodeAttr				m_SelectedNodeAttr = SceneNodeAttr_None;
		eSceneNodeAttr				m_JustSelectedNodeAttr = SceneNodeAttr_None; // In current frame

		integration::LightEditor	m_LightEditor;
		integration::MaterialEditor	m_MaterialEditor;

		ModelSceneContext			m_Context;

		asset::DrawableAssetMap& GetDrawableMap() const;

		auto GetMeshAttr(u16 nodeIndex) const -> rage::grmModel*;
		auto GetBoneAttr(u16 nodeIndex) const -> rage::crBoneData*;
		auto GetBoundAttr(u16 nodeIndex) const -> rage::phBound*;
		auto GetLightAttr(u16 nodeIndex) const -> CLightAttr*;

		rage::Vec3V GetEntityScenePos() const;
		void UpdateDrawableStats();
		void ResetCameraPosition();
		void UpdateCamera();
		void DrawSceneGraphRecurse(const graphics::SceneNode* sceneNode);
		void DrawSceneGraph(const graphics::SceneNode* sceneNode);
		void DrawSkeletonGraph();
		void DrawNodePropertiesUI(u16 nodeIndex) const;
		void DrawDrawableUI();
		void DrawStarBar();
		void UpdateScenePosition();
		void OnRender() override;
		void OnDrawableLoaded(const asset::DrawableAssetPtr& asset, gtaDrawable* drawable);

	public:
		ModelSceneUI();

		void LoadFromPatch(ConstWString path);
	};
}
#endif
