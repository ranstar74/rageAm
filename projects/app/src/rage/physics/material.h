//
// File: material.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_STANDALONE
#include "am/system/asserts.h"
#else
#include "am/integration/memory/address.h"
#endif

namespace rage
{
	// Placeholder, for now

	struct phMaterial { };

	class phMaterialMgr
	{
	public:
		static phMaterialMgr* GetInstance()
		{
#ifndef AM_STANDALONE
			static phMaterialMgr* inst = gmAddress::Scan("48 8B 15 ?? ?? ?? ?? 4C 23 42 10")
				.GetRef(3)
				.To<decltype(inst)>();
			return inst;
#else
			AM_UNREACHABLE("phMaterialMgr::GetInstance() -> Unsupported in standalone.");
#endif
		}

		phMaterial* GetDefaultMaterial() const
		{
#ifndef AM_STANDALONE
			static auto fn = gmAddress::Scan("40 53 48 83 EC 20 48 8B 01 48 8D 15").ToFunc<phMaterial * (const phMaterialMgr*)>();
			return fn(this);
#else
			AM_UNREACHABLE("phMaterialMgr::GetDefaultMaterial() -> Unsupported in standalone.");
#endif
		}
	};
}
