#include "fvf.h"

#include "device.h"
#include "fvfchannels.h"
#include "am/graphics/dxgi_utils.h"
#include "rage/atl/datahash.h"
#include "rage/system/new.h"
#include "effect.h"

rage::atFixedArray<
	rage::grcVertexDeclaration::DeclPair,
	rage::grcVertexDeclaration::MAX_CACHED_DECLS> rage::grcVertexDeclaration::sm_CachedDecls;

rage::grcVertexSemantic rage::GetVertexSemanticFromName(ConstString semanticName)
{
	u32 semanticHash = atStringHash(semanticName);
	switch (semanticHash)
	{
	case atStringHash("POSITION"):		return POSITION;
	case atStringHash("POSITIONT"):		return POSITIONT;
	case atStringHash("NORMAL"):		return NORMAL;
	case atStringHash("BINORMAL"):		return BINORMAL;
	case atStringHash("TANGENT"):		return TANGENT;
	case atStringHash("TEXCOORD"):		return TEXCOORD;
	case atStringHash("BLENDWEIGHT"):	return BLENDWEIGHT;
	case atStringHash("BLENDINDICES"):	return BLENDINDICES;
	case atStringHash("COLOR"):			return COLOR;

	default:
		AM_UNREACHABLE("GetVertexSemanticByName() -> %s is invalid name.", semanticName);
	}
}

void rage::grcFvf::UnpackChannels(
	grcVertexElement* elements, u32& elementsStartIndex,
	u8 channelBit, u8 semanticCount,
	grcVertexSemantic semantic, grcFormat overrideFormat) const
{
	u32 semanticIndex = 0;
	for (u8 i = 0; i < semanticCount; i++)
	{
		u8 currentChannelBit = channelBit + i;
		bool hasChannel = Channels & 1 << currentChannelBit;
		if (!hasChannel)
			continue;

		// Note that channel format information is packed in 4 bits
		// Calculate shift to get format:
		u8 formatShift = currentChannelBit * 4;

		// Now we can retrieve format itself
		grcFormat format = static_cast<grcFormat>(
			ChannelFormats >> formatShift & FORMAT_MASK);

		if (overrideFormat != GRC_FORMAT_INVALID)
			format = overrideFormat;

		grcVertexElement& element = elements[elementsStartIndex++];
		element.InputSlot = 0;
		element.Semantic = semantic;
		element.SemanticIndex = semanticIndex++;
		element.DataSize = grcFormatToDataSize[format];
		element.Format = format;
	}
}

u32 rage::grcFvf::UnpackAlternative(grcVertexElement* elements) const
{
	// Alternative channels additionally force format for all channels except BlendIndices
	u32 elementIndex = 0;
	UnpackChannels(elements, elementIndex, 0, 1, POSITION, GRC_FORMAT_R32G32B32A32_FLOAT);
	UnpackChannels(elements, elementIndex, 1, 1, BLENDWEIGHT, GRC_FORMAT_R32G32B32A32_FLOAT);
	UnpackChannels(elements, elementIndex, 2, 1, BLENDINDICES);
	UnpackChannels(elements, elementIndex, 3, 1, NORMAL, GRC_FORMAT_R32G32B32A32_FLOAT);
	UnpackChannels(elements, elementIndex, 4, 2, COLOR, GRC_FORMAT_R8G8B8A8_UNORM);
	UnpackChannels(elements, elementIndex, 6, 8, TEXCOORD, GRC_FORMAT_R32G32B32A32_FLOAT);
	UnpackChannels(elements, elementIndex, 14, 2, TANGENT, GRC_FORMAT_R32G32B32A32_FLOAT);
	UnpackChannels(elements, elementIndex, 16, 2, BINORMAL, GRC_FORMAT_R32G32B32A32_FLOAT);
	return elementIndex;
}

u32 rage::grcFvf::UnpackDefault(grcVertexElement* elements) const
{
	u32 elementIndex = 0;
	UnpackChannels(elements, elementIndex, 0, 1, POSITION);
	UnpackChannels(elements, elementIndex, 1, 1, BLENDWEIGHT);
	UnpackChannels(elements, elementIndex, 2, 1, BLENDINDICES);
	UnpackChannels(elements, elementIndex, 3, 1, NORMAL);
	UnpackChannels(elements, elementIndex, 4, 2, COLOR);
	UnpackChannels(elements, elementIndex, 6, 8, TEXCOORD);
	UnpackChannels(elements, elementIndex, 14, 2, TANGENT);
	UnpackChannels(elements, elementIndex, 16, 2, BINORMAL);
	return elementIndex;
}

u32 rage::grcFvf::Unpack(grcVertexElement* elements) const
{
	if (UseAlternativeChannels)
		return UnpackAlternative(elements);

	return UnpackDefault(elements);
}

void rage::grcFvf::SetChannel(u8 channelBit, bool used)
{
	Channels &= ~(1 << channelBit);
	if (used) Channels |= 1 << channelBit;
}

