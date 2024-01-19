#include "customeffect_base.h"

CCustomShaderEffectBaseType* CCustomShaderEffectBaseType::SetupMasterForModelInfo(CBaseModelInfo* modelInfo)
{
#ifdef AM_INTEGRATED

	static auto setupMasterForModelInfo =
		gmAddress::Scan("48 89 4C 24 08 48 83 EC 68 48 C7 44 24 48 00 00 00 00 48 83 7C",
			"CCustomShaderEffectBaseType::SetupMasterForModelInfo")
		.ToFunc<CCustomShaderEffectBaseType*(CBaseModelInfo*)>();
	return setupMasterForModelInfo(modelInfo);
#else
	AM_UNREACHABLE("CCustomShaderEffectBaseType::SetupMasterForModelInfo() -> Not implemented in standalone.");
#endif
}
