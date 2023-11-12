#include "drawable.h"

#include "drawablehelpers.h"
#include "hotdrawable.h"
#include "am/asset/factory.h"
#include "am/file/iterator.h"
#include "am/graphics/buffereditor.h"
#include "am/graphics/meshsplitter.h"
#include "am/graphics/render/context.h"
#include "am/xml/iterator.h"
#include "rage/grcore/effect/effectmgr.h"
#include "rage/math/math.h"
#include "rage/physics/bounds/composite.h"
#include "rage/physics/bounds/geometry.h"

void rageam::asset::MaterialTune::Param::Serialize(XmlHandle& xml) const
{
	XML_SET_ATTR(xml, Name);
	XML_SET_ATTR(xml, Type);

	switch (Type)
	{
	case Unsupported:
	case None:															break;
	case String:	xml.SetValue(std::any_cast<string>(Value));			break;
	case Bool:		xml.SetValue(std::any_cast<bool>(Value));			break;
	case Float:		xml.SetValue(std::any_cast<float>(Value));			break;
	case Vec2F:		xml.SetValue(std::any_cast<rage::Vector2>(Value));	break;
	case Vec3F:		xml.SetValue(std::any_cast<rage::Vector3>(Value));	break;
	case Vec4F:		xml.SetValue(std::any_cast<rage::Vector4>(Value));	break;
	case Color:		xml.SetColorHex(std::any_cast<u32>(Value));			break;
	}
}

void rageam::asset::MaterialTune::Param::Deserialize(const XmlHandle& xml)
{
	XML_GET_ATTR(xml, Name);
	XML_GET_ATTR(xml, Type);

	// We can't use XmlHandle::GetValue functions with std::any so use this way with macro
#define GET_VALUE(type) { type v; xml.GetValue(v); Value = std::move(v); } MACRO_END

	switch (Type)
	{
	case Unsupported:
	case None:											break;
	case Bool:		GET_VALUE(bool);					break;
	case Float:		GET_VALUE(float);					break;
	case Vec2F:		GET_VALUE(rage::Vector2);			break;
	case Vec3F:		GET_VALUE(rage::Vector3);			break;
	case Vec4F:		GET_VALUE(rage::Vector4);			break;
	case Color:		Value = xml.GetColorHex();			break;
	case String:	Value = string(xml.GetText(false)); break; // We allow empty/null strings
	}

#undef GET_VALUE
}

void rageam::asset::MaterialTune::Param::CopyValue(rage::grcInstanceVar* var)
{
	switch (Type)
	{
	case Unsupported:
	case None:													break;
	case String:	Value = string("");							break;
	case Bool:		Value = var->GetValue<bool>();				break;
	case Float:		Value = var->GetValue<float>();				break;
	case Vec2F:		Value = var->GetValue<rage::Vector2>();		break;
	case Vec3F:		Value = var->GetValue<rage::Vector3>();		break;
	case Vec4F:		Value = var->GetValue<rage::Vector4>();		break;
	case Color:		Value = 0;									break;
	}

	// Retrieve texture name
	if (var->IsTexture() && var->GetTexture())
	{
		Value = string(var->GetTexture()->GetName());
	}
}

void rageam::asset::MaterialTune::Deserialize(const XmlHandle& node)
{
	SceneTune::Deserialize(node);
	XML_GET_ATTR(node, Effect);
	XML_GET_ATTR(node, DrawBucket);
	XML_GET_ATTR(node, IsDefault);
	for (const XmlHandle& xParam : XmlIterator(node, "Param"))
	{
		Param param;
		param.Deserialize(xParam);
		Params.Emplace(std::move(param));
	}
}

void rageam::asset::MaterialTune::Serialize(XmlHandle& node) const
{
	SceneTune::Serialize(node);
	XML_SET_ATTR(node, Effect);
	XML_SET_ATTR(node, DrawBucket);
	XML_SET_ATTR(node, IsDefault);
	for (const Param& param : Params)
	{
		XmlHandle xParam = node.AddChild("Param");
		param.Serialize(xParam);
	}
}

rage::grcEffect* rageam::asset::MaterialTune::LookupEffect() const
{
	rage::grcEffect* effect = rage::grcEffectMgr::FindEffect(Effect);
	if (!effect)
	{
		AM_ERRF("MaterialTune::LookupEffect() -> Shader effect with name '%s.fxc' doesn't exists.", Effect.GetCStr());
		return nullptr;
	}
	return effect;
}

rage::grcEffect* rageam::asset::MaterialTune::LookupEffectOrUseDefault() const
{
	rage::grcEffect* effect = LookupEffect();
	if (!effect)
	{
		AM_WARNINGF("MaterialTune::LookupEffectOrUseDefault() -> Failed to find shader '%s'.fxc, using '%s'.fxc instead.",
			Effect.GetCStr(), DEFAULT_SHADER);

		effect = rage::grcEffectMgr::FindEffect(DEFAULT_SHADER);
	}
	return effect;
}

void rageam::asset::MaterialTune::InitFromEffect()
{
	// Initialize from effect shader template
	rage::grcEffect* effect = LookupEffectOrUseDefault();
	InitFromShader(&effect->GetInstanceDataTemplate());
}

void rageam::asset::MaterialTune::InitFromShader(const rage::grcInstanceData* shader)
{
	rage::grcEffect* effect = shader->GetEffect();

	u16 effectVarCount = effect->GetVarCount();

	Params.Destroy();
	Params.Reserve(effectVarCount);

	for (u16 i = 0; i < effectVarCount; i++)
	{
		rage::grcEffectVar* varInfo = effect->GetVarByIndex(i);
		rage::grcInstanceVar* var = shader->GetVarByIndex(i);

		Param param;
		param.Name = varInfo->GetName();
		param.Type = grcEffectVarTypeToParamType[varInfo->GetType()];
		param.CopyValue(var);

		Params.Emplace(std::move(param));
	}

	Effect = effect->GetName();
	DrawBucket = shader->GetDrawBucket();
}

void rageam::asset::MaterialTune::SetTextureNamesFromSceneMaterial(const graphics::SceneMaterial* sceneMaterial)
{
	rage::grcEffect* effect = LookupEffectOrUseDefault();

	u16 effectVarCount = effect->GetVarCount();

	AM_ASSERT(Params.GetSize() == effectVarCount,
		"MaterialTune::SetTextureNamesFromSceneMaterial() -> Param count don't match effect.");

	for (u16 i = 0; i < effectVarCount; i++)
	{
		rage::grcEffectVar* varInfo = effect->GetVarByIndex(i);
		if (!varInfo->IsTexture())
			continue;

		graphics::eMaterialTexture textureType = graphics::MT_Unknown;

		// We don't have concrete way to detect sampler type right now so just search in string
		string varName = varInfo->GetName();
		if (varName.Contains("diff", true))			textureType = graphics::MT_Diffuse;
		else if (varName.Contains("norm", true))	textureType = graphics::MT_Normal;
		else if (varName.Contains("bump", true))	textureType = graphics::MT_Normal;
		else if (varName.Contains("spec", true))	textureType = graphics::MT_Specular;

		// Attempt to match automatically tune texture slot to shader slot
		if (textureType != graphics::MT_Unknown)
		{
			Params[i].Value = string(sceneMaterial->GetTextureName(textureType));
		}
		else
		{
			AM_WARNINGF(
				"MaterialTune::SetTextureNamesFromSceneMaterial() -> Unable to resolve texture type for shader parameter '%s'",
				varInfo->GetName());
		}
	}
}

bool rageam::asset::MaterialTuneGroup::ExistsInScene(graphics::Scene* scene, const SceneTunePtr& tune) const
{
	return scene->GetMaterialByName(tune->Name);
}

rageam::asset::SceneTunePtr rageam::asset::MaterialTuneGroup::CreateTune() const
{
	return std::make_shared<MaterialTune>();
}

rageam::asset::SceneTunePtr rageam::asset::MaterialTuneGroup::CreateDefaultTune(graphics::Scene* scene, u16 itemIndex) const
{
	amPtr<MaterialTune> materialTune;

	// When default material is required we reserve the first index for it,
	// so we have to shift index by 1 for other items
	bool needDefaultMaterial = scene->NeedDefaultMaterial();
	bool isDefault = needDefaultMaterial && itemIndex == 0;
	if (needDefaultMaterial && isDefault)
	{
		materialTune = std::make_shared<MaterialTune>(DEFAULT_MATERIAL_NAME, true);
		materialTune->InitFromEffect();
	}
	else
	{
		u16 materialIndex = needDefaultMaterial ? itemIndex - 1 : itemIndex;

		graphics::SceneMaterial* sceneMaterial = scene->GetMaterial(materialIndex);
		materialTune = std::make_shared<MaterialTune>(sceneMaterial->GetName(), false);
		materialTune->InitFromEffect();
		materialTune->SetTextureNamesFromSceneMaterial(sceneMaterial);
	}

	return materialTune;
}

