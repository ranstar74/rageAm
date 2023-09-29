#pragma once

#include "node.h"
#include "types.h"
#include "am/file/path.h"
#include "common/types.h"

/**
 * \brief A XML Document.
 */
class XmlDoc
{
	static inline thread_local XmlDoc* m_CurrentDoc = nullptr;

	TinyDoc m_Doc;

public:
	XmlDoc()
	{
		m_CurrentDoc = this;
	}
	XmlDoc(ConstString rootName) : XmlDoc()
	{
		m_Doc.InsertFirstChild(m_Doc.NewDeclaration());
		m_Doc.InsertEndChild(m_Doc.NewElement(rootName));
	}
	~XmlDoc()
	{
		m_CurrentDoc = nullptr;
	}

	XmlHandle Root() { return m_Doc.RootElement(); }

	void LoadFromFile(const rageam::file::WPath& path);
	void SaveToFile(const rageam::file::WPath& path);

	// Prints document to standard output stream
	void DebugPrint() const
	{
		m_Doc.Print();
	}
};
