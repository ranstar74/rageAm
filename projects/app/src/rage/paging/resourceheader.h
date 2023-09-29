#pragma once

#include "resourceinfo.h"
#include "helpers/fourcc.h"

namespace rage
{
#define MAGIC_RSC FOURCC('R', 'S', 'C', '7')

	struct datResourceHeader
	{
		u32				Magic;
		u32				Version;
		datResourceInfo Info;

		bool IsValidMagic() const { return Magic == MAGIC_RSC; }
	};
}