ConstString rageam::asset::MaterialTuneGroup::GetItemName(graphics::Scene* scene, u16 itemIndex) const
{
	// Same as in CreateDefaultTune
	bool needDefaultMaterial = scene->NeedDefaultMaterial();
	bool isDefault = needDefaultMaterial && itemIndex == 0;
	u16 materialIndex = needDefaultMaterial ? itemIndex - 1 : itemIndex;
	return isDefault ? DEFAULT_MATERIAL_NAME : scene->GetMaterial(materialIndex)->GetName();
}

u16 rageam::asset::MaterialTuneGroup::GetSceneItemCount(graphics::Scene* scene) const
{
	// We add extra index for implicit default material, if required
	u16 matCount = scene->GetMaterialCount();
	return scene->NeedDefaultMaterial() ? matCount + 1 : matCount;
}

void rageam::asset::ModelTune::Serialize(XmlHandle& node) const
{
	SceneTune::Serialize(node);
	XML_SET_CHILD_VALUE(node, UseAsBound);
	XML_SET_CHILD_VALUE(node, CreateBone);
}

void rageam::asset::ModelTune::Deserialize(const XmlHandle& node)
{
	SceneTune::Deserialize(node);
	XML_GET_CHILD_VALUE(node, UseAsBound);
	XML_GET_CHILD_VALUE(node, CreateBone);
}

bool rageam::asset::ModelTuneGroup::ExistsInScene(graphics::Scene* scene, const SceneTunePtr& tune) const
{
	return scene->GetNodeByName(tune->Name) != nullptr;
}

rageam::asset::SceneTunePtr rageam::asset::ModelTuneGroup::CreateTune() const
{
	return std::make_shared<ModelTune>();
}

rageam::asset::SceneTunePtr rageam::asset::ModelTuneGroup::CreateDefaultTune(graphics::Scene* scene, u16 itemIndex) const
{
	return std::make_shared<ModelTune>();
}

ConstString rageam::asset::ModelTuneGroup::GetItemName(graphics::Scene* scene, u16 itemIndex) const
{
	return scene->GetNode(itemIndex)->GetName();
}

u16 rageam::asset::ModelTuneGroup::GetSceneItemCount(graphics::Scene* scene) const
{
	return scene->GetNodeCount();
}

void rageam::asset::LightTune::InitFromSceneLight(const graphics::SceneNode* lightNode)
{
	graphics::SceneLight* sceneLight = lightNode->GetLight();

	switch (sceneLight->GetType())
	{
	case graphics::SceneLight_Point:	Type = Point;	break;
	case graphics::SceneLight_Spot:		Type = Spot;	break;
	}

	ColorRGB = sceneLight->GetColor();
	Falloff = Type == LIGHT_SPOT ? 8.0f : 2.5f; // Make light longer if spot light is chosen
	FalloffExponent = 32;
	ConeOuterAngle = rage::Math::RadToDeg(sceneLight->GetOuterConeAngle());
	ConeInnerAngle = rage::Math::RadToDeg(sceneLight->GetInnerConeAngle());
	Flags = LF_ENABLE_SHADOWS;
	TimeFlags = LIGHT_TIME_ALWAYS_MASK;
	CoronaZBias = 0.1;
	CoronaSize = 5.0f;
	Intensity = 75.0f;
	CapsuleLength = 1.0f;
	ShadowNearClip = 0.01;
	CullingPlaneNormal = { 0, 1, 0 };
	CullingPlaneOffset = 1;
	VolumeOuterColorRGB = graphics::COLOR_WHITE;
	VolumeOuterExponent = 1;
	VolumeSizeScale = 1.0f;

	Mat44V transform = lightNode->GetLocalTransform();
	Position = rage::Vec3V(transform.Pos);
	Direction = -rage::Vec3V(transform.Up);
	Tangent = rage::Vector3(transform.Right);
}

void rageam::asset::LightTune::FromLightAttr(const CLightAttr* attr)
{
	Type = LightType(attr->Type); // Enum fully matches
	Position = attr->Position;
	Direction = attr->Direction;
	Tangent = attr->Tangent;
	ColorRGB = graphics::ColorU32(attr->ColorR, attr->ColorG, attr->ColorB);
	Flashiness = attr->Flashiness;
	Flags = attr->Flags;
	TimeFlags = attr->TimeFlags;
	Intensity = attr->Intensity;
	Falloff = attr->Falloff;
	FalloffExponent = attr->FallofExponent;
	CullingPlaneNormal = attr->CullingPlaneNormal;
	CullingPlaneOffset = attr->CullingPlaneOffset;
	ShadowBlur = attr->ShadowBlur;
	ShadowNearClip = attr->ShadowNearClip;
	CoronaSize = attr->CoronaSize;
	CoronaIntensity = attr->CoronaIntensity;
	CoronaZBias = attr->CoronaZBias;
	LightHash = attr->LightHash;
	// ProjectedTexture = attr->ProjectedTexture;
	LightFadeDistance = attr->LightFadeDistance;
	ShadowFadeDistance = attr->ShadowFadeDistance;
	SpecularFadeDistance = attr->SpecularFadeDistance;
	VolumetricFadeDistance = attr->VolumetricFadeDistance;

	VolumeIntensity = attr->VolumeIntensity;
	VolumeSizeScale = attr->VolumeSizeScale;
	VolumeOuterIntensity = attr->VolumeOuterIntensity;
	VolumeOuterExponent = attr->VolumeOuterExponent;
	VolumeOuterColorRGB = graphics::ColorU32(
		attr->VolumeOuterColorR, attr->VolumeOuterColorG, attr->VolumeOuterColorB);
	ConeInnerAngle = attr->ConeInnerAngle;
	ConeOuterAngle = attr->ConeOuterAngle;

	CapsuleLength = attr->Extent.X;
}

bool rageam::asset::LightTuneGroup::ExistsInScene(graphics::Scene* scene, const SceneTunePtr& tune) const
{
	graphics::SceneNode* sceneNode = scene->GetNodeByName(tune->Name);
	return sceneNode && sceneNode->HasLight();
}

rageam::asset::SceneTunePtr rageam::asset::LightTuneGroup::CreateTune() const
{
	return std::make_shared<LightTune>();
}

rageam::asset::SceneTunePtr rageam::asset::LightTuneGroup::CreateDefaultTune(graphics::Scene* scene, u16 itemIndex) const
{
	amPtr<LightTune> lightTune = std::make_shared<LightTune>();
	lightTune->InitFromSceneLight(scene->GetLight(itemIndex));
	return lightTune;
}

ConstString rageam::asset::LightTuneGroup::GetItemName(graphics::Scene* scene, u16 itemIndex) const
{
	return scene->GetLight(itemIndex)->GetName();
}

u16 rageam::asset::LightTuneGroup::GetSceneItemCount(graphics::Scene* scene) const
{
	return scene->GetLightCount();
}

void rageam::asset::LightTune::Serialize(XmlHandle& node) const
{
	SceneTune::Serialize(node);

	XML_SET_CHILD_VALUE(node, Type);
	node.AddChild("Position").SetValue(Position);
	node.AddChild("Direction").SetValue(Direction);
	node.AddChild("Tangent").SetValue(Tangent);
	node.AddChild("ColorRGB").SetColorHex(ColorRGB);
	XML_SET_CHILD_VALUE(node, Flashiness);
	node.AddChild("Flags", String::FormatTemp("%X", Flags));
	node.AddChild("TimeFlags", String::FormatTemp("%X", TimeFlags));
	XML_SET_CHILD_VALUE(node, Intensity);
	XML_SET_CHILD_VALUE(node, Falloff);
	XML_SET_CHILD_VALUE(node, FalloffExponent);
	node.AddChild("CullingPlaneNormal").SetValue(CullingPlaneNormal);
	XML_SET_CHILD_VALUE(node, CullingPlaneOffset);
	XML_SET_CHILD_VALUE(node, ShadowBlur);
	XML_SET_CHILD_VALUE(node, ShadowNearClip);
	XML_SET_CHILD_VALUE(node, CoronaSize);
	XML_SET_CHILD_VALUE(node, CoronaIntensity);
	XML_SET_CHILD_VALUE(node, CoronaZBias);
	//XML_SET_CHILD_VALUE(xLight, LightHash);
	//XML_SET_CHILD_VALUE(xLight, ProjectedTexture);
	XML_SET_CHILD_VALUE(node, LightFadeDistance);
	XML_SET_CHILD_VALUE(node, ShadowFadeDistance);
	XML_SET_CHILD_VALUE(node, SpecularFadeDistance);
	XML_SET_CHILD_VALUE(node, VolumetricFadeDistance);

	XML_SET_CHILD_VALUE(node, VolumeIntensity);
	XML_SET_CHILD_VALUE(node, VolumeSizeScale);
	XML_SET_CHILD_VALUE(node, VolumeOuterIntensity);
	XML_SET_CHILD_VALUE(node, VolumeOuterExponent);
	node.AddChild("VolumeOuterColorRGB").SetColorHex(VolumeOuterColorRGB);
	XML_SET_CHILD_VALUE(node, ConeInnerAngle);
	XML_SET_CHILD_VALUE(node, ConeOuterAngle);

	XML_SET_CHILD_VALUE(node, CapsuleLength);
}

