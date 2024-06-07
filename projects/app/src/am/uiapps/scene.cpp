#include "scene.h"

#ifdef AM_INTEGRATED
#include "am/integration/ui/modelinspector.h"
#include "am/integration/ui/modelscene.h"
#endif

#include <easy/profiler.h>

void rageam::ui::Scene::TryOpenPendingSceneWindow()
{
	if (String::IsNullOrEmpty(sm_PendingScenePath))
		return;

	// Old scene is still valid...
	if (!sm_OpenedSceneWeakRef.expired())
		return;

	EASY_BLOCK("Scene::TryOpenPendingSceneWindow");

	WindowManager* windows = GetUI()->Windows;
	Scene* newScene = nullptr;

	SceneType sceneType = GetSceneType(sm_PendingScenePath);
	switch (sceneType)
	{
#ifdef AM_INTEGRATED
	case Scene_Inspector: newScene = new integration::ModelInspector();	break;
	case Scene_Editor:	  newScene = new integration::ModelScene();		break;
#endif

	case Scene_Invalid:
		AM_ERRF(L"Scene::LoadScene() -> Unable to resolve for '%ls'", sm_PendingScenePath.GetCStr());
		break;
	}

	if (!newScene)
		return;

	newScene->SetPosition(sm_PendingScenePosition);
	newScene->LoadFromPath(sm_PendingScenePath);

	sm_OpenedSceneWeakRef = std::weak_ptr(std::dynamic_pointer_cast<Scene>(windows->Add(newScene)));
	sm_PendingScenePath = L"";
	sm_PendingScenePosition = {};
}

void rageam::ui::Scene::ConstructFor(ConstWString path)
{
	// Find and close existing scene window
	WindowManager* windows = GetUI()->Windows;
	WindowPtr existingScene = windows->GetExisting<Scene>();
	if (existingScene)
	{
		windows->Close(existingScene);
	}

	// We can't immediately load new scene if there's already one opened,
	// mainly because entity takes few frames to completely unload,
	// instead we put new scene in queue and wait until old one fully closes
	sm_PendingScenePath = path;
	sm_PendingScenePosition = DefaultSpawnPosition;
}
