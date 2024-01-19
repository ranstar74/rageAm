#include "effect.h"

#include "rage/grcore/txd.h"

ConstString rage::EffectValueTypeToString(grcEffectVarType e)
{
	switch (e)
	{
	case EFFECT_VALUE_NONE:		return "None";
	case EFFECT_VALUE_INT:		return "Int";
	case EFFECT_VALUE_FLOAT:	return "Float";
	case EFFECT_VALUE_VECTOR2:	return "Vector2";
	case EFFECT_VALUE_VECTOR3:	return "Vector3";
	case EFFECT_VALUE_VECTOR4:	return "Vector4";
	case EFFECT_VALUE_TEXTURE:	return "Texture";
	case EFFECT_VALUE_BOOL:		return "Bool";
	case EFFECT_VALUE_MATRIX34: return "Matrix3x4";
	case EFFECT_VALUE_MATRIX44: return "Matrix4x4";
	case EFFECT_VALUE_STRING:	return "String";
	case EFFECT_VALUE_INT1:		return "Int";
	case EFFECT_VALUE_INT2:		return "Int2";
	case EFFECT_VALUE_INT3:		return "Int3";
	case EFFECT_VALUE_INT4:		return "Int4";
	}
	AM_UNREACHABLE("Type %i is not supported", e);
}

rage::grcInstanceVar::grcInstanceVar(const datResource& rsc)
{
	rsc.Fixup(m_ValuePtr); // TODO: For now, we need smart pointer

	if (IsTexture())
	{
		// TODO:
		// Implement actual grcTextureFactoryDX11::PlaceTexture,
		// this does support regular textures only now

		/*grcTextureDX11* texture = GetValuePtr<grcTextureDX11>();
		rsc.Place(texture);*/

		grcTexture::Place(rsc, GetValuePtr<grcTexture>());
	}
}

void rage::grcInstanceData::CleanUp()
{
	if (m_Flags & 1) // OwnPreset
	{
		delete m_Preset.Ptr;
		m_Preset.Ptr = nullptr;
		m_Flags &= ~1;
	}
}

rage::grcInstanceData::grcInstanceData(const grcEffect* effect)
{
	m_Flags = 0;

	CloneFrom(effect->GetInstanceDataTemplate());
}

void rage::grcInstanceData::CloneFrom(const grcInstanceData& other)
{
	CleanUp();

	pgRscCompiler* compiler = pgRscCompiler::GetCurrent();
	pgSnapshotAllocator* virtualAllocator = pgRscCompiler::GetVirtualAllocator();

	if (compiler)
	{
		const grcEffect* otherEffect = other.GetEffect();

		char presetName[64];
		sprintf_s(presetName, sizeof presetName, "%s.sps", otherEffect->GetName());

		m_Preset.NameHash = atStringHash(presetName);
		m_Effect.NameHash = otherEffect->GetNameHash();
	}
	else
	{
		m_Effect.Ptr = other.GetEffect();
	}

	m_VarCount = other.m_VarCount;
	m_VarsAndValuesSize = other.m_VarsAndValuesSize;
	m_VarsSize = other.m_VarsSize;
	m_DrawBucket = other.m_DrawBucket;
	m_DrawBucketMask = other.m_DrawBucketMask;
	m_IsInstancedDraw = other.m_IsInstancedDraw;
	m_TextureVarCount = other.m_TextureVarCount;

	m_Flags = other.m_Flags & ~1; // Exclude OwnPreset

	m_Unknown12 = other.m_Unknown12;
	m_Unknown26 = other.m_Unknown26;
	m_Unknown25 = other.m_Unknown25;

	// Copy vars
	if (compiler)
	{
		m_Vars = (grcInstanceVar*)virtualAllocator->Allocate(m_VarsAndValuesSize);
		virtualAllocator->AddRef(m_Vars);
	}
	else
	{
		m_Vars = (grcInstanceVar*)rage_malloc(m_VarsAndValuesSize);
	}
	memcpy(m_Vars, other.m_Vars, m_VarsAndValuesSize);

	// We have to manually fixup variables data pointers to newly allocated data block
	// Textures can be skipped because they point to heap
	for (u16 i = m_TextureVarCount; i < m_VarCount; i++)
	{
		grcInstanceVar& newVar = m_Vars[i];
		grcInstanceVar& oldVar = other.m_Vars[i];

		// Textures are stored outside of var data block
		if (newVar.IsTexture())
			continue;

		// This is not the cleanest way to do it but the fastest
		// We first calculate offset of variable in old data block and then add it to base pointer of new data block
		u64 offset = reinterpret_cast<u64>(oldVar.m_ValuePtr) - reinterpret_cast<u64>(other.m_Vars);

		newVar.m_ValuePtr = reinterpret_cast<char*>(reinterpret_cast<u64>(m_Vars) + offset);
	}

	// Add compiler refs to var data pointers
	if (compiler)
	{
		for (u8 i = 0; i < m_VarCount; i++)
		{
			grcInstanceVar& var = m_Vars[i];

			// We only store texture name hash
			// TODO: Not true for embed textures!
			if (var.IsTexture())
			{
				var.m_ValuePtr = nullptr;
				continue;
			}

			virtualAllocator->AddRef(var.m_ValuePtr);
		}
	}
}

