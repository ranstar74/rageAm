#include "shadergroup.h"

void rage::grmShaderGroup::ScanForInstancedShaders()
{
	m_HasInstancedShader = false;
	for (const auto& shader : m_Shaders)
	{
		if (shader->IsInstancedDraw())
		{
			m_HasInstancedShader = true;
			break;
		}
	}
}

rage::grmShaderGroup::grmShaderGroup(const datResource& rsc)
{
	// We have to place materials after pushing embed dictionary
	if (m_EmbedTextures) grcTextureDictionary::PushDictionary(m_EmbedTextures.Get());
	for (pgUPtr<grmShader>& material : m_Shaders)
	{
		rsc.Place(&material);
	}
	if (m_EmbedTextures) grcTextureDictionary::PopDictionary();

	ScanForInstancedShaders();
}

rage::grmShaderGroup::grmShaderGroup(const grmShaderGroup& other)
{
	m_Unknown20 = other.m_Unknown20;
	m_Unknown28 = other.m_Unknown28;
	m_Unknown2A = other.m_Unknown2A;
	m_Unknown2C = other.m_Unknown2C;
	m_Unknown34 = other.m_Unknown34;
	m_Unknown38 = other.m_Unknown38;

	m_ContainerBlockSize = other.m_ContainerBlockSize;
	m_HasInstancedShader = other.m_HasInstancedShader;
	m_EmbedTextures = other.m_EmbedTextures;
	m_Shaders = other.m_Shaders;
}

void rage::grmShaderGroup::AddShader(grmShader* shader)
{
	if (shader->IsInstancedDraw())
		m_HasInstancedShader = true;

	m_Shaders.Construct(shader);
}

void rage::grmShaderGroup::RemoveShader(const grmShader* shader)
{
	for (u16 i = 0; i < m_Shaders.GetSize(); i++)
	{
		if (m_Shaders[i].Get() == shader)
		{
			RemoveShaderAt(i);
			return;
		}
	}
	AM_UNREACHABLE("grmShaderGroup::RemoveShader() -> Shader at %p doesn't belong to this group.", shader);
}

void rage::grmShaderGroup::RemoveShaderAt(u16 index)
{
	bool instancedDraw = m_Shaders[index]->IsInstancedDraw();
	m_Shaders.RemoveAt(index);

	// We have to rescan for other instanced drawn shaders only if we removed such
	if (instancedDraw)
		ScanForInstancedShaders();
}

void rage::grmShaderGroup::PreAllocateContainer(grmShaderGroup* containerInst) const
{
	u16 shaderCount = m_Shaders.GetSize();

	// We have to manually allocate items array (grmShader*) because otherwise
	// during allocation pgArray will allocate it on snapshot allocator but we need to force container
	// This operation is equal to atArray::Reserve(shaderCount)
	grmShader** elementList = new grmShader*[shaderCount];
	containerInst->m_Shaders.SetItems((pgUPtr<grmShader>*)elementList, 0, shaderCount);

	// Allocate shaders but nothing else! Otherwise shader variables will go in container too
	// and this is what we don't want
	for (u16 i = 0; i < shaderCount; i++)
	{
		containerInst->m_Shaders.Emplace(pgUPtr(new grmShader()));
	}
}

void rage::grmShaderGroup::CopyToContainer(grmShaderGroup* containerInst) const
{
	u16 shaderCount = m_Shaders.GetSize();

	// Add references, we have to do it manually
	// We also can't do this in pre-allocate because AddRef uses atArray which will mess with container
	pgSnapshotAllocator* virtualAllocator = pgRscCompiler::GetVirtualAllocator();
	virtualAllocator->AddRef(containerInst->m_Shaders.GetBufferRef());
	for (u16 i = 0; i < shaderCount; i++)
	{
		containerInst->m_Shaders[i].AddCompilerRef();
	}

	containerInst->m_EmbedTextures.Snapshot(m_EmbedTextures);
	containerInst->m_HasInstancedShader = m_HasInstancedShader;

	for (u16 i = 0; i < shaderCount; i++)
	{
		containerInst->m_Shaders[i]->CloneFrom(m_Shaders[i].GetRef());

		// On regular copying we don't care but when compiling we have to update all textures.
		// Textures that are not in embed dictionary must be replaced on references, that
		//	later game resolves by navigating TXD dependency linked list
		for (grcInstanceVar& var : containerInst->m_Shaders[i]->GetTextureIterator())
		{
			grcTexture* newTexture = containerInst->m_EmbedTextures->Find(var.GetTexture()->GetName());
			AM_ASSERT(newTexture, "grmShaderGroup::CopyToContainer() -> Reference textures are not supported.");
			var.SetTexture(newTexture);

			// Fix-up texture pointer
			pgRscCompiler::GetVirtualAllocator()->AddRef(var.Value);
		}
	}
}
