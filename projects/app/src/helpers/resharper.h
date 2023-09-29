//
// File: resharper.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

// Enables re-sharper highlight for printf
// https://youtrack.jetbrains.com/issue/RSCPP-15890

// 5030 : Attribute 'rscpp::format' is not recognized

// NOTE: On member functions hidden parameter 'this' is accounted!

#define PRINTF_ATTR(StringIndex, FirstToCheck) \
		_Pragma ("warning( push )") \
		_Pragma ("warning( disable : 5030)") \
        [[rscpp::format(printf, StringIndex, FirstToCheck)]] \
		_Pragma ("warning(pop)")

#define WPRINTF_ATTR(StringIndex, FirstToCheck) \
		_Pragma ("warning( push )") \
		_Pragma ("warning( disable : 5030)") \
        [[rscpp::format(wprintf, StringIndex, FirstToCheck)]] \
		_Pragma ("warning(pop)")