void rageam::asset::LightTune::Deserialize(const XmlHandle& node)
{
	SceneTune::Deserialize(node);

	// TODO: This code is kinda bad

	auto scanHex = [](ConstString str) -> u32
		{
			float val = 0;
			AM_ASSERT(sscanf_s(str, "%X", &val) > 0, "LightTune::Deserialize() -> Failed to parse hex value '%s'", str);
			return val;
		};

	XML_GET_CHILD_VALUE(node, Type);
	node.GetChild("Position").GetValue(Position);
	node.GetChild("Direction").GetValue(Direction);
	node.GetChild("Tangent").GetValue(Tangent);
	ColorRGB = node.GetChild("ColorRGB").GetColorHex();
	XML_GET_CHILD_VALUE(node, Flashiness);
	Flags = scanHex(node.GetChild("Flags").GetText());
	TimeFlags = scanHex(node.GetChild("TimeFlags").GetText());
	XML_GET_CHILD_VALUE(node, Intensity);
	XML_GET_CHILD_VALUE(node, Falloff);
	XML_GET_CHILD_VALUE(node, FalloffExponent);
	node.GetChild("CullingPlaneNormal").SetValue(CullingPlaneNormal);
	XML_GET_CHILD_VALUE(node, CullingPlaneOffset);
	XML_GET_CHILD_VALUE(node, ShadowBlur);
	XML_GET_CHILD_VALUE(node, ShadowNearClip);
	XML_GET_CHILD_VALUE(node, CoronaSize);
	XML_GET_CHILD_VALUE(node, CoronaIntensity);
	XML_GET_CHILD_VALUE(node, CoronaZBias);
	//XML_SET_CHILD_VALUE(xLight, LightHash);
	//XML_SET_CHILD_VALUE(xLight, ProjectedTexture);
	XML_GET_CHILD_VALUE(node, LightFadeDistance);
	XML_GET_CHILD_VALUE(node, ShadowFadeDistance);
	XML_GET_CHILD_VALUE(node, SpecularFadeDistance);
	XML_GET_CHILD_VALUE(node, VolumetricFadeDistance);

	XML_GET_CHILD_VALUE(node, VolumeIntensity);
	XML_GET_CHILD_VALUE(node, VolumeSizeScale);
	XML_GET_CHILD_VALUE(node, VolumeOuterIntensity);
	XML_GET_CHILD_VALUE(node, VolumeOuterExponent);
	VolumeOuterColorRGB = node.GetChild("VolumeOuterColorRGB").GetColorHex();
	XML_GET_CHILD_VALUE(node, ConeInnerAngle);
	XML_GET_CHILD_VALUE(node, ConeOuterAngle);

	XML_GET_CHILD_VALUE(node, CapsuleLength);
}

void rageam::asset::LodGroupTune::Serialize(XmlHandle& node) const
{
	node.AddComment("Maximum LOD render distance's (L0 to L4)");

	// Serialize threshold as vector4
	XmlHandle xLodThreshold = node.AddChild("LodThreshold");
	rage::Vector4 lodThreshold;
	lodThreshold.FromArray(LodThreshold);
	xLodThreshold.SetValue(lodThreshold);

	Models.Serialize(node);
}

void rageam::asset::LodGroupTune::Deserialize(const XmlHandle& node)
{
	// Deserialize threshold from vector4
	XmlHandle xLodThreshold = node.GetChild("LodThreshold");
	rage::Vector4 lodThreshold;
	xLodThreshold.GetValue(lodThreshold);
	lodThreshold.ToArray(LodThreshold);

	// Make sure that lod distances are incrementally continuous
	for (int i = 0; i < MAX_LOD - 1; i++)
	{
		float current = LodThreshold[i];
		float next = LodThreshold[i + 1];

		if (next >= current)
			return;

		AM_WARNINGF("LOD %i distance threshold (%g) is less than previos LOD %i (%g), clamping value...",
			i + 1, next, i, current);

		LodThreshold[i + 1] = current;
	}

	Models.Deserialize(node);
}

void rageam::asset::DrawableTune::Serialize(XmlHandle& node) const
{
	node.AddChild("SceneFile", String::ToUtf8Temp(SceneFileName));

	XmlHandle xLodGroup = node.AddChild("LodGroup");
	Lods.Serialize(xLodGroup);

	Materials.Serialize(node);
	Lights.Serialize(node);
}

void rageam::asset::DrawableTune::Deserialize(const XmlHandle& node)
{
	ConstString fileName;
	node.GetChildText("SceneFile", &fileName, false);
	SceneFileName = String::Utf8ToWideTemp(fileName);

	XmlHandle xLodGroup = node.GetChild("LodGroup");
	Lods.Deserialize(xLodGroup);

	Materials.Deserialize(node);
	Lights.Deserialize(node);
}

rage::grmModel* rageam::asset::DrawableAssetMap::GetModelFromScene(rage::rmcDrawable* drawable, u16 sceneNodeIndex) const
{
	const ModelHandle& handle = SceneNodeToModel[sceneNodeIndex];
	if (handle.Index == u16(-1))
		return nullptr;
	return drawable->GetLodGroup().GetLod(handle.Lod)->GetModel(handle.Index);
}

rage::crBoneData* rageam::asset::DrawableAssetMap::GetBoneFromScene(const rage::rmcDrawable* drawable, u16 sceneNodeIndex) const
{
	u16 boneIndex = SceneNodeToBone[sceneNodeIndex];
	if (boneIndex == u16(-1))
		return nullptr;
	return drawable->GetSkeletonData()->GetBone(boneIndex);
}

rage::phBound* rageam::asset::DrawableAssetMap::GetBoundFromScene(const gtaDrawable* drawable, u16 sceneNodeIndex) const
{
	u16 boundIndex = SceneNodeToBound[sceneNodeIndex];
	if (boundIndex == u16(-1))
		return nullptr;
	return ((rage::phBoundComposite*)drawable->GetBound())->GetBound(boundIndex).Get();
}

CLightAttr* rageam::asset::DrawableAssetMap::GetLightFromScene(gtaDrawable* drawable, u16 sceneNodeIndex) const
{
	u16 lightAttrIndex = SceneNodeToLightAttr[sceneNodeIndex];
	if (lightAttrIndex == u16(-1))
		return nullptr;
	return drawable->GetLight(lightAttrIndex);
}

bool rageam::asset::DrawableAsset::CacheEffects()
{
	m_EffectCache.Destruct();

	MaterialTuneGroup& matGroup = m_DrawableTune.Materials;
	for (const auto& material : matGroup)
	{
		u32 effectHash = Hash(material->Effect);

		// Effect was already cached
		if (m_EffectCache.ContainsAt(effectHash))
			continue;

		rage::grcEffect* effect = rage::grcEffectMgr::FindEffectByHashKey(effectHash);
		if (!effect)
		{
			AM_ERRF("CollectMaterialInfo() -> Shader effect with name '%s.fxc' doesn't exists.", material->Effect.GetCStr());
			return false;
		}

		m_EffectCache.ConstructAt(effectHash, effect);
	}
	return true;
}

void rageam::asset::DrawableAsset::PrepareForConversion()
{
	u16 nodeCount = m_Scene->GetNodeCount();

	CacheEffects();
	m_NodeToModel.Resize(nodeCount);
	m_NodeToBone.Resize(nodeCount);
	m_EmbedDict = nullptr;

	CompiledDrawableMap = std::make_unique<DrawableAssetMap>();
	CompiledDrawableMap->SceneNodeToModel.Resize(nodeCount);
	CompiledDrawableMap->SceneNodeToBound.Resize(nodeCount);
	CompiledDrawableMap->SceneNodeToBone.Resize(nodeCount);
	CompiledDrawableMap->SceneMaterialToShader.Resize(m_Scene->GetMaterialCount());
	CompiledDrawableMap->SceneNodeToLightAttr.Resize(nodeCount);
	// Default everything to -1
	for (auto& handle : CompiledDrawableMap->SceneNodeToModel)	handle = { u16(-1), u16(-1) };
	for (u16& i : CompiledDrawableMap->SceneNodeToBound)		i = u16(-1);
	for (u16& i : CompiledDrawableMap->SceneNodeToBone)			i = u16(-1);
	for (u16& i : CompiledDrawableMap->SceneMaterialToShader)	i = u16(-1);
	for (u16& i : CompiledDrawableMap->SceneNodeToLightAttr)	i = u16(-1);
}

