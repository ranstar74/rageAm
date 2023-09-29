//
// File: buffer.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <d3d11.h>

#include "fvf.h"
#include "common/types.h"
#include "am/system/ptr.h"
#include "rage/paging/ref.h"

namespace rage
{
	// Index buffers are 16bit so triangles has to be split in multiple geometries
	using grcIndexType = u16;
	using grcIndices_t = grcIndexType*;

	static constexpr DXGI_FORMAT INDEX_BUFFER_DXGI_FORMAT = DXGI_FORMAT_R16_UINT;

	class grcBufferD3D11
	{
		amComPtr<ID3D11Buffer>	m_Object;
		pgUPtr<char>			m_DataCopy;
		u32						m_Unknown10;
		u32						m_Size;
		u32						m_UseTypeAndUnusedArgument;
		HANDLE					m_Mutex;

	public:
		grcBufferD3D11()
		{
			m_Size = 0;
			m_Mutex = NULL;
			m_Unknown10 = 0;
			m_UseTypeAndUnusedArgument = 0;
		}

		// ReSharper disable once CppPossiblyUninitializedMember
		grcBufferD3D11(const datResource& rsc)
		{

		}

		grcBufferD3D11(const grcBufferD3D11& other)
		{
			// TODO: Works only for resource compiler

			m_Object = nullptr; // TODO: ...
			m_Mutex = 0;

			m_Size = other.m_Size;
			m_UseTypeAndUnusedArgument = other.m_UseTypeAndUnusedArgument;
			m_Unknown10 = other.m_Unknown10;

			if (other.m_DataCopy.IsNull())
				return;

			char* dataCopy = (char*)pgAlloc(m_Size);
			memcpy(dataCopy, other.m_DataCopy.Get(), m_Size);
			m_DataCopy = dataCopy;
			m_DataCopy.AddCompilerRef();
		}

		void Create(const void* data, u32 elementCount, u32 stride, u32 bindFlags, u32 miscFlags, D3D11_USAGE usage, u32 cpuAccess = 0, bool keepData = false);

		pVoid Bind() const;
		void Unbind() const;

		pVoid GetInitialData() const { return m_DataCopy.Get(); }

		ID3D11Buffer* GetResource() const { return m_Object.Get(); }
	};

	class grcVertexBuffer
	{
	protected:
		u16				m_VertexStride;
		u8				m_UnknownA;
		u8				m_UnknownB;
		u64				m_Unknown10;
		u32				m_VertexCount;
		pgUPtr<char>	m_VertexData;
		u64				m_Unknown28;
		// Instead of whole vertex declaration we store only flexible format
		// and then decode it when resource is being deserialized
		pgUPtr<grcFvf>	m_VertexFormat;
	public:
		grcVertexBuffer();

		// ReSharper disable once CppPossiblyUninitializedMember
		grcVertexBuffer(const datResource& rsc)
		{

		}

		grcVertexBuffer(const grcVertexBuffer& other) : m_VertexFormat(other.m_VertexFormat)
		{
			m_VertexStride = other.m_VertexStride;
			m_UnknownA = other.m_UnknownA;
			m_UnknownB = other.m_UnknownB;
			m_Unknown10 = other.m_Unknown10;
			m_VertexCount = other.m_VertexCount;
			m_Unknown28 = other.m_Unknown28;

			//// Copy vertex data
			//u32 vertexDataSize = m_VertexCount * m_VertexStride;
			//char* vertexData = (char*)pgAlloc(vertexDataSize);
			//memcpy(vertexData, other.m_VertexData.Get(), vertexDataSize);
			//m_VertexData = vertexData;
			//m_VertexData.AddCompilerRef();
		}

		const grcFvf* GetVertexFormat() const;
		void SetVertexFormat(const grcFvf& format);
		void SetVertexCount(u32 count);

		u32 GetVertexCount() const { return m_VertexCount; }

		virtual ~grcVertexBuffer() = default;
	};

	class grcVertexBufferD3D11 : public grcVertexBuffer
	{
		u64 m_Unknown38;
		grcVertexBufferD3D11* m_Previous = nullptr;
		grcVertexBufferD3D11* m_Next = nullptr;
		grcBufferD3D11 m_Buffer;
	public:
		grcVertexBufferD3D11()
		{
			//m_Buffer = grcBufferD3D11();
			m_Unknown38 = 0;
		}

