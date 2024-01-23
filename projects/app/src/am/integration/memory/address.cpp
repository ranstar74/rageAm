#include "address.h"

#include "common/logger.h"
#include "am/system/asserts.h"
#include "am/system/timer.h"
#include "am/system/datamgr.h"
#include "am/xml/doc.h"

#include <cstdlib>
#include <mutex>

#include "am/file/fileutils.h"
#include "am/xml/iterator.h"

u32 gmAddressCache::ComputeByteHash(u64 offset, int numBytes) const
{
	// One way to validate if pattern is still valid is to check if bytes still match
	return rageam::DataHash((pVoid)OffsetToAddress(offset), numBytes);
}

void gmAddressCache::EmplaceScan(Scan& scan)
{
	u32 scanIndex = m_Scans.GetSize();
	m_Scans.Emplace(std::move(scan));
	m_PatternToScanIndex.InsertAt(scan.PatternHash, scanIndex);
}

void gmAddressCache::SaveToXml() const
{
	using namespace rageam;

	file::WPath configPath = DataManager::GetAppData() / FILE_NAME;
	try
	{
		XmlDoc    xDoc("MemoryScanner");
		XmlHandle xRoot = xDoc.Root();
		XmlHandle xScans = xRoot.AddChild("Scans");

		xRoot.AddChild("ImageSize").SetTheValueAttribute(m_ModuleSize);

		for (Scan& scan : m_Scans)
		{
			XmlHandle xScan = xScans.AddChild("Item");
			xScan.AddChild("ModuleOffset").SetTheValueAttribute(scan.ModuleOffset);
			xScan.AddChild("PatternHash").SetTheValueAttribute(scan.PatternHash);
			xScan.AddChild("ByteHash").SetTheValueAttribute(scan.ByteHash);
			xScan.AddChild("NumBytes").SetTheValueAttribute(scan.NumBytes);
		}

		xDoc.SaveToFile(configPath);
	}
	catch (const XmlException& ex)
	{
		ex.Print();
	}
}

void gmAddressCache::ReadFromXml()
{
	using namespace rageam;

	file::WPath configPath = DataManager::GetAppData() / FILE_NAME;
	if (!IsFileExists(configPath))
		return;

	try
	{
		XmlDoc xDoc;
		xDoc.LoadFromFile(configPath);

		XmlHandle xRoot = xDoc.Root();
		XmlHandle xScans = xRoot.GetChild("Scans");

		// Different image size means that exe was changed
		u64 moduleSize;
		xRoot.GetChild("ImageSize").GetTheValueAttribute(moduleSize);
		if (moduleSize != m_ModuleSize)
		{
			AM_WARNINGF("Pattern Cache - Module size mismatch, invalidating cache...");
			return;
		}

		for (const XmlHandle& xScan : XmlIterator(xScans))
		{
			Scan scan;
			xScan.GetChild("ModuleOffset").GetTheValueAttribute(scan.ModuleOffset);
			xScan.GetChild("PatternHash").GetTheValueAttribute(scan.PatternHash);
			xScan.GetChild("ByteHash").GetTheValueAttribute(scan.ByteHash);
			xScan.GetChild("NumBytes").GetTheValueAttribute(scan.NumBytes);

			// If module size is the same but pattern is no longer valid,
			// this could mean that some mod patched this memory region
			if (scan.ByteHash != ComputeByteHash(scan.ModuleOffset, scan.NumBytes))
			{
				AM_WARNINGF("Pattern Cache - Invalid pattern found, most likely mod conflict, invalidating cache...");
				return;
			}

			EmplaceScan(scan);
		}

		AM_DEBUGF("Pattern Cache - Loaded %u patterns.", m_Scans.GetSize());
	}
	catch (const XmlException& ex)
	{
		ex.Print();
	}
}

gmAddressCache::gmAddressCache()
{
	GetModuleBaseAndSize(&m_ModuleBase, &m_ModuleSize);
	ReadFromXml();
}

gmAddressCache::~gmAddressCache()
{
	SaveToXml();
}

void gmAddressCache::AddScanResult(ConstString pattern, u64 offset, int numBytes)
{
	std::unique_lock lock(m_Mutex);

	Scan scan;
	scan.ByteHash = ComputeByteHash(offset, numBytes);
	scan.ModuleOffset = offset;
	scan.PatternHash = rageam::Hash(pattern);
	scan.NumBytes = numBytes;
	EmplaceScan(scan);
}

u64 gmAddressCache::GetScanResult(ConstString pattern)
{
	std::unique_lock lock(m_Mutex);

	u32* pIndex = m_PatternToScanIndex.TryGetAt(rageam::Hash(pattern));
	if (pIndex)
	{
		const Scan& scan = m_Scans[*pIndex];
		return OffsetToAddress(scan.ModuleOffset);
	}
	return 0;
}

gmAddress gmAddress::Scan(const char* patternStr, const char* debugName)
{
	static u64  s_ModuleSize;
	static u64  s_ModuleBase;
	static bool s_Init = false;
	if (!s_Init)
	{
		GetModuleBaseAndSize(&s_ModuleBase, &s_ModuleSize);
		s_Init = true;
	}

	gmAddressCache* cache = gmAddressCache::GetInstance();
	if (cache)
	{
		u64 cachedAddr = cache->GetScanResult(patternStr);
		if (cachedAddr != 0)
			return gmAddress(cachedAddr);
	}

	rageam::Timer timer = rageam::Timer::StartNew();

	// Convert string pattern into byte array form
	s16 pattern[256];
	u8 patternSize = 0;
	for (size_t i = 0; i < strlen(patternStr); i += 3)
	{
		const char* cursor = patternStr + i;

		if (cursor[0] == '?')
			pattern[patternSize] = -1;
		else
			pattern[patternSize] = static_cast<s16>(strtol(cursor, nullptr, 16));

		// Support single '?' (we're incrementing by 3 expecting ?? and space, but with ? we must increment by 2)
		if (cursor[1] == ' ')
			i--;

		patternSize++;
	}

	// In two-end comparison we approach from both sides (left & right) so size is twice smaller
	u8 scanSize = patternSize;
	if (scanSize % 2 == 0)
		scanSize /= 2;
	else
		scanSize = patternSize / 2 + 1;

	// Search for string through whole module
	// We use two-end comparison, nothing fancy but better than just brute force
	for (u64 i = 0; i < s_ModuleSize; i += 1)
	{
		const u8* modulePos = (u8*)(s_ModuleBase + i);
		for (u8 j = 0; j < scanSize; j++)
		{
			s16 lExpected = pattern[j];
			s16 lActual = modulePos[j];

			if (lExpected != -1 && lActual != lExpected) goto miss;

			s16 rExpected = pattern[patternSize - j - 1];
			s16 rActual = modulePos[patternSize - j - 1];

			if (rExpected != -1 && rActual != rExpected) goto miss;
		}
		timer.Stop();
		if (timer.GetElapsedMilliseconds() > MAX_SANE_SCAN_MS)
			AM_TRACEF("Pattern (%s, %llx) took %llums, too much! '%s'", debugName, i, timer.GetElapsedMilliseconds(), patternStr);
		if (cache)
			cache->AddScanResult(patternStr, i, patternSize);
		return { s_ModuleBase + i };
	miss:;
	}
	AM_UNREACHABLE("gmAddress::Scan(%s) -> Failed to find pattern %s", debugName, patternStr);
}