void rageam::asset::DrawableAsset::CleanUpConversion()
{
	m_EffectCache.Destruct();
	m_NodeToModel.Destroy();
	m_NodeToBone.Destroy();
	m_EmbedDict = nullptr;
}

rageam::List<rageam::asset::DrawableAsset::SplittedGeometry> rageam::asset::DrawableAsset::ConvertSceneGeometry(
	const graphics::SceneGeometry* sceneGeometry, bool skinned) const
{
	List<SplittedGeometry> geometries;

	// Retrieve material & vertex declaration for scene geometry
	const MaterialTune& material = GetGeometryMaterialTune(sceneGeometry);		// Material settings from tune.xml file (asset config)
	const EffectInfo& effectInfo = m_EffectCache.GetAt(Hash(material.Effect));	// Cached effect (.fxc shader file)

	const graphics::VertexDeclaration& decl = skinned ? effectInfo.VertexDeclSkin : effectInfo.VertexDecl; // Description of vertex buffer format

	u32 totalVertexCount = sceneGeometry->GetVertexCount();
	u32 totalIndexCount = sceneGeometry->GetIndexCount();

	// AM_DEBUGF("DrawableAsset::ConvertSceneGeometry -> %u vertices; %u indices", totalVertexCount, totalIndexCount);

	// Pack scene geometry attributes into single vertex buffer
	graphics::VertexBufferEditor sceneVertexBuffer(decl);
	sceneVertexBuffer.Init(totalVertexCount);
	sceneVertexBuffer.SetFromGeometry(sceneGeometry);
	if (!sceneVertexBuffer.IsSet(graphics::COLOR, 0))
	{
		sceneVertexBuffer.SetColorSingle(0, graphics::COLOR_WHITE);
		AM_WARNINGF(
			"DrawableAsset::ConvertSceneGeometry() -> Geometry '%u' of mesh '%s' has no vertex color! Using WHITE as default.",
			sceneGeometry->GetIndex(), sceneGeometry->GetParentMesh()->GetParentNode()->GetName());
	}

	// Remap blend indices to skeleton for skinning, they are ignored by VertexBufferEditor::SetFromGeometry
	if (skinned)
	{
		amUniquePtr<rage::Vector4[]> remappedBlendIndices = RemapBlendIndices(sceneGeometry);
		sceneVertexBuffer.SetBlendIndices(remappedBlendIndices.get(), DXGI_FORMAT_R32G32B32A32_FLOAT);
	}

	// Creates grmGeometry from given vertex data and adds it in geometries list
	auto addGeometry = [&decl, &geometries](pVoid vertices, pVoid indices, u32 vertexCount, u32 indexCount, const rage::spdAABB& bb)
		{
			// Create geometry & set vertex data
			rage::grmVertexData vertexData;
			vertexData.Vertices = vertices;
			vertexData.Indices = static_cast<u16*>(indices);
			vertexData.VertexCount = vertexCount;
			vertexData.IndexCount = indexCount;
			vertexData.Info = decl.GrcInfo;

			rage::pgUPtr grmGeometry = new rage::grmGeometryQB();
			grmGeometry->SetVertexData(vertexData);

			// Add resulting geometry in the list
			SplittedGeometry splittedGeometry;
			splittedGeometry.GrmGeometry = std::move(grmGeometry);
			splittedGeometry.BoundingBox = bb;
			geometries.Emplace(std::move(splittedGeometry));
		};

	graphics::SceneData indices;
	sceneGeometry->GetIndices(indices);

	// If indices are 16 bit we can just use them as is, otherwise we'll have to split geometry
	if (indices.Format == DXGI_FORMAT_R16_UINT)
	{
		rage::spdAABB bound = sceneGeometry->GetAABB();
		addGeometry(sceneVertexBuffer.GetBuffer(), indices.Buffer, totalVertexCount, totalIndexCount, bound);
	}
	else
	{
		AM_ASSERT(indices.Format == DXGI_FORMAT_R32_UINT, "Unsupported index buffer format %s", Enum::GetName(indices.Format));

		auto splitVertices = graphics::MeshSplitter::Split(
			sceneVertexBuffer.GetBuffer(), decl.Stride, (u32*)indices.Buffer, totalIndexCount);

		for (const graphics::MeshChunk& chunk : splitVertices)
		{
			// Use vertex editor to compute AABB
			graphics::VertexBufferEditor bufferEditor(decl);
			bufferEditor.Init(chunk.VertexCount, chunk.Vertices.get());

			rage::spdAABB chunkBound;
			bufferEditor.ComputeMinMax_Position(chunkBound.Min, chunkBound.Max);

			addGeometry(chunk.Vertices.get(), chunk.Indices.get(), chunk.VertexCount, chunk.IndexCount, chunkBound);
		}
	}

	return geometries;
}

u16 rageam::asset::DrawableAsset::GetSceneGeometryMaterialIndex(const graphics::SceneGeometry* sceneGeometry) const
{
	u16 nodeMaterial = sceneGeometry->GetMaterialIndex();
	if (m_Scene->NeedDefaultMaterial())
	{
		if (nodeMaterial == graphics::DEFAULT_MATERIAL)
			return 0;
		return nodeMaterial + 1; // Shift to reserve slot 0 for default material
	}
	return nodeMaterial;
}

const rageam::asset::MaterialTune& rageam::asset::DrawableAsset::GetGeometryMaterialTune(const graphics::SceneGeometry* sceneGeometry) const
{
	return *m_DrawableTune.Materials.Get(GetSceneGeometryMaterialIndex(sceneGeometry));
}

rage::pgUPtr<rage::grmModel> rageam::asset::DrawableAsset::ConvertSceneModel(const graphics::SceneNode* sceneNode)
{
	ReportProgress(String::FormatTemp(L"Converting model '%hs'", sceneNode->GetName()), 0.2);

	AM_ASSERT(sceneNode->HasMesh(), "DrawableAsset::ConvertSceneModel() -> Given node has no mesh!");

	graphics::SceneMesh* sceneMesh = sceneNode->GetMesh();
	bool hasSkin = sceneMesh->HasSkin();

	rage::grmModel* grmModel = new rage::grmModel();

	AM_DEBUGF("DrawableAsset() -> Converting '%s' into grmModel, '%u' initial geometries to split",
		sceneNode->GetName(), sceneMesh->GetGeometriesCount());

	// Convert every scene geometry to grmGeometry and add them to grmModel geometries array
	u32 geometryIndex = 0; // For setting material
	for (u32 i = 0; i < sceneMesh->GetGeometriesCount(); i++)
	{
		const graphics::SceneGeometry* sceneGeometry = sceneMesh->GetGeometry(i);

		// If scene geometry was pretty high poly then it'll be split on many chunks that fit in 16 bit index buffer
		for (SplittedGeometry& splittedGeometry : ConvertSceneGeometry(sceneGeometry, hasSkin))
		{
			u16 materialIndex = GetSceneGeometryMaterialIndex(sceneGeometry);

			grmModel->AddGeometry(splittedGeometry.GrmGeometry, splittedGeometry.BoundingBox);
			grmModel->SetMaterialIndex(geometryIndex++, materialIndex); // Link with ShaderGroup material
		}
	}

	grmModel->ComputeAABB();

	m_NodeToModel[sceneNode->GetIndex()] = grmModel;

	return grmModel;
}

amUniquePtr<rage::Vector4[]> rageam::asset::DrawableAsset::RemapBlendIndices(const graphics::SceneGeometry* sceneGeometry) const
{
	struct IndicesU32 { u8 X, Y, Z, W; };

	graphics::SceneData indicesData;
	AM_ASSERT(sceneGeometry->GetAttribute(indicesData, graphics::BLENDINDICES, 0),
		"DrawableAsset::RemapBlendIndices() -> Geometry had no blend indices data.");

	AM_ASSERT(indicesData.Format == DXGI_FORMAT_R8G8B8A8_UINT,
		"DrawableAsset::RemapBlendIndices() -> Unsupported blend indices format '%s'", Enum::GetName(indicesData.Format));

	// TODO: Cache it for node?
	// Build new bone map, how it works:
	// Blend indices point to bone indices relative to the scene, but after creating skeleton
	// bone order is totally different (we may add some extra bones or change the order)
	// So in order to solve this we map scene index (aka blend index) to skeleton bone index
	u8 sceneBoneToSkeleton[MAX_BONES];
	auto sceneNode = sceneGeometry->GetParentNode();
	for (u16 i = 0; i < sceneNode->GetBoneCount(); i++)
	{
		auto sceneNodeBone = sceneNode->GetBone(i);
		auto sceneNodeBoneIndex = sceneNodeBone->GetIndex();
		sceneBoneToSkeleton[i] = m_NodeToBone[sceneNodeBoneIndex]->GetIndex();
	}

	u32 vertexCount = sceneGeometry->GetVertexCount();

	rage::Vector4* remappedIndices = new rage::Vector4[vertexCount];
	IndicesU32* sourceIndices = indicesData.GetBufferAs<IndicesU32>();
	for (u32 i = 0; i < vertexCount; i++)
	{
		IndicesU32 indices = sourceIndices[i];

		// Remap indices from scene to skeleton
		indices.X = sceneBoneToSkeleton[indices.X];
		indices.Y = sceneBoneToSkeleton[indices.Y];
		indices.Z = sceneBoneToSkeleton[indices.Z];
		indices.W = sceneBoneToSkeleton[indices.W];

		// Convert to float[4]
		constexpr float maxBones = 255.0f;
		constexpr float epsilon = 0.0015f;
		remappedIndices[i] =
		{ // Add small margin to ensure that casting won't cause rounding down
				(float)indices.X / maxBones + epsilon,
				(float)indices.Y / maxBones + epsilon,
				(float)indices.Z / maxBones + epsilon,
				(float)indices.W / maxBones + epsilon,
		};
	}

	return amUniquePtr<rage::Vector4[]>(remappedIndices);
}

