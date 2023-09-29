#pragma once
#ifdef AM_INTEGRATED

#include "gameupdate.h"
#include "updatecomponent.h"

namespace rageam::integration
{
	class GameIntegration
	{
	public:
		void Init() const
		{
			HookGameUpdate();
		}
	};

	// Update functions are declared in gameupdate.h

	inline void EarlyGameUpdateFn()
	{
		IUpdateComponent::EarlyUpdateAll();
	}

	inline void LateGameUpdateFn()
	{
		IUpdateComponent::LateUpdateAll();
	}
}

extern rageam::integration::GameIntegration* GIntegration;

void CreateIntegrationContext();
void DestroyIntegrationContext();
#endif
