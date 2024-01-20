#include "updatecomponent.h"

#ifdef AM_INTEGRATED

#include "script/core.h"

void rageam::integration::ComponentManager::EarlyUpdateAll()
{
	scrBegin();

	m_UpdateComponents.AddRange(m_UpdateComponentsToAdd);
	m_UpdateComponentsToAdd.Destroy();

	SmallList<u16> componentsToRemove;
	for (u16 i = 0; i < m_UpdateComponents.GetSize(); i++)
	{
		IUpdateComponent* component = m_UpdateComponents[i];

		if (component->m_Aborted)
		{
			// In order to delete multiple indices we have to delete them from the end
			// otherwise elements will shift and indices won't be valid anymore
			componentsToRemove.Insert(0, i);
			continue; // Component is aborted, no need to update anymore
		}

		// Init on first call
		if (!component->m_Started)
		{
			component->OnStart();
			component->m_Started = true;
		}

		component->OnEarlyUpdate();
	}

	for (u16 i : componentsToRemove)
	{
		delete m_UpdateComponents[i];
		m_UpdateComponents.RemoveAt(i);
	}

	scrEnd();
}

void rageam::integration::ComponentManager::LateUpdateAll() const
{
	scrBegin();
	for (IUpdateComponent* component : m_UpdateComponents)
		component->OnLateUpdate();
	scrEnd();
}

#endif
