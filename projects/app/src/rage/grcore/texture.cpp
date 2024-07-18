#include "texture.h"
#include "texturepc.h"
#include "texturereference.h"

rage::grcTexture::grcTexture(eTextureType type, ConstString name)
{
	m_Texture = {};
	m_Name = name;
	m_RefCount = 1;
	m_ResourceTypeAndConversionFlags = type;
	m_PhysicalSizeAndTemplateType = 0;
	m_LayerCount = 0;
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

rage::grcTextureDX11* rage::grcTexture::ToDX11()
{
	AM_ASSERTS(GetResourceType() == TEXTURE_NORMAL);
	return reinterpret_cast<rage::grcTextureDX11*>(this);
}

rage::grcTexture* rage::grcTexture::Place(const datResource& rsc, grcTexture* that)
{
	AM_ASSERT(that->GetResourceType() == TEXTURE_NORMAL || that->GetResourceType() == TEXTURE_REFERENCE, 
		"grcTexture::Place() -> Invalid resource type '%i'", that->GetResourceType());
	if (that->GetResourceType() == TEXTURE_NORMAL)
		return new (that) grcTextureDX11(rsc);
	else
		return new (that) grcTextureReference(rsc);
}

rage::grcTexture* rage::grcTexture::Snapshot(pgSnapshotAllocator* allocator, grcTexture* from)
{
	AM_ASSERTS(from);
	AM_ASSERTS(from->GetResourceType() == TEXTURE_NORMAL);
	pVoid block = allocator->Allocate(sizeof grcTextureDX11);
	return new (block) grcTextureDX11(*reinterpret_cast<grcTextureDX11*>(from));
}
