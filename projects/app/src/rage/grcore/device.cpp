#include "device.h"

#include "buffer.h"
#include "am/graphics/dxgi_utils.h"
#include "am/graphics/render.h"
#include "am/graphics/vertexbufferiterator.h"
#include "am/integration/memory/address.h"

rage::grcVertexDeclaration* rage::grcDevice::CreateVertexDeclaration(const grcVertexElement* elements, u32 elementCount)
{
	grcVertexDeclaration* decl = grcVertexDeclaration::Allocate(elementCount);
	for (u32 i = 0; i < elementCount; i++)
	{
		const grcVertexElement& element = elements[i];
		grcVertexElementDesc& desc = decl->Elements[i];
		desc.SemanticName = VertexSemanticName[element.Semantic];
		desc.SemanticIndex = element.SemanticIndex;
		desc.Format = grcFormatToDXGI[element.Format];
		desc.InputSlot = element.InputSlot;
		//desc.AlignedByteOffset = decl->Stride; // We can use stride as offset as we accumulate it
		desc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT; // Works the same as option above
		// Game has code related to those remaining but it's either broken
		// or I don't know, it doesn't make any sense.
		desc.InstanceDataStepRate = 0;
		desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		decl->Stride += element.DataSize;
	}
	return decl;
}

rage::grcVertexDeclaration* rage::grcDevice::CreateVertexDeclaration(const grcVertexElementDesc* elementDescs, u32 elementCount)
{
	grcVertexDeclaration* decl = grcVertexDeclaration::Allocate(elementCount);
	for (u32 i = 0; i < elementCount; i++)
	{
		// Straight copy elements to vertex declaration
		grcVertexElementDesc& element = decl->Elements[i] = elementDescs[i];

		u32 elementSize = rageam::graphics::DXGI::BytesPerPixel(element.Format);
		decl->Stride += elementSize;
	}
	return decl;
}

void rage::grcDevice::SetDrawMode()
{

}

void rage::grcDevice::SetIndexBuffer(grcIndexBuffer* buffer)
{
	ID3D11DeviceContext* ctx = rageam::graphics::RenderGetContext();
	grcIndexBufferD3D11* bufferD3D11 = (grcIndexBufferD3D11*)buffer;
	ctx->IASetIndexBuffer(bufferD3D11->GetBuffer()->GetResource(), INDEX_BUFFER_DXGI_FORMAT, 0);
}

void rage::grcDevice::DrawIndexedPrimitive(
	grcDrawMode drawMode,
	grcVertexDeclarationPtr vtxDeclaration,
	grcVertexBuffer* vtxBuffer, grcIndexBuffer* idxBuffer, u32 idxCount)
{
	// TODO: Unknown function called in the beginning

	sm_CurrentVertexDeclaration = vtxDeclaration;

	// TODO: For now, need more research on grcProgram
	static auto fn = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"44 8B 44 24 50 33 D2 8B 4C 24 30", "rage::grcDevice::DrawIndexedPrimitive").GetAt(-0x5F)
#else
		"E8 ?? ?? ?? ?? B9 01 00 00 00 E8 ?? ?? ?? ?? EB 14")
		.GetCall()
#endif
		.To<decltype(&DrawIndexedPrimitive)>();
	fn(drawMode, vtxDeclaration, vtxBuffer, idxBuffer, idxCount);
}

void rage::grcDevice::SetWorldMtx(const Mat34V& mtx)
{
	static auto fn = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"FF 15 ?? ?? ?? ?? 48 81 C4 B8 03 00 00", "rage::grcViewport::SetCurrentWorldMtxFn_NonInstanced ref")
#else
		"E8 ?? ?? ?? ?? 40 B5 01 8B D3")
#endif
		.GetCall()
		.To<decltype(&SetWorldMtx)>();
	fn(mtx);
}

void rage::grcDevice::GetScreenSize(u32& width, u32& height)
{
#if APP_BUILD_2699_16_RELEASE_NO_OPT
	static char* sm_CurrentWindows = gmAddress::Scan(
		"48 8D 0D ?? ?? ?? ?? 8B 54 24 28 89 54 01 04 EB C3", "rage::grcDevice::sm_CurrentWindows")
		.GetRef(3)
		.To<char*>();

	width = *(u32*) (sm_CurrentWindows + 0);
	height = *(u32*) (sm_CurrentWindows + 4);
#else
	static gmAddress addr = gmAddress::Scan("44 8B 1D ?? ?? ?? ?? 48 8B 3C D0");
	height = *addr.GetRef(3).To<u32*>();
	width = *addr.GetRef(0xB + 2).To<u32*>();
#endif
}

rage::grcDevice::MSAAMode rage::grcDevice::GetMSAA()
{
	static MSAAMode* pMode = gmAddress::Scan(
		"8B 44 24 20 89 04 24 8B 04 24 89 05", "rage::grcDevice::SetSamplesAndFragments+0x8")
		.GetAt(-0x8)
		.GetAt(0x12)
		.GetRef(0x2)
		.To<MSAAMode*>();
	return *pMode;
}

void rage::grcDrawLine()
{

}
