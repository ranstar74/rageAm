//
// File: program.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <d3d11.h>

#include "fvf.h"
#include "am/system/ptr.h"
#include "rage/atl/array.h"

namespace rage
{
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
	// Reads string length, string itself and computes joaat.
	u32 ReadStringAndComputeHash(const fiStreamPtr& stream);

	struct grcConstantBufferData
	{
		int32_t dword0;
		byte IsDirty;
		byte byte5__IsMapped;
		byte byte6;
		byte byte7;
		int64_t qword8;
		char Data;
	};

	struct grcConstantBuffer
	{
		u32 BufferSize;
		u16 word4;
		u16 word6; // PSSlot
		u16 word8;
		u16 wordA;
		u16 wordC;
		u16 wordE;
		u32 NameHash;
		const char* Name;
		amComPtr<ID3D11Buffer> BufferObject;
		grcConstantBufferData* BufferData;
		u16 word30;
		u8 word32;
		u16 word34;
		u32 dword38__Stride;
		u32 dword40;

		void Deserialize(const fiStreamPtr& stream)
		{
			//BufferSize = Math::Min<u32>(stream->ReadU32(), 0x10000);

			//stream->Read(&word4, 12);

			//// TODO: HeapString? Or something
			//u8 nameSize = stream->ReadU8();
			//char* name = new char[nameSize];
			//stream->Read(name, nameSize);

			//Name = name;
			//NameHash = joaat(name);
		}
	};

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
		grcConstantBuffer* m_CBuffers[14];

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
}
