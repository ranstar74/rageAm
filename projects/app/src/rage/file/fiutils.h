#pragma once

#include "device.h"
#include "iterator.h"
#include "common/types.h"

namespace rage
{
	// Gets whether directory at given path has any sub (children) directory.
	inline bool HasSubDirectories(ConstString path)
	{
		fiDevicePtr device = fiDevice::GetDeviceImpl(path);
		if (!device)
			return false;

		u32 attributes = device->GetAttributes(path);
		if ((attributes & FI_ATTRIBUTE_DIRECTORY) == 0)
			return false;

		fiIterator iterator(path);
		while (iterator.Next())
		{
			if (iterator.IsDirectory())
			{
				return true;
			}
		}
		return false;
	}

	inline u64 fiGetFileTime(ConstString path)
	{
		fiDevicePtr device = fiDevice::GetDeviceImpl(path);
		if (!device)
			return 0;
		return device->GetFileTime(path);
	}
}
