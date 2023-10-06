#include "drawable.h"

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
	case None:									break;
	case String:	GET_VALUE(ConstString);		break;
	case Bool:		GET_VALUE(bool);			break;
	case Float:		GET_VALUE(float);			break;
	case Vec2F:		GET_VALUE(rage::Vector2);	break;
	case Vec3F:		GET_VALUE(rage::Vector3);	break;
	case Vec4F:		GET_VALUE(rage::Vector4);	break;
	case Color:		Value = xml.GetColorHex();	break;
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
}

void rageam::asset::MaterialTune::Deserialize(const XmlHandle& xml)
{
	XML_GET_ATTR(xml, Name);
	XML_GET_ATTR(xml, Shader);
	for (const XmlHandle& xParam : XmlIterator(xml, "Param"))
	{
		Param param;
		param.Deserialize(xParam);
		Params.Emplace(std::move(param));
	}
}

void rageam::asset::MaterialTune::Serialize(XmlHandle& xml) const
{
	XML_SET_ATTR(xml, Name);
	XML_SET_ATTR(xml, Shader);
	for (const Param& param : Params)
	{
		XmlHandle xParam = xml.AddChild("Param");
		param.Serialize(xParam);
	}
}

rage::grcEffect* rageam::asset::MaterialTune::LookupEffect() const
{
	rage::grcEffect* effect = rage::grcEffectMgr::FindEffectByHashKey(ShaderHash);
	if (!effect)
	{
		AM_ERRF("MaterialTune::LookupEffect() -> Shader effect with name '%s.fxc' doesn't exists.", Shader.GetCStr());
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
			Shader.GetCStr(), DEFAULT_SHADER);

		effect = rage::grcEffectMgr::FindEffect(DEFAULT_SHADER);
	}
	return effect;
}

