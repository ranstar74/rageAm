#include "updatecomponent.h"

#include "integration.h"
#ifdef AM_INTEGRATED

#include "shvthread.h"

namespace
{
	std::recursive_mutex s_Mutex; // Recurse allows components to own other components (or reference them)
}

void rageam::integration::IUpdateComponent::Abort()
{
	m_AbortRequested = true;
	ReleaseAllRefs();
}

bool rageam::integration::ComponentManager::HasAnyComponent() const
{
	std::unique_lock lock(s_Mutex);
	return m_UpdateComponents.Any();
}

rageam::integration::ComponentManager* rageam::integration::ComponentManager::GetCurrent()
{
	return sm_Instance;
}

void rageam::integration::ComponentManager::SetCurrent(ComponentManager* instance)
{
	sm_Instance = instance;
}

std::recursive_mutex& rageam::integration::ComponentManager::GetLock()
{
	return s_Mutex;
}

void rageam::integration::ComponentManager::AbortAll() const
{
	std::unique_lock lock(s_Mutex);
	for (amUniquePtr<IUpdateComponent>& component : m_UpdateComponents)
	{
		component->Abort();
	}
}

bool rageam::integration::ComponentManager::IsAllAborted() const
{
	std::unique_lock lock(s_Mutex);
	return !m_UpdateComponents.Any();
}

void rageam::integration::ComponentManager::EarlyUpdateAll()
{
	std::unique_lock lock(s_Mutex);

	scrBegin();

	rage::atArray<u16> componentsToRemove;
	for (u16 i = 0; i < m_UpdateComponents.GetSize(); i++)
	{
		amUniquePtr<IUpdateComponent>& component = m_UpdateComponents[i];

		if (component->m_AbortRequested)
		{
			// We keep updating component until it completely unloads
			if (component->OnAbort())
			{
				// In order to delete multiple indices we have to delete them from the end
				// otherwise elements will shift and indices won't be valid anymore
				componentsToRemove.Insert(0, i);
				continue; // Component is aborted, no need to update anymore
			}
		}

		// Init on first call
		if (!component->m_Initialized)
		{
			component->OnStart();
			component->m_Initialized = true;
		}

		component->OnEarlyUpdate();
	}

	for (u16 i : componentsToRemove)
		m_UpdateComponents.RemoveAt(i);

	scrEnd();
}

void rageam::integration::ComponentManager::LateUpdateAll() const
{
	std::unique_lock lock(s_Mutex);
	scrBegin();
	for (amUniquePtr<IUpdateComponent>& component : m_UpdateComponents)
	{
		component->OnLateUpdate();
	}
	scrEnd();
}

void rageam::integration::ComponentManager::BeginAbortAll() const
{
	AbortAll();
	while (!IsAllAborted())
	{
		// Wait until every component is aborted...
	}
}

void rageam::integration::ComponentManager::UIUpdateAll() const
{
	if (GameIntegration::GetInstance()->IsPauseMenuActive())
		return;

	scrBegin();
	for (amUniquePtr<IUpdateComponent>& component : m_UpdateComponents)
	{
		component->OnUiUpdate();
	}
	scrEnd();
}

#endif
