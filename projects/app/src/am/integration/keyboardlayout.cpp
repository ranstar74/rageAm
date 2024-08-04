#include "keyboardlayout.h"

#ifdef AM_INTEGRATED

#include <unordered_map>
#include <string>

#include <Windows.h>

static const std::unordered_map<std::string, rageam::integration::eKeyboardLayout> keyboardLayoutToEnumMap = {
	{"QWERTY", rageam::integration::KeyboardLayout_QWERTY},
	{"QWERTZ", rageam::integration::KeyboardLayout_QWERTZ},
	{"AZERTY", rageam::integration::KeyboardLayout_AZERTY}
};

// Scan codes for US QWERTY based on DirectInput
static const UINT QwertyScanCodes[] = {
	0x00000010,
	0x00000011,
	0x00000012,
	0x00000013,
	0x00000014,
	0x00000015,
};

rageam::integration::eKeyboardLayout rageam::integration::GetCurrentKeyboardLayout()
{
	std::string layout;

	for (const auto& code : QwertyScanCodes)
	{
		UINT vk = MapVirtualKey(code, MAPVK_VSC_TO_VK);
		char val = static_cast<char>(vk);
		layout += val;
	}

	auto it = keyboardLayoutToEnumMap.find(layout);
	if (it != keyboardLayoutToEnumMap.end())
	{
		return it->second;
	}

	// Assume it's a QWERTY layout if we can't find it, a better approach would be to directly map key codes
	return KeyboardLayout_QWERTY;
}

#endif