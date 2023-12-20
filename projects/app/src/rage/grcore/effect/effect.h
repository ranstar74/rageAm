//
// File: effect.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/integration/memory/address.h"
#include "common/types.h"
#include "rage/atl/array.h"
#include "rage/crypto/joaat.h"
#include "rage/file/stream.h"
#include "rage/grcore/program.h"
#include "rage/grcore/texture.h"
#include "rage/math/vecv.h"

#define TECHNIQUE_DRAW "draw"
#define TECHNIQUE_DRAWSKINNED "drawskinned"

namespace rage
{
	class grcInstanceData;
	class grcEffect;

	inline grcInstanceData* LookupShaderPreset(u32 nameHash)
	{
		gmAddress addr = gmAddress::Scan("48 89 5C 24 08 4C 8B 05");
		return addr.To<decltype(&LookupShaderPreset)>()(nameHash);
	}

	inline grcEffect* FindEffectByHashKey(u32 nameHash)
	{
		gmAddress addr = gmAddress::Scan("48 89 5C 24 08 4C 63 1D ?? ?? ?? ?? 48");
		return addr.To<decltype(&FindEffectByHashKey)>()(nameHash);
	}

	inline void ResolveTexture(grcTexture** texture, datResource* rsc)
	{
		gmAddress addr = gmAddress::Scan("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 8B FA 48 8B 11 48");
		addr.To<decltype(&ResolveTexture)>()(texture, rsc);
	}

	template<typename T>
	void DeserializeProgramArray(const fiStreamPtr& stream, ConstString fileName, atArray<T>& programs)
	{
		u8 programCount = stream->ReadU8();
		programs.Resize(programCount);
		for (u8 i = 0; i < programCount; i++)
		{
			programs[i].Deserialize(stream, fileName);
		}
	}

	enum grcRenderFlags : u8
	{
		RF_VISIBILITY = 1 << 0,
		RF_SHADOWS = 1 << 1,
		RF_REFLECTIONS = 1 << 2,
		RF_MIRROR = 1 << 3,
		RF_TESSELLATION = 1 << 7,

		RF_ALL = 0xFF,
	};

	/**
	 * \brief Draw / Render mask is used to effectively render only models that we are need,
	 * for example we can create 'Target' render mask with Tessellated flag set to True and
	 * easily render only tessellated models, same idea applies to models that must cast shadows and so on.
	 */
	struct grcRenderMask
	{
		// Based on pong I can make assumption - tessellation, shadows, reflection are buckets too
		// This would make sense because in pong buckets actually have names and render mask is called 'DrawBucketMask'
		// Few names from pong for example:
		// - LevelDiffuse:		0
		// - LevelAlpha1:		1
		// - LevelAlpha2:		2
		// - LevelAlpha3:		3
		// - CharacterDiffuse:	4
		// - CharacterSkin:		5

		static constexpr u32 BASE_BUCKETS_MASK = 0xFF;
		static constexpr u32 RENDER_FLAGS_SHIFT = 8;

		u32 Mask;

		grcRenderMask()
		{
			if (!datResource::IsBuilding())
			{
				// Default render mask is unspecified draw bucket & all other options toggled on
				Mask = 0xFFFFFF00;
			}
		}
		grcRenderMask(u32 mask) { Mask = mask; }

		u8 GetDrawBucket() const { return BitScanR32(Mask & BASE_BUCKETS_MASK); }
		void SetDrawBucket(u32 bucket)
		{
			Mask &= ~BASE_BUCKETS_MASK;
			Mask |= 1 << bucket;
		}

		u8 GetRenderFlags() const { return static_cast<u8>(Mask >> RENDER_FLAGS_SHIFT & 0xFF); }
		void SetRenderFlags(u8 flags)
		{
			Mask &= ~(0xFF << RENDER_FLAGS_SHIFT); // Erase existing
			Mask |= static_cast<u32>(flags) << RENDER_FLAGS_SHIFT;
		}

		bool GetTesellated() const { return Mask & (RF_TESSELLATION << RENDER_FLAGS_SHIFT); }
		void SetTessellated(bool toggle)
		{
			Mask &= ~(RF_TESSELLATION << RENDER_FLAGS_SHIFT);
			if (toggle) Mask |= (RF_TESSELLATION << RENDER_FLAGS_SHIFT);
		}

