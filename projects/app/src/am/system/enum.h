//
// File: enum.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "asserts.h"

#define MAGIC_ENUM_RANGE_MIN (-128)
#define MAGIC_ENUM_RANGE_MAX (256)

#include "magic_enum.hpp"

namespace rageam
{
	/**
	 * \brief Compile-time reflection for enumerations.
	 * \remarks Note that this only supports value ranges from -128 to 256 and has to be extended if more is needed.
	 */
	class Enum
	{
	public:
		template<typename TEnum>
		static bool TryParse(ConstString value, TEnum& outValue)
		{
			auto enumValue = magic_enum::enum_cast<TEnum>(value);
			if (!enumValue.has_value())
				return false;
			outValue = enumValue.value();
			return true;
		}

		template<typename TEnum>
		static TEnum Parse(ConstString value)
		{
			TEnum outValue;
			if (!TryParse(value, outValue))
			{
				AM_UNREACHABLE("Enum::Parse() -> Enum %s has no value %s", GetName<TEnum>(), value);
			}
			return outValue;
		}

		template<typename TEnum>
		static ConstString GetName()
		{
			return magic_enum::enum_type_name<TEnum>().data();
		}

		template<typename TEnum>
		static ConstString GetName(TEnum value)
		{
			return magic_enum::enum_name(value).data();
		}
	};
}
