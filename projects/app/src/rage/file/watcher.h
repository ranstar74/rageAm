#pragma once

#include "common/logger.h"
#include "am/system/thread.h"
#include "rage/atl/string.h"
#include "rage/file/device.h"
#include "rage/file/iterator.h"
#include "zlib.h"
#include "rage/atl/datahash.h"

namespace rage
{
	/**
	 * \brief Utility to watch directory content changes (only one level of depth).
	 */
	class fiDirectoryWatcher
	{
		static constexpr u32 WATCH_UPDATE_INTERVAL = 300; // In milliseconds

		bool			m_IsEnabled = true;
		bool			m_Initialized = false;
		bool			m_Changed = false;
		std::mutex		m_Mutex;
		atString		m_Path;
		rageam::Thread	m_Thread;
		bool			m_IsDirectory = false;
		fiDevicePtr		m_Device = nullptr;

		static u32 ThreadEntry(const rageam::ThreadContext* ctx)
		{
			u32 oldChecksum = 0;

			fiDirectoryWatcher* watcher = static_cast<fiDirectoryWatcher*>(ctx->Param);
			while (!ctx->Thread->ExitRequested())
			{
				watcher->m_Mutex.lock();
				if (watcher->m_IsEnabled && watcher->m_Device != nullptr)
				{
					// The easiest & fastest way to check if anything was modified is to
					// use some sort of hash-sum, we're using atStringHash
					u32 newChecksum = 0;

					// Take hash-sum for all directory files and for files simply write time hash
					if (watcher->m_IsDirectory)
					{
						fiIterator it(watcher->m_Path, watcher->m_Device);
						while (it.Next())
						{
							const fiFindData* findData = &it.GetFindData();
							newChecksum = atDataHash(findData, sizeof fiFindData, newChecksum);
						}
					}
					else
					{
						u64 writeTime = watcher->m_Device->GetFileTime(watcher->m_Path);
						newChecksum = atDataHash(&writeTime, sizeof u64);
					}

					if (watcher->m_Initialized && oldChecksum != newChecksum)
					{
						AM_DEBUGF("DirectoryWatcher -> Detected change in %s", watcher->m_Path.GetCStr());

						if (watcher->OnChangeCallback)
							watcher->OnChangeCallback();

						watcher->m_Changed = true;
					}

					watcher->m_Initialized = true;
					oldChecksum = newChecksum;
				}
				watcher->m_Mutex.unlock();

				std::this_thread::sleep_for(std::chrono::milliseconds(WATCH_UPDATE_INTERVAL));
			}
			return 0;
		}
	public:
		fiDirectoryWatcher() : m_Thread("DirectoryWatcher", ThreadEntry, this)
		{

		}

		void SetEntry(ConstString path)
		{
			std::unique_lock lock(m_Mutex);
			if (m_Path.Equals(path, true))
				return;

			m_Device = fiDevice::GetDeviceImpl(path);
			if (!m_Device)
				return;

			m_IsDirectory = m_Device->GetAttributes(path) & FI_ATTRIBUTE_DIRECTORY;
			m_Path = path;
			m_Initialized = false;
		}

		ConstString GetEntry()
		{
			return m_Path;
		}

		bool GetChangeOccuredAndReset()
		{
			std::unique_lock lock(m_Mutex);
			bool changed = m_Changed;
			m_Changed = false;
			return changed;
		}

		void SetEnabled(bool on)
		{
			std::unique_lock lock(m_Mutex);
			m_IsEnabled = on;
		}

		// Note: This callback is not thread safe!
		std::function<void()> OnChangeCallback;
	};
}
