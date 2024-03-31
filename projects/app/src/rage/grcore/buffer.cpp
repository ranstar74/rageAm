#include "buffer.h"

#include "am/graphics/render.h"
#include "rage/system/new.h"

void rage::grcBufferD3D11::UpdateGPUFromCPUCopy(u32 offset, u32 size)
{
	AM_ASSERTS(m_Object != nullptr);

	D3D11_BUFFER_DESC desc;
	m_Object->GetDesc(&desc);

	// Take actual size from buffer
	if (offset == 0 && size == 0)
		size = desc.ByteWidth;

	AM_ASSERTS(offset + size <= desc.ByteWidth);

	D3D11_BOX box = {};
	box.right = size;
	box.bottom = 1;
	box.back = 1;

	if (m_CreateType == grcsBufferCreate_ReadDynamicWrite || 
		m_CreateType == grcsBufferCreate_DynamicWrite)
	{
		// TODO: Native code uses staging buffer to move data to GPU + CopySubresourceRegion
		AM_UNREACHABLE("grcBufferD3D11::UpdateGPUFromCPUCopy() -> Buffers without CPU access are not supported yet.");
	}

	ID3D11DeviceContext* context = rageam::graphics::RenderGetContext();
	context->UpdateSubresource(m_Object.Get(), 0, &box, static_cast<char*>(m_CPUCopyOfData) + offset, size, size);
}

void rage::grcBufferD3D11::Reset()
{
	m_CPUCopyOfData = nullptr;
	m_Mutex = INVALID_HANDLE_VALUE;
	m_LockSize = 0;
	m_LockOffset = 0;
	m_CreateType = 0;
	m_SyncType = 0;
	m_WeOwnCPUCopy = 0;
	m_HasBeenDeleted = 0;
	m_LockType = 0;
	m_Dirty = 0;
}

rage::grcBufferD3D11::grcBufferD3D11()
{
	Reset();
}

rage::grcBufferD3D11::grcBufferD3D11(const grcBufferD3D11& other)
{
	if (IsResourceCompiling())
	{
		Reset();
		return;
	}

	// Nothing to copy
	if (!other.m_Object)
		return;

	// Retrieve original description from DX object
	D3D11_BUFFER_DESC desc;
	other.m_Object.Get()->GetDesc(&desc);
	u32 stride = desc.StructureByteStride;
	u32 count = desc.ByteWidth / stride;

	// We can't really restore original create parameters so reuse what we have
	Initialize(count, stride, 
		grcBindFlags(desc.BindFlags), 
		grcBufferCreateType(other.m_CreateType), 
		grcsBufferSyncType(other.m_SyncType), 
		other.m_CPUCopyOfData, 
		other.m_WeOwnCPUCopy, 
		grcBufferMiscFlags(desc.MiscFlags));
}

void rage::grcBufferD3D11::Destroy()
{
	if (m_WeOwnCPUCopy)
	{
		rage_free(m_CPUCopyOfData);
		m_CPUCopyOfData = nullptr;
		m_WeOwnCPUCopy = false;
	}

	if (m_LockType == grcsBufferSync_Mutex)
	{
		sysIpcDeleteMutex(m_Mutex);
		m_Mutex = nullptr;
	}
	else if (m_LockType == grcsBufferSync_DirtySemaphore)
	{
		sysIpcDeleteSema(m_DirtySemaphore);
		m_DirtySemaphore = nullptr;
	}

	m_Object = nullptr;
	m_HasBeenDeleted = true;
}

