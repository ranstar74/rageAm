#ifdef AM_INTEGRATED
#include "sceneingame.h"

#include "starbar.h"
#include "am/integration/components/camera.h"

void rageam::integration::SceneInGame::TrySetEntityPosition(const Vec3V& pos)
{
	if (m_GameEntity)
		m_GameEntity->SetPosition(pos);
}

void rageam::integration::SceneInGame::CreateEntity(const gtaDrawablePtr& drawable, const amPtr<CBaseArchetypeDef>& archetypeDef)
{
	m_Drawable = drawable;
	m_GameEntity.Create(drawable, archetypeDef, m_CachedPosition);
	m_DrawableRender.Create();
	m_DrawableRender->SetEntity(m_GameEntity.Get());

	StarBar* starbar = ui::GetUI()->FindAppByType<StarBar>();
	if (starbar && StarBar::FocusCameraOnScene)
	{
		starbar->SetCameraEnabled(true);
		FocusCamera();
	}
}

void rageam::integration::SceneInGame::DestroyEntity()
{
	m_Drawable = nullptr;

	// Update components are not destroyed immediately, make sure that destroyed entity won't be referenced
	if (m_DrawableRender)
		m_DrawableRender->SetEntity(nullptr);

	m_DrawableRender = nullptr;
	m_GameEntity = nullptr;
}

const rageam::Vec3V& rageam::integration::SceneInGame::GetPosition() const
{
	// At the moment we always return cached position because on model spawn
	// entity position is not updated yet and when we're trying to focus camera on it, camera always
	// focuses on the world origin (0, 0, 0)
	return m_CachedPosition; /*m_GameEntity ? m_GameEntity->GetPosition() : m_CachedPosition;*/
}

void rageam::integration::SceneInGame::SetPosition(const Vec3V& pos)
{
	m_CachedPosition = pos;
	TrySetEntityPosition(pos);
}

void rageam::integration::SceneInGame::FocusCamera()
{
	ICameraComponent* camera = ICameraComponent::GetActiveCamera();
	if (!camera)
		return;

	rage::Vec3V camPos;
	rage::Vec3V targetPos;
	const rage::Vec3V& scenePos = GetPosition();

	// Set view on drawable center if was spawned, otherwise just target scene position
	if (m_Drawable)
	{
		rage::rmcLodGroup& lodGroup = m_Drawable->GetLodGroup();
		const AABB&		   bb = lodGroup.GetBoundingBox();
		const Sphere&      bs = lodGroup.GetBoundingSphere();

		// Entities are spawned with bottom of bounding box aligned to specified coord
		Vec3V modelCenter = scenePos + rage::VEC_UP * bb.Height() * rage::S_HALF;

		// Shift camera away to fully see bounding sphere + add light padding
		Vec3V offsetDir = (rage::VEC_BACK + rage::VEC_RIGHT * 0.2f + rage::VEC_UP * 0.2f).NormalizedEstimate();
		camPos = modelCenter + offsetDir * (bs.GetRadius() * 3.25f);
		targetPos = modelCenter;
	}
	else
	{
		camPos = scenePos + rage::VEC_FORWARD + rage::VEC_UP;
		targetPos = scenePos;
	}

	camera->SetPosition(camPos);
	camera->LookAt(targetPos);
}
#endif
