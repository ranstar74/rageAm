#include "customeffect_base.h"
#include "am/integration/memory/address.h"

CCustomShaderEffectBaseType* CCustomShaderEffectBaseType::SetupMasterForModelInfo(CBaseModelInfo* modelInfo)
{
	static auto fn = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"48 89 4C 24 08 48 83 EC 68 48 C7 44 24 48 00 00 00 00 48 83 7C", 
#else
		"48 89 5C 24 10 57 48 83 EC 30 33 DB 48",
#endif
		"CCustomShaderEffectBaseType::SetupMasterForModelInfo")
		.ToFunc<CCustomShaderEffectBaseType*(CBaseModelInfo*)>();
	return fn(modelInfo);
}
