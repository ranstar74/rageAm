//
// File: enum.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "common/logger.h"
#include "asserts.h"

#include <magic_enum.hpp>

#define MAGIC_ENUM_RANGE_MIN (-128)
#define MAGIC_ENUM_RANGE_MAX (256)

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
		static constexpr bool TryParse(ConstString value, TEnum& outValue)
		{
			auto enumValue = magic_enum::enum_cast<TEnum>(value);
			if (!enumValue.has_value())
				return false;
			outValue = enumValue.value();
			return true;
		}

		template<typename TEnum>
		static constexpr TEnum Parse(ConstString value)
		{
			TEnum outValue;
			if (!TryParse(value, outValue))
			{
				AM_UNREACHABLE("Enum::Parse() -> Enum %s has no value %s", GetName<TEnum>(), value);
			}
			return outValue;
		}

		template<typename TEnum>
		static constexpr ConstString GetName()
		{
			return magic_enum::enum_type_name<TEnum>().data();
		}

		template<typename TEnum>
		static constexpr ConstString GetName(TEnum value)
		{
			return magic_enum::enum_name(value).data();
		}

		template<typename TEnum>
		static constexpr int GetCount()
		{
			return magic_enum::enum_count<TEnum>();
		}

		// NOTE: Returned string is valid until next call in current thread!
		// NOTE: There must be ToString function implemented for enum!
		template<typename TEnum>
		static ConstString FlagsToString(TEnum value, int prefixSkip = 0)
		{
			using TEnumValue = std::underlying_type_t<TEnum>;
			constexpr int ENUM_VALUE_BITS = sizeof(TEnumValue) * 8;
			constexpr int BUFFER_SIZE = 2048;
			thread_local char tl_Buffer[BUFFER_SIZE];
			int cursor = 0;
			for (int bit = 0; bit < ENUM_VALUE_BITS; bit++)
			{
				int mask = value & (1 << bit);
				if (mask == 0) // Bit is not set
					continue;

				ConstString name = ToString((TEnum) mask);
				AM_ASSERT(name, "Enum::FlagsToString() -> Enum '%s' has no name for bit '%i'.", GetName<TEnum>(), bit);

				// Add space before appending new name
				if (cursor > 0)
					tl_Buffer[cursor++] = ' ';

				// Append name to buffer
				name += prefixSkip;
				while (*name)
				{
					tl_Buffer[cursor++] = *name;
					++name;
				}
			}

			tl_Buffer[cursor++] = '\0';

			AM_ASSERTS(cursor < BUFFER_SIZE, "Enum::FlagsToString() -> Buffer overflow...");

			return tl_Buffer;
		}

		// NOTE: There must be FromString function implemented for enum!
		template<typename TEnum, typename TValue>
		static void ParseFlags(ConstString str, TValue& outValue, ConstString prefix = "")
		{
			outValue = 0;
			while (*str)
			{
				char* tokenEnd = (char*) strchr(str, ' ');
				if (!tokenEnd) // Last token
					tokenEnd = (char*) (str + strlen(str));

				// Copy token to buffer + append prefix
				*tokenEnd = '\0'; // Trick to make sprintf detect end of token
				char buffer[128];
				sprintf_s(buffer, 128, "%s%s", prefix, str);
				*tokenEnd = ' ';

				TEnum value;
				if (FromString(buffer, value))
				{
					outValue |= value;
				}
				else
				{
					AM_ERRF("Enum::ParseFlags() -> Invalid flag '%s' in enum '%s'", buffer, Enum::GetName<TEnum>());
				}

				str = tokenEnd + 1;
			}
		}
	};
}

#define IMPLEMENT_FLAGS_TO_STRING(enumName, prefix)												\
	inline ConstString FlagsToString_ ##enumName(std::underlying_type_t<enumName> value)				\
	{																							\
		return rageam::Enum::FlagsToString<enumName>((enumName) value, cstr::strlen(prefix));   \
	}																							\
	inline std::underlying_type_t<enumName> StringToFlags_##enumName(ConstString str)					\
	{																							\
		std::underlying_type_t<enumName> result = 0;											\
		rageam::Enum::ParseFlags<enumName>(str, result, prefix);								\
		return result;																			\
	} MACRO_END
