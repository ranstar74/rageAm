#include "rage/file/device.h"

#include "rage/paging/resourceheader.h"
#include "common/logger.h"
#include "local.h"

rage::fiDevice* rage::fiDevice::GetDeviceImpl(ConstString path, bool isReadOnly)
{
#ifdef AM_STANDALONE
	return fiDeviceLocal::GetInstance(); // For now...
#else
	// 48 89 5C 24 08 88 54 24 10 55 56 57 41 54 41 55 41 56 41 57 48 83
	return fiDeviceLocal::GetInstance();
#endif
}

bool rage::fiDevice::FileExists(ConstString path)
{
	fiDevice* device = GetDeviceImpl(path);
	if (!device)
		return false;

	u32 attributes = device->GetAttributes(path);
	return attributes != FI_INVALID_ATTRIBUTES && attributes != FI_ATTRIBUTE_DIRECTORY;
}

bool rage::fiDevice::SafeRead(fiHandle_t file, pVoid buffer, u32 size)
{
	if (size == 0)
		return true;

	u32 totalBytesRead = 0;
	while (true)
	{
		u32 remaining = size - totalBytesRead;
		u32 bytesRead = Read(file, buffer, remaining);
		if (bytesRead == FI_INVALID_RESULT)
			return false;

		totalBytesRead += bytesRead;
		if (totalBytesRead >= size)
			return true;
	}
}

bool rage::fiDevice::SafeWrite(fiHandle_t file, pConstVoid buffer, u32 size)
{
	if (size == 0)
		return true;

	u32 totalBytesRead = 0;
	while (true)
	{
		u32 remaining = size - totalBytesRead;
		u32 bytesRead = Write(file, buffer, remaining);
		if (bytesRead == FI_INVALID_RESULT)
			return false;

		totalBytesRead += bytesRead;
		if (totalBytesRead >= size)
			return true;
	}
}

u32 rage::fiDevice::GetResourceInfo(ConstString path, datResourceInfo& info)
{
	if (GetFileSize(path) < sizeof datResourceHeader)
	{
		AM_ERRF("fiDevice::GetResourceInfo() -> Invalid file (empty / doesn't exists) %s", path);
		return 0;
	}

	u64 offset;
	u32 version = 0;

	fiHandle_t file = OpenBulk(path, offset);
	if (file == FI_INVALID_HANDLE)
	{
		AM_ERRF("fiDevice::GetResourceInfo() -> Can't open %s", path);
		return 0;
	}

	datResourceHeader header{};
	if (ReadBulk(file, offset, &header, sizeof datResourceHeader) != FI_INVALID_RESULT)
	{
		version = header.Version;
		info = header.Info;
	}
	CloseBulk(file);

	return version;
}
