//
// File: event.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <any>
#include <functional>

#include "am/system/ptr.h"
#include "rage/atl/array.h"

namespace rageam
{
	/**
	 * \brief Wraps std::function with template invoke function.
	 */
	class EventHandler
	{
		// To prevent std::function template hell and pass it anywhere we use std::any
		std::any m_FunctionHolder;

	public:
		template<typename... TArgs>
		EventHandler(std::function<void(TArgs...)> fn)
		{
			m_FunctionHolder = fn;
		}

		template<typename... TArgs>
		void Invoke(TArgs... args)
		{
			std::function<void(TArgs...)> fn = std::any_cast<decltype(fn)>(m_FunctionHolder);
			fn(args...);
		}
	};

	/**
	 * \brief We have to keep track of event and consumer object's life time.
	 * If either of them get destructed, we have to disconnect them to prevent accessing into destructed object.
	 */
	class EventConnection
	{
		EventHandler m_Handler;
		bool m_Connected = true;

	public:
		EventConnection(EventHandler handler) : m_Handler(std::move(handler)) {}

		void Disconnect() { m_Connected = false; }
		bool IsConnected() const { return m_Connected; }

		EventHandler& GetHandler() { return m_Handler; }
	};
	using EventConnections = rage::atArray<amPtr<EventConnection>>;

	/**
	 * \brief Holds event connections and disconnects them on object destruction.
	 */
	class EventAwareBase
	{
	protected:

		EventConnections m_Connections;
	public:
		EventAwareBase() = default;

		void AddConnection(const amPtr<EventConnection>& connection)
		{
			m_Connections.Add(connection);
		}

		~EventAwareBase()
		{
			// We can't really put this into EventConnection destructor
			// because connection is shared between Event & Consumer and destructor
			// will be called only when they both are destroyed...

			for (amPtr<EventConnection>& connection : m_Connections)
			{
				// Now there's no connection between event and consumer
				connection->Disconnect();
			}
		}
	};

	/**
	 * \brief C# Style event.
	 * \tparam TRet		Return type of event handler.
	 * \tparam TArgs	Argument list of event handler.
	 */
	template<typename TRet, typename... TArgs>
	class Event : public EventAwareBase
	{
	public:
		void Invoke(TArgs... args)
		{
			rage::atArray<u16> connectionsToRemove;
			for (u16 i = 0; i < m_Connections.GetSize(); i++)
			{
				amPtr<EventConnection>& connection = m_Connections[i];
				if (!connection->IsConnected())
				{
					// Connected object was destructed, add it in garbage collection list
					connectionsToRemove.Add(i);
					continue;
				}

				connection->GetHandler().Invoke(args...);
			}

			// Clean up broken connections
			for (u16 index : connectionsToRemove)
			{
				m_Connections.RemoveAt(index);
			}
		}

		void Listen(EventAwareBase* consumer, std::function<TRet(TArgs...)> fn)
		{
			EventHandler handler(fn);

			auto connection = std::make_shared<EventConnection>(handler);

			this->AddConnection(connection);
			consumer->AddConnection(connection);
		}
	};

	// Event without parameters and return value.
	using SimpleEvent = Event<void>;

	// Event with parameters without return value.
	template<typename... TArgs>
	using ActionEvent = Event<void, TArgs...>;

	// Event with parameters and bool return value.
	template<typename... TArgs>
	using PredicateEvent = Event<bool, TArgs...>;

	// Shortcut for adding new event handler.
#define AM_LISTEN(event, handler) event.Listen(this, handler)
}
