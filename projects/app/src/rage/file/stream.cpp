#include "stream.h"

#include <cstdio>
#include <Windows.h>

rage::fiStream rage::fiStream::sm_Streams[MAX_STREAMS];
rage::sysCriticalSectionToken rage::fiStream::sm_Mutex;
rage::sysCriticalSectionToken rage::fiStream::sm_FormatMutex;

rage::fiStream::fiStream()
{
	m_Device = nullptr;
	m_Buffer = nullptr;
	m_File = FI_INVALID_HANDLE;

	//m_DeviceCursorPos = 0;
	//m_BufferCursorPos = 0;
	//m_BufferContentEnd = 0;

	m_BufferSize = 0;
}

rage::fiStream::fiStream(fiDevice* pDevice, fiHandle_t handle, char* buffer) : fiStream()
{
	m_Device = pDevice;
	m_Buffer = buffer;
	m_File = handle;

	// TODO: Buffer can be custom
	// Not sure what is the point of every stream having
	// size of buffer when it's constant. (is it?)
	// Though game has checks if it's set to zero.
	m_BufferSize = STREAM_BUFFER_SIZE;
}

u32 rage::fiStream::GetNumActiveStreams()
{
	sysCriticalSectionLock lock(sm_Mutex);
	return sm_ActiveStreams;
}

bool rage::fiStream::HasAvailableStreams()
{
	sysCriticalSectionLock lock(sm_Mutex);
	return sm_ActiveStreams < MAX_STREAMS;
}

rage::fiStream* rage::fiStream::CreateWithDevice(const char* path, fiDevice* pDevice)
{
	if (!pDevice)
		return nullptr;

	if (!HasAvailableStreams())
		return nullptr;

	fiHandle_t handle = pDevice->Create(path);

	if (handle == FI_INVALID_HANDLE)
		return nullptr;

	return AllocStream(path, handle, pDevice);
}

rage::fiStream* rage::fiStream::FromHandle(fiHandle_t handle, fiDevice* pDevice)
{
	if (!pDevice)
		return nullptr;

	if (!HasAvailableStreams())
		return nullptr;

	if (handle == FI_INVALID_HANDLE)
		return nullptr;

	return AllocStream("<undefined>", handle, pDevice);
}

rage::fiStream* rage::fiStream::Create(const char* path)
{
	fiDevice* device = fiDevice::GetDeviceImpl(path, false);
	return CreateWithDevice(path, device);
}

rage::fiStream* rage::fiStream::OpenWithDevice(const char* path, fiDevice* pDevice, bool isReadOnly)
{
	// g_Log.LogT("fiStream::Open({}, {})", path, isReadOnly);

	if (!pDevice)
		return nullptr;

	if (!HasAvailableStreams())
		return nullptr;

	fiHandle_t handle = pDevice->Open(path, isReadOnly);

	if (handle == FI_INVALID_HANDLE)
		return nullptr;

	return AllocStream(path, handle, pDevice);
}

rage::fiStream* rage::fiStream::Open(const char* path, bool isReadOnly)
{
	fiDevice* pDevice = fiDevice::GetDeviceImpl(path, isReadOnly);
	return OpenWithDevice(path, pDevice, isReadOnly);
}

rage::fiStream* rage::fiStream::AllocStream(const char* path, fiHandle_t handle, fiDevice* pDevice)
{
	sysCriticalSectionLock lock(sm_Mutex);

	//g_Log.Log("Allocating stream for: {}. Handle: {:X}. Device: {:X}",
	//	path, handle, reinterpret_cast<intptr_t>(pDevice));

	if (!HasAvailableStreams())
		return nullptr;

	// Find slot with closed stream
	int i = 0;
	for (; i < MAX_STREAMS; i++)
	{
		fiStream& slot = sm_Streams[i];

		if (slot.m_Device == nullptr)
			break;
	}

	char* buffer = sm_StreamBuffers[i];
	sm_Streams[i] = fiStream(pDevice, handle, buffer);
	sm_ActiveStreams++;

	return &sm_Streams[i];
}

void rage::fiStream::Close()
{
	if (!m_Device) // Already closed
		return;

	sysCriticalSectionLock lock(sm_Mutex);

	//if (m_BufferContentEnd == 0 && m_BufferCursorPos != 0)
	if (!m_ReadOffset && m_BufferOffset)
		Flush();

	m_Device->Close(m_File);
	m_Device = nullptr;

	m_File = FI_INVALID_HANDLE;

	sm_ActiveStreams--;
}