void rage::grcFvf::SetFormat(u8 channelBit, grcFormat fmt)
{
	// Note that we have to cast it to U64 here because otherwise shift overflows
	u64 formatShift = static_cast<u64>(channelBit) * 4;
	u64 formatMask = static_cast<u64>(0xF) << formatShift;

	ChannelFormats &= ~formatMask;
	ChannelFormats |= static_cast<u64>(fmt) << formatShift;
}

bool rage::grcVertexDeclaration::DeclPair::SortCompare(const DeclPair& lhs, const DeclPair& rhs)
{
	return lhs.HashKey < rhs.HashKey;
}

bool rage::grcVertexDeclaration::DeclPair::SearchCompare(const DeclPair& lhs, const u32& rhs)
{
	return lhs.HashKey < rhs;
}

rage::grcVertexDeclaration::~grcVertexDeclaration()
{
	if (--RefCount == 0)
		rage_free(this);
}

u32 rage::grcVertexDeclaration::GetElementOffset(u32 index) const
{
	AM_ASSERT(index < ElementCount,
		"grcVertexDeclaration::GetElementOffset() -> Index %u is out of bounds.", index);

	u32 offset = 0;
	for (u32 i = 0; i < index; i++)
	{
		DXGI_FORMAT format = Elements[i].Format;
		offset += rageam::graphics::DXGI::BytesPerPixel(format);
	}
	return offset;
}

void rage::grcVertexDeclaration::CleanUpCache()
{
	sm_CachedDecls.Clear();
}

void rage::grcVertexDeclaration::EncodeFvf(grcFvf& format) const
{
	memset(&format, 0, sizeof grcFvf);

	format.Stride = Stride;
	format.ChannelCount = ElementCount;
	format.UseAlternativeChannels = false;

	for (u32 i = 0; i < ElementCount; i++)
	{
		const grcVertexElementDesc& element = Elements[i];
		u8 channel = GetFvfChannelBySemanticName(element.SemanticName, element.SemanticIndex);
		if (channel == CHANNEL_INVALID)
			AM_UNREACHABLE("grcVertexDeclaration::EncodeFvf() -> Unsupported semantic %s", element.SemanticName);

		format.SetChannel(channel, true);
		format.SetFormat(channel, DXGIFormatToGrcFormat(element.Format));
	}
}

rage::grcVertexDeclaration* rage::grcVertexDeclaration::CreateFromFvf(const grcFvf& format)
{
	grcVertexElement elements[grcFvf::MAX_CHANNELS]{};
	u32 elementCount = format.Unpack(elements);
	return BuildCachedDeclarator(elements, elementCount);
}

rage::grcVertexDeclaration* rage::grcVertexDeclaration::BuildCachedDeclarator(const grcVertexElement* elements, u32 elementCount)
{
	// Simply hash all elements to get hash-sum
	u32 hashkey = 0;
	for (u32 i = 0; i < elementCount; i++)
		hashkey += atDataHash(elements + i, sizeof grcVertexElement, hashkey);

	// Try to retrieve existing declaration from cache using binary search
	DeclPair* cachedDecl = std::lower_bound(sm_CachedDecls.begin(), sm_CachedDecls.end(), hashkey, DeclPair::SearchCompare);
	if (cachedDecl != sm_CachedDecls.end())
		return cachedDecl->Declaration;

	// Create new declaration and cache it
	grcVertexDeclaration* decl = grcDevice::CreateVertexDeclaration(elements, elementCount);
	sm_CachedDecls.Add(DeclPair(hashkey, decl));
	sm_CachedDecls.Sort(DeclPair::SortCompare); // Array must be sorted for binary search

	return decl;
}

rage::grcVertexDeclaration* rage::grcVertexDeclaration::Allocate(u32 elementCount)
{
	// Calculate size of allocation including given number of attributes
	// Note: This is a C-Style dynamically sized struct, Attributes[1] is placeholder
	constexpr size_t baseSize =
		offsetof(grcVertexDeclaration, Elements) -
		offsetof(grcVertexDeclaration, ElementCount);
	size_t size = baseSize + sizeof grcVertexElementDesc * elementCount;

	grcVertexDeclaration* decl = static_cast<grcVertexDeclaration*>(rage_malloc(size));
	decl->ElementCount = elementCount;
	decl->Stride = 0;
	decl->RefCount = 1;
	return decl;
}

void rage::grcVertexFormatInfo::FromEffect(grcEffect* effect, bool skinned)
{
	ConstString techniqueName = skinned ? TECHNIQUE_DRAWSKINNED : TECHNIQUE_DRAW;

	grcEffectTechnique* technique = effect->GetTechnique(techniqueName);
	AM_ASSERT(technique, "VertexDeclaration::FromEffect() -> Effect '%s' is don't have draw technique!", effect->GetName());

	grcVertexProgram* vs = effect->GetVS(technique);
	AM_ASSERT(technique, "VertexDeclaration::FromEffect() -> Effect '%s' %s technique has NULL vertex shader!", effect->GetName(), techniqueName);

	vs->ReflectVertexFormat(&Decl, &Fvf);
}
