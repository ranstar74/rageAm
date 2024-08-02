//
// File: dispatcher.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/types.h"

#include <mutex>
#include <future>

namespace rageam
{
	/**
	 * \brief Class that allows to execute tasks on particular thread.
	 * Kind of thread pool, but with just one thread.
	 * Similar concept to Dispatcher in WPF/Avalonia, but without any priority management.
	 */
	class Dispatcher
	{
		std::mutex                  m_Mutex;
		List<std::function<void()>> m_Tasks;
	public:
		Dispatcher() = default;
		Dispatcher(const Dispatcher&) = delete;

		// Must be called from main dispatcher thread, for e.g. update or render thread
		void ExecuteTasks()
		{
			std::unique_lock lock(m_Mutex);
			for (auto task : m_Tasks)
				task();
			m_Tasks.Clear();
		}

		template<typename F, typename R = std::invoke_result_t<std::decay_t<F>>>
		[[nodiscard]] std::future<R> RunAsync(F&& task)
		{
			std::unique_lock lock(m_Mutex);
			std::shared_ptr<std::promise<R>> promise = std::make_shared<std::promise<R>>();
			m_Tasks.Emplace([task = std::forward<F>(task), promise]
			{
				try
				{
					if constexpr (std::is_void_v<R>)
					{
						task();
						promise->set_value();
					}
					else
					{
						promise->set_value(task());
					}
				}
				catch (...)
				{
					promise->set_exception(std::current_exception());
				}
			});
			return promise->get_future();
		}

		// Returns dispatcher that runs in main update thread
		static Dispatcher& UpdateThread()
		{
			static Dispatcher s_Dispatcher;
			return s_Dispatcher;
		}
	};
}
