//
// File: protect_new.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//

// ReSharper disable CppClangTidyClangDiagnosticInlineNewDelete
// ReSharper disable CppClangTidyClangDiagnosticImplicitExceptionSpecMismatch
// ReSharper disable CppParameterNamesMismatch
// ReSharper disable CppClangTidyReadabilityRedundantDeclaration
#pragma once

#pragma warning( push )
#pragma warning( disable : 28251 ) // Inconsistent SAL annotation

// This is a small utility to detect overruns in allocated memory

//#define AM_USE_SYS_OVERRUN_DETECTION

#ifdef AM_USE_SYS_OVERRUN_DETECTION
void* operator new(size_t size, size_t align);
void* operator new (size_t size);

void* operator new[](size_t size);
void* operator new[](size_t size, size_t align);

void operator delete(void* block);
void operator delete[](void* block);
#endif

#pragma warning( pop ) 
