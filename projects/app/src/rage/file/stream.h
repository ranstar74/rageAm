//
// File: stream.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"

#include "rage/system/ipc/criticalsection.h"
#include "rage/file/device.h"
#include "helpers/resharper.h"

namespace rage
{
	// TODO: Buffering is totally broken!

	/**
	 * \brief Stream is used when there's need to write/read small files or perform often I/O operations.
	 * Stream uses internal buffering to prevent I/O operation spam, which may be slow.
	 * Buffered data is sent on Flush command and when stream is closed.
	 */
	class fiStream
	{
		static constexpr u32 STREAM_BUFFER_SIZE = 0x1000;
		static constexpr u32 MAX_STREAMS = 16;

		static inline u32 sm_ActiveStreams;

		static inline char sm_StreamBuffers[MAX_STREAMS][STREAM_BUFFER_SIZE];
		static fiStream sm_Streams[MAX_STREAMS];

		static sysCriticalSectionToken sm_Mutex;
		static sysCriticalSectionToken sm_FormatMutex;

		fiDevice* m_Device;
		fiHandle_t m_File;
		char* m_Buffer;

		u64 m_FileOffset = 0;
		u32 m_BufferOffset = 0;
		u32 m_ReadOffset = 0;

		u32 m_BufferSize;

		static fiStream* AllocStream(const char* path, fiHandle_t handle, fiDevice* pDevice);
	public:
		fiStream();
		fiStream(fiDevice* pDevice, fiHandle_t handle, char* buffer);

		static u32 GetNumActiveStreams();
		static bool HasAvailableStreams();

		/**
		 * \brief Creates new file using given device, overwriting existing file.
		 * \return Pointer to allocated stream, if successfully; Otherwise NULL.
		 * \remark Not supported on following devices: fiPackfile
		 */
		static fiStream* CreateWithDevice(const char* path, fiDevice* pDevice);

		/**
		 * \brief Creates stream from given handle and device.
		 * \return Pointer to allocated stream, if successfully; Otherwise NULL.
		 */
		static fiStream* FromHandle(fiHandle_t handle, fiDevice* pDevice);

		/**
		 * \brief Creates new file, overwriting existing file.
		 * \return Pointer to allocated stream, if successfully; Otherwise NULL.
		 * \remark Not supported on following devices: fiPackfile
		 */
		static fiStream* Create(const char* path);

		/**
		 * \brief Opens existing file using given device.
		 * \return Pointer to allocated stream, if successfully; Otherwise NULL.
		 * \remark Writing is not supported on following devices: fiPackfile
		 */
		static fiStream* OpenWithDevice(const char* path, fiDevice* pDevice, bool isReadOnly = true);

		/**
		 * \brief Opens existing file.
		 * \return Pointer to allocated stream, if successfully; Otherwise NULL.
		 * \remark Write mode not supported on following devices: fiPackfile
		 */
		static fiStream* Open(const char* path, bool isReadOnly = true);

		/**
		 * \brief Performs buffer flushing and closes remote stream.
		 */
		void Close();

		/**
		 * \brief Immediately sends all buffered data to remote stream.
		 */
		bool Flush();

		/**
		 * \brief Gets size of the file in bytes.
		 */
		u32 Size() const;

		/**
		 * \brief Reads given number of bytes from stream.
		 * \return Number of bytes was read; FI_INVALID_SIZE if unsuccessful.
		 */
		u32 Read(pVoid dest, u32 size);

		/**
		 * \brief Writes data to stream buffer, or if data is large, immediately to remote file.
		 * \return Number of bytes written; FI_INVALID_SIZE if unsuccessful.
		 */
		u32 Write(pConstVoid src, u32 size);

		bool PutCh(char c);

		void WriteLineVA(const char* fmt, const va_list& args);
		PRINTF_ATTR(2, 3) void WriteLinef(const char* fmt, ...);
		void WriteLine(const char* line);

		// Helper methods

		template<typename T>
		T ReadValue()
		{
			T value;
			Read(&value, sizeof(T));
			return value;
		}

		u8 ReadU8() { return ReadValue<u8>(); }
		u16 ReadU16() { return ReadValue<u16>(); }
		u32 ReadU32() { return ReadValue<u32>(); }
		u64 ReadU64() { return ReadValue<u64>(); }
	};

	/**
	 * \brief Smart unique pointer for fiStream.
	 */
	class fiStreamPtr
	{
		fiStream* m_Stream;
	public:
		fiStreamPtr(fiStream* stream) { m_Stream = stream; }
		fiStreamPtr(const fiStreamPtr& other) = delete;
		fiStreamPtr(fiStreamPtr&& other) noexcept
		{
			std::swap(m_Stream, other.m_Stream);
		}
		~fiStreamPtr()
		{
			if (m_Stream == nullptr)
				return;

			m_Stream->Close();
			m_Stream = nullptr;
		}

		fiStream* operator->() const { return m_Stream; }

		fiStreamPtr& operator=(const fiStreamPtr& other) = delete;
		fiStreamPtr& operator=(fiStreamPtr&& other) noexcept
		{
			std::swap(m_Stream, other.m_Stream);
			return *this;
		}
	};
}