bool rageam::asset::DrawableAsset::GenerateSkeleton()
{
	u16 sceneNodeCount = m_Scene->GetNodeCount();
	bool needRootBone = NeedAdditionalRootBone();

	// Compute bone count, we can't resize skeleton so it must be precomputed
	u16 boneCount = 0;
	if (needRootBone)
		boneCount += 1; // 'Insert' root bone

	for (u16 i = 0; i < m_Scene->GetNodeCount(); i++)
	{
		if (CanBecameBone(m_Scene->GetNode(i)))
			boneCount++;
	}

	// Empty skeleton, skip
	if (boneCount == 0)
		return true;

	// TODO: Find out why shader allows up to 255 but after 128 skeleton explodes
	if (boneCount > 128)
	{
		AM_ERRF("DrawableAsset::GenerateSkeleton() -> Too much bones in skeleton (%u), maximum allowed 128.", boneCount);
		return false;
	}

	rage::crSkeletonData* skeletonData = new rage::crSkeletonData();
	skeletonData->Init(boneCount);

	// Setup root bone
	if (needRootBone)
	{
		char rootName[64];
		sprintf_s(rootName, 64, "%s_root", m_Scene->GetName());
		skeletonData->GetBone(0)->SetName(rootName);

		// We have to add it to ensure valid indexing
		CompiledDrawableMap->BoneToSceneNode.Add(u16(-1));
	}

	// Setup bone for every scene node
	u16 boneStartIndex = needRootBone ? 1 : 0;
	for (u16 nodeIndex = 0, boneIndex = boneStartIndex; nodeIndex < sceneNodeCount; nodeIndex++)
	{
		graphics::SceneNode* sceneNode = m_Scene->GetNode(nodeIndex);
		if (!CanBecameBone(sceneNode))
			continue;

		rage::crBoneData* bone = skeletonData->GetBone(boneIndex);
		m_NodeToBone[nodeIndex] = bone;

		// Self index
		bone->SetIndex(boneIndex);

		// TODO: Allow tag override from tune file
		bone->SetName(sceneNode->GetName());

		// Apply transformation
		rage::Vec3V trans = sceneNode->GetTranslation();
		rage::Vec3V scale = sceneNode->GetScale();
		rage::QuatV rot = sceneNode->GetRotation();
		bone->SetTransform(&trans, &scale, &rot);

		// Map graphics::SceneNode -> crBoneData
		CompiledDrawableMap->SceneNodeToBone[nodeIndex] = boneIndex;
		CompiledDrawableMap->BoneToSceneNode.Add(nodeIndex);

		boneIndex++;
	}

	// Setup parent & sibling indices
	// We can't do it in first loop because index of next bone is unknown
	// TODO: Solution - We can store previous bone and link the same way as in GL
	for (u16 nodeIndex = 0; nodeIndex < sceneNodeCount; nodeIndex++)
	{
		rage::crBoneData* bone = m_NodeToBone[nodeIndex];
		if (!bone)
			continue;

		graphics::SceneNode* sceneNode = m_Scene->GetNode(nodeIndex);

		s32 parentBoneIndex = GetNodeBoneIndex(sceneNode->GetParent());
		s32 siblingBoneIndex = -1;
		// For next sibling bone we'll have to search until we find a node that is bone,
		// such scenario may occur when:
		// Node 1: Is Bone
		// Node 2: Is Not Bone
		// Node 3: Is Bone
		// In skeleton Node 3 will appear after Node 1 but as 'NextSibling' in scene it won't
		graphics::SceneNode* nextSublingBoneNode = sceneNode->GetNextSibling();
		while (nextSublingBoneNode)
		{
			s32 nextSiblingBoneIndex = GetNodeBoneIndex(nextSublingBoneNode);
			if (nextSiblingBoneIndex != -1)
			{
				siblingBoneIndex = nextSiblingBoneIndex;
				break;
			}
			nextSublingBoneNode = nextSublingBoneNode->GetNextSibling();
		}

		// We have to link root bone with first child manually
		if (needRootBone && parentBoneIndex == -1)
			parentBoneIndex = 0; // Bones with parent index -1 are getting new root parent at index 0

		bone->SetParentIndex(parentBoneIndex);
		bone->SetNextIndex(siblingBoneIndex);
	}

	skeletonData->Finalize();
	m_Drawable->SetSkeletonData(skeletonData);
	return true;
}

void rageam::asset::DrawableAsset::LinkModelsToSkeleton()
{
	// No skeleton was generated
	if (!m_Drawable->GetSkeletonData())
		return;

	for (u16 i = 0; i < m_Scene->GetNodeCount(); i++)
	{
		rage::crBoneData* bone = m_NodeToBone[i];
		if (!bone) // Node was excluded from skeleton
			continue;

		// Link model with bone
		graphics::SceneNode* sceneNode = m_Scene->GetNode(i);
		rage::grmModel* grmModel = m_NodeToModel[sceneNode->GetIndex()];
		if (!grmModel) // Model will be null for scene nodes without mesh (aka dummies)
			continue;

		if (sceneNode->HasSkin()) // Weighted skinned model
		{
			grmModel->SetIsSkinned(true, sceneNode->GetBoneCount());
		}
		else // Skinned without bone weights, works like just transform
		{
			grmModel->SetBoneIndex(bone->GetIndex());
		}
	}
}

bool rageam::asset::DrawableAsset::NeedAdditionalRootBone() const
{
	graphics::SceneNode* node = m_Scene->GetFirstNode();

	u16 numRootBones = 0;
	while (node)
	{
		if (!IsCollisionNode(node)) // Collision nodes are ignored in drawable and skeleton
		{
			if (node->HasTransform())
				return true;

			numRootBones++;
			if (numRootBones > 1)
				return true;
		}

		node = node->GetNextSibling();
	}

	return false;
}

bool rageam::asset::DrawableAsset::CanBecameBone(const graphics::SceneNode* sceneNode) const
{
	bool forceBone = m_DrawableTune.Lods.Models.Get(sceneNode->GetIndex())->CreateBone;
	if (forceBone)
		return true;

	if (IsCollisionNode(sceneNode))
		return false;

	if (sceneNode->HasLight())
		return false;

	if (sceneNode->HasSkin())
		return true;

	if (sceneNode->HasTransform())
		return true;

	// If any child node is a bone, this node becomes a bone too
	if (sceneNode->HasTransformedChild())
		return true;

	return false;
}

s32 rageam::asset::DrawableAsset::GetNodeBoneIndex(const graphics::SceneNode* sceneNode) const
{
	if (!sceneNode)
		return -1;

	rage::crBoneData* bone = m_NodeToBone[sceneNode->GetIndex()];
	if (!bone)
		return -1;

	return bone->GetIndex();
}

void rageam::asset::DrawableAsset::SetupLodModels()
{
	// TODO: 2 type lod support - hierarchy and separate model

	rage::rmcLodGroup& lodGroup = m_Drawable->GetLodGroup();
	rage::rmcLod* lod = lodGroup.GetLod(rage::LOD_HIGH);
	rage::grmModels& lodModels = lod->GetModels();
	for (u32 i = 0; i < m_Scene->GetNodeCount(); i++)
	{
		graphics::SceneNode* sceneNode = m_Scene->GetNode(i);

		if (!sceneNode->HasMesh())
			continue;

		if (IsCollisionNode(sceneNode))
			continue;

		// Map rage::grmModel -> graphics::SceneNode
		u16 modelIndex = lodModels.GetSize();
		CompiledDrawableMap->SceneNodeToModel[sceneNode->GetIndex()] = { 0, modelIndex }; // TODO: Specify lod

		lodModels.Emplace(ConvertSceneModel(sceneNode));
	}
}