void rageam::asset::MaterialTune::InitFromShader()
{
	rage::grcEffect* effect = LookupEffectOrUseDefault();

	u16 effectVarCount = effect->GetVarCount();

	Params.Destroy();
	Params.Reserve(effectVarCount);

	for (u16 i = 0; i < effectVarCount; i++)
	{
		rage::grcEffectVar* varInfo = effect->GetVarByIndex(i);
		rage::grcInstanceVar* templateVar = effect->GetInstanceDataTemplate().GetVarByIndex(i);

		Param param;
		param.Name = varInfo->GetName();
		param.Type = grcEffectVarTypeToParamType[varInfo->GetType()];
		param.CopyValue(templateVar);

		Params.Emplace(std::move(param));
	}
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

void rageam::asset::MaterialTuneGroup::Serialize(XmlHandle& node) const
{
	for (MaterialTune& material : Materials)
	{
		XmlHandle xMaterial = node.AddChild("Item");
		material.Serialize(xMaterial);
	}
}

void rageam::asset::MaterialTuneGroup::Deserialize(const XmlHandle& node)
{
	for (const XmlHandle& xMaterial : XmlIterator(node, "Item"))
	{
		MaterialTune material;
		material.Deserialize(xMaterial);
		Materials.Emplace(std::move(material));
	}
}

void rageam::asset::LodGroupTune::Serialize(XmlHandle& node) const
{
	node.AddComment("Maximum LOD render distance's (L0 to L4)");

	// Serialize threshold as vector4
	XmlHandle xLodThreshold = node.AddChild("LodThreshold");
	rage::Vector4 lodThreshold;
	lodThreshold.FromArray(LodThreshold);
	xLodThreshold.SetValue(lodThreshold);
}

void rageam::asset::LodGroupTune::Deserialize(const XmlHandle& node)
{
	// Serialize threshold from vector4
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
}

void rageam::asset::DrawableTune::Serialize(XmlHandle& node) const
{
	XmlHandle xMaterialTuneGroup = node.AddChild("Materials");
	MaterialTuneGroup.Serialize(xMaterialTuneGroup);

	XmlHandle xLodGroup = node.AddChild("LodGroup");
	LodGroup.Serialize(xLodGroup);

	XmlHandle xLights = node.AddChild("Lights");
}

void rageam::asset::DrawableTune::Deserialize(const XmlHandle& node)
{
	XmlHandle xMaterialTuneGroup = node.GetChild("Materials");
	MaterialTuneGroup.Deserialize(xMaterialTuneGroup);

	XmlHandle xLodGroup = node.GetChild("LodGroup");
	LodGroup.Deserialize(xLodGroup);

	XmlHandle xLights = node.GetChild("Lights");
}

bool rageam::asset::DrawableAsset::CacheEffects()
{
	m_EffectCache.Destruct();

	MaterialTuneGroup& matGroup = m_DrawableTune.MaterialTuneGroup;
	for (MaterialTune& material : matGroup.Materials)
	{
		// Effect was already cached
		if (m_EffectCache.ContainsAt(material.ShaderHash))
			continue;

		rage::grcEffect* effect = rage::grcEffectMgr::FindEffectByHashKey(material.ShaderHash);
		if (!effect)
		{
			AM_ERRF("CollectMaterialInfo() -> Shader effect with name '%s.fxc' doesn't exists.", material.Shader.GetCStr());
			return false;
		}

		m_EffectCache.ConstructAt(material.ShaderHash, effect);
	}
	return true;
}

void rageam::asset::DrawableAsset::PrepareForConversion()
{
	CacheEffects();
	m_NodeToModel.Resize(m_Scene->GetNodeCount());
	m_NodeToBone.Resize(m_Scene->GetNodeCount());
	m_EmbedDict = nullptr;
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
	const EffectInfo& effectInfo = m_EffectCache.GetAt(material.ShaderHash);	// Cached effect (.fxc shader file)

	const graphics::VertexDeclaration& decl = skinned ? effectInfo.VertexDeclSkin : effectInfo.VertexDecl; // Description of vertex buffer format

	u32 totalVertexCount = sceneGeometry->GetVertexCount();
	u32 totalIndexCount = sceneGeometry->GetIndexCount();

	// AM_DEBUGF("DrawableAsset::ConvertSceneGeometry -> %u vertices; %u indices", totalVertexCount, totalIndexCount);

	// Pack scene geometry attributes into single vertex buffer
	graphics::VertexBufferEditor sceneVertexBuffer(decl);
	sceneVertexBuffer.Init(totalVertexCount);
	sceneVertexBuffer.SetFromGeometry(sceneGeometry);

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

const rageam::asset::MaterialTune& rageam::asset::DrawableAsset::GetGeometryMaterialTune(
	const graphics::SceneGeometry* sceneGeometry) const
{
	return m_DrawableTune.MaterialTuneGroup.Materials[GetSceneGeometryMaterialIndex(sceneGeometry)];
}

rage::pgUPtr<rage::grmModel> rageam::asset::DrawableAsset::ConvertSceneModel(const graphics::SceneNode* sceneNode)
{
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
		if (!sceneNode->HasSkin()) // Skinned bone transform is computed later
		{
			rage::Vec3V trans = sceneNode->GetTranslation();
			rage::Vec3V scale = sceneNode->GetScale();
			rage::QuatV rot = sceneNode->GetRotation();
			bone->SetTransform(&trans, &scale, &rot);
		}

		boneIndex++;
	}

	// Compute weighted skinned bone transforms
	for (u16 i = 0; i < sceneNodeCount; i++)
	{
		graphics::SceneNode* sceneNode = m_Scene->GetNode(i);
		if (!sceneNode->HasSkin())
			continue;

		for (u16 k = 0; k < sceneNode->GetBoneCount(); k++)
		{
			graphics::SceneNode* sceneBone = sceneNode->GetBone(k);
			rage::crBoneData* bone = m_NodeToBone[sceneBone->GetIndex()];

			// TODO: Gltf + blender exports skeleton Y axis UP so until we implement FBX
			// no weight skinned transforms are imported, they are not important in any case
			// Only used for skeleton rendering + exporting ydr model back to 3d scene
			rage::Mat44V mtx = rage::Mat44V::Identity();
			bone->SetTransform(mtx);
		}
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
		for (u16 nextNodeIndex = nodeIndex + 1; nextNodeIndex < sceneNodeCount; nextNodeIndex++)
		{
			if (m_NodeToBone[nextNodeIndex] == nullptr)
				continue;

			// We found next node that is also a bone
			siblingBoneIndex = GetNodeBoneIndex(m_Scene->GetNode(nextNodeIndex));
			break;
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

	if (m_DrawableTune.ForceGenerateSkeleton)
		return true;

	// TODO: We need option to check out nodes from participating in skeleton
	// <Skeleton>
	//	<ExcludeFromSkeleton>
	//		<Node>NodeName</Node>
	//	</ExcludeFromSkeleton>
	// </Skeleton>

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
	auto& lodModels = lod->GetModels();
	for (u32 i = 0; i < m_Scene->GetNodeCount(); i++)
	{
		graphics::SceneNode* sceneNode = m_Scene->GetNode(i);

		if (!sceneNode->HasMesh())
			continue;

		if (IsCollisionNode(sceneNode))
			continue;

		lodModels.Emplace(ConvertSceneModel(sceneNode));
	}
}

void rageam::asset::DrawableAsset::CalculateLodExtents() const
{
	m_Drawable->GetLodGroup().CalculateExtents();
}

void rageam::asset::DrawableAsset::CreateMaterials() const
{
	rage::grmShaderGroup* shaderGroup = m_Drawable->GetShaderGroup();

	// For each material tune param:
	// - Try find parameter metadata (grcEffectVar) in grcEffect
	// - Simply copy value for regular types, for texture we have to resolve it from string
	for (MaterialTune& material : m_DrawableTune.MaterialTuneGroup.Materials)
	{
		const EffectInfo& effectInfo = m_EffectCache.GetAt(material.ShaderHash);

		rage::grcEffect* effect = effectInfo.Effect;
		rage::grmShader* shader = new rage::grmShader(effect);
		for (MaterialTune::Param& param : material.Params)
		{
			u16 varIndex;
			rage::grcEffectVar* varInfo = effect->GetVar(param.Name, &varIndex);

			// Invalid variable name or shader was changed, skipping...
			if (!varInfo)
			{
				AM_WARNINGF("Unable to find variable '%s' in shader effect '%s'.fxc",
					param.Name.GetCStr(), material.Shader.GetCStr());
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
						param.Name.GetCStr(), material.Name.GetCStr());
					textureResolved = false;
				}
				else if (!ResolveAndSetTexture(var, textureName))
				{
					AM_WARNINGF("Texture '%s' is not found in any known dictionary in material '%s'.",
						textureName.GetCStr(), material.Name.GetCStr());
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

		shaderGroup->AddShader(shader);
	}
}

bool rageam::asset::DrawableAsset::ResolveAndSetTexture(rage::grcInstanceVar* var, ConstString textureName) const
{
	// Try to resolve first in embed dictionary (it has highest priority)
	if (m_EmbedDict)
	{
		rage::pgUPtr<rage::grcTextureDX11>* texture;
		if (m_EmbedDict->Find(rage::joaat(textureName), &texture))
		{
			var->SetTexture(texture->Get());
			return true;
		}
	}

	// TODO:
	// Advanced system for resolving textures, we support only embed dictionary right now
	// For now we search only in embed dictionary
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

		if (!IsCollisionNode(sceneNode))
			continue;

		auto nodeBounds = CreateBoundsFromNode(sceneNode);

		// Map all bounds created from scene node to the node
		u16 startIndex = bounds.GetSize();
		for (u16 k = 0; k < nodeBounds.GetSize(); k++)
			boundToSceneIndex[startIndex + k] = i;

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

void rageam::asset::DrawableAsset::CreateLights() const
{
	for (u16 i = 0; i < m_Scene->GetNodeCount(); i++)
	{
		graphics::SceneNode* sceneNode = m_Scene->GetNode(i);
		if (!sceneNode->HasLight())
			continue;

		graphics::SceneLight* sceneLight = sceneNode->GetLight();
		graphics::ColorU32 sceneLightColor = sceneLight->GetColor();

		CLightAttr& light = m_Drawable->m_Lights.Construct();;
		light.FixupVft();

		light.SetMatrix(sceneNode->GetLocalTransform());

		switch (sceneLight->GetType())
		{
		case graphics::SceneLight_Point:	light.Type = LIGHT_POINT;	break;
		case graphics::SceneLight_Spot:		light.Type = LIGHT_SPOT;	break;
		}

		light.ColorR = sceneLightColor.R;
		light.ColorG = sceneLightColor.G;
		light.ColorB = sceneLightColor.B;

		light.Falloff = 2.5;
		light.FallofExponent = 32;
		if (light.Type == LIGHT_SPOT)
			light.Falloff = 8.0f; // Make light longer if spot light is chosen

		light.ConeOuterAngle = rage::Math::RadToDeg(sceneLight->GetOuterConeAngle());
		light.ConeInnerAngle = rage::Math::RadToDeg(sceneLight->GetInnerConeAngle());

		light.Flags = LF_ENABLE_SHADOWS;

		light.CoronaZBias = 0.1;
		light.CoronaSize = 5.0f;

		light.Intensity = 75;
		light.TimeFlags = 16777215;
		light.Extent = { 1, 1 ,1 };
		light.ProjectedTexture = 0;
		light.ShadowNearClip = 0.01;
		light.CullingPlaneNormal = { 0, 0, -1 };
		light.CullingPlaneOffset = 1;

		light.VolumeOuterColorR = 255;
		light.VolumeOuterColorG = 255;
		light.VolumeOuterColorB = 255;
		light.VolumeOuterExponent = 1;
		light.VolumeSizeScale = 1.0f;

		// Locate first parent bone and link it with light
		light.BoneTag = 0;
		graphics::SceneNode* boneNode = sceneNode;
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
	AM_DEBUGF("DrawableAsset::CreateLights() -> Created %u lights.", m_Drawable->m_Lights.GetSize());
}

bool rageam::asset::DrawableAsset::TryCompileToGame()
{
	if (!CompileAndSetEmbedDict())
	{
		AM_ERRF("DrawableAsset::TryCompileToGame() -> Conversion canceled, failed to convert embed dictionary.");
		return false;
	}

	// We must generate skeleton first because we'll have to remap skinning blend indices
	AM_DEBUGF("DrawableAsset() -> Creating skeleton");
	if (!GenerateSkeleton())
		return false;

	AM_DEBUGF("DrawableAsset() -> Setting up lod models");
	SetupLodModels();
	AM_DEBUGF("DrawableAsset() -> Linking models to skeleton");
	LinkModelsToSkeleton();
	AM_DEBUGF("DrawableAsset() -> Creating materials");
	CreateMaterials();
	AM_DEBUGF("DrawableAsset() -> Creating collision bounds");
	CreateAndSetCompositeBound();
	AM_DEBUGF("DrawableAsset() -> Posing bounds from scene");
	PoseModelBoundsFromScene();
	CalculateLodExtents();
	CreateLights();

	return true;
}

void rageam::asset::DrawableAsset::InitializeEmbedDict()
{
	m_EmbedDictPath = GetDirectoryPath() / EMBED_DICT_NAME;
	m_EmbedDictPath += ASSET_ITD_EXT;

	if (!IsFileExists(m_EmbedDictPath))
	{
		CreateDirectoryW(m_EmbedDictPath, NULL);
	}

	// Order is important here because otherwise old embed dict would overwrite new one
	m_EmbedDictTune = nullptr;
	m_EmbedDictTune = AssetFactory::LoadFromPath<TxdAsset>(m_EmbedDictPath);

	GeneratePaletteTexture();
}

void rageam::asset::DrawableAsset::CreateMaterialTuneGroupFromScene()
{
	if (m_Scene->NeedDefaultMaterial())
	{
		MaterialTune defaultMaterial("default");
		defaultMaterial.InitFromShader();
		m_DrawableTune.MaterialTuneGroup.Materials.Emplace(std::move(defaultMaterial));
	}

	for (u16 i = 0; i < m_Scene->GetMaterialCount(); i++)
	{
		graphics::SceneMaterial* sceneMaterial = m_Scene->GetMaterial(i);

		MaterialTune material(sceneMaterial->GetName());
		material.InitFromShader();
		material.SetTextureNamesFromSceneMaterial(sceneMaterial);
		m_DrawableTune.MaterialTuneGroup.Materials.Emplace(std::move(material));
	}
}

rageam::asset::DrawableAsset::DrawableAsset(const file::WPath& path) : GameRscAsset(path), m_DrawableTune(this, path)
{
	InitializeEmbedDict();
}

bool rageam::asset::DrawableAsset::CompileToGame(gtaDrawable* ppOutGameFormat)
{
	// TODO: Move scene loading here, loading scene in refresh is ridiculous
	if (!m_Scene)
		return false;

	m_Drawable = ppOutGameFormat;
	PrepareForConversion();
	bool result = TryCompileToGame();
	CleanUpConversion();
	m_Drawable = nullptr;
	return result;
}

void rageam::asset::DrawableAsset::Refresh()
{
	// Nothing to refresh after config was setup first time
	if (HasSavedConfig())
	{
		// TODO: We still have to load scene from config file...
	}

	m_ScenePath = L"";

	file::Iterator iterator(GetDirectoryPath() / L"*.*");
	file::FindData data;

	file::WPath modelPath;

	// Take first supported model file
	while (iterator.Next())
	{
		iterator.GetCurrent(data);
		file::WPath extension = data.Path.GetExtension();

		if (graphics::SceneFactory::IsSupportedFormat(extension))
		{
			modelPath = data.Path;
			break;
		}
	}

	if (String::IsNullOrEmpty(modelPath))
	{
		AM_ERRF(L"DrawableAsset::Refresh() -> No model file found in %ls", GetDirectoryPath().GetCStr());
		return;
	}

	m_Scene = graphics::SceneFactory::LoadFrom(modelPath);
	m_ScenePath = modelPath;

	if (!m_Scene) // Failed to load model file
		return;

	CreateMaterialTuneGroupFromScene();
	InitializeEmbedDict();
}

void rageam::asset::DrawableAsset::Serialize(XmlHandle& node) const
{
	file::WPath fileName = file::GetFileName(m_ScenePath.GetCStr());

	node.AddChild("FileName", String::ToUtf8Temp(fileName));

	m_DrawableTune.Serialize(node);
}

void rageam::asset::DrawableAsset::Deserialize(const XmlHandle& node)
{
	ConstString fileName = node.GetChild("FileName", true).GetText();

	m_ScenePath = String::ToWideTemp(fileName);
}
