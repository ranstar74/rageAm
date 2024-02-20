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
	enum UpdateStage
	{
		UpdateStage_Undefined,
		UpdateStage_Early,
		UpdateStage_SafeArea,
		UpdateStage_Late,
	};

	/**
	 * \brief Similar to unity component system, provides interface with two update functions.
	 * \n Use ComponentManager::GetInstance()->Create<T>() templated factory
	 */
	class IUpdateComponent
	{
		friend class ComponentManager;

		bool m_Started = false;
		int  m_RefCount = 0;
		int  m_AbortAttempts = 0;

	protected:
		// Make constructor private, ComponentOwner::Create must be used instead
		IUpdateComponent() = default;

	public:
		virtual ~IUpdateComponent() = default;

		// For components that require multiple frames to unload all the resources (for example if draw list holds a reference)
		virtual bool OnAbort() { return true; }

		// Called only one time before OnEarlyUpdate
		virtual void OnStart() {}
		// Called in the beginning of the frame, parallel render thread is still might be active
		virtual void OnEarlyUpdate() {}
		// Called after rendering is done and before new draw lists are being built, render thread is paused
		virtual void OnUpdate() {}
		// Called at the end of the frame, parallel render thread might be active
		virtual void OnLateUpdate() {}

		void AddRef() { ++m_RefCount; }
		int  Release() { AM_ASSERT(m_RefCount > 0, "IUpdateComponent::Release() -> Called too much times!"); return --m_RefCount; }
	};

	class ComponentManager : public Singleton<ComponentManager>
	{
		SmallList<IUpdateComponent*> m_UpdateComponents;
		SmallList<IUpdateComponent*> m_UpdateComponentsToAdd;
		UpdateStage					 m_UpdateStage = UpdateStage_Undefined;

	public:
		void RemoveAbortedComponents();
		void EarlyUpdateAll();
		void UpdateAll();
		void LateUpdateAll();

		bool HasAnythingToUpdate() const { return m_UpdateComponents.Any() || m_UpdateComponentsToAdd.Any(); }

		// Pointer must be allocated via operator new
		void Add(IUpdateComponent* comp) { m_UpdateComponentsToAdd.Add(comp); }

		static UpdateStage GetUpdateStage() { return GetInstance()->m_UpdateStage; }
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
				// Pointer will be freed up in ComponentManager::RemoveAbortedComponents
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

		TBase* Get() const { return m_Component; }

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
