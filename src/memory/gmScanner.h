#pragma once
#include <unordered_map>

#include "Logger.h"
#include "gmAddress.h"

// TODO: Storage isnt working correctly
namespace gm
{
	/**
	 * \brief For improving loading time, instead of scanning patterns every time
	 * this class acts as storage for pre-scanned addresses because they may change
	 * only with new .exe or under some rare circumstances.
	 */
	class gmScanner
	{
		const char* m_addrStorageName = "rageAm/rageAm_storage.bin";
		std::unordered_map<std::string, uintptr_t> m_addrStorage;

		const int8_t SCANNER_STORE_FORMAT_VER = 1;

		uint32_t m_GtaSize;
		uint8_t* m_GtaStartAddress;

		void Load();
		void Save() const;
		void ValidateStorage();
		void ClearStorage();
	public:
		gmScanner();
		~gmScanner();

		uintptr_t GetAddressFromStorage(const char* name) const;

		gmAddress ScanPatternModule(const char* name, const char* module, const char* pattern);
		gmAddress ScanPattern(const char* name, const char* pattern);
	};
}

inline gm::gmScanner g_Scanner;
