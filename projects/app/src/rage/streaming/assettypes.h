#pragma once

#include "common/types.h"
#include "am/system/asserts.h"

using strAssetID = u32;

enum eStrAssetId
{
	STR_ASSET_ID_RPF = 0,
	STR_ASSET_ID_IDR = 1,
	STR_ASSET_ID_IFT = 2,
	STR_ASSET_ID_IDD = 3,
	STR_ASSET_ID_ITD = 4,
	STR_ASSET_ID_ICD = 5,
	STR_ASSET_ID_IBN = 6,
	STR_ASSET_ID_IBS = 8,
	STR_ASSET_ID_ILD = 9,
	STR_ASSET_ID_IPM = 10,
	STR_ASSET_ID_IED = 11,
	STR_ASSET_ID_IPT = 12,
	STR_ASSET_ID_IMT = 13,
	STR_ASSET_ID_IMAP = 14,
	STR_ASSET_ID_IPDB = 15,
	STR_ASSET_ID_MRF = 16,
	STR_ASSET_ID_INV = 17,
	STR_ASSET_ID_IHN = 18,
	STR_ASSET_ID_ISC = 19,
	STR_ASSET_ID_CUT = 20,
	STR_ASSET_ID_GFX = 21,
	STR_ASSET_ID_IPL = 22,
	STR_ASSET_ID_IAM = 23,
	STR_ASSET_ID_IND = 24,
	STR_ASSET_ID_NOD = 25,
	STR_ASSET_ID_IVR = 26,
	STR_ASSET_ID_IWR = 27,
	STR_ASSET_ID_IMF = 28,
	STR_ASSET_ID_ITYP = 29,
	STR_ASSET_ID_INH = 30,
	STR_ASSET_ID_IFD = 31,
	STR_ASSET_ID_ILDB = 32
};

namespace rage
{
	inline const char* GetPlatformAssetExtensionByID(strAssetID id)
	{
		if (id == 0) return "rpf";
		if (id == 1) return "ydr";
		if (id == 2) return "yft";
		if (id == 3) return "ydd";
		if (id == 4) return "ytd";
		if (id == 5) return "ycd";
		if (id == 6) return "ybn";
		if (id == 8) return "ybs";
		if (id == 9) return "yld";
		if (id == 10) return "ypm";
		if (id == 11) return "yed";
		if (id == 12) return "ypt";
		if (id == 13) return "ymt";
		if (id == 14) return "ymap";
		if (id == 15) return "ypdb";
		if (id == 16) return "mrf";
		if (id == 17) return "ynv";
		if (id == 18) return "yhn";
		if (id == 19) return "ysc";
		if (id == 20) return "cut";
		if (id == 21) return "gfx";
		if (id == 22) return "ypl";
		if (id == 23) return "yam";
		if (id == 24) return "ynd";
		if (id == 25) return "nod";
		if (id == 26) return "yvr";
		if (id == 27) return "ywr";
		if (id == 28) return "ymf";
		if (id == 29) return "ytyp";
		if (id == 30) return "ynh";
		if (id == 31) return "yfd";
		if (id == 32) return "yldb";
		AM_UNREACHABLE("GetPlatformAssetExtensionByID() -> ID %u is unknown!", id);
	}
}