		// Checks if this render mask satisfies target
		bool DoTest(grcRenderMask target) const
		{
			return Mask & target.Mask != 0;
		}

		operator u32() const { return Mask; }
	};

	/**
	 * \brief Supported shader value types.
	 */
	enum grcEffectVarType
	{
		EFFECT_VALUE_NONE = 0,		// 
		EFFECT_VALUE_INT = 1,		// 4 bytes
		EFFECT_VALUE_FLOAT = 2,		// 4 bytes
		EFFECT_VALUE_VECTOR2 = 3,	// 8 bytes
		EFFECT_VALUE_VECTOR3 = 4,	// 16 bytes (used only 3)
		EFFECT_VALUE_VECTOR4 = 5,	// 16 bytes
		EFFECT_VALUE_TEXTURE = 6,	// grcTexture*, 8 bytes
		EFFECT_VALUE_BOOL = 7,		// 1 byte
		EFFECT_VALUE_MATRIX34 = 8,	// 
		EFFECT_VALUE_MATRIX44 = 9,	// 64 bytes, 4x4 matrix (see vehicle_tire.fxc)
		EFFECT_VALUE_STRING = 10,	// 
		EFFECT_VALUE_INT1 = 11,		// int[1]
		EFFECT_VALUE_INT2 = 12,		// int[2]
		EFFECT_VALUE_INT3 = 13,		// int[3]
		EFFECT_VALUE_INT4 = 14,		// int[4]
	};
	ConstString EffectValueTypeToString(grcEffectVarType e);
	
	static u8 grcEffectVarSize[40]
	{
		0, 4, 4, 8, 12, 16, 0, 4, 16, 16,	// NONE to MATRIX44
		0, 4, 8, 12, 16, 0, 0, 4, 4, 4, 4,	// STRING To ...
		// 20 - 39, I'm not sure if those are used
		0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0
	};

	class grcInstanceVar
	{
		friend class grcInstanceData;

		// Count
		// Register
		// SamplerStateSet
		// Flags
		// Texture
	public:
		uint8_t m_ValueCount;
		int8_t unk1;
		uint8_t SamplerIndex;
		int8_t unk3;
		int32_t unk4;

		// Use char here because void ends up with compiler error, but actual type is void*
		//pgUPtr<char> Data;
		char* m_ValuePtr;

	public:
		grcInstanceVar() = default;

		// ReSharper disable once CppPossiblyUninitializedMember
		grcInstanceVar(const datResource& rsc);

		u8 GetValueCount() const { return m_ValueCount; }

		template<typename T>
		T* GetValuePtr() { return reinterpret_cast<T*>(m_ValuePtr); }
		template<typename T>
		T GetValue() { return *GetValuePtr<T>(); }

		// Do not use this for textures! See SetTexture;
		template<typename T>
		void SetValue(const T& value)
		{
			memcpy(m_ValuePtr, &value, sizeof(T));
		}

		grcTexture* GetTexture() { return GetValuePtr<grcTexture>(); }
		void SetTexture(grcTexture* texture)
		{
			m_ValuePtr = (char*)texture;
		}

		// If you need exact variable type, lookup variable with corresponding index in effect
		// See grcInstanceData::GetEffect, grcEffect::GetVarByIndex
		bool IsTexture() const
		{
			// Texture vars don't have data block so we can distinct them easily from other var types
			return m_ValueCount == 0;
		}
	};

	struct grcEffectPass
	{
		uint8_t VS;
		uint8_t PS;
		uint8_t CS;
		uint8_t DS;
		uint8_t GS;
		uint8_t HS;
		uint8_t uint6;
		uint8_t uint7;
		uint8_t uint8;
		uint8_t uint9;
		uint8_t uintA;
		uint8_t uintB;
	};

	class grcEffectTechnique
	{
	public:
		u32 m_NameHash;
		ConstString m_Name;
		grcEffectPass* m_Passes;
		uint16_t m_PassCount;
		int16_t unk1A;
		int16_t unk1C;
		int16_t unk20;
	};

	struct grcEffectAnnotation
	{
		u64 qword0;
		u64 qword8;
	};