rage::grcInstanceData::grcInstanceData(const grcInstanceData& other)
{
	CloneFrom(other);
}

// ReSharper disable CppPossiblyUninitializedMember
// ReSharper disable CppObjectMemberMightNotBeInitialized
rage::grcInstanceData::grcInstanceData(const datResource& rsc)
{
	#ifdef AM_STANDALONE
	AM_WARNING("grcInstanceData -> Not supported in standalone.");
	return;
	#endif
	//	// NOTE: This is very simplified implementation comparing to native implementation,
	//	// it seems that game allows serialized (resource binary) instance data
	//	// to skip some variables and few other minor things.
	//	// I decided that they are not necessary, I added assert to ensure that game
	//	// don't have such resources

	rsc.Fixup(m_Vars); // TODO: pgUPtr

	// Resolve effect by name hash
	u32 effectNameHash = m_Effect.NameHash;
	grcEffect* effect = FindEffectByHashKey(effectNameHash);
	m_Effect.Ptr = effect;

	AM_ASSERT(effect != nullptr,
		"grcInstanceData -> Effect %u is not preloaded or doesn't exists.", effectNameHash);

	// Resolve data preset
	// As explained above, presets contain default values but we ignore them (for now)
	u32 presetNameHash = m_Preset.NameHash;
	grcInstanceData* dataPreset = LookupShaderPreset(presetNameHash);
	m_Preset.Ptr = dataPreset;

	AM_ASSERT(m_VarCount == effect->GetVarCount(),
		"grcInstanceData -> Parameter count mismatch! Report this to ranstar74.");

	AM_ASSERT(dataPreset->GetEffect() == effect,
		"grcInstanceData -> Preset use different effect.");

	// Fixup variables
	for (u16 i = 0; i < m_VarCount; i++)
	{
		grcInstanceVar* var = &m_Vars[i];
		rsc.Place(var);
	}

	UpdateTessellationBucket();
}
// ReSharper restore CppPossiblyUninitializedMember
// ReSharper restore CppObjectMemberMightNotBeInitialized

rage::grcInstanceData::~grcInstanceData()
{
	CleanUp();
}

void rage::grcInstanceData::UpdateTessellationBucket()
{
	bool tesselalationOn = false;
	fxHandle_t useTesselationHandle = GetEffect()->LookupVarByName("usetessellation");
	if (useTesselationHandle != INVALID_FX_HANDLE) // Check if shader supports tessellation at all
	{
		grcInstanceVar* useTesselationVar = GetVar(useTesselationHandle);
		tesselalationOn = useTesselationVar->GetValue<float>() == 1.0f;
	}
	m_DrawBucketMask.SetTessellated(tesselalationOn);
}

