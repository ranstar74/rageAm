#pragma once

#ifdef AM_INTEGRATED
#include "am/integration/components/drawablerender.h"
#include "am/integration/updatecomponent.h"
#include "am/integration/gameentity.h"
#include "am/uiapps/scene.h"

namespace rageam::integration
{
	/**
	 * \brief Base class for 3D scenes loaded in the game (using native entity system).
	 */
	class SceneInGame : public ui::Scene
	{
		gtaDrawablePtr				   m_Drawable;
		ComponentOwner<DrawableRender> m_DrawableRender;
		ComponentOwner<GameEntity>	   m_GameEntity;
		Vec3V						   m_CachedPosition = rage::VEC_ORIGIN; // Used when model is loading
		Vec3V						   m_CachedRotation = rage::VEC_ZERO;

		void TrySetEntityPosition(const Vec3V& pos);
		void TrySetEntityRotation(const Vec3V& angle);

	protected:
		void CreateEntity(ConstString name, const gtaDrawablePtr& drawable, const amPtr<CBaseArchetypeDef>& archetypeDef);
		void DestroyEntity();

	public:
		SceneInGame() = default;
		~SceneInGame() override { DestroyEntity(); }

		const Vec3V& GetRotation() const override;
		const Vec3V& GetPosition() const override;
		void         SetPosition(const Vec3V& pos) override;
		void         SetRotation(const Vec3V& angle) override;
		void         FocusCamera() override;
		GameEntity*  GetEntity() const { return m_GameEntity.Get(); }
	};
}
#endif
