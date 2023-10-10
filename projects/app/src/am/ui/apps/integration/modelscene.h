#pragma once
#pragma once
#include "am/ui/styled/slwidgets.h"

#ifdef AM_INTEGRATED

#include "am/types.h"
#include "am/asset/factory.h"
#include "am/integration/hooks/streaming.h"
#include "am/integration/shvthread.h"
#include "am/integration/updatecomponent.h"
#include "am/integration/components/camera.h"
#include "am/ui/app.h"
#include "game/archetype.h"
#include "rage/file/watcher.h"
#include "rage/math/vecv.h"
#include "scripthook/shvnatives.h"
#include "am/task/worker.h"
#include "lighteditor.h"

#include <mutex>

namespace rageam
{
	class BackgroundTask;

	// State that we are querying from UI thread every tick, everything packed
	// in one struct to prevent too much mutex use
	struct ModelSceneState
	{
		bool			IsLoading;
		bool			IsEntitySpawned;
		rage::Mat44V	EntityWorld;
		u32				EntityHandle;
		pVoid			EntityPtr;
	};

	class ModelScene : public integration::IUpdateComponent
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

		void GetState(ModelSceneState& state);

		std::function<void(asset::DrawableAssetPtr&, gtaDrawable*)> LoadCallback;
	};

	class ModelSceneApp : public ui::App
	{
		using CameraOwner = integration::ComponentOwner<integration::ICameraComponent>;
		using ModelSceneOwner = integration::ComponentOwner<ModelScene>;

		static inline const rage::Vec3V DEFAULT_ISOLATED_POS = { -1700, -6000, 310 };
		static inline const rage::Vec3V DEFAULT_POS = { -676, 167, 73.55f };
		static constexpr u32 RAGEAM_HASH = Hash("RAGEAM_TESTBED_ARCHETYPE");

		enum eSceneNodeAttr
		{
			SceneNodeAttr_None,
			SceneNodeAttr_Mesh,
			SceneNodeAttr_Bone,
			SceneNodeAttr_Light,
			SceneNodeAttr_Collision,
		};

		static constexpr ConstString SceneNodeAttrDisplay[] =
		{
			"None", "Mesh", "Bone", "Light", "Collision"
		};

		file::WPath					m_AssetPath; // User/Desktop/rageAm.idr
		rage::Vec3V					m_ScenePosition = DEFAULT_POS;
		rage::Vector3				m_Dimensions;
		u32							m_NumLods = 0;
		u32							m_NumModels = 0;
		u32							m_NumGeometries = 0;
		u32							m_VertexCount = 0;
		u32							m_TriCount = 0;
		u32							m_LightCount = 0;
		ModelSceneOwner				m_ModelScene;
		CameraOwner					m_Camera;
		bool						m_IsolatedSceneActive = false;
		bool						m_CameraEnabled = false;
		bool						m_UseOrbitCamera = true;
		integration::LightEditor	m_LightEditor;
		graphics::ScenePtr			m_Scene; // Drawable scene
		asset::DrawableAssetMap		m_DrawableSceneMap;
		gtaDrawable*                m_Drawable;
		ModelSceneState				m_ModelState;
		std::mutex					m_Mutex;
		//SmallList<ImRect>			m_GraphRectStack;

		// Graph View
		s32							m_SelectedNodeIndex = -1;
		eSceneNodeAttr				m_SelectedNodeAttr = SceneNodeAttr_None;

		bool SceneTreeNode(ConstString text, bool& selected, bool& toggled, SlGuiTreeNodeFlags flags);

		rage::Vec3V GetEntityScenePos() const;
		void UpdateDrawableStats();
		void ResetCameraPosition();
		void UpdateCamera();
		void DrawSceneGraphRecurse(const graphics::SceneNode* sceneNode);
		void DrawSceneGraph(const graphics::SceneNode* sceneNode);
		void DrawSkeletonGraph();
		void DrawDrawableUI();
		void DrawStarBar();
		void UpdateScenePosition();
		void OnRender() override;
		void OnDrawableLoaded(const asset::DrawableAssetPtr& asset, gtaDrawable* drawable);

	public:
		ModelSceneApp();
	};
}
#endif