	struct grcEffectVar
	{
	public: // TODO: Temp
		u8 m_DataType;
		u8 byte1;
		u8 byte2;
		u8 byte3;
		ConstString m_Name;
		ConstString m_DisplayName;
		u32 m_NameHash;
		u32 m_DisplayNameHash;
		grcEffectAnnotation* qword20;
		void* pvoid28;
		u16 m_UnknownData30;
		u8 m_SamplerIndex;
		u8 gap33;
		// Offset in case of CBuffer variable or Slot in case of texture
		u32 m_SlotOrOffset;
		u32 CBufferNameHash;
		grcConstantBuffer* CBuffer;

		ConstString GetName() const { return m_Name; }
		ConstString GetDisplayName() const { return m_DisplayName; }
		u32 GetNameHash() const { return m_NameHash; }
		u32 GetDisplayNameHash() const { return m_DisplayNameHash; }

		grcEffectVarType GetType() const { return grcEffectVarType(m_DataType); }
		bool IsTexture() const { return GetType() == EFFECT_VALUE_TEXTURE; }

		u32 GetValueSize() const { return grcEffectVarSize[m_DataType]; }

		void Deserialize(const fiStreamPtr& stream)
		{
			//byte0 = stream->ReadU8();
			//byte1 = stream->ReadU8();
			//auto Char = stream->ReadU8();
			//this->word30 &= 0xFFC0u;
			//this->word30 = (Char >> 6 << 6) | this->word30 & 0xFF3F | Char & 0x3F;

			//auto v9 = (this->word30 | ((stream->ReadU8() & 0x7F) << 8)) & 0x7FFF;
			//this->word30 = v9;
			///*if (!gRageDirect3dDeviceInst)
			//	this->word30 = v9 & 0x80FF;*/




			///*sub_7FF71FC0D338(stream, &this->Name1);
			//this->dword18 = rage::atStringHash(this->Name1, 0);
			//sub_7FF71FC0D338(stream, &this->Name2);
			//v10 = rage::atStringHash(this->Name2, 0);
			//v63 = this->byte0 == 6;
			//this->dword1C = v10;*/
			//u8 nameSize1 = stream->ReadU8();
			//char* name1 = new char[nameSize1];
			//stream->Read(name1, nameSize1);

			//u8 nameSize2 = stream->ReadU8();
			//char* name2 = new char[nameSize2];
			//stream->Read(name2, nameSize2);
		}
	};

	// Effect handles (which is just index + 1) were used most likely to make API more similar to DirectX FX,
	// where NULL indicating invalid handle

#define INVALID_FX_HANDLE 0
	using fxHandle_t = u16;
	inline fxHandle_t fxIndexToHandle(u16 index) { return index + 1; }
	inline u16 fxHandleToIndex(fxHandle_t handle)
	{
		AM_ASSERT(handle != 0, "ConvertHandleToIndex() -> Handle was NULL.");
		return static_cast<u16>(handle - 1);
	}

	/**
	 * \brief In simple words - Material.
	 * Contains per-instance (i.e. per spawned drawable) shader effect values.
	 */
	class grcInstanceData
	{
	protected:
		// TODO: If we make it smart pointer, its gonna fixup only first array item, what do we do?
		grcInstanceVar* m_Vars; // Data

		// Hash is used in serialized resource,
		// during deserializing it gets resolved to actual effect
		union
		{
			grcEffect* Ptr;
			u32 NameHash;
		} m_Effect;

		u8		m_VarCount;
		u8		m_DrawBucket;
		u8		m_Unknown12;
		// Known:
		// 1 << 0 - OwnPreset, preset pointer lifetime maintained by this instance
		u8		m_Flags;
		// TODO: There's also var name hashes block after grcInstanceVar
		// m_Vars is memory block that contains two things -
		// variables array and their values that are placed right at the end of it
		// So grcInstanceVar::m_ValuePtr point to second half of this block
		// This was made mainly to minimize memory fragmentation
		// by putting all allocations into one continuous heap
		u16		m_VarsSize;				// SpuSize
		u16		m_VarsAndValuesSize;	// TotalSize

		union
		{
			grcInstanceData* Ptr;
			u32 NameHash;
		} m_Preset;

		grcRenderMask m_DrawBucketMask;

		bool	m_IsInstancedDraw;
		u8		m_Unknown25;
		u8		m_Unknown26;
		u8		m_TextureVarCount;
		u64		m_Unknown28;

		void CleanUp();

	public:
		grcInstanceData() = default;
		grcInstanceData(const grcEffect* effect);
		grcInstanceData(const grcInstanceData& other);
		grcInstanceData(grcInstanceData&& other) noexcept = default;
		grcInstanceData(const datResource& rsc);
		~grcInstanceData();

