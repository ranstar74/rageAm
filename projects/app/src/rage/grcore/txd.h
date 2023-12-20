//
// File: txd.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "texture.h"
#include "rage/paging/template/dictionary.h"

namespace rage 
{
	using grcTextureDictionary = pgDictionary<grcTexture>;
	using grcTextureDictionaryPtr = pgPtr<grcTextureDictionary>;
	using grcTexturePtr = pgPtr<grcTexture>;
	using grcTextureUPtr = pgUPtr<grcTexture>;
}
