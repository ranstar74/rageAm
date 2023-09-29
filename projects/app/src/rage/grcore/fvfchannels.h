//
// File: fvfchannels.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/crypto/joaat.h"

namespace rage
{
	// Channel bits of default vertex format, see grcFvf
	enum grcVertexChannel
	{
		CHANNEL_POSITION = 0,
		CHANNEL_BLENDWEIGHT = 1,
		CHANNEL_BLENDINDICES = 2,
		CHANNEL_NORMAL = 3,
		CHANNEL_COLOR0 = 4,
		CHANNEL_COLOR1 = 5,
		CHANNEL_TEXCOORD0 = 6,
		CHANNEL_TEXCOORD1 = 7,
		CHANNEL_TEXCOORD2 = 8,
		CHANNEL_TEXCOORD3 = 9,
		CHANNEL_TEXCOORD4 = 10,
		CHANNEL_TEXCOORD5 = 11,
		CHANNEL_TEXCOORD6 = 12,
		CHANNEL_TEXCOORD7 = 13,
		CHANNEL_TANGENT0 = 14,
		CHANNEL_TANGENT1 = 15,
		CHANNEL_BINORMAL0 = 16,
		CHANNEL_BINORMAL1 = 17,

		CHANNEL_INVALID = -1,
	};

	inline grcVertexChannel GetFvfChannelBySemanticName(const char* semanticName, u32 semanticIndex)
	{
		grcVertexChannel channel = CHANNEL_INVALID;

		u32 semanticHash = joaat(semanticName);

		switch (semanticHash)
		{
		case joaat("POSITION"):		channel = CHANNEL_POSITION;		break;
		case joaat("BLENDWEIGHT"):	channel = CHANNEL_BLENDWEIGHT;	break;
		case joaat("BLENDINDICES"): channel = CHANNEL_BLENDINDICES; break;
		case joaat("NORMAL"):		channel = CHANNEL_NORMAL;		break;
		case joaat("COLOR"):
			switch (semanticIndex)
			{
			case 0: channel = CHANNEL_COLOR0; break;
			case 1: channel = CHANNEL_COLOR1; break;
			}
			break;
		case joaat("TEXCOORD"):
			switch (semanticIndex)
			{
			case 0: channel = CHANNEL_TEXCOORD0; break;
			case 1: channel = CHANNEL_TEXCOORD1; break;
			case 2: channel = CHANNEL_TEXCOORD2; break;
			case 3: channel = CHANNEL_TEXCOORD3; break;
			case 4: channel = CHANNEL_TEXCOORD4; break;
			case 5: channel = CHANNEL_TEXCOORD5; break;
			case 6: channel = CHANNEL_TEXCOORD6; break;
			case 7: channel = CHANNEL_TEXCOORD7; break;
			}
			break;
		case joaat("TANGENT"):
			switch (semanticIndex)
			{
			case 0: channel = CHANNEL_TANGENT0; break;
			case 1: channel = CHANNEL_TANGENT1; break;
			}
			break;
		case joaat("BINORMAL"):
			switch (semanticIndex)
			{
			case 0: channel = CHANNEL_BINORMAL0; break;
			case 1: channel = CHANNEL_BINORMAL1; break;
			}
			break;
		}
		return channel;
	}
}
