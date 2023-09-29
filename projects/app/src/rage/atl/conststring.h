#pragma once
#include <cstring>

#include "common/types.h"
#include "rage/paging/compiler/compiler.h"
#include "rage/paging/compiler/snapshotallocator.h"

namespace rage
{
	// NOTE:
	// Originally, apparently, this class is called 'ConstString' (not really part of angel template library).
	// But we have this name reserved for 'const char*' so uh...
	// Mostly used on resource classes

	/**
	 * \brief Mutable string with dynamic storage.
	 */
	class atConstString
	{
		char* m_String;

		void Set(const char* string)
		{
			if (string == nullptr)
			{
				delete[] m_String;
				m_String = nullptr;
				return;
			}

			u32 availableSize = 0;
			u32 stringSize = static_cast<u32>(strlen(string)) + 1; // Including NUL

			pgSnapshotAllocator* pAllocator = pgRscCompiler::GetVirtualAllocator();
			if (pAllocator) // Copy string during resource compilation
			{
				delete[] m_String;
				pAllocator->AllocateRef(m_String, stringSize);
			}
			else // Otherwise try to fit into existing buffer or allocate larger one
			{
				// Check if we can fit string in existing memory block
				if (m_String)
				{
					availableSize = static_cast<u32>(strlen(m_String)) + 1;
				}

				// Nope we can't, allocate larger block
				if (stringSize > availableSize)
				{
					delete[] m_String;
					m_String = new char[stringSize];
				}
			}

			memcpy(m_String, string, stringSize);
		}
	public:
		atConstString()
		{
			if (const datResource* rsc = datResource::GetCurrent())
			{
				rsc->Fixup(m_String);
			}
			else
			{
				m_String = nullptr;
			}
		}
		atConstString(const atConstString& other)
		{
			Set(other.m_String);
		}
		atConstString(const char* string) { Set(string); }
		~atConstString()
		{
			delete[] m_String;
			m_String = nullptr;
		}

		const char* GetCStr() const { return m_String; }

		operator const char* () const { return m_String; }

		atConstString& operator=(const atConstString& other)  // NOLINT(bugprone-unhandled-self-assignment)
		{
			Set(other.m_String);
			return *this;
		}
		atConstString& operator=(const char* string)
		{
			Set(string);
			return *this;
		}
	};
}
