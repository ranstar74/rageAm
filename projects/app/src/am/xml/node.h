// ReSharper disable CppClangTidyBugproneMacroParentheses
// ReSharper disable CppClangTidyClangDiagnosticDoublePromotion
#pragma once

#include <any>

#include "exception.h"
#include "textconversion.h"
#include "types.h"
#include "helpers/macro.h"
#include "am/system/enum.h"
#include "rage/atl/string.h"
#include "rage/math/vec.h"
#include "am/types.h"

#define XML_ATTRIBUTE_VALUE "Value"

/**
 * \brief A handle of element that may be NULL.
 */
class XmlHandle
{
	// Last non-null element in current thread, for error handling
	static inline thread_local TinyElement m_LastElement;
	static inline thread_local int m_LastElementLine = -1;

	TinyElement m_Element;

	static ConstString FormatTemp(ConstString fmt, ...);

	void Assert(bool expr, ConstString msg, ...) const;

	// Ensures that element is not NULL
	void AssetHandle() const;

	// Gets name of current element if not NULL or name of last element that was not null (if any)
	// Using just m_LastElement might be not accurate because of creation order
	ConstString GetCurrentElementName() const;

	template<typename T>
	bool GetAttribute_Internal(ConstString name, T& value, bool allowNull) const
	{
		if (!IsNull())
		{
			ConstString attribute = m_Element->Attribute(name);
			if (!String::IsNullOrEmpty(attribute))
				return FromString<T>(attribute, value);
		}

		if (allowNull)
			return false;

		throw XmlException(
			FormatTemp("Failed to get required attribute '%s' in '%s'", name, GetCurrentElementName()),
			m_LastElementLine);
	}

	template<typename T>
	void SetAttribute_Internal(ConstString name, T value)
	{
		AssetHandle();
		m_Element->SetAttribute(name, ToString<T>(value));
	}

public:
	XmlHandle();
	XmlHandle(TinyElement element);
	XmlHandle(const XmlHandle& other);
	bool IsNull() const { return m_Element == nullptr; }
	TinyElement Get() const { return m_Element; }

	void RemoveIfEmpty(const ConstString* ignoreAtts, int ignoreAttsCount);
	void RemoveIfEmpty_IgnoreName()
	{
		ConstString s = "Name";
		RemoveIfEmpty(&s, 1);
	}

	u32 GetChildCount(ConstString name = nullptr) const;

	// Gets the first child with given name
	// <Element>
	//     <Child />		<!-- Returned if name is NULL or 'Child' -->
	//     <Child />
	// </Element>
	XmlHandle GetChild(ConstString name = nullptr, bool errorOnNull = false) const;
	void GetChildText(ConstString name, ConstString* outText, bool errorOnNull = false) const
	{
		XmlHandle child = GetChild(name, errorOnNull);
		*outText = child.GetText(errorOnNull);
	}

	// Gets the next sibling element
	// <Element />			<!-- Current element -->
	// <SiblingElement />	<!-- Returned -->
	XmlHandle Next(ConstString name = nullptr) const;

	// Gets element inner text. Element must be not null!
	// <Element>Inner Text</Element>
	ConstString GetText(bool errorOnNull = true) const;

	// Gets name of the element
	// <Name />
	ConstString GetName() const;

	// Sets name of the element
	// <Name />
	void SetName(ConstString name) const;

	// Sets inner text of current element, may be NULL
	// <Element>Inner Text</Element>
	void SetText(ConstString text) const;

	// Inserts comment as last element child
	// <Element>
	//     <Child />
	//     <!-- Here goes your comment -->
	// </Element>
	void AddComment(ConstString text) const;

	// Inserts a new child in the end
	// <Element>		<!-- Current element -->
	//     <Child />
	//     <Name />		<!-- Added child -->
	// </Element>
	XmlHandle AddChild(ConstString name) const;

	// Inserts a new child in the end with given inner text
	// <Element>					<!-- Current element -->
	//     <Child />
	//     <Name>Inner Text</Name>  <!-- Added child -->
	// </Element>
	void AddChild(ConstString name, ConstString value) const;

