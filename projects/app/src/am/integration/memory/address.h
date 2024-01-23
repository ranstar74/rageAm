//
// File: address.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/types.h"
#include "am/system/singleton.h"

class gmAddressCache : public rageam::Singleton<gmAddressCache>
{
	static constexpr ConstWString FILE_NAME = L"Memory.xml";

	struct Scan
	{
		u64 ModuleOffset;
		u32 PatternHash;
		u32 ByteHash;
		int NumBytes;
	};

	rageam::List<Scan>   m_Scans;
	rageam::HashSet<u32> m_PatternToScanIndex;
	u64                  m_ModuleBase;
	u64                  m_ModuleSize;
	std::mutex           m_Mutex;

	u32 ComputeByteHash(u64 offset, int numBytes) const;
	u64 OffsetToAddress(u64 offset) const { return m_ModuleBase + offset; }

	void EmplaceScan(Scan& scan);

	void SaveToXml() const;
	void ReadFromXml();

public:
	gmAddressCache();
	~gmAddressCache() override;

	void AddScanResult(ConstString pattern, u64 offset, int numBytes);
	u64  GetScanResult(ConstString pattern);
};

/**
 * \brief Provides compact utility to scan patterns and manipulate addresses.
 */
struct gmAddress
{
	static constexpr u64 MAX_SANE_SCAN_MS = 50;

	u64 Value = 0;

	gmAddress(u64 value) : Value(value) {}
	gmAddress() : gmAddress(0) {}

	static gmAddress Scan(const char* patternStr, const char* debugName = nullptr);

	// A bit of ugly address manipulations, here's short explanation how this works and used:
	//
	// Instruction that copies static variable value to RCX register (say it can be address of g_TxdStore)
	//		mov rcx, [7FF79A4DB628]
	// in byte representation it looks like this:
	//		48 8B 0D 49DD9501
	// What we are interested in is 4th value, which is offset from end of this instruction, not an absolute address
	// So in order to evaluate actual address we have to:
	// - Shift our address to offset
	// - Read it and add it to address
	// - Add 4 (offset is stored as 32bit integer) to resulting address
	// In C its implemented as: x + 4 + *(int*)x;
	//
	// Getting called function address is done the same way - call opcode takes 1 byte, so we offset address by 1

	gmAddress GetAt(s32 offset) const { return Value != 0 ? Value + offset : 0; }
	gmAddress GetRef(s32 offset = 0) const { return Value != 0 ? Value + offset + 4 + *(s32*)(Value + offset) : 0; }
	gmAddress GetCall() const { return GetRef(1); }

	template<typename T>
	T To() const { return (T)Value; }

	template<typename T>
	T* ToFunc() const { return To<T*>(); }

	gmAddress& operator=(u64 value) { Value = value; return *this; }
	operator u64() const { return Value; }
	operator pVoid() const { return (pVoid)Value; }
};
