#pragma once

#include <exception>

#include "common/logger.h"
#include "common/types.h"

class XmlException : public std::exception
{
	int m_Line;
public:
	XmlException(ConstString msg, int line) noexcept : std::exception(msg)
	{
		m_Line = line;
	}

	int GetLine() const { return m_Line; }

	// Logs this xml exception as error
	void Print() const
	{
		AM_ERRF("An error occurred while parsing XML at line %i - %s", m_Line, what());
	}
};
