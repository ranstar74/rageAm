#pragma once

#include "node.h"
#include "common/types.h"

/**
 * \brief Iterates XML node children.
 */
class XmlIterator
{
	XmlHandle	m_Node;
	ConstString m_Name;

public:
	XmlIterator(XmlHandle node, ConstString childName = nullptr)
	{
		m_Node = node.GetChild(); // Take first child element
		m_Name = childName;
	}

	XmlIterator(const XmlIterator& other) = default;

	XmlHandle operator*() const { return m_Node; }
	XmlIterator operator++() { m_Node = m_Node.Next(m_Name); return *this; }
	XmlIterator operator++(int) const { return m_Node.Next(m_Name); }
	bool operator==(const XmlIterator& other) const { return m_Node == other.m_Node; }

	XmlIterator begin() const { return *this; }
	XmlIterator end() const { return XmlIterator(nullptr); }
};
