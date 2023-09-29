#include "integration.h"
#ifdef AM_INTEGRATED

rageam::integration::GameIntegration* GIntegration = nullptr;

void CreateIntegrationContext()
{
	GIntegration = new rageam::integration::GameIntegration();
	GIntegration->Init();
}

void DestroyIntegrationContext()
{
	delete GIntegration;
	GIntegration = nullptr;
}
#endif
