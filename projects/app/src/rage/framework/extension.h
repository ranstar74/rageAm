#pragma once

#include "rage/paging/template/array.h"
#include "rage/atl/hashstring.h"
#include "rage/paging/ref.h"

namespace rage
{
	class fwExtensionDef
	{
		atHashValue m_Name;

	public:
	};

	using fwExtensionDefPtr = pgUPtr<fwExtensionDef>;
	using fwExtensionDefs = pgArray<fwExtensionDefPtr>;
}
