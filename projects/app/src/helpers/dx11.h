//
// File: dx11.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/asserts.h"
#include "am/string/string.h"

#include <d3d11.h>

PRINTF_ATTR(2, 3) inline void SetObjectDebugName(ID3D11DeviceChild* child, ConstString fmt, ...)
{
	char name[256];
	va_list args;
	va_start(args, fmt);
	String::FormatVA(name, sizeof name, fmt, args);
	va_end(args);
	AM_ASSERT_STATUS(child->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(name), name));
}
