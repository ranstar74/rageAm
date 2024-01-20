//
// File: updatecomponents.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "am/system/singleton.h"
#include "am/system/ptr.h"
#include "am/types.h"

namespace rageam::integration
{
	/**
	 * \brief Similar to unity component system, provides interface with two update functions.
	 * \n Use ComponentManager::GetInstance()->Create<T>() templated factory
	 */
	class IUpdateComponent
	{
		friend class ComponentManager;

		bool m_Started = false;
		bool m_Aborted = false;
		int  m_RefCount = 0;

	protected:
		// Make constructor private, ComponentOwner::Create must be used instead
		IUpdateComponent() = default;

	public:
		virtual ~IUpdateComponent() = default;

		virtual void OnStart() {}
		virtual void OnEarlyUpdate() {}
		virtual void OnLateUpdate() {}

		void AddRef() { ++m_RefCount; }
		int  Release() { return --m_RefCount; }
	};

	class ComponentManager : public Singleton<ComponentManager>
	{
		SmallList<IUpdateComponent*> m_UpdateComponents;
		SmallList<IUpdateComponent*> m_UpdateComponentsToAdd;

	public:
		void EarlyUpdateAll();
		void LateUpdateAll() const;

		bool HasAnythingToUpdate() const { return m_UpdateComponents.Any() || m_UpdateComponentsToAdd.Any(); }

		// Pointer must be allocated via operator new
		void Add(IUpdateComponent* comp) { m_UpdateComponentsToAdd.Add(comp); }
		// Pointer will be freed up automatically
		void AbortAndDelete(IUpdateComponent* comp) const { comp->m_Aborted = true; }
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
				ComponentManager::GetInstance()->AbortAndDelete(m_Component);
				m_Component = nullptr;
			}
		}

		// Creates instance of the base class
		template<typename ...Args>
		void Create(Args... args) { Create<TBase>(args...); }
		// Allows to create instance of derived class
		template<typename T, typename ...Args>
		void Create(Args... args)
		{
			Release();
			m_Component = new T(args...);
			m_Component->AddRef();
			ComponentManager::GetInstance()->Add(m_Component);
		}

		const TBase* operator->() const { AM_ASSERTS(m_Component); return m_Component; }
		TBase* operator->() { AM_ASSERTS(m_Component); return m_Component; }
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
