//
// File: buffer.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/ptr.h"
#include "rage/system/ipc.h"
#include "rage/paging/resource.h"
#include "rage/paging/ref.h"
#include "rage/dat/base.h"
#include "device.h"

#include <d3d11.h>

namespace rage
{
	// Index buffers are 16bit so triangles has to be split in multiple geometries
	using grcIndex_t = u16;

	static constexpr DXGI_FORMAT INDEX_BUFFER_DXGI_FORMAT = DXGI_FORMAT_R16_UINT;

	enum grcBufferMiscFlags_
	{
		grcBufferMisc_None             = 0x0,
		grcBufferMisc_GenerateMips     = 0x1,
		grcBufferMisc_Shared           = 0x2,
		grcBufferMisc_TextureCube      = 0x4,
		grcBufferMisc_DrawIndirectArgs = 0x10,
		grcBufferMisc_AllowRawViews    = 0x20,
		grcBufferMisc_BufferStructured = 0x40,
		grcBufferMisc_ResourceClamp    = 0x80,
		grcBufferMisc_SharedMutex      = 0x100,
		grcBufferMisc_GDICompatible    = 0x200,
	};
	using grcBufferMiscFlags = int;

	enum grcBufferCreateType
	{
		grcsBufferCreate_NeitherReadNorWrite = 0x0, // Expects data to be available upon creation (equivalent to D3D11_USAGE_IMMUTABLE)
		grcsBufferCreate_ReadOnly			 = 0x1, // Buffer can only be read from (equivalent to D3D11_USAGE_IMMUTABLE + CPU copy).
		grcsBufferCreate_ReadWrite			 = 0x2, // Buffer can be read from and written to (equivalent to D3D11_USAGE_DEFAULT + CPU copy).
		grcsBufferCreate_ReadWriteOnceOnly	 = 0x3, // Buffer can be read from and written to once only (equivalent to D3D11_USAGE_DEFAULT + temporary CPU copy).
		grcsBufferCreate_ReadDynamicWrite	 = 0x4, // Buffer can only be written to. (equivalent to D3D11_USAGE_DYNAMIC + CPU copy)
		grcsBufferCreate_DynamicWrite		 = 0x5, // Buffer can only be written to. (equivalent to D3D11_USAGE_DYNAMIC)
	};

	enum grcsBufferSyncType
	{
		grcsBufferSync_Mutex		  = 0x0, // Uses a mutex on the buffer, minimal stalls, can be called only once per frame, "frame skips" in GPU copy possible.
		grcsBufferSync_DirtySemaphore = 0x1, // Uses a semaphore to prevent overwriting data before the GPU has been updated. Supports calling once per frame only, could incur long stalls.
		grcsBufferSync_CopyData		  = 0x2, // Makes a copy of the data. Supports multiple calls per frame but has to copy buffer contents.
		grcsBufferSync_None			  = 0x3, // Expects only to be updated on the render thread or not at all.
	};

	/**
	 * \brief Generic DX11 buffer for any kind of data.
	 */
	class grcBufferD3D11
	{
		amComPtr<ID3D11Buffer> m_Object;
		pVoid m_CPUCopyOfData;
		u32   m_LockOffset;
		u32   m_LockSize;

		u32 m_CreateType     : 3;
		u32 m_SyncType       : 2;
		u32 m_WeOwnCPUCopy   : 1;
		u32 m_HasBeenDeleted : 1;
		u32 m_LockType       : 7;
		u32 m_Dirty          : 1; // Unused

		union
		{
			sysIpcMutex m_Mutex;
			sysIpcSema  m_DirtySemaphore;
		};

		void UpdateGPUFromCPUCopy(u32 offset, u32 size);
		void Reset();

	public:
		grcBufferD3D11();
		grcBufferD3D11(const grcBufferD3D11& other);
		~grcBufferD3D11();

		void Destroy();
		void Initialize(
			u32 count, u32 stride,
			grcBindFlags bindFlags,
			grcBufferCreateType createType,
			grcsBufferSyncType syncType = grcsBufferSync_None,
			pVoid initialData = nullptr,
			bool allocateCPUCopy = false,
			grcBufferMiscFlags miscFlags = grcBufferMisc_None);

		pVoid Lock(grcLockFlags flags, u32 offset = 0, u32 size = 0);
		void  Unlock();