	// We keep template hell away from user because it would mess with implicit cast operators

#define IMPL_ATTR(type) \
	bool GetAttribute(ConstString name, type& value, bool allowNull = false) const { return GetAttribute_Internal(name, value, allowNull); } \
	void SetAttribute(ConstString name, type value) { SetAttribute_Internal(name, value); } \
	bool GetTheValueAttribute(type& value, bool allowNull = false) const { return GetAttribute_Internal(XML_ATTRIBUTE_VALUE, value, allowNull); } \
	void SetTheValueAttribute(type value) { SetAttribute_Internal(XML_ATTRIBUTE_VALUE, value); } \
	MACRO_END

	IMPL_ATTR(ConstString);
	IMPL_ATTR(u8);
	IMPL_ATTR(s8);
	IMPL_ATTR(u16);
	IMPL_ATTR(s16);
	IMPL_ATTR(u32);
	IMPL_ATTR(s32);
	IMPL_ATTR(u64);
	IMPL_ATTR(s64);
	IMPL_ATTR(float);
	IMPL_ATTR(double);
	IMPL_ATTR(bool);
#undef IMPL_ATTR

	// We can implicit cast atString -> const char* but not opposite, same added below for GetText

	void GetAttribute(ConstString name, rage::atString& value, bool allowNull = false) const
	{
		ConstString str;
		GetAttribute(name, str, allowNull);
		value = str;
	}

	void GetTheValueAttribute(rage::atString& value, bool allowNull = false) const
	{
		ConstString str;
		GetTheValueAttribute(str, allowNull);
		value = str;
	}

	void GetTheValueAttribute(rage::atWideString& value, bool allowNull = false) const
	{
		ConstString text;
		GetTheValueAttribute(text, allowNull);
		thread_local wchar_t buffer[512];
		String::Utf8ToWide(buffer, 512, text);
		value = buffer;
	}

	// Template implementation for enums

	template<typename T> requires std::is_enum_v<T>
	void SetAttribute(ConstString name, T value)
	{
		SetAttribute(name, rageam::Enum::GetName(value));
	}

	template<typename T> requires std::is_enum_v<T>
	bool GetAttribute(ConstString name, T& value, bool allowNull = false) const
	{
		ConstString enumStr;
		if (!GetAttribute(name, enumStr, allowNull))
		{
			if (allowNull)
				return false;

			throw XmlException(FormatTemp("'%s' value must be specified", name), m_LastElementLine);
		}

		if (!rageam::Enum::TryParse(enumStr, value))
		{
			throw XmlException(FormatTemp("'%s' has invalid value '%s'", name, enumStr), m_LastElementLine);
		}
		return true;
	}

	template<typename T> requires std::is_enum_v<T>
	bool GetTheValueAttribute(T& value, bool allowNull = false)
	{
		return GetAttribute(XML_ATTRIBUTE_VALUE, value, allowNull);
	}

	template<typename T> requires std::is_enum_v<T>
	T GetTheValueAttribute()
	{
		T value;
		GetAttribute(XML_ATTRIBUTE_VALUE, value);
		return value;
	}

	template<typename T> requires std::is_enum_v<T>
	void SetTheValueAttribute(T value)
	{
		SetAttribute(XML_ATTRIBUTE_VALUE, value);
	}

	void SetTheValueAttribute(ConstWString value)
	{
		thread_local char buffer[512];
		String::WideToUtf8(buffer, 512, value);
		SetAttribute(XML_ATTRIBUTE_VALUE, buffer);
	}

#define XML_VEC2_FMT "%g %g"
#define XML_VEC3_FMT "%g %g %g"
#define XML_VEC4_FMT "%g %g %g %g"

	void SetTheValueAttribute(const rage::spdAABB& value)
	{
		const rage::Vec3V& min = value.Min;
		const rage::Vec3V& max = value.Max;
		SetAttribute("Min", FormatTemp(XML_VEC3_FMT, min.X(), min.Y(), min.Z()));
		SetAttribute("Max", FormatTemp(XML_VEC3_FMT, max.X(), max.Y(), max.Z()));
	}

	void SetTheValueAttribute(const rage::spdSphere& value)
	{
		const rage::Vec3V& center = value.GetCenter();
		const rage::ScalarV& radius = value.GetRadius();
		SetAttribute("Center", FormatTemp(XML_VEC3_FMT, center.X(), center.Y(), center.Z()));
		SetAttribute("Radius", FormatTemp("%g", radius.Get()));
	}

