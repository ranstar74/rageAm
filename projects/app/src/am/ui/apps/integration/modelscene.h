#pragma once
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

#include <mutex>

namespace rageam
{
	class BackgroundTask;

	class ModelScene : public integration::IUpdateComponent
	{
		using ModelInfo = CBaseModelInfo*;
		static constexpr u32 RAGEAM_HASH = Hash("RAGEAM_TESTBED_ARCHETYPE");

		struct LoadRequest
		{
			wstring	Path;
		};

		rage::Vec3V					m_EntityPos;
		SHV::Object					m_Entity = 0;
		rage::strLocalIndex			m_DrawableSlot = rage::INVALID_STR_INDEX;
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

		gtaDrawable* GetDrawable();
		void SetDrawable(gtaDrawable* drawable);
		u32 GetEntityHandle() const { return m_Entity; }

		// Is drawable being currently loaded in background thread
		bool IsLoading();

		std::function<void()> LoadCallback;
	};

	class ModelSceneApp : public ui::App
	{
		using CameraOwner = integration::ComponentOwner<integration::ICameraComponent>;
		using ModelSceneOwner = integration::ComponentOwner<ModelScene>;

		static inline const rage::Vec3V DEFAULT_ISOLATED_POS = { -1700, -6000, 310 };
		static inline const rage::Vec3V DEFAULT_POS = { -676, 167, 73.55f };
		static constexpr u32 RAGEAM_HASH = Hash("RAGEAM_TESTBED_ARCHETYPE");

		file::WPath			m_AssetPath; // User/Desktop/rageAm.idr
		rage::Vector3		m_Dimensions;
		u32					m_NumLods = 0;
		u32					m_NumModels = 0;
		u32					m_NumGeometries = 0;
		u32					m_VertexCount = 0;
		u32					m_TriCount = 0;
		ModelSceneOwner		m_ModelScene;
		CameraOwner			m_Camera;
		bool				m_IsolatedSceneActive = false;
		bool				m_CameraEnabled = false;
		bool				m_UseOrbitCamera = true;

		rage::Vec3V GetEntityScenePos() const;
		void UpdateDrawableStats();
		void ResetCameraPosition();
		void UpdateCamera();
		void DrawDrawableUi(const gtaDrawable* drawable) const;
		void OnRender() override;

	public:
		ModelSceneApp();
	};
}
#endif
