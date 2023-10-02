#pragma once
#ifdef AM_INTEGRATED

#include "updatecomponent.h"

namespace rageam::integration
{
	class GameIntegration
	{
	public:
		amUniquePtr<ComponentManager> ComponentMgr;

		GameIntegration();
		~GameIntegration();
	};
}

extern rageam::integration::GameIntegration* GIntegration;

void CreateIntegrationContext();
void DestroyIntegrationContext();
#endif
