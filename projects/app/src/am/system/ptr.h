//
// File: ptr.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <memory>
#include "helpers/com.h"

template<typename T>
class amComPtr
{
	T* m_Ptr = nullptr;
public:
	amComPtr() = default;

	amComPtr(T* t)
	{
		m_Ptr = t;
		SAFE_ADDREF(m_Ptr);
	}

	amComPtr(const amComPtr& other)
	{
		SAFE_RELEASE(m_Ptr);
		m_Ptr = other.m_Ptr;
		SAFE_ADDREF(m_Ptr);
	}

	amComPtr(amComPtr&& other) noexcept
	{
		std::swap(m_Ptr, other.m_Ptr);
	}

	~amComPtr()
	{
		Reset();
	}

	void Reset()
	{
		SAFE_RELEASE(m_Ptr);
		m_Ptr = nullptr;
	}

	T* Get() const { return m_Ptr; }

	amComPtr& operator=(const amComPtr& other)
	{
		SAFE_RELEASE(m_Ptr);
		m_Ptr = other.m_Ptr;
		SAFE_ADDREF(m_Ptr);
		return *this;
	}

	amComPtr& operator=(amComPtr&& other) noexcept
	{
		std::swap(m_Ptr, other.m_Ptr);
		return *this;
	}

	bool operator!() const { return !m_Ptr; }
	operator bool() const { return m_Ptr; }
};

template<typename T>
using amPtr = std::shared_ptr<T>;

template<typename T>
using amUniquePtr = std::unique_ptr<T>;
