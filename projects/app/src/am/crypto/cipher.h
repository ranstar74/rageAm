//
// File: cypher.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/integration/memory/address.h"
#include "am/system/asserts.h"

#ifdef AM_SOVIET_KEYS
#include "gta5aes.h"
#endif

#define CIPHER_KEY_ID_NONE 0x00000000 // No encryption key, CW supports it
#define CIPHER_KEY_ID_OPEN 0x4E45504F // "OPEN" - No encryption key from OpenIV
#define CIPHER_KEY_ID_AES  0x0FFFFFF9 // There are RPFs with AES shipped by mistake in GTAV, but they can't be loaded by the game
#define CIPHER_KEY_ID_TFIT 0x0FEFFFFF // "NG Encryption"
#define CIPHER_KEY_ID_SAVE 0x0FFFFFF5 // Used on game save files
#define CIPHER_TFIT_NUM_KEYS 101

namespace rageam::crypto
{
	/**
	 * \brief Cryptography keys is the last thing you'd want to upload publicly because of DMCA takedown risk.
	 * This interface was designed as an easy way to extend cipher functionality easily, as long as you can obtain the keys.
	 */
	class ICipher
	{
		static inline ICipher* sm_Instance = nullptr;

	public:
		virtual ~ICipher() = default;

		virtual bool CanEncrypt() const = 0;
		virtual bool Encrypt(u32 keyID, u32 selector, pVoid block, u32 blockSize) const = 0;
		virtual bool Decrypt(u32 keyID, u32 selector, pVoid block, u32 blockSize) const = 0;

		static ICipher* GetInstance()
		{
			AM_ASSERT(sm_Instance != nullptr, "Cipher is not set!");
			return sm_Instance;
		}
		static void SetInstance(ICipher* instance) { sm_Instance = instance; }
	};

#ifdef AM_INTEGRATED
	/**
	 * \brief 'Key-less' interface for public use that hooks decryption function in running game executable, the most legal way.
	 */
	class GTAVNativeCipher : public ICipher
	{
	public:
		// TFIT Encryption keys are not shipped with the game
		bool CanEncrypt() const override { return false; }
		bool Encrypt(u32 keyID, u32 selector, pVoid block, u32 blockSize) const override { return false; }

		bool Decrypt(u32 keyID, u32 selector, pVoid block, u32 blockSize) const override
		{
			static auto fn = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
				"44 89 4C 24 20 4C 89 44 24 18 89 54 24 10 89 4C 24 08 48 81 EC 38",
#else
				"48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 81 EC 20 02 00 00 8B EA",
#endif
				"rage::AES::Decrypt")
				.ToFunc<bool(u32, u32, pVoid, u32)>();
			return fn(keyID, selector, block, blockSize);
		}
	};
#endif

#ifdef AM_SOVIET_KEYS
	class GTAVSovietCipher : public ICipher
	{
	public:
		bool CanEncrypt() const override { return true; }
		bool Encrypt(u32 keyID, u32 selector, pVoid block, u32 blockSize) const override
		{
			return rage::AES::Encrypt(keyID, selector, block, blockSize);
		}
		bool Decrypt(u32 keyID, u32 selector, pVoid block, u32 blockSize) const override
		{
			return rage::AES::Decrypt(keyID, selector, block, blockSize);
		}
	};
#endif

#define CIPHER (*rageam::crypto::ICipher::GetInstance())
}
