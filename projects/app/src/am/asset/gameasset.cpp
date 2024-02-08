#include "gameasset.h"

#include "workspace.h"
#include "am/system/datetime.h"
#include "am/xml/doc.h"

rageam::asset::AssetBase::AssetBase(const file::WPath& path)
{
	m_HasSavedConfig = IsFileExists(GetConfigPath());
	SetNewPath(path, true);
}

void rageam::asset::AssetBase::SetNewPath(ConstWString newPath, bool updateWorkspace)
{
	m_Directory = newPath;
	m_HashKey = AssetPathHashFn(newPath);
	if (updateWorkspace)
	{
		Workspace::GetParentWorkspacePath(newPath, m_WorkspaceDirectory);
		m_WorkspaceDirectoryHash = AssetPathHashFn(m_WorkspaceDirectory);
	}
}

bool rageam::asset::AssetBase::LoadConfig(bool temp)
{
	static Logger logger("asset_config");
	LoggerScoped scopedLogger(logger);

	file::WPath configPath = GetConfigPath(temp);
	// Temporary config might not exist... use regular one then
	if (temp && !IsFileExists(configPath))
		configPath = GetConfigPath();

	// If config file doesn't exist - create default one
	if (!IsFileExists(configPath))
	{
		Refresh();
		// Don't save config on first asset opening, let user do this
		// return AM_VERIFY(SaveConfig(), "Unable to save just created config");
		return true;
	}

	try
	{
		XmlDoc xDoc;
		xDoc.LoadFromFile(configPath);

		XmlHandle xRoot = xDoc.Root();

		// Root element name must match to asset-specific name constant
		ConstWString documentRootName = String::ToWideTemp(xRoot.GetName());
		ConstWString expectedRootName = GetXmlName();
		if (!String::Equals(documentRootName, expectedRootName))
		{
			AM_ERRF(L"Root element name doesn't match (file: %ls, actual: %ls), XML file is invalid %ls",
				documentRootName, expectedRootName, configPath.GetCStr());
			return false;
		}

		// Verify format version
		u32 version;
		xRoot.GetAttribute("Version", version);
		if (version != GetFormatVersion()) // TODO: Ideally we would have to support different format version
		{
			AM_ERRF(L"Format version doesn't match (file: %u, actual: %u) %ls", version, GetFormatVersion(), configPath.GetCStr());
			return false;
		}

		Deserialize(xRoot);
	}
	catch (const XmlException& ex)
	{
		ex.Print();
		return false;
	}

	Refresh(); // Actualize config

	return true;
}

bool rageam::asset::AssetBase::SaveConfig(bool temp) const
{
	const file::WPath& configPath = GetConfigPath(temp);

	// Create root element with asset-specific name, (for txd its TextureDictionary).
	// It will help to detect errors in xml file faster.
	XmlDoc xDoc(String::ToAnsiTemp(GetXmlName()));

	// Add asset format version
	XmlHandle xRoot = xDoc.Root();
	xRoot.SetAttribute("Version", GetFormatVersion());

	// Add timestamp
	char timeBuffer[36];
	DateTime::Now().Format(timeBuffer, sizeof timeBuffer, "s");
	xRoot.SetAttribute("Timestamp", timeBuffer);

	// Write config to xml
	try
	{
		Serialize(xRoot);
		xDoc.SaveToFile(configPath);
	}
	catch (const XmlException& ex)
	{
		ex.Print();
		return false;
	}
	return true;
}

rageam::asset::AssetSource::AssetSource(AssetBase* parent, ConstWString filePath)
{
	m_Parent = parent;
	SetFilePath(filePath);
}

void rageam::asset::AssetSource::SetFilePath(ConstWString path)
{
	m_FilePath = path;
	m_HashKey = ComputeHashKey(path);
}
