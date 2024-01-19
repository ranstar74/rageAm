#include "streamingmodule.h"

rage::strStreamingModule::strStreamingModule(
	ConstString name, u32 assetVersion, strAssetID assetTypeID, u32 defaultSize, bool needTempMemory)
{
	m_Name = name;
	m_AssetVersion = assetVersion;
	m_AssetTypeID = assetTypeID;
	m_Size = defaultSize;
	m_NeedTempMemory = needTempMemory;

	m_CanDefragment = false;
	m_UsesExtraMemory = false;
}
