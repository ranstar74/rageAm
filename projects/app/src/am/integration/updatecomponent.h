//
// File: updatecomponents.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "rage/atl/fixedarray.h"
#include "am/system/ptr.h"
#include "rage/atl/array.h"
#include "am/system/singleton.h"

#include <atomic>
#include <any>
#include <mutex>

namespace rageam::integration
{
	/**
	 * \brief Provides interface for two update functions that will be called before (early) and after (late)
	 * updating game. Called by main thread and it's the only thread where you can safely invoke natives.
	 * \remarks Component must destruct all script resources manually using scrInvoke function in destructor.
	 */
	class IUpdateComponent
	{
		friend class ComponentManager;

		bool					m_Initialized = false;
		std::atomic_bool		m_AbortRequested = false;
		std::atomic_uint32_t	m_RefCount = 0;

	public:
		virtual ~IUpdateComponent() = default;

		virtual void OnStart() {}
		virtual void OnEarlyUpdate() {}
		virtual void OnLateUpdate() {}
		// ImGui (including Im3D, at the moment) can't be safely used on game update, this function has to be used instead
		// NOTE: UI update is not called when game is paused because it's mostly used to draw 3D text
		virtual void OnUiUpdate() {}
		// Must return true once all resources are released and instance is ready to be destructed
		virtual bool OnAbort() { return true; }
		// It takes some time to destroy component so all inner refs must be released in this function
		virtual void ReleaseAllRefs() {}

		// Aborted component will stop updating, current thread will wait stop of component execution
		void Abort();

		void AddRef() { ++m_RefCount; }
		u32 Release() { return --m_RefCount; }
	};

	class ComponentManager : public Singleton<ComponentManager>
	{
		rage::atArray<amUniquePtr<IUpdateComponent>> m_UpdateComponents;

		static std::recursive_mutex& GetLock();

		// Begins aborting of every existing component
		void AbortAll() const;
		// Whether there's no updating component left
		bool IsAllAborted() const;

	public:
		// Update & abort functions must be called from game thread
		void EarlyUpdateAll();
		void LateUpdateAll() const;
		void BeginAbortAll() const;
		void UIUpdateAll() const;

		template<typename T>
		T* CreateComponent()
		{
			std::unique_lock lock(GetLock());
			return (T*)m_UpdateComponents.Construct(new T()).get();
		}

		template<typename T>
		void DestroyComponent(T* component)
		{
			// We only mark component as no longer needed and not actually destroy it,
			// it will be destroyed at the beginning of next game frame
			component->Abort();
		}

		bool HasAnyComponent() const;
	};

	/**
	 * \brief Smart pointer for game components.
	 * \tparam TBase specifies base (interface) type, use Create<T> for derived classes.
	 * Or non-template Create overload if TBase is final.
	 */
	template<typename TBase>
	class ComponentOwner
	{
		TBase* m_Component = nullptr;

	public:
		ComponentOwner() = default;
		ComponentOwner(const ComponentOwner& other)
		{
			m_Component = other.m_Component;
			m_Component->AddRef();
		}
		ComponentOwner(ComponentOwner&&) = default;
		~ComponentOwner()
		{
			Release();
		}

		void Release()
		{
			if (m_Component && m_Component->Release() == 0)
			{
				ComponentManager::GetInstance()->DestroyComponent(m_Component);
				m_Component = nullptr;
			}
		}

		// Allows to create instance of derived class
		template<typename T>
		void Create()
		{
			Release();
			m_Component = ComponentManager::GetInstance()->CreateComponent<T>();
			m_Component->AddRef();
		}

		// Creates instance of the base class
		void Create() { Create<TBase>(); }

		const TBase* operator->() const { return m_Component; }
		TBase* operator->() { return m_Component; }
		operator bool() const { return m_Component != nullptr; }
		bool operator!() const { return !m_Component; }
		bool operator==(std::nullptr_t) const { return m_Component == nullptr; }
		ComponentOwner& operator=(std::nullptr_t) { Release(); return *this; }
		ComponentOwner& operator=(const ComponentOwner& other)
		{
			m_Component = other.m_Component;
			m_Component->AddRef();
			return *this;
		}
		ComponentOwner& operator=(ComponentOwner&&) = default;
	};
}
#endif