void rageam::asset::DrawableAsset::CalculateLodExtents() const
{
	m_Drawable->GetLodGroup().CalculateExtents();
}

void rageam::asset::DrawableAsset::CreateMaterials()
{
	rage::grmShaderGroup* shaderGroup = m_Drawable->GetShaderGroup();

	// Since we're using list to store materials, quick lookup table
	HashSet<graphics::SceneMaterial*> matNameToSceneMat;
	for (u16 i = 0; i < m_Scene->GetMaterialCount(); i++)
	{
		graphics::SceneMaterial* sceneMat = m_Scene->GetMaterial(i);
		matNameToSceneMat.InsertAt(sceneMat->GetNameHash(), sceneMat);
	}

	// For each material tune param:
	// - Try find parameter metadata (grcEffectVar) in grcEffect
	// - Simply copy value for regular types, for texture we have to resolve it from string
	for (const auto& matTune : m_DrawableTune.Materials)
	{
		const EffectInfo& effectInfo = m_EffectCache.GetAt(Hash(matTune->Effect));

		rage::grcEffect* effect = effectInfo.Effect;
		rage::grmShader* shader = new rage::grmShader(effect);
		shader->SetDrawBucket(matTune->DrawBucket);
		shader->SetTessellated(false);
		for (MaterialTune::Param& param : matTune->Params)
		{
			u16 varIndex;
			rage::grcEffectVar* varInfo = effect->GetVar(param.Name, &varIndex);

			// Invalid variable name or shader was changed, skipping...
			if (!varInfo)
			{
				AM_WARNINGF("Unable to find variable '%s' in shader effect '%s'.fxc",
					param.Name.GetCStr(), matTune->Effect.GetCStr());
				continue;
			}

			rage::grcInstanceVar* var = shader->GetVarByIndex(varIndex);

			// Locate texture
			if (varInfo->IsTexture())
			{
				bool textureResolved = true;

				string textureName = param.GetValue<string>();
				if (String::IsNullOrEmpty(textureName))
				{
					AM_WARNINGF("Texture is not specified for '%s' in material '%s'",
						param.Name.GetCStr(), matTune->Name.GetCStr());
					textureResolved = false;
				}
				else if (!ResolveAndSetTexture(var, textureName))
				{
					AM_WARNINGF("Texture '%s' is not found in any known dictionary in material '%s'.",
						textureName.GetCStr(), matTune->Name.GetCStr());
					textureResolved = false;
				}

				if (!textureResolved)
					SetMissingTexture(var);
			}
			else // Other simple types
			{
				// We have to cast each type manually because there seems to be no way
				// to get value pointer from std::any
				switch (param.Type)
				{
				case MaterialTune::eParamType::Unsupported:
				case MaterialTune::eParamType::None:
				case MaterialTune::eParamType::String:
				case MaterialTune::eParamType::Color:														break;

				case MaterialTune::eParamType::Bool:		var->SetValue(param.GetValue<bool>());			break;
				case MaterialTune::eParamType::Float:		var->SetValue(param.GetValue<float>());			break;
				case MaterialTune::eParamType::Vec2F:		var->SetValue(param.GetValue<rage::Vector2>());	break;
				case MaterialTune::eParamType::Vec3F:		var->SetValue(param.GetValue<rage::Vector3>());	break;
				case MaterialTune::eParamType::Vec4F:		var->SetValue(param.GetValue<rage::Vector4>());	break;
				}
			}
		}

		u16 shaderIndex = shaderGroup->GetShaderCount();
		// Map graphics::SceneMaterial -> rage::grmShader
		if (matTune->IsDefault || matTune->IsRemoved) // Default implicit & orphan materials have no scene material linkage
		{
			CompiledDrawableMap->ShaderToSceneMaterial.Add(u16(-1));
		}
		else
		{
			u16 materialIndex = m_Scene->GetMaterialByName(matTune->Name)->GetIndex();
			CompiledDrawableMap->SceneMaterialToShader[materialIndex] = shaderIndex;
			CompiledDrawableMap->ShaderToSceneMaterial.Add(materialIndex);
		}
		shaderGroup->AddShader(shader);
	}
}

bool rageam::asset::DrawableAsset::ResolveAndSetTexture(rage::grcInstanceVar* var, ConstString textureName)
{
	// Try to resolve first in embed dictionary (it has highest priority)
	if (m_EmbedDict)
	{
		rage::grcTexture* embedTexture = m_EmbedDict->Find(textureName);
		if (embedTexture)
		{
			var->SetTexture(embedTexture);
			return true;
		}
	}

	// Texture was not in embed dictionary, try to find it in workspace
	for (u16 i = 0; i < WorkspaceTXD->GetTexDictCount(); i++)
	{
		const TxdAssetPtr& sharedTxdAsset = WorkspaceTXD->GetTexDict(i);

		if (!sharedTxdAsset->ContainsTexture(textureName))
			continue;

		// Texture is used in this txd, first look if we compiled it before
		HotTxd* cachedHotTxd = SharedTXDs.TryGetAt(sharedTxdAsset->GetHashKey());
		if (!cachedHotTxd)
		{
			// TXD was not in cache, compile and add it there
			HotTxd hotTxd(sharedTxdAsset);
			if (!hotTxd.TryCompile()) // TODO: Ideally we should compile only single texture
			{
				AM_ERRF(L"DrawableAsset::ResolveAndSetTexture() -> Failed to compile dictionary with referenced texture '%ls'.",
					sharedTxdAsset->GetAssetName());
				break;
			}

			cachedHotTxd = &SharedTXDs.Emplace(std::move(hotTxd));
		}

		rage::grcTexture* sharedTexture = cachedHotTxd->Dict->Find(textureName);
		if (sharedTexture)
		{
			var->SetTexture(sharedTexture);
			return true;
		}
	}

	// Texture was not found or failed to compile, mark it as missing...
	var->SetTexture(CreateMissingTexture(textureName));

	return false;
}

void rageam::asset::DrawableAsset::SetMissingTexture(rage::grcInstanceVar* var) const
{
	rage::grcTexture* checker = GRenderContext->CheckerTexture.GetTexture();
	var->SetTexture(checker);
}

bool rageam::asset::DrawableAsset::CompileAndSetEmbedDict()
{
	// Don't create empty embed dictionary 
	if (m_EmbedDictTune->GetTextureCount() == 0)
		return true;

	rage::grcTextureDictionary* embedDict = new rage::grcTextureDictionary();
	if (!m_EmbedDictTune->CompileToGame(embedDict))
	{
		AM_ERRF("DrawableAsset::CompileToGame() -> Failed to compile embeed texture dictionary.");
		delete embedDict;
		return false;
	}

	m_EmbedDict = embedDict;
	m_Drawable->GetShaderGroup()->SetEmbedTextureDict(embedDict);

	return true;
}

rageam::SmallList<rage::phBoundPtr> rageam::asset::DrawableAsset::CreateBoundsFromNode(const graphics::SceneNode* sceneNode) const
{
	// TODO: Other bound types...

	SmallList<rage::phBoundPtr> bounds;
	if (!sceneNode->HasMesh())
		return bounds;

	const graphics::SceneMesh* sceneMesh = sceneNode->GetMesh();

	bounds.Reserve(sceneMesh->GetGeometriesCount());
	for (u16 i = 0; i < sceneMesh->GetGeometriesCount(); i++)
	{
		graphics::SceneGeometry* sceneGeom = sceneMesh->GetGeometry(i);

		graphics::SceneData indices;
		graphics::SceneData vertices;
		sceneGeom->GetIndices(indices);
		sceneGeom->GetAttribute(vertices, graphics::POSITION, 0);

		// We can split it of course but such high dense collision with 65K+ indices is not good
		if (indices.Format != DXGI_FORMAT_R16_UINT)
		{
			AM_WARNINGF("DrawableAsset::CreateBound() -> Geometry #%i in model '%s' has too much indices (%u) - maximum allowed 65 535, skipping...",
				i, sceneNode->GetName(), sceneGeom->GetIndexCount());
			continue;
		}

		rage::phBoundGeometry* geometryBound = new rage::phBoundGeometry();
		geometryBound->SetMesh(
			sceneGeom->GetAABB(), vertices.GetBufferAs<rage::Vector3>(), indices.GetBufferAs<u16>(), sceneGeom->GetVertexCount(), sceneGeom->GetIndexCount());

		bounds.Add(geometryBound);
	}
	return bounds;
}