void rage::grcBufferD3D11::Initialize(
	u32 count, u32 stride,
	grcBindFlags bindFlags, 
	grcBufferCreateType createType,
	grcsBufferSyncType syncType,
	pVoid initialData,
	bool allocateCPUCopy,
	grcBufferMiscFlags miscFlags)
{
	ID3D11Device* device = rageam::graphics::RenderGetDevice();

	AM_ASSERTS(m_Object == nullptr, "grcBufferD3D11::Initialize() -> Buffer is already created!");
	if (grcsBufferCreate_ReadOnly)
		AM_ASSERTS(initialData != nullptr, "grcBufferD3D11::Initialize() -> Can't create read-only buffer without initial data.");

	u32 bufferSize = count * stride;

	m_HasBeenDeleted = false;
	m_CreateType = createType;
	m_SyncType = syncType;
	m_WeOwnCPUCopy = false;

	D3D11_BUFFER_DESC desc;
	desc.StructureByteStride = stride;
	desc.BindFlags = bindFlags;
	desc.MiscFlags = miscFlags;
	desc.ByteWidth = bufferSize;
	desc.CPUAccessFlags = grcCPUNoAccess;

	if (createType == grcsBufferCreate_NeitherReadNorWrite || createType == grcsBufferCreate_ReadOnly)
	{
		desc.Usage = static_cast<D3D11_USAGE>(grcUsageImmutable);
	}
	else if (createType == grcsBufferCreate_DynamicWrite || createType == grcsBufferCreate_ReadDynamicWrite)
	{
		desc.Usage = static_cast<D3D11_USAGE>(grcUsageDynamic);
		desc.CPUAccessFlags = grcCPUWrite;
	}
	else
	{
		desc.Usage = static_cast<D3D11_USAGE>(grcUsageDefault);
	}

	D3D11_SUBRESOURCE_DATA initData;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;
	initData.pSysMem = initialData;
	D3D11_SUBRESOURCE_DATA* pInitData = &initData;
	if (desc.Usage == D3D11_USAGE_DYNAMIC && initialData == nullptr)
		pInitData = nullptr;

	AM_ASSERT_STATUS(device->CreateBuffer(&desc, pInitData, &m_Object));

	// Allocate CPU copy of the data
	// We allow to force allocate copy even if buffer is read-only because
	//	initial data pointer might be temp
	bool needWrite = 
		createType == grcsBufferCreate_ReadWrite ||
		createType == grcsBufferCreate_ReadWriteOnceOnly ||
		createType == grcsBufferCreate_ReadDynamicWrite;
	if (allocateCPUCopy || (needWrite && allocateCPUCopy))
	{
		m_CPUCopyOfData = rage_malloc(bufferSize);
		if (initialData) memcpy(m_CPUCopyOfData, initialData, bufferSize);
		else			 memset(m_CPUCopyOfData, 0, bufferSize);
		m_WeOwnCPUCopy = true;
	}
	else if (createType == grcsBufferCreate_ReadOnly)
	{
		m_CPUCopyOfData = initialData; // Buffer is read-only, we don't have to allocate copy
	}

	// Create lock primitives
	if (syncType == grcsBufferSync_Mutex)
	{
		m_Mutex = sysIpcCreateMutex();
	}
	else
	{
		m_DirtySemaphore = sysIpcCreateSema(1);
	}
}

rage::grcBufferD3D11::~grcBufferD3D11()
{
	Destroy();
}

pVoid rage::grcBufferD3D11::Lock(grcLockFlags flags, u32 offset, u32 size)
{
	AM_ASSERTS(m_Object != nullptr);

	if (flags & grcsDiscard)
		flags |= grcsNoOverwrite;

	m_LockType = flags;
	m_LockOffset = offset;
	m_LockSize = size;

	// Use locking mechanism only if write permission is requested from non-render thread
	if (flags & (grcsWrite | grcsDiscard | grcsNoOverwrite) && !grcDevice::IsRenderThread())
	{
		if (m_SyncType == grcsBufferSync_Mutex)
		{
			sysIpcLockMutex(m_Mutex);
		}
		else if (m_SyncType == grcsBufferSync_DirtySemaphore)
		{
			sysIpcWaitSema(m_DirtySemaphore);
		}
	}

	if (m_CPUCopyOfData)
	{
		return static_cast<char*>(m_CPUCopyOfData) + offset;
	}

	AM_ASSERTS(grcDevice::IsRenderThread());

	ID3D11DeviceContext* context = rageam::graphics::RenderGetContext();
	D3D11_MAP mapType;
	if (m_LockType & grcsDiscard)
		mapType = D3D11_MAP_WRITE_DISCARD;
	else if (m_LockType & grcsNoOverwrite)
		mapType = D3D11_MAP_WRITE_NO_OVERWRITE;
	else
		mapType = D3D11_MAP_WRITE;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	AM_ASSERT_STATUS(context->Map(m_Object.Get(), 0, mapType, 0, &mappedResource));
	return static_cast<char*>(mappedResource.pData) + offset;
}

