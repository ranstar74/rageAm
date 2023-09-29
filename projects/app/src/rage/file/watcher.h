#pragma once

#include "common/logger.h"
#include "am/system/thread.h"
#include "rage/atl/string.h"
#include "rage/file/device.h"
#include "rage/file/iterator.h"
#include "zlib.h"

namespace rage
{
	/**
	 * \brief Utility to watch directory content changes (only one level of depth).
	 */
	class fiDirectoryWatcher
	{
		static constexpr u32 WATCH_UPDATE_INTERVAL = 200; // In milliseconds

		bool			m_IsEnabled = true;
		bool			m_Initialized = false;
		bool			m_Changed = false;
		std::mutex		m_Mutex;
		atString		m_Path;
		rageam::Thread	m_Thread;

		static u32 ThreadEntry(const rageam::ThreadContext* ctx)
		{
			u32 oldChecksum = 0;

			fiDirectoryWatcher* watcher = static_cast<fiDirectoryWatcher*>(ctx->Param);
			while (!ctx->Thread->ExitRequested())
			{
				watcher->m_Mutex.lock();
				if (watcher->m_IsEnabled)
				{
					if (!String::IsNullOrEmpty(watcher->m_Path))
					{
						// The easiest & fastest way to check if anything was modified is to
						// use some sort of hash-sum, we're going to use crc32 implementation from zlib
						u32 newChecksum = 0;

						fiIterator it(watcher->m_Path);
						while (it.Next())
						{
							const fiFindData* findData = &it.GetFindData();

							newChecksum = crc32(newChecksum, reinterpret_cast<const Bytef*>(findData), sizeof fiFindData);
						}

						if (watcher->m_Initialized && oldChecksum != newChecksum)
						{
							AM_DEBUGF("DirectoryWatcher -> Detected changed in %s", watcher->m_Path.GetCStr());

							if (watcher->OnChangeCallback)
								watcher->OnChangeCallback();

							watcher->m_Changed = true;
						}

						watcher->m_Initialized = true;
						oldChecksum = newChecksum;
					}
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
			m_Path = path;
			m_Initialized = false;
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
