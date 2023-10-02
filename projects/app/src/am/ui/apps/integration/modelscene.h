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

#include <mutex>

namespace rageam
{
	class ModelScene : integration::IUpdateComponent
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
		std::atomic_bool			m_IsLoaded = false;
		std::recursive_mutex		m_Mutex;
		amUniquePtr<LoadRequest>	m_LoadRequest;
		bool						m_HasModelRequest = false;
		bool						m_CleanUpRequested = false;
		rage::fiDirectoryWatcher	m_FileWatcher;

		void CreateEntity(const rage::Vec3V& coors);
		void DeleteEntity();
		bool LoadAndCompileDrawable(ConstWString path);
		void RegisterArchetype();
		void UnregisterArchetype();
		void FinalizeOldArchetype();
		void RegisterDrawable();
		void UnregisterDrawable();
		void RequestReload();
		void DeleteDrawable();

	public:
		~ModelScene() override;

		void OnEarlyUpdate() override;
		void OnLateUpdate() override;

		void SetupFor(ConstWString path, const rage::Vec3V& coors);
		void CleanUp();

		void SetEntityPos(const rage::Vec3V& pos);

		gtaDrawable* GetDrawable();
		void SetDrawable(gtaDrawable* drawable);
		u32 GetEntityHandle() const { return m_Entity; }

		std::function<void()> LoadCallback;
	};

	class ModelSceneApp : public ui::App
	{
		// static inline const ConstWString ASSET_PATH = L"C:/Users/falco/Desktop/collider.idr";

		using Camera = amUniquePtr<integration::ICameraComponent>;

		static inline const rage::Vec3V DEFAULT_ISOLATED_POS = { -1700, -6000, 310 };
		static inline const rage::Vec3V DEFAULT_POS = { -676, 167, 73.55f };
		static constexpr u32 RAGEAM_HASH = Hash("RAGEAM_TESTBED_ARCHETYPE");

		file::WPath		m_AssetPath; // User/Desktop/rageAm.idr
		rage::Vector3	m_Dimensions;
		u32				m_NumLods = 0;
		u32				m_NumModels = 0;
		u32				m_NumGeometries = 0;
		u32				m_VertexCount = 0;
		u32				m_TriCount = 0;
		ModelScene		m_ModelScene;
		Camera			m_Camera;
		bool			m_IsolatedSceneActive = false;
		bool			m_CameraEnabled = false;
		bool			m_UseOrbitCamera = true;

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