void rage::grcBufferD3D11::Unlock()
{
	AM_ASSERTS(m_Object != nullptr);

	// Read-only lock, nothing to do
	if (!(m_LockType & (grcsWrite | grcsDiscard | grcsNoOverwrite)))
		return;

	if (m_CPUCopyOfData == nullptr)
	{
		AM_ASSERTS(grcDevice::IsRenderThread());

		ID3D11DeviceContext* context = rageam::graphics::RenderGetContext();
		context->Unmap(m_Object.Get(), 0);
		return;
	}

	// Locking context is expensive operation and game uses alternative approach
	// via draw list command instead, this is not currently implemented (and not needed)
	bool isRenderThread = grcDevice::IsRenderThread();
	if (isRenderThread) grcDevice::LockContext();
	UpdateGPUFromCPUCopy(m_LockOffset, m_LockSize);
	if (isRenderThread) grcDevice::UnlockContext();
}

rage::grcVertexBuffer::grcVertexBuffer()
{
	m_VertexData = nullptr;
	m_VertexCount = 0;
	m_LockPointer = 0;
	m_Stride = 0;
	m_Flags = 0;

	m_ReservedA = 0;
	m_ReservedB = 0;
}

// ReSharper disable once CppPossiblyUninitializedMember
rage::grcVertexBuffer::grcVertexBuffer(const datResource& rsc)
{

}

rage::grcVertexBuffer::grcVertexBuffer(const grcVertexBuffer& other) : m_VertexFormat(other.m_VertexFormat)
{
	m_VertexCount = other.m_VertexCount;
	m_LockPointer = other.m_LockPointer;
	m_Stride = other.m_Stride;
	m_Flags = other.m_Flags;
	m_ReservedA = other.m_ReservedA;
	m_ReservedB = other.m_ReservedB;
	m_VertexData = nullptr; // Resolved in grcVertexBufferD3D11
}

void rage::grcVertexBuffer::SetVertexFormat(const grcFvf& format)
{
	m_VertexFormat = new grcFvf(format);
	m_Stride = format.Stride;
}

void rage::grcVertexBuffer::SetVertexCount(u32 count)
{
	m_VertexCount = count;
}

const rage::grcFvf* rage::grcVertexBuffer::GetVertexFormat() const
{
	return m_VertexFormat.Get();
}

void rage::grcVertexBufferD3D11::CreateInternal(grcBufferCreateType createType, grcsBufferSyncType syncType, bool allocateCPUCopy)
{
	m_Buffer.Initialize(m_VertexCount, m_Stride, grcBindVertexBuffer, createType, syncType, m_VertexData, allocateCPUCopy);
	if (allocateCPUCopy)
	{
		m_VertexData = static_cast<char*>(m_Buffer.GetCPUCopyPointer());
	}
}

rage::grcVertexBufferD3D11::grcVertexBufferD3D11()
{
	m_Unused = 0;
	m_Buffer = {};
}

// ReSharper disable once CppPossiblyUninitializedMember
rage::grcVertexBufferD3D11::grcVertexBufferD3D11(const datResource& rsc) : grcVertexBuffer(rsc)
{
	rsc.Fixup(m_VertexData);
	if (!m_VertexData)
		return;

	if (m_Flags & (FlagReadWrite | FlagDynamic))
	{
		CreateInternal(grcsBufferCreate_ReadWrite, grcsBufferSync_Mutex, false);
	}
	else
	{
		CreateInternal(grcsBufferCreate_ReadOnly, grcsBufferSync_None, false);
	}
}

rage::grcVertexBufferD3D11::grcVertexBufferD3D11(const grcVertexBufferD3D11& other) : grcVertexBuffer(other), m_Buffer(other.m_Buffer)
{
	m_Unused = 0;

	// When compiling, vertex data is owned by this instance, not grcBufferD3D11::m_CPUCopyOfData
	if (IsResourceCompiling())
	{
		u32 dataSize = m_VertexCount * m_Stride;
		pgRscCompiler::GetVirtualAllocator()->AllocateRef(m_VertexData, dataSize);
		memcpy(m_VertexData, other.m_VertexData, dataSize);
	}
	else
	{
		m_VertexData = m_Buffer.GetCPUCopyPointer();
	}
}

