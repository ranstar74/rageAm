#include "material.h"

#include "am/integration/memory/address.h"

rage::phMaterialMgr* rage::phMaterialMgr::GetInstance()
{
#ifndef AM_STANDALONE
	static phMaterialMgr* inst = 
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		*gmAddress::Scan("48 8B 05 ?? ?? ?? ?? 48 8B 80 00 02 00 00 48 39 44 24 40", "rage::phMaterialMgr::sm_Instance")
		.GetRef(3)
		.To<phMaterialMgr**>();
#else
		gmAddress::Scan("48 8B 15 ?? ?? ?? ?? 4C 23 42 10", "rage::phMaterialMgr::sm_Instance")
		.GetRef(3)
		.To<rage::phMaterialMgr*>();
#endif
	return inst;
#else
	AM_UNREACHABLE("phMaterialMgr::GetInstance() -> Unsupported in standalone.");
#endif
}

rage::phMaterial* rage::phMaterialMgr::GetMaterialByIndex(u32 index) const
{
	AM_ASSERTS(index < m_NumMaterials);
	return reinterpret_cast<phMaterial*>(m_Materials + m_MaterialsSize * index);
}

rage::phMaterial* rage::phMaterialMgr::GetMaterialById(Id id) const
{
	return GetMaterialByIndex(id & m_MaterialIndexMask);
}