		void CloneFrom(const grcInstanceData& other);

		// Looks up tessellation variable and sets appropriate bucket mask if it's used
		void UpdateTessellationBucket();

		// Useful when need to transfer existing parameters to instance data that uses different effect
		void CopyVars(const grcInstanceData* to) const;

		void SetTessellated(bool toggle) { m_DrawBucketMask.SetTessellated(toggle); }
		bool IsTessellated() const { return m_DrawBucketMask.GetTesellated(); }
		bool IsInstancedDraw() const { return m_IsInstancedDraw; }

		grcInstanceData* GetPreset() const { return m_Preset.Ptr; }

		grcEffect* GetEffect() const { return m_Effect.Ptr; }
		void SetEffect(grcEffect* effect, bool keepValues = true);

		u16 GetVarCount() const { return m_VarCount; }
		grcInstanceVar* GetVarByIndex(u16 index) const
		{
			AM_ASSERT(index < m_VarCount, "grcInstanceData::GetVar() -> Index %u is out of range.", index);
			return &m_Vars[index];
		}
		grcInstanceVar* GetVar(fxHandle_t handle) const { return GetVarByIndex(fxHandleToIndex(handle)); }

		grcRenderMask& GetDrawBucketMask() { return m_DrawBucketMask; }
		u8 GetDrawBucket() const { return m_DrawBucket; }
		void SetDrawBucket(u8 bucket)
		{
			m_DrawBucket = bucket;
			m_DrawBucketMask.SetDrawBucket(bucket);
		}

		u16 GetTextureVarCount() const { return m_TextureVarCount; }
	};

	class grcEffect
	{
	public:
		atArray<grcEffectTechnique> m_Techniques;
		atArray<grcEffectVar> m_Variables;
		atArray<grcEffectVar*> m_Locals;
		atArray<grcVertexProgram> m_VertexPrograms;
		atArray<grcFragmentProgram> m_FragmentPrograms;
		//u32 dword4C;
		u64 m_TechniqueHandles[64];
		u64 m_Heap;
		u32 dword258;
		int32_t unk25c;
		char m_Name[40];
		grcInstanceData m_InstanceDataTemplate;
		u64 m_FileTime;
		const char* m_FilePath;
		u32 m_NameHash;
		//int32_t dword2CC;

		//struct struct2D8
		//{
		//	u32 Hash;
		//	u32 dword4;
		//	pVoid Block;

		//	void FromStream(const fiStreamPtr& stream)
		//	{
		//		u8 unk = stream->ReadU8();
		//		if (unk == 2)
		//		{

		//		}
		//	}
		//};
		//atArray<struct2D8> m_Array2D8;

		//int32_t qword2dc;
		//u32 dword2e0;
		//int16_t m_Unknown2E4;
		//int16_t word2e6;
		//u64 qword2E8;
		//u32 dword2F0;
		//u8 gap2F4[4];

		int32_t dword2CC;
		u64 qword2D0;
		u16 word2D8;
		u16 word2DA;
		int32_t qword2dc;
		int32_t dword2e0;
		int16_t m_Unknown2E4;
		int16_t word2e6;
		u64 qword2E8;
		u32 dword2F0;
		u8 gap2F4[4];

		atArray<grcComputeProgram>	m_ComputePrograms;
		atArray<grcDomainProgram>	m_DomainPrograms;
		atArray<grcGeometryProgram> m_GeometryPrograms;
		atArray<grcHullProgram>		m_HullPrograms;

	public:
		ConstString GetName() const { return m_Name; }
		u32 GetNameHash() const { return m_NameHash; }

		u16 GetVarCount() const { return m_Variables.GetSize(); }
		grcEffectVar* GetVarByIndex(u16 index) { return &m_Variables.Get(index); }
		grcEffectVar* GetVar(fxHandle_t handle) { return GetVarByIndex(fxHandleToIndex(handle)); }

		const grcInstanceData& GetInstanceDataTemplate() const { return m_InstanceDataTemplate; }

