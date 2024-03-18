#include "scene.h"

#ifdef AM_INTEGRATED
#include "am/integration/ui/modelinspector.h"
#include "am/integration/ui/modelscene.h"
#endif

void rageam::ui::Scene::OpenWindowForSceneAndLoad(ConstWString path)
{
	sm_Instance = nullptr;

	WindowManager* windows = GetUI()->Windows;

	// Find and close existing scene window
	WindowPtr existingScene = windows->GetExisting<Scene>();
	if (existingScene)
		windows->Close(existingScene);

	Scene* openedScene = nullptr;

	SceneType sceneType = GetSceneType(path);
	switch (sceneType)
	{
#ifdef AM_INTEGRATED
		case Scene_Inspector: openedScene = new integration::ModelInspector();	break;
		case Scene_Editor:	  openedScene = new integration::ModelScene();		break;
#endif

		case Scene_Invalid:
			AM_ERRF(L"Scene::LoadScene() -> Unable to resolve for '%ls'", path);
			break;
	}

	if (!openedScene)
		return;

	windows->Add(openedScene);
	openedScene->LoadFromPath(path);

	sm_Instance = openedScene;
}