	void GetTheValueAttribute(rage::spdAABB& value, bool allowNull = false) const
	{
		bool ok = true;
		ConstString min, max;
		if (!GetAttribute("Min", min)) ok = false;
		if (!GetAttribute("Max", max)) ok = false;
		if (!ok)
		{
			if (allowNull)
				return;
			throw XmlException(FormatTemp("Min or Max is not specified in bounding box"), m_LastElementLine);
		}
		rage::Vector3 mins, maxs;
		Assert(sscanf_s(GetText(), XML_VEC3_FMT, &mins.X, &mins.Y, &mins.Z) != 0, "Unable to parse AABB Min");
		Assert(sscanf_s(GetText(), XML_VEC3_FMT, &maxs.X, &maxs.Y, &maxs.Z) != 0, "Unable to parse AABB Max");
		value = { mins, maxs };
	}

	void GetTheValueAttribute(rage::spdSphere& value, bool allowNull = false) const
	{
		bool ok = true;
		ConstString center, radius;
		if (!GetAttribute("Center", center)) ok = false;
		if (!GetAttribute("Radius", radius)) ok = false;
		if (!ok)
		{
			if (allowNull)
				return;
			throw XmlException(FormatTemp("Center or Radius is not specified in bounding sphere"), m_LastElementLine);
		}
		rage::Vector3 centers;
		float radiusf;
		Assert(sscanf_s(GetText(), XML_VEC3_FMT, &centers.X, &centers.Y, &centers.Z) != 0, "Unable to parse Sphere Center");
		Assert(sscanf_s(GetText(), "%g", &radiusf) != 0, "Unable to parse Sphere Radius");
		value = { centers, radiusf };
	}

	// Utils for inner text with other types

	void SetValue(ConstString v) const { SetText(v); }
	void SetValue(const rage::Vector2& v) const { SetText(FormatTemp(XML_VEC2_FMT, v.X, v.Y)); }
	void SetValue(const rage::Vector3& v) const { SetText(FormatTemp(XML_VEC3_FMT, v.X, v.Y, v.Z)); }
	void SetValue(const rage::Vector4& v) const { SetText(FormatTemp(XML_VEC4_FMT, v.X, v.Y, v.Z, v.W)); }
	void SetValue(int v) const { SetText(ToString(v)); }
	void SetValue(bool v) const { SetText(ToString(v)); }
	void SetValue(float v) const { SetText(ToString(v)); }
	void SetValue(double v) const { SetText(ToString(v)); }

	void GetValue(ConstString& v) const { v = GetText(); }
	void GetValue(rage::Vector2& v) const { Assert(sscanf_s(GetText(), XML_VEC2_FMT, &v.X, &v.Y) != 0, "Unable to parse Vector2"); }
	void GetValue(rage::Vector3& v) const { Assert(sscanf_s(GetText(), XML_VEC3_FMT, &v.X, &v.Y, &v.Z) != 0, "Unable to parse Vector3"); }
	void GetValue(rage::Vector4& v) const { Assert(sscanf_s(GetText(), XML_VEC4_FMT, &v.X, &v.Y, &v.Z, &v.W) != 0, "Unable to parse Vector4"); }
	void GetValue(int& v) const { FromString(GetText(), v); }
	void GetValue(float& v) const { FromString(GetText(), v); }
	void GetValue(double& v) const { FromString(GetText(), v); }
	void GetValue(bool& v) const { FromString(GetText(), v); }
	void GetValue(rage::atString& v) const { v = GetText(); }
	void GetValue(rage::atWideString& v) const
	{
		thread_local wchar_t buffer[512];
		ConstString text = GetText();
		String::Utf8ToWide(buffer, 512, text);
		v = buffer;
	}

	void SetColorHex(u32 col) const { SetText(FormatTemp("#%X", col)); }
	void GetColorHex(u32& col) const
	{
		ConstString text = GetText();
		Assert(sscanf_s(text, "#%X", &col) != 0, "Unable to parse hex color '%s', it must be in #000000 format.", text);
	}
	u32 GetColorHex() const { u32 v; GetColorHex(v); return v; }

	bool operator==(const XmlHandle& other) const { return m_Element == other.m_Element; }
	bool operator!() const { return !m_Element; }
};
