//
// File: ptr.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <memory>
#include <wrl/client.h>

template<typename T>
class amComPtr : public Microsoft::WRL::ComPtr<T>
{
public:
	using TCom = Microsoft::WRL::ComPtr<T>;

	using TCom::TCom;
};

template<typename T>
using amPtr = std::shared_ptr<T>;

template<typename T>
using amUniquePtr = std::unique_ptr<T>;