bool rage::fiStream::Flush()
{
	if (m_ReadOffset)
	{
		if (m_ReadOffset != m_BufferOffset)
			m_Device->Seek64(
				m_File,
				m_FileOffset + m_BufferOffset,
				SEEK_FILE_BEGIN);
	}
	else
	{
		if (m_BufferOffset)
			m_Device->SafeWrite(m_File, m_Buffer, m_BufferOffset);
	}
	m_ReadOffset = 0;
	m_FileOffset += m_BufferOffset;
	m_BufferOffset = 0;
	return m_Device->Flush(m_File);
}

u32 rage::fiStream::Size() const
{
	return m_Device->Size(m_File);
}

u32 rage::fiStream::Read(pVoid dest, u32 size)
{
	return m_Device->Read(m_File, dest, size);
	// TODO: Rest is broken

	char* cDest = static_cast<char*>(dest);

	// Prepare buffer for reading after writing
	if (m_ReadOffset == 0 && m_BufferOffset != 0)
	{
		if (!Flush())
			return FI_INVALID_RESULT;
	}

	u32 availableSize = m_ReadOffset - m_BufferOffset;
	if (size <= availableSize)
	{
	readFromBuffer:
		if (size > availableSize)
			size = availableSize;
		memmove(dest, m_Buffer + m_BufferOffset, size);
		m_BufferOffset += size;
		return  size;
	}

	auto v9 = 0;
	if (m_ReadOffset != m_BufferOffset)
	{
		memmove(dest, m_Buffer + m_BufferOffset, availableSize);

		cDest += availableSize;
		size -= availableSize;
		v9 = availableSize;

		m_BufferOffset = m_ReadOffset;
	}

	m_FileOffset += m_BufferOffset;

	if (size < m_BufferSize)
	{
		u32 bytesRead = m_Device->Read(m_File, m_Buffer, m_BufferSize);
		m_BufferOffset = 0;
		m_ReadOffset = bytesRead;
		if (bytesRead == FI_INVALID_RESULT)
		{
			m_BufferOffset = 0;
			m_ReadOffset = 0;
			return FI_INVALID_RESULT;
		}
		goto readFromBuffer;
	}

	// Can't fit in internal buffer
	u32 bytesRead = m_Device->Read(m_File, dest, size);
	m_ReadOffset = 0;
	if (bytesRead == FI_INVALID_RESULT)
	{
		m_BufferOffset = 0;
		return FI_INVALID_RESULT;
	}

	m_FileOffset += bytesRead;
	m_BufferOffset = 0;
	return bytesRead + v9;
}

u32 rage::fiStream::Write(pConstVoid src, u32 size)
{
	return m_Device->Write(m_File, src, size);

	const char* cSrc = static_cast<const char*>(src);

	// Prepare buffer for writing after reading
	if (m_ReadOffset != 0)
	{
		if (!Flush())
			return FI_INVALID_RESULT;
	}

	// Case 1: There's not enough space in internal buffer, we write directly to remote device
	if (size >= m_BufferSize)
	{
		u32 bytesWritten = m_Device->Write(m_File, cSrc, size);
		if (bytesWritten == FI_INVALID_RESULT)
			return FI_INVALID_RESULT;

		m_FileOffset += bytesWritten;
		return bytesWritten;
	}

	// Case 2: Internal buffer have enough total space, but there's not enough free space.
	// We write what we possibly can to internal buffer, flush it to remote device
	// and write remaining to internal buffer

	u32 availableSize = m_BufferSize - m_BufferOffset;
	if (size >= availableSize)
	{
		// Move what we can to internal buffer
		memmove(m_Buffer + m_BufferOffset, cSrc, availableSize);
		m_BufferOffset = m_BufferSize;

		cSrc += availableSize;
		size -= availableSize;

		// Send our fully packed buffer to device
		if (!Flush())
			return FI_INVALID_RESULT;

		// Now internal buffer has enough free space to perform Case 3
	}

	// Case 3: We have enough free memory in internal buffer, simply copy memory there

	memmove(m_Buffer + m_BufferOffset, cSrc, size);
	m_BufferOffset += size;

	return size;
}

bool rage::fiStream::PutCh(char c)
{
	return Write(&c, 1) != FI_INVALID_RESULT;
}

void rage::fiStream::WriteLineVA(const char* fmt, const va_list& args)
{
	sysCriticalSectionLock lock(sm_FormatMutex);

	static char buffer[2048];

	vsnprintf_s(buffer, 2048, fmt, args);

	Write(buffer, strlen(buffer));
}

void rage::fiStream::WriteLinef(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WriteLineVA(fmt, args);
	va_end(args);
}

void rage::fiStream::WriteLine(const char* line)
{
	Write(line, strlen(line));
}
