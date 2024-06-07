//
// File: propertystack.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "macro.h"

// This is a helper to easily push/pop variable in a class
#define IMPLEMENT_PROPERTY_STACK(type, name, default)					\
private:																\
	static inline type sm_Default##name = default;						\
	List<type> m_Stack##name;											\
public:																	\
	void Push##name(const type& var)									\
	{																	\
		m_Stack##name.Add(var);											\
	}																	\
	void Pop##name()													\
	{																	\
		AM_ASSERTS(m_Stack##name.Any(), "Stack mismatch in "##name);	\
		m_Stack##name.RemoveLast();										\
	}																	\
	const type& Get##name()												\
	{																	\
		if (!m_Stack##name.Any())										\
			return sm_Default##name;									\
		return m_Stack##name.Last();									\
	}																	\
	MACRO_END
