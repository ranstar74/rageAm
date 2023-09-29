#include "updatecomponent.h"
#ifdef AM_INTEGRATED

#include <mutex>

#include "shvthread.h"

namespace { std::mutex s_Mutex; }

rage::atFixedArray<
	rageam::integration::IUpdateComponent*,
	rageam::integration::MAX_UPDATE_COMPONENTS>
	rageam::integration::IUpdateComponent::sm_UpdateComponents;

rageam::integration::IUpdateComponent::IUpdateComponent()
{
	std::unique_lock lock(s_Mutex);
	sm_UpdateComponents.Add(this);
}

rageam::integration::IUpdateComponent::~IUpdateComponent()
{
	std::unique_lock lock(s_Mutex);
	sm_UpdateComponents.Remove(this);
}

void rageam::integration::IUpdateComponent::EarlyUpdateAll()
{
	std::unique_lock lock(s_Mutex);
	scrBegin();
	for (auto component : sm_UpdateComponents)
	{
		// Init on first call
		if (!component->m_Initialized)
		{
			component->OnStart();
			component->m_Initialized = true;
		}

		component->OnEarlyUpdate();
	}
	scrEnd();
}

void rageam::integration::IUpdateComponent::LateUpdateAll()
{
	std::unique_lock lock(s_Mutex);
	scrBegin();
	for (auto component : sm_UpdateComponents)
		component->OnLateUpdate();
	scrEnd();
}
#endif