void rage::grcVertexBufferD3D11::CreateWithData(const grcFvf& fvf, pVoid vertices, u32 vertexCount)
{
	SetVertexCount(vertexCount);
	SetVertexFormat(fvf);

	// We allocate CPU copy of the data because GTA need it (see comment of m_VertexData)
	m_Buffer.Initialize(m_VertexCount, m_Stride, grcBindVertexBuffer, grcsBufferCreate_ReadOnly, grcsBufferSync_None, vertices, true);
	m_VertexData = m_Buffer.GetCPUCopyPointer();
}

void rage::grcVertexBufferD3D11::CreateDynamic(const grcFvf& fvf, u32 vertexCount, grcsBufferSyncType syncType)
{
	SetVertexFormat(fvf);
	SetVertexCount(vertexCount);

	// Same allocateCPUCopy = true as in CreateWithData
	m_Buffer.Initialize(m_VertexCount, m_Stride, grcBindVertexBuffer, grcsBufferCreate_DynamicWrite, syncType, nullptr, true);
	m_VertexData = m_Buffer.GetCPUCopyPointer();
}

rage::grcIndexBuffer::grcIndexBuffer()
{
	m_IndexCountAndFlags = 0;
	m_IndexData = nullptr;
}

// ReSharper disable once CppPossiblyUninitializedMember
rage::grcIndexBuffer::grcIndexBuffer(const datResource& rsc)
{
	rsc.Fixup(m_IndexData);
}

rage::grcIndexBuffer::grcIndexBuffer(const grcIndexBuffer& other)
{
	m_IndexCountAndFlags = other.m_IndexCountAndFlags;
	m_IndexData = nullptr; // Resolved in grcIndexBufferD3D11
}

rage::grcIndexBufferD3D11::grcIndexBufferD3D11()
{
	m_Unused = 0;
	m_Buffer = {};
}

// ReSharper disable once CppPossiblyUninitializedMember
rage::grcIndexBufferD3D11::grcIndexBufferD3D11(const datResource& rsc) : grcIndexBuffer(rsc)
{
	m_Buffer.Initialize(
		GetIndexCount(), sizeof grcIndex_t, grcBindIndexBuffer, grcsBufferCreate_ReadOnly, grcsBufferSync_None, m_IndexData);
}

rage::grcIndexBufferD3D11::grcIndexBufferD3D11(const grcIndexBufferD3D11& other) : grcIndexBuffer(other)
{
	m_Unused = other.m_Unused;

	// When compiling, index data is owned by this instance, not grcBufferD3D11::m_CPUCopyOfData
	if (IsResourceCompiling())
	{
		u32 dataSize = GetIndexCount() * sizeof grcIndex_t;
		pgRscCompiler::GetVirtualAllocator()->AllocateRef(m_IndexData, dataSize);
		memcpy(m_IndexData, other.m_IndexData, dataSize);
	}
	else
	{
		m_IndexData = static_cast<grcIndex_t*>(m_Buffer.GetCPUCopyPointer());
	}
}

void rage::grcIndexBufferD3D11::CreateWithData(u16* indices, u32 indexCount)
{
	SetIndexCount(indexCount);

	// We allocate CPU copy of the data because GTA need it (see comment of m_IndexData)
	m_Buffer.Initialize(indexCount, sizeof grcIndex_t, grcBindVertexBuffer, grcsBufferCreate_ReadOnly, grcsBufferSync_None, indices, true);
	m_IndexData = static_cast<u16*>(m_Buffer.GetCPUCopyPointer());
}

void rage::grcIndexBufferD3D11::CreateDynamic(u32 indexCount, grcsBufferSyncType syncType)
{
	SetIndexCount(indexCount);

	// Same allocateCPUCopy = true as in CreateWithData
	m_Buffer.Initialize(indexCount, sizeof grcIndex_t, grcBindVertexBuffer, grcsBufferCreate_DynamicWrite, syncType, nullptr, true);
	m_IndexData = static_cast<u16*>(m_Buffer.GetCPUCopyPointer());
}
