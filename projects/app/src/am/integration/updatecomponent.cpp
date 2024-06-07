#include "updatecomponent.h"

#ifdef AM_INTEGRATED

#include "script/core.h"
#include <easy/profiler.h>

void rageam::integration::ComponentManager::RemoveAbortedComponents()
{
	EASY_BLOCK("ComponentManager::RemoveAbortedComponents");

	m_UpdateComponents.AddRange(m_UpdateComponentsToAdd);
	m_UpdateComponentsToAdd.Destroy();

	SmallList<u16> componentsToRemove;
	for (u16 i = 0; i < m_UpdateComponents.GetSize(); i++)
	{
		IUpdateComponent* component = m_UpdateComponents[i];
		if (component->m_RefCount != 0)
			continue;

		if(!component->OnAbort())
		{
			component->m_AbortAttempts++;
			AM_ASSERT(component->m_AbortAttempts < 100, 
				"ComponentManager::RemoveAbortedAttempts() -> Possible stall.");
			continue;
		}

		// In order to delete multiple indices we have to delete them from the end
		// otherwise elements will shift and indices won't be valid anymore
		componentsToRemove.Insert(0, i);
	}

	for (u16 i : componentsToRemove)
	{
		delete m_UpdateComponents[i];
		m_UpdateComponents.RemoveAt(i);
	}
}

void rageam::integration::ComponentManager::EarlyUpdateAll()
{
	EASY_BLOCK("ComponentManager::EarlyUpdateAll");

	m_UpdateStage = UpdateStage_Early;

	scrBegin();
	RemoveAbortedComponents();
	for (IUpdateComponent* component : m_UpdateComponents)
	{
		// Init on first call
		if (!component->m_Started)
		{
			component->OnStart();
			component->m_Started = true;
		}

		component->OnEarlyUpdate();
	}
	scrEnd();
}

void rageam::integration::ComponentManager::UpdateAll()
{
	EASY_BLOCK("ComponentManager::UpdateAll");

	m_UpdateStage = UpdateStage_SafeArea;

	scrBegin();
	RemoveAbortedComponents();
	for (IUpdateComponent* component : m_UpdateComponents)
		component->OnUpdate();
	scrEnd();
}

void rageam::integration::ComponentManager::LateUpdateAll()
{
	EASY_BLOCK("ComponentManager::LateUpdateAll");

	m_UpdateStage = UpdateStage_Late;

	scrBegin();
	for (IUpdateComponent* component : m_UpdateComponents)
		component->OnLateUpdate();
	scrEnd();
}

#endif