		pVoid		  GetCPUCopyPointer() const { return m_CPUCopyOfData; }
		ID3D11Buffer* GetResource() const { return m_Object.Get(); }
	};

	class grcVertexBuffer : public datBase
	{
	protected:
		enum Flags
		{
			FlagDynamic            = 1 << 0,
			FlagPreallocatedMemory = 1 << 1,
			FlagReadWrite          = 1 << 2,
		};

		u16            m_Stride;
		u8             m_ReservedA;
		u8             m_Flags;
		u64            m_LockPointer;
		u32            m_VertexCount;
		// Note: We keep CPU copy of the data because game uses it for things like aligning bullet hole decals
		// Pointer is NOT owned by this instance!
		pVoid		   m_VertexData;
		u32            m_ReservedB;
		// Instead of whole vertex declaration we store only flexible format
		// and then decode it when resource is being deserialized
		pgUPtr<grcFvf> m_VertexFormat;

		void SetVertexFormat(const grcFvf& format);
		void SetVertexCount(u32 count);

	public:
		grcVertexBuffer();
		grcVertexBuffer(const datResource& rsc);
		grcVertexBuffer(const grcVertexBuffer& other);

		const grcFvf* GetVertexFormat() const;
		u32			  GetVertexCount() const { return m_VertexCount; }
	};

	class grcVertexBufferD3D11 : public grcVertexBuffer
	{
		u64                   m_Unused;
		grcVertexBufferD3D11* m_Previous = nullptr;
		grcVertexBufferD3D11* m_Next = nullptr;
		grcBufferD3D11        m_Buffer;

		void CreateInternal(grcBufferCreateType createType, grcsBufferSyncType syncType, bool allocateCPUCopy);

	public:
		grcVertexBufferD3D11();
		grcVertexBufferD3D11(const datResource& rsc);
		grcVertexBufferD3D11(const grcVertexBufferD3D11& other);

		// Creates immutable vertex buffer with given initial data
		void CreateWithData(const grcFvf& fvf, pVoid vertices, u32 vertexCount);
		// Creates mutable vertex buffer with given capacity, use Lock/Unlock to change data
		void CreateDynamic(const grcFvf& fvf, u32 vertexCount, grcsBufferSyncType syncType = grcsBufferSync_Mutex);

		grcBufferD3D11* GetBuffer() { return &m_Buffer; }
	};

	class grcIndexBuffer : public datBase
	{
	protected:
		enum : u32
		{
			FLAG_PREALLOCATED = 0x01000000,
			FLAGS_MASK        = 0xFF000000,
			IDX_COUNT_MASK    = 0x00FFFFFF,
		};

		u32  m_IndexCountAndFlags;
		// Same as in grcVertexBuffer, pointer is owned by grcIndexBufferD3D11::m_Buffer
		u16* m_IndexData;

		void SetIndexCount(u32 count)
		{
			AM_ASSERT((count & IDX_COUNT_MASK) == count, 
				"grcIndexBuffer::SetIndexCount() -> '%u' cannot be stored!", count);
			m_IndexCountAndFlags &= ~IDX_COUNT_MASK;
			m_IndexCountAndFlags |= count & IDX_COUNT_MASK;
		}

	public:
		grcIndexBuffer();
		grcIndexBuffer(const datResource& rsc);
		grcIndexBuffer(const grcIndexBuffer& other);

		u32 GetIndexCount() const { return m_IndexCountAndFlags & IDX_COUNT_MASK; }
	};

	class grcIndexBufferD3D11 : public grcIndexBuffer
	{
		u64                  m_Unused;
		grcIndexBufferD3D11* m_Previous = nullptr;
		grcIndexBufferD3D11* m_Next = nullptr;
		grcBufferD3D11       m_Buffer;

	public:
		grcIndexBufferD3D11();
		grcIndexBufferD3D11(const datResource& rsc);
		grcIndexBufferD3D11(const grcIndexBufferD3D11& other);

		// Creates immutable vertex buffer with given initial data
		void CreateWithData(u16* indices, u32 indexCount);
		// Creates mutable vertex buffer with given capacity, use Lock/Unlock to change data
		void CreateDynamic(u32 indexCount, grcsBufferSyncType syncType = grcsBufferSync_Mutex);

		grcBufferD3D11* GetBuffer() { return &m_Buffer; }
	};
}
