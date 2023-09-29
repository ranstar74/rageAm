#include "doc.h"
#include "exception.h"

void XmlDoc::LoadFromFile(const rageam::file::WPath& path)
{
	FILE* file;
	if (_wfopen_s(&file, path.GetCStr(), L"rb") != 0)
	{
		throw XmlException("File can't be opened for reading", 0);
	}

	TinyError error = m_Doc.LoadFile(file);
	fclose(file);

	if (error != tinyxml2::XML_SUCCESS)
		throw XmlException(TinyDoc::ErrorIDToName(error), m_Doc.ErrorLineNum());
}

void XmlDoc::SaveToFile(const rageam::file::WPath& path)
{
	FILE* file;
	if (_wfopen_s(&file, path.GetCStr(), L"w") != 0)
	{
		throw XmlException("File can't be opened for writing", 0);
	}

	tinyxml2::XMLError error = m_Doc.SaveFile(file);
	fclose(file);

	if(error != tinyxml2::XML_SUCCESS)
		throw XmlException(TinyDoc::ErrorIDToName(error), m_Doc.ErrorLineNum());
}
