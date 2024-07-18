#include "node.h"

#define VERIFY_HANDLE() if (!m_Element) return XmlHandle(nullptr)

ConstString XmlHandle::FormatTemp(ConstString fmt, ...)
{
	static char buffer[256];
	va_list args;
	va_start(args, fmt);
	vsprintf_s(buffer, sizeof buffer, fmt, args);
	va_end(args);
	return buffer;
}

void XmlHandle::Assert(bool expr, ConstString msg, ...) const
{
	if (expr) return;

	static char buffer[256];
	va_list args;
	va_start(args, msg);
	vsprintf_s(buffer, sizeof buffer, msg, args);
	va_end(args);

	throw XmlException(msg, m_LastElementLine);
}

void XmlHandle::AssetHandle() const
{
	AM_ASSERT(m_Element, "XmlHandle -> Element was NULL.");
}

ConstString XmlHandle::GetCurrentElementName() const
{
	if (m_Element) return m_Element->Name();
	if (m_LastElement) return m_LastElement->Name();
	return "Unknown";
}

XmlHandle::XmlHandle() : m_Element(nullptr)
{

}

XmlHandle::XmlHandle(TinyElement element) : m_Element(element)
{
	if (element)
	{
		m_LastElement = element;
		m_LastElementLine = element->GetLineNum();
	}
}

XmlHandle::XmlHandle(const XmlHandle& other)
{
	m_Element = other.m_Element;
}

void XmlHandle::RemoveIfEmpty(const ConstString* ignoreAtts, int ignoreAttsCount)
{
	int attrCount = 0;
	auto attr = m_Element->FirstAttribute();
	while (attr)
	{
		ConstString attrName = attr->Name();
		attr = attr->Next();

		// Check if attribute must be ignored
		bool ignore = false;
		for (int i = 0; i < ignoreAttsCount; i++)
		{
			if (String::Equals(attrName, ignoreAtts[i]))
			{
				ignore = true;
				break;
			}
		}

		if (!ignore)
			attrCount++;
	}

	if (m_Element->FirstChild() || attrCount > 0)
		return;

	m_Element->Parent()->DeleteChild(m_Element);
	m_Element = nullptr;
}

u32 XmlHandle::GetChildCount(ConstString name) const
{
	u32 count = 0;
	XmlHandle child = GetChild(name);
	while (!child.IsNull())
	{
		count++;
		child = child.Next(name);
	}
	return count;
}

XmlHandle XmlHandle::GetChild(ConstString name, bool errorOnNull) const
{
	VERIFY_HANDLE();
	XmlHandle element = m_Element->FirstChildElement(name);
	if (element.IsNull() && errorOnNull)
		throw XmlException(FormatTemp("Required element '%s' is missing", name), m_LastElementLine);

	return element;
}

XmlHandle XmlHandle::Next(ConstString name) const
{
	VERIFY_HANDLE();
	return m_Element->NextSiblingElement(name);
}

ConstString XmlHandle::GetText(bool errorOnNull) const
{
	if (!errorOnNull && IsNull())
		return nullptr;

	AssetHandle();

	ConstString text = m_Element->GetText();
	if (!errorOnNull)
		return text ? text : "";

	if (text && strcmp(text, "") != 0) // Not Null & Not Empty
		return text;

	throw XmlException(
		FormatTemp("'%s' must have value.", GetCurrentElementName()),
		m_LastElementLine);
}

ConstString XmlHandle::GetName() const
{
	AssetHandle();
	return m_Element->Name();
}

void XmlHandle::SetName(ConstString name) const
{
	AssetHandle();
	m_Element->SetName(name);
}

void XmlHandle::SetText(ConstString text) const
{
	if (!text) text = ""; // We allow empty strings
	return m_Element->SetText(text);
}

void XmlHandle::AddComment(ConstString text) const
{
	AssetHandle();
	m_Element->InsertNewComment(text);
}

XmlHandle XmlHandle::AddChild(ConstString name) const
{
	VERIFY_HANDLE();
	return m_Element->InsertNewChildElement(name);
}

void XmlHandle::AddChild(ConstString name, ConstString value) const
{
	m_Element->InsertNewChildElement(name)->SetText(value);
}
