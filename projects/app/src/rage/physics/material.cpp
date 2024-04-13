#include "material.h"

#include "am/integration/memory/address.h"

rage::phMaterialMgr* rage::phMaterialMgr::GetInstance()
{
#ifndef AM_STANDALONE
			static phMaterialMgr* inst = gmAddress::Scan("48 8B 15 ?? ?? ?? ?? 4C 23 42 10")
				.GetRef(3)
				.To<decltype(inst)>();
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