void rage::grcInstanceData::CopyVars(const grcInstanceData* to) const
{
	grcEffect* lhsEffect = GetEffect();
	grcEffect* rhsEffect = to->GetEffect();

	for (u16 i = 0; i < m_VarCount; i++)
	{
		grcEffectVar* varInfo = lhsEffect->GetVarByIndex(i);

		// Attempt to find variable in destination effect
		fxHandle_t rhsHandle = rhsEffect->LookupVarByHashKey(varInfo->GetNameHash());
		if (rhsHandle == INVALID_FX_HANDLE)
			continue;

		u16 rhsIndex = fxHandleToIndex(rhsHandle);

		grcInstanceVar* lhsVar = GetVarByIndex(i);
		grcInstanceVar* rhsVar = to->GetVarByIndex(rhsIndex);

		grcEffectVarType lhsType = varInfo->GetType();
		grcEffectVarType rhsType = rhsEffect->GetVarByIndex(rhsIndex)->GetType();

		// Although variables may have the same name, we must ensure that type matches too
		if (lhsType != rhsType)
			continue;

		// Now we can copy data to destination
		if (varInfo->GetType() == EFFECT_VALUE_TEXTURE)
		{
			rhsVar->SetTexture(lhsVar->GetTexture());
		}
		else
		{
			void* src = lhsVar->GetValuePtr<void>();
			void* dst = rhsVar->GetValuePtr<void>();
			// Data is stored in 16 byte blocks, this was most likely made to iterate them easier (game does that)
			// another possible explanation is that this is union array with Vector4 as largest element
			//
			// Also It would make sense that matrices are array of Vector4 because
			// otherwise data block would take 64 bytes instead of 16
			u8 dataSize = 16 * lhsVar->GetValueCount();
			memcpy(dst, src, dataSize);
		}
	}
}

void rage::grcInstanceData::SetEffect(grcEffect* effect, bool keepValues)
{
	CloneFrom(effect->GetInstanceDataTemplate());

	// TODO: ...
	//u16 effectVarCount = effect->GetVarCount();

	//m_Effect.Ptr = effect;
	//m_VarCount = static_cast<u8>(effectVarCount);
	//m_Vars = new grcInstanceVar[effectVarCount]{};

	//// Allocate data block for every variable
	//for (u16 i = 0; i < effectVarCount; i++)
	//{
	//	grcInstanceVar& var = m_Vars[i];
	//	grcEffectVar* varInfo = effect->GetVarByIndex(i);
	//	grcEffectVarType type = varInfo->GetType();

	//	u8 dataSize = grcEffectVarTypeToStride[type];
	//	if (dataSize == 0) // EFFECT_VALUE_NONE
	//		continue;

	//	// For textures we don't have to allocate value block because it's stored directly
	//	if (type != EFFECT_VALUE_TEXTURE)
	//		var.m_ValuePtr = new char[dataSize];

	//	if (type == EFFECT_VALUE_TEXTURE)
	//		var.SamplerIndex = 3; // For now...

	//	var.m_ValueCount = 1; // For now...

	//	// Now we have to properly initialize it with default value
	//	// We can't just rely on memset for floats & matrices
	//	switch (type)
	//	{
	//	case EFFECT_VALUE_NONE: break;
	//	case EFFECT_VALUE_INT1:
	//	case EFFECT_VALUE_INT: var.SetValue(0); break;
	//	case EFFECT_VALUE_INT2: /* TODO */ break;
	//	case EFFECT_VALUE_INT3: /* TODO */ break;
	//	case EFFECT_VALUE_INT4: /* TODO */ break;
	//	case EFFECT_VALUE_FLOAT:var.SetValue(0.0f); break;
	//	case EFFECT_VALUE_VECTOR2: var.SetValue(Vector2(0.0f)); break;
	//	case EFFECT_VALUE_VECTOR3: var.SetValue(Vector3(0.0f)); break;
	//	case EFFECT_VALUE_VECTOR4: var.SetValue(Vector4(0.0f)); break;
	//	case EFFECT_VALUE_TEXTURE: var.SetTexture(nullptr); break;
	//	case EFFECT_VALUE_BOOL: var.SetValue(false); break;
	//	case EFFECT_VALUE_MATRIX34: /* TODO: Identity */ break;
	//	case EFFECT_VALUE_MATRIX44: /* TODO: Identity */ break;
	//	case EFFECT_VALUE_STRING: var.SetValue(nullptr);
	//	}
	//}

	//if (keepValues)
	//{
	//	// TODO: ...
	//}

	//// TODO:
	//// We should probably update draw bucket and draw bucket mask, and
	//// with that we have to recompute draw mask for rmcDrawable
}
