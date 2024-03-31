//
// File: effect.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/atl/array.h"
#include "rage/atl/hashstring.h"
#include "rage/file/stream.h"
#include "rage/grcore/texture.h"
#include "rage/paging/ref.h"
#include "rage/system/memcontainer.h"
#include "drawbucket.h"

#include "am/integration/memory/address.h"

#define TECHNIQUE_DRAW "draw"
#define TECHNIQUE_DRAWSKINNED "drawskinned"

namespace rage
{
	class grcHullProgram;
	class grcGeometryProgram;
	class grcDomainProgram;
	class grcComputeProgram;
	class grcFragmentProgram;
	class grcVertexProgram;
	class grcInstanceData;
	class grcEffect;
	class grcRenderTargetDX11;
	class grcBufferD3D11;
	class grcBufferD3D11Resource;

	typedef grcBufferD3D11			grcBufferBasic;
	typedef grcBufferD3D11Resource	grcBufferUAV;
	typedef grcRenderTargetDX11		grcTextureUAV;

	static constexpr u32 GRC_CACHE_LINE_SIZE = 64;

	/**
	 * \brief Effect local parameter types.
	 */
	enum grcEffectVarType
	{
		VT_NONE				   = 0,	 // 
		VT_INT				   = 1,	 // 4 bytes
		VT_FLOAT			   = 2,	 // 4 bytes
		VT_VECTOR2			   = 3,	 // 8 bytes
		VT_VECTOR3			   = 4,	 // 16 bytes (used only 3)
		VT_VECTOR4			   = 5,	 // 16 bytes
		VT_TEXTURE			   = 6,	 // grcTexture*, 8 bytes
		VT_BOOL				   = 7,	 // 1 byte
		VT_MATRIX34			   = 8,	 // 
		VT_MATRIX44			   = 9,  // 64 bytes, 4x4 matrix (see vehicle_tire.fxc)
		VT_STRING			   = 10, // 
		VT_INT1				   = 11, // int[1]
		VT_INT2				   = 12, // int[2]
		VT_INT3				   = 13, // int[3]
		VT_INT4				   = 14, // int[4]
		VT_STRUCTURED_BUFFER   = 15,
		VT_SAMPLER_STATE	   = 16,
		VT_UNUSED1			   = 17,
		VT_UNUSED2			   = 18,
		VT_UNUSED3			   = 19,
		VT_UNUSED4			   = 20,
		VT_UAV_STRUCTURED	   = 21,
		VT_UAV_TEXTURE		   = 22,
		VT_BYTE_ADDRESS_BUFFER = 23,
		VT_UAV_BYTE_ADDRESS	   = 24,
		VT_COUNT			   = 25,
	};
	// Size of data in single group
	static u8 grcEffectVarTypeToSize[]
	{
		0, 4, 4, 8, 12, 16, 0, 4, 16, 16,	// NONE				 to MATRIX44
		0, 4, 8, 12, 16, 0, 0, 4, 4, 4, 4,	// STRING			 to VT_UNUSED4
		0, 0, 0, 0							// STRUCTURED_BUFFER to VT_UAV_BYTE_ADDRESS
	};
	static_assert(ARRAYSIZE(grcEffectVarTypeToSize) == VT_COUNT);
	// Effect vars are stored in 16 byte groups. Int will take 1 group (16 bytes), but matrix44 - 4 groups (64 bytes)
	static u8 grcEffectVarTypeToBlockCount[]
	{
		0, 1, 1, 1, 1, 1, 0, 1, 4, 4,		// NONE				 to MATRIX44
		0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1,	// STRING			 to VT_UNUSED4
		0, 0, 0, 0							// STRUCTURED_BUFFER to VT_UAV_BYTE_ADDRESS
	};
	static_assert(ARRAYSIZE(grcEffectVarTypeToBlockCount) == VT_COUNT);

	enum grcShaderType
	{
		grcShaderVS,
		grcShaderPS,
		grcShaderCS,
		grcShaderDS,
		grcShaderGS,
		grcShaderHS,
		grcShaderCount,
	};

