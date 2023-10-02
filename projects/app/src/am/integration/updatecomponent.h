//
// File: updatecomponents.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once
#include "rage/atl/array.h"

#ifdef AM_INTEGRATED

#include "rage/atl/fixedarray.h"
#include "am/system/ptr.h"

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

		bool				m_Initialized = false;
		std::atomic_bool	m_AbortRequested = false;

	public:
		virtual ~IUpdateComponent() = default;

		virtual void OnStart() {}
		virtual void OnEarlyUpdate() {}
		virtual void OnLateUpdate() {}
		// Must return true once all resources are released and instance is ready to be destructed
		virtual bool OnAbort() { return true; }

		// Aborted component will stop updating, current thread will wait stop of component execution
		void Abort();
	};

	class ComponentManager
	{
		static inline ComponentManager* sm_Instance = nullptr;

		rage::atArray<amUniquePtr<IUpdateComponent>> m_UpdateComponents;

		static std::mutex& GetLock();

		// Begins aborting of every existing component
		void AbortAll() const;
		// Whether there's no updating component left
		bool IsAllAborted() const;

	public:
		// Update & abort functions must be called from game thread
		void EarlyUpdateAll();
		void LateUpdateAll() const;
		void BeginAbortAll() const;

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

		static ComponentManager* GetCurrent();
		static void SetCurrent(ComponentManager* instance);
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
		~ComponentOwner()
		{
			Destroy();
		}

		void Destroy()
		{
			if (m_Component)
			{
				ComponentManager::GetCurrent()->DestroyComponent(m_Component);
				m_Component = nullptr;
			}
		}

		// Allows to create instance of derived class
		template<typename T>
		void Create()
		{
			Destroy();
			m_Component = ComponentManager::GetCurrent()->CreateComponent<T>();
		}

		// Creates instance of the base class
		void Create() { Create<TBase>(); }

		const TBase* operator->() const { return m_Component; }
		TBase* operator->() { return m_Component; }
		operator bool() const { return m_Component != nullptr; }
		bool operator!() const { return !m_Component; }
		bool operator==(std::nullptr_t) const { return m_Component == nullptr; }
		ComponentOwner& operator=(std::nullptr_t) { Destroy(); return *this; }
	};
}
#endif