void rageam::asset::DrawableAsset::CreateAndSetCompositeBound() const
{
	u16 nodeCount = m_Scene->GetNodeCount();

	// Collect all bounds from scene models in single list
	SmallList<rage::phBoundPtr> bounds;
	auto boundToSceneIndex = std::unique_ptr<u16[]>(new u16[nodeCount]);
	for (u16 i = 0; i < m_Scene->GetNodeCount(); i++)
	{
		graphics::SceneNode* sceneNode = m_Scene->GetNode(i);

		if (!sceneNode->HasMesh())
			continue;

		bool forceUseAsBound = m_DrawableTune.Lods.Models.Get(i)->UseAsBound;
		if (!forceUseAsBound && !IsCollisionNode(sceneNode))
			continue;

		auto nodeBounds = CreateBoundsFromNode(sceneNode);

		// Map all bounds created from scene node to the node
		u16 startIndex = bounds.GetSize();
		for (u16 k = 0; k < nodeBounds.GetSize(); k++)
		{
			u16 boundIndex = startIndex + k;
			boundToSceneIndex[boundIndex] = i;

			// Map graphics::SceneNode -> rage::phBound
			CompiledDrawableMap->SceneNodeToBound[i] = boundIndex;
			CompiledDrawableMap->BoneToSceneNode.Add(i);
		}

		bounds.AddRange(std::move(nodeBounds));
	}

	if (!bounds.Any())
		return;

	rage::phBoundComposite* composite = new rage::phBoundComposite();
	composite->Init(bounds.GetSize(), true);

	// Set bounds & matrices
	for (u16 i = 0; i < bounds.GetSize(); i++)
	{
		graphics::SceneNode* sceneNode = m_Scene->GetNode(boundToSceneIndex[i]);

		const rage::Mat44V& worldMtx = sceneNode->GetWorldTransform();

		// TODO:
		// Scale causes weird issues on geometry bound, may be worth to 'apply transform'
		// but it may cause issues with animation, i'm not sure
		rage::Vec3V scale;
		worldMtx.Decompose(nullptr, &scale, nullptr);
		if (!scale.AlmostEqual(rage::S_ONE))
		{
			AM_WARNINGF(
				"DrawableAsset::CreateAndSetCompositeBound() -> Element '%s' (index %u) has non-identity (1, 1, 1) scale (%f, %f, %f), collision may work unstable",
				sceneNode->GetName(), i, scale.X(), scale.Y(), scale.Z());
		}

		composite->SetBound(i, bounds[i]);
		composite->SetMatrix(i, worldMtx);

		// TODO: Test flags
		using namespace rage;
		composite->SetTypeFlags(i, CF_MAP_WEAPON | CF_MAP_DYNAMIC | CF_MAP_ANIMAL | CF_MAP_COVER | CF_MAP_VEHICLE);
		composite->SetIncludeFlags(i, CF_VEHICLE_NOT_BVH | CF_VEHICLE_BVH | CF_PED | CF_RAGDOLL | CF_ANIMAL | CF_ANIMAL_RAGDOLL | CF_OBJECT | CF_PLANT | CF_PROJECTILE | CF_EXPLOSION | CF_FORKLIFT_FORKS | CF_TEST_WEAPON | CF_TEST_CAMERA | CF_TEST_AI | CF_TEST_SCRIPT | CF_TEST_VEHICLE_WHEEL | CF_GLASS);
	}

	composite->CalculateExtents(); // TODO: Should we include matrices in composite bound extents?
	m_Drawable->SetBound(composite);
}

bool rageam::asset::DrawableAsset::IsCollisionNode(const graphics::SceneNode* sceneNode) const
{
	ImmutableString modelName = sceneNode->GetName();
	return modelName.EndsWith(COL_MODEL_EXT, true);
}

void rageam::asset::DrawableAsset::PoseModelBoundsFromScene() const
{
	if (!m_Scene->HasTransform())
		return;

	for (u16 i = 0; i < m_Scene->GetNodeCount(); i++)
	{
		graphics::SceneNode* sceneNode = m_Scene->GetNode(i);
		if (!sceneNode->HasTransform())
			continue;

		rage::grmModel* model = m_NodeToModel[i];
		if (!model) // Dummy with no mesh or was excluded (eg .COL nodes)
			continue;

		model->TransformAABB(sceneNode->GetWorldTransform());
	}
}

void rageam::asset::DrawableAsset::GeneratePaletteTexture()
{
	// TODO: ...

	//m_EmbedDict->Refresh();
}

void rageam::asset::DrawableAsset::CreateLights()
{
	for (const auto& lightTune : m_DrawableTune.Lights)
	{
		graphics::SceneNode* lightNode = m_Scene->GetNodeByName(lightTune->Name);

		u16 lightNodeIndex = lightNode->GetIndex();

		// Map graphics::SceneLight -> CLightAttr
		u16 lightIndex = m_Drawable->GetLightCount();
		CompiledDrawableMap->SceneNodeToLightAttr[lightNodeIndex] = lightIndex;
		CompiledDrawableMap->LightAttrToSceneNode.Add(lightNodeIndex);

		// Create light attribute
		CLightAttr& light = m_Drawable->AddLight();
		// light.SetMatrix(lightNode->GetLocalTransform());

		// Convert general properties
		light.Type = eLightType(lightTune->Type);
		light.Position = lightTune->Position;
		light.Direction = lightTune->Direction;
		light.Tangent = lightTune->Tangent;
		light.ColorR = lightTune->ColorRGB.R;
		light.ColorG = lightTune->ColorRGB.G;
		light.ColorB = lightTune->ColorRGB.B;
		light.Flashiness = lightTune->Flashiness;
		light.Intensity = lightTune->Intensity;
		light.Flags = lightTune->Flags;
		light.TimeFlags = lightTune->TimeFlags;
		light.Falloff = lightTune->Falloff;
		light.FallofExponent = lightTune->FalloffExponent;
		light.CullingPlaneNormal = lightTune->CullingPlaneNormal;
		light.CullingPlaneOffset = lightTune->CullingPlaneOffset;
		light.ShadowBlur = lightTune->ShadowBlur;
		light.ShadowNearClip = lightTune->ShadowNearClip;
		light.CoronaSize = lightTune->CoronaSize;
		light.CoronaIntensity = lightTune->CoronaIntensity;
		light.CoronaZBias = lightTune->CoronaZBias;
		light.LightHash = lightTune->LightHash;
		// light.ProjectedTexture = Hash(lightTune->ProjectedTexture);
		light.LightFadeDistance = lightTune->LightFadeDistance;
		light.ShadowFadeDistance = lightTune->ShadowFadeDistance;
		light.SpecularFadeDistance = lightTune->SpecularFadeDistance;
		light.VolumetricFadeDistance = lightTune->VolumetricFadeDistance;
		// Spot
		light.VolumeIntensity = lightTune->VolumeIntensity;
		light.VolumeSizeScale = lightTune->VolumeSizeScale;
		light.VolumeOuterIntensity = lightTune->VolumeOuterIntensity;
		light.VolumeOuterExponent = lightTune->VolumeOuterExponent;
		light.VolumeOuterColorR = lightTune->VolumeOuterColorRGB.R;
		light.VolumeOuterColorG = lightTune->VolumeOuterColorRGB.G;
		light.VolumeOuterColorB = lightTune->VolumeOuterColorRGB.B;
		light.ConeInnerAngle = lightTune->ConeInnerAngle;
		light.ConeOuterAngle = lightTune->ConeOuterAngle;
		// Capsule
		light.Extent.X = lightTune->CapsuleLength;

		// Locate first parent bone and link it with light
		light.BoneTag = 0;
		graphics::SceneNode* boneNode = lightNode;
		while (boneNode)
		{
			rage::crBoneData* bone = m_NodeToBone[boneNode->GetIndex()];
			if (bone)
			{
				light.BoneTag = bone->GetBoneTag();
				break;
			}
			boneNode = boneNode->GetParent();
		}
	}

	AM_DEBUGF("DrawableAsset::CreateLights() -> Created %u lights.", m_Drawable->GetLightCount());
}

bool rageam::asset::DrawableAsset::ValidateScene() const
{
	// Validate that there's no material in the scene that has reversed default identifier
	for (u16 i = 0; i < m_Scene->GetMaterialCount(); i++)
	{
		if(m_Scene->GetMaterial(i)->GetNameHash() == Hash(DEFAULT_MATERIAL_NAME))
		{
			AM_ERRF("DrawableAsset::TryCompileToGame() -> Material at index '%u' uses reserved name identifier for default material '%s'!", 
			        i, DEFAULT_MATERIAL_NAME);
			return false;
		}
	}
	return true;
}

