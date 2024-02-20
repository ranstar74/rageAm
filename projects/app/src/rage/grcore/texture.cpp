#include "texture.h"

#include "texturepc.h"

rage::grcTexture::grcTexture(eTextureType type)
{
	m_Texture = {};
	m_Name = TEXTURE_DEFAULT_NAME;
	m_RefCount = 1;
	m_ResourceTypeAndConversionFlags = TEXTURE_NORMAL;
	m_LayerCount = 0;
	m_PhysicalSizeAndTemplateType = type;
	m_HandleIndex = 0;
}

rage::grcTexture::grcTexture(const grcTexture& other) : pgBase(other)
{
	m_Texture = other.m_Texture;
	m_Name = other.m_Name;
	m_ResourceTypeAndConversionFlags = other.m_ResourceTypeAndConversionFlags;
	m_LayerCount = other.m_LayerCount;
	m_PhysicalSizeAndTemplateType = other.m_PhysicalSizeAndTemplateType;
	m_HandleIndex = other.m_HandleIndex;

	if (pgRscCompiler::GetCurrent())
		m_RefCount = other.m_RefCount;
	else
		m_RefCount = 1;
}

// ReSharper disable once CppPossiblyUninitializedMember
rage::grcTexture::grcTexture(const datResource& rsc)
{

}

rage::grcTexture* rage::grcTexture::Place(const datResource& rsc, grcTexture* that)
{
	AM_ASSERT(that->GetResourceType() == TEXTURE_NORMAL, "grcTexture::Place() -> Only normal texture type is currently supported!");
	return new (that) grcTextureDX11(rsc);
}

rage::grcTexture* rage::grcTexture::Snapshot(pgSnapshotAllocator* allocator, grcTexture* from)
{
	pVoid block = allocator->Allocate(sizeof grcTextureDX11);
	return new (block) grcTextureDX11(*reinterpret_cast<grcTextureDX11*>(from));
}
