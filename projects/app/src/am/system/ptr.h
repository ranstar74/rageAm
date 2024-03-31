//
// File: ptr.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "helpers/com.h"

#include <wrl.h>
#include <memory>

template<typename T>
using amPtr = std::shared_ptr<T>;

template<typename T>
using amUniquePtr = std::unique_ptr<T>;

template<typename T>
using amUPtr = std::unique_ptr<T>;

template<typename T>
using amWeakPtr = std::weak_ptr<T>;

template<typename T>
using amComPtr = Microsoft::WRL::ComPtr<T>;