		grcVertexBufferD3D11(const grcVertexBufferD3D11& other) : grcVertexBuffer(other), m_Buffer(other.m_Buffer)
		{
			m_Unknown38 = other.m_Unknown38;

			//m_VertexData = (char*)m_Buffer.CopyResourceData();
			//m_VertexData.AddCompilerRef();
			//m_VertexData.Snapshot(other.m_VertexData, true);
			m_VertexData.Set((char*)m_Buffer.GetInitialData());
			m_VertexData.AddCompilerRef();
		}

		// ReSharper disable once CppPossiblyUninitializedMember
		grcVertexBufferD3D11(const datResource& rsc) : grcVertexBuffer(rsc), m_Buffer(rsc)
		{
			// Note that we keep (clone) data in vertex buffer, game uses it for aligning bullet hole decals (at least)
			m_Buffer.Create(m_VertexData.Get(), m_VertexCount, m_VertexStride, D3D11_BIND_VERTEX_BUFFER, 0, D3D11_USAGE_DEFAULT, 0, true);
		}

		~grcVertexBufferD3D11() override
		{
			m_VertexData.SuppressDelete(); // Pointer is owned by grcBufferD3D11
		}

		// Creates immutable vertex buffer with given initial data
		void CreateWithData(const grcFvf& fvf, const void* vertices, u32 vertexCount);
		// Creates mutable vertex buffer with given capacity, use Bind/Unbind to set contents
		void CreateDynamic(const grcFvf& fvf, u32 vertexCount);

		grcBufferD3D11* GetBuffer() { return &m_Buffer; }
	};

	class grcIndexBuffer
	{
	protected:
		u32			m_IndicesCountAndUnknown;
		pgUPtr<u16> m_Indices;


	public:
		grcIndexBuffer()
		{
			m_IndicesCountAndUnknown = 0;
		}

		// ReSharper disable once CppPossiblyUninitializedMember
		grcIndexBuffer(const datResource& rsc)
		{

		}

		grcIndexBuffer(const grcIndexBuffer& other)
		{
			m_IndicesCountAndUnknown = other.m_IndicesCountAndUnknown;

			//// Copy index data
			//u32 indexDataSize = GetIndexCount() * sizeof u16;
			//u16* indexData = (u16*)pgAlloc(indexDataSize);
			//memcpy(indexData, other.m_Indices.Get(), indexDataSize);
			//m_Indices = indexData;
			//m_Indices.AddCompilerRef();
		}

		void SetIndexCount(u32 count)
		{
			m_IndicesCountAndUnknown &= ~0xFFFFFF;
			m_IndicesCountAndUnknown |= count & 0xFFFFFF;
		}
		u16 GetIndexCount() const { return static_cast<u16>(m_IndicesCountAndUnknown & 0xFFFFFFu); }

		virtual ~grcIndexBuffer() = default;
	};

	class grcIndexBufferD3D11 : public grcIndexBuffer
	{
		u64 m_Unknown18;
		grcIndexBufferD3D11* m_Previous = nullptr;
		grcIndexBufferD3D11* m_Next = nullptr;
		grcBufferD3D11 m_Buffer;
	public:
		grcIndexBufferD3D11()
		{
			//m_Buffer = grcBufferD3D11();
			m_Unknown18 = 0;
		}

		// ReSharper disable once CppPossiblyUninitializedMember
		grcIndexBufferD3D11(const datResource& rsc) : grcIndexBuffer(rsc), m_Buffer(rsc)
		{
			m_Buffer.Create(m_Indices.Get(), GetIndexCount(), sizeof u16, D3D11_BIND_INDEX_BUFFER, 0, D3D11_USAGE_DEFAULT, 0, false);
		}

		grcIndexBufferD3D11(const grcIndexBufferD3D11& other) : grcIndexBuffer(other), m_Buffer(other.m_Buffer)
		{
			m_Unknown18 = other.m_Unknown18;

			//m_Indices = (u16*)m_Buffer.CopyResourceData();
			//m_Indices.AddCompilerRef();
			//m_Indices.Snapshot(other.m_Indices, true);


			m_Indices.Set((u16*)m_Buffer.GetInitialData());
			m_Indices.AddCompilerRef();
		}

		~grcIndexBufferD3D11() override
		{
			m_Indices.SuppressDelete(); // Pointer is owned by grcBufferD3D11
		}

		// Creates immutable vertex buffer with given initial data
		void CreateWithData(const u16* indices, u32 indexCount);
		// Creates mutable vertex buffer with given capacity, use Bind/Unbind to set contents
		void CreateDynamic(u32 indexCount);

		grcBufferD3D11* GetBuffer() { return &m_Buffer; }
	};
}
