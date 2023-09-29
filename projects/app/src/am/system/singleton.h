//
// File: singleton.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

namespace rageam
{
	/**
	 * \brief Class with single static instance.
	 */
	template<typename T>
	class Singleton
	{
	protected:
		Singleton() {}
	public:
		Singleton(const Singleton& other) = delete;

		static T* GetInstance()
		{
			static T instance;
			return &instance;
		}
	};
}
