#include "texturereference.h"

#include "txd.h"

rage::grcTextureReference::grcTextureReference(ConstString name, const pgPtr<grcTexture>& ref)
	: grcTexture(TEXTURE_REFERENCE, name)
{
	SetReference(ref);
}

rage::grcTextureReference::grcTextureReference(const datResource& rsc)
	: grcTexture(rsc)
{
	atHashValue nameHash = atStringHash(m_Name);

	// Traverse TXD parent hierarchy in starting from embed (if exists) to last linked
	grcTextureDictionary* dict = grcTextureDictionary::GetCurrent();
	while (dict)
	{
		int texIndex = dict->IndexOf(nameHash);
		if (texIndex != -1)
		{
			SetReference(dict->GetValueRefAt(texIndex));
			break;
		}

		dict = dict->GetParent().Get();
	}

	if (!m_Reference)
	{
		AM_WARNINGF("rage::grcTextureReference -> Unable to resolve referenced texture '%s'!", m_Name.GetCStr());

		// TODO: Not found texture...
	}
}

void rage::grcTextureReference::SetReference(const pgPtr<grcTexture>& ref)
{
	if (ref == nullptr)
	{
		m_Reference = nullptr;
		m_CachedTexture = nullptr;
		return;
	}

	AM_ASSERT(ref.Get() != this, "grcTextureReference::SetReference() -> Attempt to set reference on itself!");
	m_Reference = ref;
	m_CachedTexture = ref->m_CachedTexture;
}