		void FromStream(const fiStreamPtr& stream, ConstString fileName)
		{
			//dword2e0 = stream->ReadU32();
			//u8 count2DA = stream->ReadU8();
			//m_Array2D8.Reserve(count2DA);
			//for (u8 i = 0; i < count2DA; i++)
			//{

			//}

			//DeserializeProgramArray(stream, fileName, m_VertexPrograms);
			//DeserializeProgramArray(stream, fileName, m_FragmentPrograms);
			//DeserializeProgramArray(stream, fileName, m_ComputePrograms);
			//DeserializeProgramArray(stream, fileName, m_DomainPrograms);
			//DeserializeProgramArray(stream, fileName, m_GeometryPrograms);
			//DeserializeProgramArray(stream, fileName, m_HullPrograms);

			//u8 cBufferCount = stream->ReadU8();
			//for (u8 i = 0; i < cBufferCount; i++)
			//{
			//	// TODO: Pool
			//	grcConstantBuffer buffer;
			//	buffer.Deserialize(stream);
			//}

			// Var
		/*	u8 uCount = stream->ReadU8();
			for(u8 i = 0; i < uCount; i++)
			{
				struct_a1 a{};
				a.Deserialize(stream);
			}*/
		}

		fxHandle_t LookupVarByName(ConstString name) { return LookupVarByHashKey(joaat(name)); }
		fxHandle_t LookupVarByHashKey(u32 hash)
		{
			for (u16 i = 0; i < m_Variables.GetSize(); i++)
			{
				grcEffectVar* var = &m_Variables[i];
				if (var->GetNameHash() == hash || var->GetDisplayNameHash() == hash)
					return fxIndexToHandle(i);
			}
			return INVALID_FX_HANDLE;
		}

		fxHandle_t LookupTechniqueByHashKey(u32 hash)
		{
			for (u16 i = 0; i < m_Techniques.GetSize(); i++)
			{
				if (m_Techniques[i].m_NameHash == hash)
					return fxIndexToHandle(i);
			}
			return INVALID_FX_HANDLE;
		}

		fxHandle_t LookupTechnique(ConstString name)
		{
			return LookupTechniqueByHashKey(joaat(name));
		}

		grcEffectTechnique* GetTechniqueByHandle(fxHandle_t handle)
		{
			return &m_Techniques[fxHandleToIndex(handle)];
		}

		grcEffectVar* GetVarByHandle(fxHandle_t handle)
		{
			u16 index = fxHandleToIndex(handle);
			return &m_Variables[index];
		}

		grcEffectTechnique* GetTechnique(ConstString name) const
		{
			u32 hash = joaat(name);
			for (grcEffectTechnique& technique : m_Techniques)
			{
				if (technique.m_NameHash == hash)
					return &technique;
			}
			return nullptr;
		}

		grcEffectVar* GetVar(ConstString name, u16* outIndex = nullptr)
		{
			if (outIndex) *outIndex = u16(-1);

			u32 hash = joaat(name);
			for (u16 i = 0; i < m_Variables.GetSize(); i++)
			{
				grcEffectVar& var = m_Variables[i];
				if (var.GetNameHash() == hash || var.GetDisplayNameHash() == hash)
				{
					if (outIndex) *outIndex = i;
					return &var;
				}
			}
			return nullptr;
		}

		grcVertexProgram* GetVS(const grcEffectTechnique* technique)
		{
			return &m_VertexPrograms[technique->m_Passes[0].VS];
		}

		void GetVarInfo(fxHandle_t handle, u32* outDisplayNameHash, u32* outNameHash, grcEffectVarType* outType)
		{
			grcEffectVar* var = GetVarByHandle(handle);
			if (outDisplayNameHash)
				*outDisplayNameHash = var->GetDisplayNameHash();
			if (outNameHash)
				*outNameHash = var->GetNameHash();
			if (outType)
				*outType = var->GetType();
		}

		// -- Draw functions

		void BeginPass(u32 passIndex, grcInstanceData* instanceData) const
		{
			static u16(*fn)(const grcEffect*, u32, grcInstanceData*) =
				gmAddress::Scan("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 50 89 15 ?? ?? ?? ?? 8B").To<decltype(fn)>();
			fn(this, passIndex, instanceData);
		}

		void EndPass() const
		{
			static u16(*fn)(const grcEffect*) =
				gmAddress::Scan("48 83 EC 28 8A 15 ?? ?? ?? ?? 48").To<decltype(fn)>();
			fn(this);
		}
	};
	// No smart pointers, shaders (effects) are loaded once and never unloaded
	using grcEffectPtr = grcEffect*;
}
