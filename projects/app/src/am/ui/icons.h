//
// File: icons.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/ui/image.h"
#include "rage/atl/set.h"

namespace rageam::ui
{
	class IExplorerEntry;

	enum eIconSize
	{
		IconSize_16,
		IconSize_24,
		IconSize_32,
		IconSize_40,
		IconSize_48,
		IconSize_64,
		IconSize_96,
		IconSize_128,
		IconSize_256,
		IconSize_COUNT,
	};

	/**
	 * Static icons from 'rageAm/Icons/'
	 */
	class Icons
	{
		// Must match eIconSize
		static constexpr int sm_IconSizes[] =
		{
			16, 24, 32, 40, 48, 64, 96, 128, 256,
		};

		struct Icon
		{
			bool IsIco = false;
			Image Images[IconSize_COUNT];
		};

		rage::atSet<Icon> m_Icons;
	public:
		Icons();

		/**
		 * \brief Gets image icon from path "data\icons\*.png"
		 * \return Pointer to icon instance if file found; Otherwise nullptr.
		 */
		Image* GetIcon(ConstString name, eIconSize size) const;
	};
}