	enum grcVarUsage
	{
		grcVarUsageVertexProgram   = 1 << 0,
		grcVarUsageFragmentProgram = 1 << 1,
		grcVarUsageGeometryProgram = 1 << 2,
		grcVarUsageHullProgram     = 1 << 3,
		varVarUsageDomainProgram   = 1 << 4,
		grcVarUsageComputeProgram  = 1 << 5,
		grcVarUsageAnyProgram      = (1 << 6) - 1,
		grcVarUsageMaterial        = 1 << 6,		// Parameter is per-material, not per-instance
		grcVarUsageComparison	   = 1 << 7,		// Parameter is sampler with comparison filter
	};

	inline grcInstanceData* LookupShaderPreset(u32 nameHash)
	{
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		static auto fn = gmAddress::Scan("48 83 EC 38 48 C7 44 24 20 00 00 00 00 48 83 3D", "rage::grcEffect::LookupMaterial+0x4")
			.GetAt(-0x4).ToFunc<grcInstanceData* (u32)>();
		return fn(nameHash);
#else
		static gmAddress addr = gmAddress::Scan("48 89 5C 24 08 4C 8B 05", "rage::grcEffect::LookupMaterial");
		return addr.To<decltype(&LookupShaderPreset)>()(nameHash);
#endif
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

	/**
	 * \brief rage::grcInstanceData::Entry
	 */
	struct grcInstanceVar
	{
		u8 ValueCount;
		u8 Register;
		u8 SamplerIndex;
		u8 SavedSamplerIndex;

		union // NOTE: We can't put Vector types here because max item size must be 8 bytes
		{
			char*		Value;
			int			Int;
			float		Float;
			grcTexture* Texture;
		};

		grcInstanceVar();

		void CopyFrom(grcInstanceVar* other);

		u8 GetValueCount() const { return ValueCount; }

		template<typename T>
		T* GetValuePtr() { return reinterpret_cast<T*>(Value); }
		template<typename T>
		T GetValue() { return *GetValuePtr<T>(); }

		// Do not use this for textures! See SetTexture;
		template<typename T>
		void SetValue(const T& value)
		{
			memcpy(Value, &value, sizeof(T));
		}

		grcTexture* GetTexture() { return GetValuePtr<grcTexture>(); }
		void SetTexture(grcTexture* texture)
		{
			Value = (char*)texture;
		}

		// If you need exact variable type, lookup variable with corresponding index in effect
		// See grcInstanceData::GetEffect, grcEffect::GetVarByIndex
		bool IsTexture() const
		{
			// Texture vars don't have data block, we can easily them distinct from other var types
			return ValueCount == 0;
		}
	};

	/**
	 * \brief Constant buffer.
	 */
	class grcCBuffer
	{
		struct Header
		{
			u32  Reserved;
			bool IsDirty;
			bool IsLocked;
			bool IsFirstUse;
		};

		u32                    m_Size;
		u16                    m_ShaderSlots[grcShaderCount];
		u32                    m_NameHash;
		atConstString          m_Name;
		amComPtr<ID3D11Buffer> m_BufferObject;
		// Local copy of the buffer, not really used on pc
		char*                  m_AlignedBackingStore; // Aligned to cache line
		char*                  m_AllocatedBackingStore;
		// Used only for multithreaded rendering, where we have buffer per thread
		// on win32 D3D11 there's only single rendering thread
		u32                    m_BufferStride;

		Header* GetHeader() const { return (Header*) m_AlignedBackingStore; }

	public:
		grcCBuffer() = default;
		~grcCBuffer();

		ConstString GetName() const { return m_Name; }
		u32			GetNameHash() const { return m_NameHash; }

		u32	  GetSize() const { return m_Size; }
		bool  IsLocked() const { return GetHeader()->IsLocked; }
		pVoid Lock() const;
		void  Unlock() const;
		
		void Init();
		void Load(const fiStreamPtr& stream);

		u32 ComputeHashKey() const;
	};

	/**
	 * \brief rage::grcParameter
	 */
	struct grcEffectVar
	{
		struct Annotation
		{
			enum AnnoType { AT_INT, AT_FLOAT, AT_STRING };

			AnnoType Type;
			u32		 NameHash;
			union
			{
				int			  Int;
				float		  Float;
				atConstString String;
			};

			Annotation();
			~Annotation();

			void Load(const fiStreamPtr& stream);
		};

		enum TextureSubtype
		{
			TEXTURE_SAMPLER, // texture IS a sampler
			TEXTURE_REGULAR, // texture with an associated sampler
			TEXTURE_PURE,	 // texture with no sampler
			TEXTURE_MSAA,	 // MSAA texture (with no sampler)
		};

		static constexpr u8 TEXTURE_REGISTER_BITS = 6;
		static constexpr u8 TEXTURE_TYPE_BITS = 8 - TEXTURE_REGISTER_BITS;
		static constexpr u8 TEXTURE_USAGE_BITS = 7;
		static constexpr u8 TEXTURE_COMPARISON_BITS = 8 - TEXTURE_USAGE_BITS;

		u8            m_Type;
		u8            m_Count;
		u8            m_DataSize;
		u8            m_AnnotationCount;
		atConstString m_Name;
		atConstString m_Semantic;
		u32           m_NameHash;
		u32           m_SemanticHash;
		Annotation*   m_Annotations;
		char*         m_Data; // Default parameter value, string in case of texture
		u16           m_Register         : TEXTURE_REGISTER_BITS;
		u16           m_TextureType      : TEXTURE_TYPE_BITS;
		u16           m_Usage            : TEXTURE_USAGE_BITS;
		u16           m_ComparisonFilter : TEXTURE_COMPARISON_BITS;
		u8            m_SamplerIndex;
		u8            m_SavedSamplerIndex;
		// Offset in case of CBuffer variable or Slot in case of texture
		u32           m_SlotOrCBufferOffset;
		u32           m_CBufferNameHash;
		union
		{
			grcCBuffer*          CBuffer;
			const grcTexture*    Texture;
			const grcBufferUAV*  RO_Buffer;
			grcBufferUAV*        RW_Buffer;
			const grcTextureUAV* RW_Texture;
		};

		ConstString GetName() const { return m_Name; }
		ConstString GetSemantic() const { return m_Semantic; }
		u32         GetNameHash() const { return m_NameHash; }
		u32         GetSemanticHash() const { return m_SemanticHash; }

		grcEffectVarType GetType() const { return grcEffectVarType(m_Type); }
		bool			 IsTexture() const { return GetType() == VT_TEXTURE; }

		u32 GetValueSize() const { return grcEffectVarTypeToSize[m_Type]; }

		void Load(int type, u32 textureNameHash, const fiStreamPtr& stream);
	};

	/**
	 * \brief Effect handles (just index + 1). NULL indicating invalid handle.
	 */
	struct grcHandle
	{
		int Handle;

		explicit grcHandle(u16 index) { Handle = index + 1; }

		bool IsValid() const { return Handle > 0; }
		bool IsNull() const { return Handle == 0; }
		u16	 GetIndex() const
		{
			AM_ASSERT(Handle != 0, "fxHandle::GetIndex() -> Handle was NULL.");
			return u16(Handle - 1);
		}

		bool operator!() const { return IsNull(); }

		static grcHandle Null() { return grcHandle(0); }
	};

	/**
	 * \brief In simple words - Material.
	 * Contains per-instance (i.e. per spawned drawable) shader effect values.
	 */
	class grcInstanceData
	{
	protected:
		enum
		{
			FLAG_OWN_MATERIAL = 1 << 0,
		};

		// Single data block containing:
		// - Array of grcInstanceVar
		// - 16 byte groups of data for holding grcInstanceVar::Value (except for textures)
		// - Variable name hashes
		grcInstanceVar* m_Vars;

		// Hash is used in serialized resource,
		// during deserializing it gets resolved to actual effect
		union
		{
			grcEffect* Ptr;
			u32		   NameHash;
		} m_Effect;

		u8  m_VarCount;
		u8  m_DrawBucket;
		u8  m_PhysMtl;
		u8  m_Flags;
		u16 m_VarsSize;
		u16 m_TotalSize;

		union
		{
			grcInstanceData* Ptr;
			u32				 NameHash;
			ConstString		 Name;
		} m_Preset;

		grcDrawMask m_DrawBucketMask;
		bool        m_IsInstancedDraw;
		u8          m_UserFlags;
		u8          m_Padding;
		u8          m_TextureCount;
		u64         m_SortKey;

		void Destroy();
		u32* GetVarNameHashes() const { return (u32*)((char*) m_Vars + m_VarsSize); }
		void RestoreValues(const u32* origNameHashes, grcInstanceVar* origVars, u8 origVarCount) const;

	public:
		grcInstanceData() = default;
		grcInstanceData(const grcEffect* effect);
		grcInstanceData(const grcInstanceData& other);
		grcInstanceData(grcInstanceData&& other) noexcept = default;
		grcInstanceData(const datResource& rsc);
		~grcInstanceData();

		void CloneFrom(const grcInstanceData& other);
		// Useful when need to transfer existing parameters to instance data that uses different effect
		void CopyVars(const grcInstanceData* to) const;

		// Looks up tessellation variable and sets appropriate bucket mask if it's used
		void UpdateTessellationBucket();

		void SetTessellated(bool toggle) { m_DrawBucketMask.SetTessellated(toggle); }
		bool IsTessellated() const { return m_DrawBucketMask.GetTesellated(); }
		bool IsInstancedDraw() const { return m_IsInstancedDraw; }

		grcInstanceData* GetPreset() const { return m_Preset.Ptr; }

		grcEffect* GetEffect() const { return m_Effect.Ptr; }
		void       SetEffect(const grcEffect* effect, bool keepValues = true);

		u16             GetVarCount() const { return m_VarCount; }
		grcInstanceVar* GetVar(u16 index) const;
		grcInstanceVar* GetVar(grcHandle handle) const { return GetVar(handle.GetIndex()); }
		// Texture variables are always placed first
		u16	GetTextureCount() const { return m_TextureCount; }

		u32  GetDrawBucketMask() const { return m_DrawBucketMask; }
		void SetDrawBucketMask(u32 mask) { m_DrawBucketMask = mask; }

		u8   GetDrawBucket() const { return m_DrawBucket; }
		void SetDrawBucket(u8 bucket)
		{
			m_DrawBucket = bucket;
			m_DrawBucketMask.SetDrawBucket(bucket);
		}

		ConstString GetPresetName() const { return m_Preset.Ptr ? m_Preset.Ptr->m_Preset.Name : nullptr; }

		class Iterator
		{
			grcInstanceVar* m_Begin;
			grcInstanceVar* m_End;
		public:
			Iterator(grcInstanceVar* begin, grcInstanceVar* end) : m_Begin(begin), m_End(end) {}

			grcInstanceVar* begin() const { return m_Begin; }
			grcInstanceVar* end() const { return m_End; }
		};
		enum
		{
			ITERATOR_TEX_MODE_INCLUDE, // All variables
			ITERATOR_TEX_MODE_EXCLUDE, // Ignore texture variables
			ITERATOR_TEX_MODE_ONLY,	   // Only texture variables
		};
		Iterator GetIterator(int texMode = ITERATOR_TEX_MODE_INCLUDE) const
		{
			switch (texMode)
			{
				// Textures always placed first
				case ITERATOR_TEX_MODE_INCLUDE: return Iterator(m_Vars, m_Vars + m_VarCount);
				case ITERATOR_TEX_MODE_EXCLUDE: return Iterator(m_Vars + m_TextureCount, m_Vars + m_VarCount);
				case ITERATOR_TEX_MODE_ONLY:	return Iterator(m_Vars, m_Vars + m_TextureCount);
				default: AM_UNREACHABLE("grcInstanceData::GetIterator() -> Unknown tex mode '%i'!", texMode);
			}
		}
		Iterator GetTextureIterator() const { return GetIterator(ITERATOR_TEX_MODE_ONLY); }
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
	struct grcVertexDeclaration;
	class fiStreamPtr;

	enum grcProgramType
	{
		GRC_PROGRAM_VERTEX,
		GRC_PROGRAM_PIXEL,
		GRC_PROGRAM_COMPUTE,
		GRC_PROGRAM_GEOMETRY,
		GRC_PROGRAM_HULL,
		GRC_PROGRAM_DOMAIN,
	};

	ID3D11DeviceChild* grcCreateProgramByTypeCached(ConstString name, grcProgramType type, pVoid bytecode, u32 bytecodeSize);

	// Helper function for effect deserializing.
	// Reads string length, string itself and computes atStringHash.
	u32 ReadStringAndComputeHash(const fiStreamPtr& stream);

	/**
	 * \brief Base class for GPU programs (shaders).
	 */
	class grcProgram
	{
	public:  // TODO: Test

		char* m_Name = nullptr;
		atArray<u32> m_VarNameHashes;

		ID3D11Buffer** m_CbResources;
		u32 m_CbHashSum;
		u8 m_CbMinSlot;
		u8 m_CbMaxSlot;
		u32 m_CbCount;
		grcCBuffer* m_CBuffers[14];

		u32 dwordA8;
		u8 gapAC[4];
		char charB0[368];
	public:
		grcProgram() = default;
		virtual ~grcProgram();

		virtual u32 GetBytecodeSize() = 0;

		// ReSharper disable once CppHiddenFunction
		void Deserialize(const fiStreamPtr& stream, ConstString fileName);

		ConstString GetName() const { return m_Name; }
	};

	class grcVertexProgram : public grcProgram
	{
	public:  // TODO: Test
		// For caching input layout
		struct DeclNode
		{
			grcVertexDeclaration* Declaration;
			ID3D11InputLayout* Layout;
			DeclNode* Next;
		};

		// There's actually two of them... not sure why (in all program types)
		// Possibly to quickly swap Debug/Release shaders? Who knows...
		amComPtr<ID3D11VertexShader> m_Shader_;
		amComPtr<ID3D11VertexShader> m_Shader;

		u32	m_BytecodeSize = 0;
		pVoid m_Bytecode = nullptr;

		// Cached input layouts are stored for every vertex declaration in linked list
		DeclNode* m_DeclList = nullptr;
	public:
		~grcVertexProgram() override;

		u32 GetBytecodeSize() override { return m_BytecodeSize; }

		// ReSharper disable once CppHidingFunction
		void Deserialize(const fiStreamPtr& stream, ConstString fileName);
		bool HasByteCode() const { return m_Bytecode != nullptr; }
		void ReflectVertexFormat(grcVertexDeclaration** ppDecl, grcFvf* pFvf = nullptr) const;
	};

	class grcFragmentProgram : public grcProgram
	{
		amComPtr<ID3D11PixelShader> m_Shader_;
		amComPtr<ID3D11PixelShader> m_Shader;
		u32 m_BytecodeSize = 0;
	public:
		u32 GetBytecodeSize() override { return m_BytecodeSize; }

		// ReSharper disable once CppHidingFunction
		void Deserialize(const fiStreamPtr& stream, ConstString fileName);
	};

	class grcComputeProgram : public grcProgram
	{
		amComPtr<ID3D11ComputeShader> m_Shader_;
		amComPtr<ID3D11ComputeShader> m_Shader;
		u32 m_BytecodeSize = 0;
		u32 m_Unused234 = 0;
	public:
		u32 GetBytecodeSize() override { return m_BytecodeSize; }

		// ReSharper disable once CppHidingFunction
		void Deserialize(const fiStreamPtr& stream, ConstString fileName);
	};

	class grcDomainProgram : public grcProgram
	{
		amComPtr<ID3D11DomainShader> m_Shader_;
		amComPtr<ID3D11DomainShader> m_Shader;
		u32 m_BytecodeSize = 0;
		u32 m_Unused234 = 0;
	public:
		u32 GetBytecodeSize() override { return m_BytecodeSize; }

		// ReSharper disable once CppHidingFunction
		void Deserialize(const fiStreamPtr& stream, ConstString fileName);
	};

	class grcGeometryProgram : public grcProgram
	{
		struct UnknownData
		{
			u8 Unknown0[16];
			u8 Unknown16;
			u8 Unknown17;
			u8 Unknown18;
			u8 Unknown19;
		};

		amComPtr<ID3D11GeometryShader> m_Shader_;
		amComPtr<ID3D11GeometryShader> m_Shader;
		u32 m_BytecodeSize = 0;

		UnknownData* m_UnknownDatas = nullptr;
		u32 m_UnknownDataCount = 0;
	public:
		~grcGeometryProgram() override;

		u32 GetBytecodeSize() override { return m_BytecodeSize; }

		// ReSharper disable once CppHidingFunction
		void Deserialize(const fiStreamPtr& stream, ConstString fileName);
	};

	class grcHullProgram : public grcProgram
	{
		amComPtr<ID3D11HullShader> m_Shader_;
		amComPtr<ID3D11HullShader> m_Shader;
		u32 m_BytecodeSize = 0;
	public:
		u32 GetBytecodeSize() override { return m_BytecodeSize; }

		// ReSharper disable once CppHidingFunction
		void Deserialize(const fiStreamPtr& stream, ConstString fileName);
	};

	class grcEffect
	{
		struct VariableInfo
		{
			// Metadata from loaded effect file, we don't really need it...
		};

		atArray<grcEffectTechnique>       m_Techniques;
		atArray<grcEffectVar>             m_Locals;
		atArray<grcEffectVar*>            m_LocalsCBuf;
		atArray<grcVertexProgram>         m_VertexPrograms;
		atArray<grcFragmentProgram>       m_FragmentPrograms;
		u8                                m_TechniqueMap[64 * 8];
		sysMemContainerData               m_Container;
		char                              m_Name[40];
		grcInstanceData                   m_InstanceDataTemplate;
		u64                               m_FileTime;
		ConstString                       m_FilePath;
		u32                               m_NameHash;
		atArray<grcEffectVar::Annotation> m_Properties;
		u32                               m_Dcl;
		u16                               m_Ordinal;
		atArray<VariableInfo>             m_VarInfo;
		atArray<grcComputeProgram>        m_ComputePrograms;
		atArray<grcDomainProgram>         m_DomainPrograms;
		atArray<grcGeometryProgram>       m_GeometryPrograms;
		atArray<grcHullProgram>           m_HullPrograms;

	public:
		ConstString GetName() const { return m_Name; }
		u32			GetNameHash() const { return m_NameHash; }

		const grcInstanceData& GetInstanceDataTemplate() const { return m_InstanceDataTemplate; }

		u16                 GetVarCount() const { return m_Locals.GetSize(); }
		grcEffectVar*       GetVar(u16 index) { return &m_Locals.Get(index); }
		grcEffectTechnique* GetTechnique(grcHandle handle) { return &m_Techniques[handle.GetIndex()]; }
		grcEffectTechnique* GetTechnique(ConstString name) const;
		grcEffectVar*       GetVar(grcHandle handle) { return &m_Locals[handle.GetIndex()]; }
		grcEffectVar*       GetVar(ConstString name, u16* outIndex = nullptr);
		grcHandle           LookupVar(ConstString name) { return LookupVar(atStringHash(name)); }
		grcHandle           LookupVar(u32 hash);
		grcHandle           LookupTechnique(u32 hash);
		grcHandle           LookupTechnique(ConstString name) { return LookupTechnique(atStringHash(name)); }

		grcVertexProgram* GetVS(const grcEffectTechnique* technique)
		{
			return &m_VertexPrograms[technique->m_Passes[0].VS];
		}

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
}
