//
// File: timer.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <chrono>

#include "common/types.h"

namespace rageam
{
	class Timer
	{
		using TClock = std::chrono::steady_clock;
		using TTime = std::chrono::time_point<TClock>;

		TTime m_StartTime;
		TTime m_EndTime;

		bool m_IsRunning = false;
	public:
		Timer()
		{
			Reset();
		}

		static Timer StartNew()
		{
			Timer result;
			result.Start();
			return result;
		}

		void Start()
		{
			m_StartTime = TClock::now();
			m_IsRunning = true;
		}

		void Stop()
		{
			if (!m_IsRunning)
				return;

			m_EndTime = TClock::now();
			m_IsRunning = false;
		}

		void Restart()
		{
			Reset();
			Start();
		}

		void Reset()
		{
			TTime time = TClock::now();
			m_StartTime = time;
			m_EndTime = time;
		}

		bool GetIsRunning() const { return m_IsRunning; }

		u64 GetElapsedTicks() const
		{
			return (m_EndTime - m_StartTime).count();
		}

		u64 GetElapsedMilliseconds() const
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(m_EndTime - m_StartTime).count();
		}

		u64 GetElapsedMicroseconds() const
		{
			return std::chrono::duration_cast<std::chrono::microseconds>(m_EndTime - m_StartTime).count();
		}
	};
}