bool rageam::asset::DrawableAsset::TryCompileToGame()
{
	ReportProgress(L"Validating scene", 0.0);
	if (!ValidateScene())
		return false;

	ReportProgress(L"Compiling embed dictionary", 0.0);
	if (!CompileAndSetEmbedDict())
	{
		AM_ERRF("DrawableAsset::TryCompileToGame() -> Conversion canceled, failed to convert embed dictionary.");
		return false;
	}

	// We must generate skeleton first because we'll have to remap skinning blend indices
	AM_DEBUGF("DrawableAsset() -> Creating skeleton");
	ReportProgress(L"Creating skeleton", 0.1);
	if (!GenerateSkeleton())
		return false;

	AM_DEBUGF("DrawableAsset() -> Setting up lod models");
	ReportProgress(L"Creating LODs", 0.2);
	SetupLodModels();

	// First lod must have at least one model! That's requirement of the game because it accesses it on creating entity
	if (!m_Drawable->GetLodGroup().GetLod(0)->GetModels().Any())
	{
		AM_ERRF("DrawableAsset::TryCompileToGame() -> There must be a least one model in LOD0!");
		return false;
	}

	AM_DEBUGF("DrawableAsset() -> Linking models to skeleton");
	ReportProgress(L"Linking models to skeleton", 0.3);
	LinkModelsToSkeleton();

	AM_DEBUGF("DrawableAsset() -> Creating materials");
	ReportProgress(L"Generating materials", 0.4);
	CreateMaterials();

	AM_DEBUGF("DrawableAsset() -> Creating collision bounds");
	ReportProgress(L"Creating collision bounds", 0.5);
	CreateAndSetCompositeBound();

	AM_DEBUGF("DrawableAsset() -> Creating lights");
	ReportProgress(L"Creating lights", 0.6);
	CreateLights();

	AM_DEBUGF("DrawableAsset() -> Posing bounds from scene");
	ReportProgress(L"Posing bound", 0.7);
	PoseModelBoundsFromScene();

	ReportProgress(L"Calculating lod extents", 0.8);
	CalculateLodExtents();

	m_Drawable->GetLodGroup().ComputeBucketMask(m_Drawable->GetShaderGroup());

	return true;
}

void rageam::asset::DrawableAsset::RefreshEmbedDict()
{
	m_EmbedDictPath = GetDirectoryPath() / EMBED_DICT_NAME;
	m_EmbedDictPath += ASSET_ITD_EXT;

	if (!IsFileExists(m_EmbedDictPath))
	{
		CreateDirectoryW(m_EmbedDictPath, NULL);
	}

	// Order is important here because otherwise old embed dict would overwrite new one
	// because otherwise old one will be destructed after new one is created
	m_EmbedDictTune = nullptr;
	m_EmbedDictTune = AssetFactory::LoadFromPath<TxdAsset>(m_EmbedDictPath);

	GeneratePaletteTexture();
}

//void rageam::asset::DrawableAsset::CreateMaterialTuneGroupFromScene()
//{
//	bool needDefaultMaterial = m_Scene->NeedDefaultMaterial();
//	if (needDefaultMaterial)
//	{
//		MaterialTune defaultMaterial("default", u16(-1));
//		defaultMaterial.InitFromEffect();
//		m_DrawableTune.Materials.Items.Emplace(std::move(defaultMaterial));
//	}
//
//	for (u16 i = 0; i < m_Scene->GetMaterialCount(); i++)
//	{
//		graphics::SceneMaterial* sceneMaterial = m_Scene->GetMaterial(i);
//
//		MaterialTune material(sceneMaterial->GetName(), i);
//		material.InitFromEffect();
//		material.SetTextureNamesFromSceneMaterial(sceneMaterial);
//		m_DrawableTune.Materials.Items.Emplace(std::move(material));
//	}
//}

void rageam::asset::DrawableAsset::RefreshTXDWorkspace()
{
	// We need workspace only for TXDs
	WorkspaceTXD = Workspace::FromPath(GetWorkspacePath(), WF_LoadTx);
}

bool rageam::asset::DrawableAsset::TryToFindFirstSceneFile(file::WPath& outPath) const
{
	file::Iterator iterator(GetDirectoryPath() / L"*.*");
	file::FindData data;

	// Take first supported model file
	while (iterator.Next())
	{
		iterator.GetCurrent(data);
		file::WPath extension = data.Path.GetExtension();

		if (graphics::SceneFactory::IsSupportedFormat(extension))
		{
			outPath = data.Path;
			return true;
		}
	}
	return true;
}

bool rageam::asset::DrawableAsset::RefreshSceneFile()
{
	file::WPath scenePath = GetScenePath();

	bool needModelRescan = false;
	if (String::IsNullOrEmpty(m_DrawableTune.SceneFileName))
	{
		needModelRescan = true;
		AM_DEBUGF("DrawableAsset::RefreshModel() -> Scanning model for the first time...");
	}
	else if (!IsFileExists(scenePath))
	{
		needModelRescan = true;
		AM_DEBUGF("DrawableAsset::RefreshModel() -> Model file was not found! Trying to find any...");
	}

	// Model file is most likely fine (well it exists at least...)
	if (!needModelRescan)
		return true;

	if (!TryToFindFirstSceneFile(scenePath))
	{
		AM_ERRF("DrawableAsset::RefreshModel() -> Failed to locate any 3D scene file.");
		return false;
	}

	m_DrawableTune.SceneFileName = scenePath.GetFileName();

	return true;
}

void rageam::asset::DrawableAsset::RefreshTuneFromScene()
{
	// It's a little bit complicated how we should handle certain changes in model scene,
	// for example when node or material was removed but tune still exists
	// Regular tune can be removed because it's not hard to setup,
	// but for material tune we have to mark it as 'orphan' and let user to decide what to do
	m_DrawableTune.Materials.Refresh(m_Scene.get());
	m_DrawableTune.Lights.Refresh(m_Scene.get());
	m_DrawableTune.Lods.Models.Refresh(m_Scene.get());
}

rageam::asset::DrawableAsset::DrawableAsset(const file::WPath& path) : GameRscAsset(path)
{
	RefreshEmbedDict();
}

bool rageam::asset::DrawableAsset::CompileToGame(gtaDrawable* ppOutGameFormat)
{
	Refresh();

	file::WPath scenePath = GetScenePath();

	// Scene is not loaded or outdated, attempt to load it
	u64 sceneModifyTime = GetFileModifyTime(scenePath);
	if (!m_Scene || sceneModifyTime != m_SceneFileTime)
	{
		m_SceneFileTime = sceneModifyTime;
		m_Scene = graphics::SceneFactory::LoadFrom(scenePath);
		if (!m_Scene)
		{
			AM_ERRF("DrawableAsset::CompileToGame() -> Failed to load scene file...");
			return false;
		}
	}

	m_Drawable = ppOutGameFormat;
	ReportProgress(L"Preparing assets", 0.0);
	PrepareForConversion();
	bool result = TryCompileToGame();
	ReportProgress(L"Cleaning up", 0.99);
	CleanUpConversion();
	m_Drawable = nullptr;
	return result;
}

void rageam::asset::DrawableAsset::ParseFromGame(gtaDrawable* object)
{
	// Drawable was compiled before, just map drawable and restore values
	if (CompiledDrawableMap)
	{
		// Materials
		{
			auto shaderGroup = object->GetShaderGroup();
			auto& matTunes = m_DrawableTune.Materials;
			for (u16 i = 0; i < matTunes.GetCount(); i++)
			{
				// rage::grmShader index directly map to MaterialTune
				matTunes.Get(i)->InitFromShader(shaderGroup->GetShader(i));
			}
		}

		// Lod Group
		{

		}

		// Lights
		{
			auto& lightTunes = m_DrawableTune.Lights;
			for (u16 i = 0; i < lightTunes.GetCount(); i++)
			{
				// CLightAttr index directly map to LightTune
				lightTunes.Get(i)->FromLightAttr(object->GetLight(i));
			}
		}
	}
	else
	{
		// TODO:
		// Main issue we have to figure out here is how do we handle converting mesh to obj/fbx,
		// basically every asset kind will require such options (for example output texture format for YTD, with DDS as default)
		AM_UNREACHABLE("DrawableAsset::ParseFromGame() -> Decompiling without source is not yet supported!");
	}
}

void rageam::asset::DrawableAsset::Refresh()
{
	if (!RefreshSceneFile())
		return;

	graphics::SceneLoadOptions sceneLoadOptions = {};
	sceneLoadOptions.SkipMeshData = true; // We need only metadata to refresh

	m_Scene = graphics::SceneFactory::LoadFrom(GetScenePath(), &sceneLoadOptions);
	if (!m_Scene)
	{
		AM_ERRF("DrawableAsset::Refresh() -> Failed to load scene medata.");
		return;
	}

	RefreshTXDWorkspace();
	RefreshEmbedDict();
	RefreshTuneFromScene();

	// Since it only contains metadata, we don't need it anymore
	m_Scene = nullptr;
}

void rageam::asset::DrawableAsset::Serialize(XmlHandle& node) const
{
	m_DrawableTune.Serialize(node);
}

void rageam::asset::DrawableAsset::Deserialize(const XmlHandle& node)
{
	m_DrawableTune.Deserialize(node);
}
