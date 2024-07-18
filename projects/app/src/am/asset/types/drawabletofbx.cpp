#include "drawabletofbx.h"

#include "am/graphics/buffereditor.h"
#include "am/graphics/meshgen.h"
#include "am/graphics/vertexdeclaration.h"
#include "am/system/nullable.h"
#include "rage/grcore/texturepc.h"
#include "rage/paging/builder/builder.h"
#include "game/drawable.h"
#include "game/physics/material.h"
#include "rage/math/mathv.h"
#include "rage/physics/bounds/boundbvh.h"
#include "rage/physics/bounds/boundcomposite.h"
#include "rage/physics/bounds/boundprimitives.h"

#include <easy/arbitrary_value.h>

FbxSurfacePhong* rageam::DrawableToFBX::CreateFbxMaterial(FbxObject* container, ConstString name)
{
	Map.AddNewMaterial();
	m_Counters.MAP_Material++;
	return FbxSurfacePhong::Create(m_FbxScene, name);
}

FbxNode* rageam::DrawableToFBX::CreateFbxNode(ConstString name)
{
	Map.AddNewNode();
	m_Counters.MAP_Node++;
	return FbxNode::Create(m_FbxScene, name);
}

FbxMesh* rageam::DrawableToFBX::GetFbxMeshFromFbxNode(FbxNode* node) const
{
	FbxNodeAttribute* nodeAttribute = node->GetNodeAttribute();
	AM_ASSERTS(nodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh);
	return (FbxMesh*)nodeAttribute;
}

FbxNode* rageam::DrawableToFBX::CreateFbxNodeFromMeshData(ConstString name, const List<Vec3V>& vertices, const List<int>& indices)
{
	FbxMesh* fbxMesh = FbxMesh::Create(m_FbxScene, "");
	int vertexCount = vertices.GetSize();
	int polyCount = indices.GetSize() / 3;
	fbxMesh->SetControlPointCount(vertexCount);
	fbxMesh->ReservePolygonCount(polyCount);
	for (int i = 0; i < vertexCount; i++)
	{
		Vec3S pos = vertices[i];
		fbxMesh->GetControlPoints()[i] = FbxVector4(pos.X, pos.Y, pos.Z);
	}
	for (int i = 0; i < polyCount; i++)
	{
		fbxMesh->BeginPolygon();
		fbxMesh->AddPolygon(indices[i * 3 + 0]);
		fbxMesh->AddPolygon(indices[i * 3 + 1]);
		fbxMesh->AddPolygon(indices[i * 3 + 2]);
		fbxMesh->EndPolygon();
	}

	// This is mostly needed to generate non-smoothed normals, because 3Ds Max generates non-smoothed by default, but blender smooths them!
	// Doesn't look good on collision
	fbxMesh->GenerateNormals();

	FbxNode* fbxNode = CreateFbxNode(name);
	fbxNode->SetNodeAttribute(fbxMesh);
	return fbxNode;
}

void rageam::DrawableToFBX::SetFbxNodeTransformFromBone(FbxNode* fbxNode, const rage::crBoneData* boneData) const
{
	// crBoneData stores transform relative to parent, set local transform
	Vec3S translation = boneData->GetTranslation();
	Vec3S scale = boneData->GetScale();
	Vec3S rotation = boneData->GetRotation().ToEuler();
	fbxNode->LclTranslation = FbxDouble3(translation.X, translation.Y, translation.Z);
	fbxNode->LclScaling = FbxDouble3(scale.X, scale.Y, scale.Z);
	fbxNode->LclRotation = FbxDouble3(rotation.X, rotation.Y, rotation.Z);
}

void rageam::DrawableToFBX::SetFbxMeshMaterialsPerPolygonVertex(FbxNode* fbxNode, const List<int>& materials) const
{
	FbxMesh* fbxMesh = GetFbxMeshFromFbxNode(fbxNode);
	AM_ASSERTS(fbxMesh->GetPolygonVertexCount() == materials.GetSize());
	FbxGeometryElementMaterial* fbxElementMaterial = fbxMesh->CreateElementMaterial();
	fbxElementMaterial->SetReferenceMode(FbxLayerElement::eIndexToDirect);
	fbxElementMaterial->SetMappingMode(FbxLayerElement::eByPolygonVertex);
	auto& fbxMaterialMap = fbxElementMaterial->GetIndexArray();
	fbxMaterialMap.Resize(materials.GetSize(), eUninitialized);
	for (int i = 0; i < materials.GetSize(); i++)
		fbxMaterialMap.SetAt(i, materials.Get(i));
}

void rageam::DrawableToFBX::SetFbxMeshMaterialsPerPolygon(FbxNode* fbxNode, const List<int>& materials) const
{
	FbxMesh* fbxMesh = GetFbxMeshFromFbxNode(fbxNode);
	AM_ASSERTS(fbxMesh->GetPolygonCount() == materials.GetSize());
	FbxGeometryElementMaterial* fbxElementMaterial = fbxMesh->CreateElementMaterial();
	fbxElementMaterial->SetReferenceMode(FbxLayerElement::eIndexToDirect);
	fbxElementMaterial->SetMappingMode(FbxLayerElement::eByPolygon);
	auto& fbxMaterialMap = fbxElementMaterial->GetIndexArray();
	fbxMaterialMap.Resize(materials.GetSize(), eUninitialized);
	for (int i = 0; i < materials.GetSize(); i++)
		fbxMaterialMap.SetAt(i, materials.Get(i));
}

void rageam::DrawableToFBX::SetFbxMeshMaterialSingle(FbxNode* fbxNode, int material) const
{
	FbxGeometryElementMaterial* fbxElementMaterial = GetFbxMeshFromFbxNode(fbxNode)->CreateElementMaterial();
	fbxElementMaterial->SetReferenceMode(FbxLayerElement::eIndexToDirect);
	auto& fbxMaterialMap = fbxElementMaterial->GetIndexArray();
	fbxElementMaterial->SetMappingMode(FbxLayerElement::eAllSame);
	fbxMaterialMap.Resize(1, eUninitialized);
	fbxMaterialMap.SetAt(0, material);
}

void rageam::DrawableToFBX::CreateAndExportTexturesFromShaderGroup(ConstWString outTexDirectory)
{
	EASY_FUNCTION();

	// Don't export textures if directory path wasn't specified
	bool skipExport = false;
	if (!outTexDirectory)
	{
		outTexDirectory = L"";
		skipExport = true;
	}

	file::Path texDirectory = PATH_TO_UTF8(outTexDirectory);

	rage::grmShaderGroup* shaderGroup = m_Drawable->GetShaderGroup();
	for (u16 i = 0; i < shaderGroup->GetShaderCount(); i++)
	{
		rage::grmShader* shader = shaderGroup->GetShader(i);
		for (rage::grcInstanceVar& textureVar : shader->GetTextureIterator())
		{
			rage::grcTexture* texture = textureVar.GetTexture();
			if (!texture)
				continue;

			ConstString textureName = texture->GetName();
			if (m_FbxTextures.ContainsAt(Hash(textureName))) // Texture was already cached
				continue;

			// All textures exported to .DDS format
			ConstString textureFileName = String::FormatTemp("%s.dds", textureName);
			file::Path  textureFilePath = texDirectory / textureFileName;

			FbxFileTexture* fbxTexture = FbxFileTexture::Create(m_FbxScene, textureName);
			fbxTexture->SetFileName(textureFilePath);
			fbxTexture->SetTextureUse(FbxTexture::eStandard);
			fbxTexture->SetMappingType(FbxTexture::eUV);
			fbxTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
			fbxTexture->SetTranslation(0.0, 0.0);
			fbxTexture->SetScale(1.0, 1.0);
			fbxTexture->SetRotation(0.0, 0.0);
			// fbxTexture->UVSet.Set // TODO: ?

			m_FbxTextures.InsertAt(Hash(textureName), fbxTexture);

			// Extract texture to file
			if (!skipExport && texture->GetResourceType() == rage::TEXTURE_NORMAL)
			{
				texture->ToDX11()->ExportToDDS(PATH_TO_WIDE(textureFilePath));
			}
		}
	}
}

void rageam::DrawableToFBX::CreateSkeletonAndBoneNodes()
{
	EASY_FUNCTION();

	// TODO: Skip non-skinned bones
	rage::crSkeletonData* skeletonData = m_Drawable->GetSkeletonData().Get();
	if (!skeletonData)
		return;

	for (u16 boneIndex = 0; boneIndex < skeletonData->GetBoneCount(); boneIndex++)
	{
		rage::crBoneData* boneData = skeletonData->GetBone(boneIndex);

		FbxNode* fbxBoneNode = CreateFbxNode(boneData->GetName());

		FbxSkeleton* fbxSkeleton = FbxSkeleton::Create(m_FbxScene, boneData->GetName());
		fbxBoneNode->SetNodeAttribute(fbxSkeleton);

		SetFbxNodeTransformFromBone(fbxBoneNode, boneData);

		s16 parentIndex = skeletonData->GetParentIndex(boneIndex);
		if (parentIndex == -1) // Root node, always first
		{
			m_RootNode->AddChild(fbxBoneNode);
			fbxSkeleton->SetSkeletonType(FbxSkeleton::eRoot);
		}
		else
		{
			m_FbxSkeletonNodes[parentIndex]->AddChild(fbxBoneNode);
			fbxSkeleton->SetSkeletonType(FbxSkeleton::eLimbNode);
		}

		m_FbxSkeletonNodes.Add(fbxBoneNode);

		// Add to map
		Map.SceneNodeToBone[GetCurrentNodeIndex()] = boneIndex;
		Map.BoneToSceneNode.Add(GetCurrentNodeIndex());
	}
}

void rageam::DrawableToFBX::CreateMaterialsAndSetBasicTextures()
{
	EASY_FUNCTION();

	rage::grmShaderGroup* shaderGroup = m_Drawable->GetShaderGroup();

	m_FbxMaterials.Resize(shaderGroup->GetShaderCount());
	for (u16 i = 0; i < shaderGroup->GetShaderCount(); i++)
	{
		// TODO: Export other basic parameters such as specular?

		rage::grmShader* shader = shaderGroup->GetShader(i);
		rage::grcEffect* effect = shader->GetEffect();

		FbxSurfacePhong* fbxMaterial = CreateFbxMaterial(m_FbxScene, String::FormatTemp("Material#%02d", i));
		
		// Looks up texture with given variable name in shader and connects it to fbx material property
		auto connectTexture = [&](FbxProperty& materialProperty, ConstString varName)
		{
			rage::grcHandle handle = effect->LookupVar(varName);
			if (!handle.IsValid())
				return;

			rage::grcTexture* texture = shader->GetVar(handle.GetIndex())->GetTexture();
			if (texture)
				materialProperty.ConnectSrcObject(m_FbxTextures.GetAt(Hash(texture->GetName())));
		};

		connectTexture(fbxMaterial->Diffuse, "DiffuseSampler");
		connectTexture(fbxMaterial->Bump, "BumpSampler");
		connectTexture(fbxMaterial->Specular, "SpecSampler");

		m_FbxMaterials[i] = fbxMaterial;

		// Add to map
		Map.SceneMaterialToShader[GetCurrentMaterialIndex()] = i;
		Map.ShaderToSceneMaterial.Add(GetCurrentMaterialIndex());
	}
}

int rageam::DrawableToFBX::CreateFbxMaterialForBound(FbxNode* node, const gtaMaterialId& gtaMaterial)
{
	auto addToMap = [&] (const BoundMaterial& boundMaterial)
		{
			Map.SceneMaterialToBounds[boundMaterial.MaterialIndex].Add(
				asset::BoundMaterialHandle(m_Counters.MAP_COL, boundMaterial.MaterialIndex));
		};

	u64 id = gtaMaterial.Id;

	// Try to find previously added material with the same options
	if (Options.MergePhysicalMaterials)
	{
		BoundMaterial* existingBoundMaterial = m_ColMaterials.TryGetAt(gtaMaterial.Packed);
		if (existingBoundMaterial)
		{
			addToMap(*existingBoundMaterial);
			return node->AddMaterial(existingBoundMaterial->FbxMaterial);
		}
	}

	// Material colors are not stored in game bounds (although there are color properties...)
	// They're only defined in materialfx.dat and used in beta build
	u32 color = g_MaterialInfo[id].Color;

	// Get name from physics material and convert it to upper case
	rage::phMaterial* phMaterial = rage::phMaterialMgr::GetInstance()->GetMaterialById(id);
	char materialName[64];
	String::Copy(materialName, 64, phMaterial->Name);
	MutableString(materialName).ToUpper();

	// Format name with counter - CONCRETE#5
	u32 materialNameHash = Hash(materialName);
	int* materialNameCounter = m_Counters.COL_Materials.TryGetAt(materialNameHash);
	if (!materialNameCounter)
		materialNameCounter = &m_Counters.COL_Materials.InsertAt(materialNameHash, 0);
	char countedMaterialName[64];
	sprintf_s(countedMaterialName, 64, "%s#%02d", materialName, *materialNameCounter);

	rage::Vector4 colorf = graphics::ColorU32(color).ToVec4();

	FbxSurfacePhong* fbxMaterial = CreateFbxMaterial(node, countedMaterialName);
	fbxMaterial->Diffuse.Set(FbxDouble3(colorf.X, colorf.Y, colorf.Z));
	int fbxMaterialIndex = node->AddMaterial(fbxMaterial);

	BoundMaterial boundMaterial;
	boundMaterial.FbxMaterial = fbxMaterial;
	boundMaterial.MaterialIndex = GetCurrentMaterialIndex();

	m_ColMaterials.InsertAt(gtaMaterial.Packed, boundMaterial);
	addToMap(boundMaterial);

	++(*materialNameCounter);

	return fbxMaterialIndex;
}

FbxNode* rageam::DrawableToFBX::ConvertModel(rage::grmModel& model)
{
	EASY_FUNCTION();

	// Create node and mesh for model
	FbxNode* fbxNode = CreateFbxNode(String::FormatTemp("model_%02d", m_Counters.Model++));
	FbxMesh* fbxMesh = FbxMesh::Create(m_FbxScene, "");
	fbxNode->SetNodeAttribute(fbxMesh);

	// Apply non-skinned (non-deformable) transform from skeleton
	// Deformable geometries are processed below
	if (model.GetBoneIndex() != 0)
	{
		rage::crBoneData* boneData = m_Drawable->GetSkeletonData()->GetBone(model.GetBoneIndex());
		SetFbxNodeTransformFromBone(fbxNode, boneData);
	}

	// In FBX all materials are added per node, for some reason
	// Add all materials to node for simplicity
	for (FbxSurfaceMaterial* material : m_FbxMaterials)
		fbxNode->AddMaterial(material);

	// Control points in FBX define vertex positions
	u32 modelVertexCount = model.ComputeTotalVertexCount();
	u32 modelPolygonCount = model.ComputeTotalPolygonCount();
	fbxMesh->SetControlPointCount((int) modelVertexCount);
	fbxMesh->ReservePolygonCount((int) modelPolygonCount);
	FbxVector4* fbxControlPoints = fbxMesh->GetControlPoints();

	// Material
	FbxGeometryElementMaterial* fbxElementMaterial = fbxMesh->CreateElementMaterial();
	fbxElementMaterial->SetMappingMode(FbxLayerElement::eByPolygon);
	fbxElementMaterial->SetReferenceMode(FbxLayerElement::eIndexToDirect);
	auto& fbxMaterialMap = fbxElementMaterial->GetIndexArray();
	fbxMaterialMap.Resize(modelPolygonCount, eUninitialized);

	// Deformed skinning
	rage::atFixedArray<FbxCluster*, MAX_BONES> fbxClusters; // Clusters for used bones by this model
	fbxClusters.Resize(MAX_BONES);
	FbxSkin* fbxSkin = nullptr;
	if (model.IsSkinned())
	{
		fbxSkin = FbxSkin::Create(m_FbxScene, "");
		fbxSkin->SetSkinningType(FbxSkin::eLinear);
		fbxMesh->AddDeformer(fbxSkin);

		// TODO: 
		// FbxPose* fbxPose = FbxPose::Create(fbxScene, "");
		// fbxPose->SetIsBindPose(false);
	}

	// Combine all geometries into single mesh, we must remap all indices from geometry to mesh space
	int modelVertexIndex = 0;
	int modelVertexOffset = 0;
	int modelPolygonIndex = 0;

	rage::grmGeometries& geometries = model.GetGeometries();
	for (int geometryIndex = 0; geometryIndex < geometries.GetSize(); geometryIndex++)
	{
		EASY_BLOCK("Geometry");
		EASY_VALUE("Index", geometryIndex)

		rage::grmGeometry& geometry = *geometries[geometryIndex];

		rage::grcVertexBuffer* vb = geometry.GetVertexBuffer(0);
		rage::grcIndexBuffer*  ib = geometry.GetIndexBuffer(0);

		graphics::VertexDeclaration  decl(vb->GetVertexFormat());
		graphics::VertexBufferEditor bufferEditor(decl);
		bufferEditor.InitFromExisting(vb->GetVertexCount(), vb->GetVertexData());

		// Gather basic attributes from geometry (position, blend weights, blend indices)
		EASY_BLOCK("Positions And Skinning");
		for (int i = 0; i < vb->GetVertexCount(); i++)
		{
			Vec3S pos = bufferEditor.GetAttribute<Vec3S>(i, graphics::POSITION, 0);
			fbxControlPoints[modelVertexIndex] = FbxVector4(pos.X, pos.Y, pos.Z);

			// Process skinning weights - add only unique, non zero weights
			if (fbxSkin)
			{
				// TODO: We need to ensure that all blend data is U8[4], and not float
				auto blendIndices = bufferEditor.GetAttribute<u8[4]>(i, graphics::BLENDINDICES, 0);
				auto blendWeights = bufferEditor.GetAttribute<u8[4]>(i, graphics::BLENDWEIGHT, 0);

				// Link cluster
				for (int j = 0; j < 4; j++)
				{
					u8 blendIndex = blendIndices[j];
					u8 blendWeight = blendWeights[j];

					// There's not point to add zero weight... save up some space
					if (blendWeight == 0)
						continue;

					// Check if weight been added before, often we have 4 indices for root bone and 4 zero weights
					bool duplicateWeight = false;
					for (int k = j - 1; k >= 0; k--) // First blend index is skipped
					{
						if (blendIndex == blendIndices[k] && blendWeights[j] == blendWeights[k])
						{
							duplicateWeight = true;
							break;
						}
					}
					// Weight was already specified for this bone index, skip
					if (duplicateWeight)
						continue;

					// Convert bone index from geometry to skeleton space
					blendIndex = geometry.GetBoneIDs()[blendIndex];

					FbxCluster* fbxCluster = fbxClusters[blendIndex];
					if (!fbxCluster) // This bone (cluster) was not referenced by this model before, add new one
					{
						FbxNode* fbxClusterNode = m_FbxSkeletonNodes[blendIndex];

						fbxCluster = FbxCluster::Create(m_FbxScene, fbxClusterNode->GetName());

						fbxCluster->SetLink(fbxClusterNode);
						fbxCluster->SetLinkMode(FbxCluster::eTotalOne);

						FbxAMatrix transform = fbxClusterNode->EvaluateGlobalTransform();
						fbxCluster->SetTransformLinkMatrix(transform);

						fbxClusters[blendIndex] = fbxCluster;
						fbxSkin->AddCluster(fbxCluster);
					}

					float blendWeightf = static_cast<float>(blendWeights[j]) / 255.0f;
					fbxCluster->AddControlPointIndex(modelVertexIndex, blendWeightf);
				}
			}

			modelVertexIndex++;
		}
		EASY_END_BLOCK;

		// We ignore:
		// - POSITION	already exported
		// - BINORMAL	not used
		EASY_BLOCK("Extra Attributes");
		for (graphics::VertexAttribute attribute : decl.Attributes)
		{
			EASY_BLOCK(graphics::FormatSemanticName(attribute.Semantic, attribute.SemanticIndex));

			struct Vec4U16 // Might be used on colors
			{
				u16 X, Y, Z, W;
			};
			// FBX use double vectors, write simple dummy structs for conversion
			// Those constructors are called in GetAttributeArray when converting TIn type to TOut
			struct Vec4D
			{
				double X, Y, Z, W;
				Vec4D() = default;
				Vec4D(const Vec3S& v) { X = v.X; Y = v.Y; Z = v.Z; W = 0.0f; }		// Normal
				Vec4D(const Vec4S& v) { X = v.X; Y = v.Y; Z = v.Z; W = v.W; }		// Tangent, Color
				Vec4D(u32 v)
				{
					graphics::ColorU32 c = v;
					X = c.R; Y = c.G; Z = c.B; W = c.A;
				} // Color
				Vec4D(const Vec4U16& v)
				{
					X = static_cast<float>(v.X) / UINT16_MAX; Y = static_cast<float>(v.Y) / UINT16_MAX;
					Z = static_cast<float>(v.Z) / UINT16_MAX; W = static_cast<float>(v.W) / UINT16_MAX;
				} // Color
			};
			struct Vec2DInvertY // This is a little hack, but we need ot invert Y axis for texcoord, it is easier to do it once here
			{
				double X, Y;
				Vec2DInvertY() = default;
				Vec2DInvertY(const Vec2S& v) { X = v.X; Y = 1.0f - v.Y; }
			};

			// For some reason there's no function to get existing or create new, here's ugly macro to do it
#define GET_MESH_ELEMENT(element, ...) fbxMesh->GetElement ##element ##Count() > 0 ? fbxMesh->GetElement ##element ##() : fbxMesh->CreateElement ##element ##(__VA_ARGS__)
			// Another macro to append mesh data, extra va argument is used to pass UV name to fbxMesh->CreateElementUV
#define APPEND_MESH_DATA(element, typeIn, typeOut, ...) \
			AppendFbxMeshElementData(modelVertexOffset, modelVertexCount, GET_MESH_ELEMENT(element, __VA_ARGS__), bufferEditor.GetAttributeArray<typeIn, typeOut>(attribute.Semantic, attribute.SemanticIndex))

			if (attribute.Semantic == graphics::NORMAL)		
				APPEND_MESH_DATA(Normal, Vec3S, Vec4D);
			if (attribute.Semantic == graphics::TANGENT)	
				APPEND_MESH_DATA(Tangent, Vec4S, Vec4D);
			if (attribute.Semantic == graphics::TEXCOORD)
			{
				ConstString uvName = String::FormatTemp("UVMap %i", attribute.SemanticIndex);
				if (attribute.Format == DXGI_FORMAT_R32G32_FLOAT)
					APPEND_MESH_DATA(UV, Vec2S, Vec2DInvertY, uvName);
				else
					AM_WARNINGF("DrawableToFBX::ConvertModel() -> Unsupported texcoord data type %i!", attribute.Format);
			}
			if (attribute.Semantic == graphics::COLOR)
			{
				if (attribute.Format == DXGI_FORMAT_R32G32B32A32_FLOAT)		 
					APPEND_MESH_DATA(VertexColor, Vec4S, Vec4D);		// float[4]
				else if (attribute.Format == DXGI_FORMAT_R8G8B8A8_UNORM)	 
					APPEND_MESH_DATA(VertexColor, u32, Vec4D);			// u32
				else if (attribute.Format == DXGI_FORMAT_R16G16B16A16_UINT)  
					APPEND_MESH_DATA(VertexColor, Vec4U16, Vec4D);		// u16[4]
				else // Not sure if I missed any possible color type, but just in case
					AM_WARNINGF("DrawableToFBX::ConvertModel() -> Unsupported color data type %i!", attribute.Format);
			}

#undef APPEND_MESH_DATA
#undef GET_MESH_ELEMENT
		}
		EASY_END_BLOCK;

		// Create polygons from indices
		u32 polyCount = ib->GetIndexCount() / 3;
		EASY_BLOCK("Polygons");
		for (u32 i = 0; i < polyCount; i++)
		{
			fbxMesh->BeginPolygon();
			fbxMesh->AddPolygon(modelVertexOffset + ib->GetIndexData()[i * 3 + 0]);
			fbxMesh->AddPolygon(modelVertexOffset + ib->GetIndexData()[i * 3 + 1]);
			fbxMesh->AddPolygon(modelVertexOffset + ib->GetIndexData()[i * 3 + 2]);
			fbxMesh->EndPolygon();

			// Set polygon material
			fbxMaterialMap.SetAt(modelPolygonIndex, model.GetMaterialIndex(geometryIndex));

			modelPolygonIndex++;
		}
		EASY_END_BLOCK;

		// For correctly converting indices form local (geometry) to global (model) space, we
		// set it to total vertex count of already processed geometries
		modelVertexOffset = modelVertexIndex;
	}

	return fbxNode;
}

void rageam::DrawableToFBX::ConvertLodGroup()
{
	EASY_FUNCTION();

	rage::rmcLodGroup& lodGroup = m_Drawable->GetLodGroup();
	for (int lod = 0; lod <= Options.MaxLod; lod++)
	{
		EASY_BLOCK(String::FormatTemp("LOD %i", lod));

		rage::grmModels& lodModels = lodGroup.GetLod(lod)->GetModels();
		if (!lodModels.Any())
			continue;

		FbxNode* fbxLodNode = CreateFbxNode(String::FormatTemp(".lod%i", lod));
		m_RootNode->AddChild(fbxLodNode);

		for (int modelIndex = 0; modelIndex < lodModels.GetSize(); modelIndex++)
		{
			FbxNode* fbxModelNode = ConvertModel(*lodModels[modelIndex]);

			// Add model to map
			Map.SceneNodeToLOD[GetCurrentNodeIndex()] = lod;
			Map.SceneNodeToModel[lod][GetCurrentNodeIndex()] = m_Counters.MAP_Model[lod]++;
			Map.ModelToSceneNode[lod].Add(GetCurrentNodeIndex());

			fbxLodNode->AddChild(fbxModelNode);
		}
	}
}

void rageam::DrawableToFBX::ConvertBounds()
{
	EASY_FUNCTION();

	rage::phBound* bound = m_Drawable->GetBound().Get();
	if (!bound)
		return;

	FbxNode* rootNode = CreateFbxNode(".col");
	ConvertBoundRecurse(bound, rootNode, Mat44V::Identity(), -1);
	m_RootNode->AddChild(rootNode);
}

void rageam::DrawableToFBX::ConvertBoundRecurse(rage::phBound* bound, FbxNode* parentNode, const Mat44V& transform, int compositeIndex)
{
	EASY_FUNCTION();

	FbxNode* fbxNode = nullptr;
	switch(bound->GetShapeType())
	{
	case rage::PH_BOUND_SPHERE:
	case rage::PH_BOUND_CAPSULE:
	case rage::PH_BOUND_BOX:
	case rage::PH_BOUND_DISC:
	case rage::PH_BOUND_CYLINDER:
	case rage::PH_BOUND_PLANE:
		fbxNode = ConvertBoundPrimitive((rage::phBoundPrimitive*)bound, transform);
		break;
	case rage::PH_BOUND_BVH: 
		fbxNode = ConvertBoundBVH((rage::phBoundBVH*)bound, transform);
		break;
	case rage::PH_BOUND_GEOMETRY:
		fbxNode = ConvertBoundGeometry((rage::phBoundGeometry*)bound, transform);
		break;
	case rage::PH_BOUND_COMPOSITE:
	{
		EASY_BLOCK("Composite");
		fbxNode = CreateFbxNode(String::FormatTemp("composite%02d.col", m_Counters.COL_Composite++));
		// Composite children are processed later...
		break;
	}

	default:
		AM_WARNINGF("DrawableToFBX::ConvertBoundRecurse() -> Type '%i' is not supported!", bound->GetShapeType());
		break;
	}

	if (!fbxNode)
		return;

	parentNode->AddChild(fbxNode);

	// Get primitive type
	graphics::PrimitiveType primitiveType = graphics::PrimitiveInvalid;
	switch (bound->GetShapeType())
	{
	case rage::PH_BOUND_BOX:      primitiveType = graphics::PrimitiveBox;		break;
	case rage::PH_BOUND_CAPSULE:  primitiveType = graphics::PrimitiveCapsule;	break;
	case rage::PH_BOUND_SPHERE:   primitiveType = graphics::PrimitiveSphere;	break;
	case rage::PH_BOUND_CYLINDER: primitiveType = graphics::PrimitiveCylinder;	break;
	case rage::PH_BOUND_GEOMETRY: primitiveType = graphics::PrimitiveMesh;		break;
	default: break;
	}

	// We convert composite children just after that to setup indices in map correctly, because adding composite children will shift composite back
	Map.SceneNodeToBound[GetCurrentNodeIndex()].Add(m_Counters.MAP_COL++);
	Map.SceneNodeToBoundType[GetCurrentNodeIndex()] = primitiveType;
	Map.BoundToSceneNode.Add(GetCurrentNodeIndex());
	Map.BoundToComposite.Add(compositeIndex);

	if (bound->GetShapeType() == rage::PH_BOUND_COMPOSITE)
	{
		int newCompositeIndex = m_Counters.MAP_COL - 1;

		rage::phBoundComposite* composite = bound->AsComposite();
		for (u16 i = 0; i < composite->GetNumBounds(); i++)
		{
			Mat44V compositeTransform = ((Mat34V*)&composite->GetMatrix(i))->To44(); // TODO: Composite actually stores Mat34V!
			Mat44V childTransform = transform * compositeTransform;
			ConvertBoundRecurse(composite->GetBound(i).Get(), fbxNode, childTransform, newCompositeIndex);
		}
	}
}

FbxNode* rageam::DrawableToFBX::ConvertBoundGeometry(const rage::phBoundGeometry* bound, const Mat44V& transform)
{
	EASY_FUNCTION();

	int numMaterials = bound->GetNumMaterials();

	List<int> polyMaterials;
	bool	  isSingleMaterial = numMaterials <= 1;

	m_TempVerts.Clear();
	m_TempIndices.Clear();

	int vertexCount = static_cast<int>(bound->GetVertexCount());
	m_TempVerts.Reserve(vertexCount);
	for (int i = 0; i < vertexCount; i++)
		m_TempVerts.Add(bound->GetVertex(i).Transform(transform));

	int polyCount = static_cast<int>(bound->GetPolygonCount());
	m_TempIndices.Reserve(polyCount * 3);
	if (!isSingleMaterial)
		polyMaterials.Reserve(polyCount);
	for (int i = 0; i < polyCount; i++)
	{
		rage::phPolygon& poly = bound->GetPolygon(i);
		int vi0 = poly.GetVertexIndex(0);
		int vi1 = poly.GetVertexIndex(1);
		int vi2 = poly.GetVertexIndex(2);

		m_TempIndices.Add(vi0);
		m_TempIndices.Add(vi1);
		m_TempIndices.Add(vi2);

		if (!isSingleMaterial)
		{
			int polygonMaterialIndex = bound->GetPolygonMaterial(i);
			polyMaterials.Add(polygonMaterialIndex);
		}
	}

	ConstString name = String::FormatTemp("geometry%02d.col", m_Counters.COL_Geometry++);
	FbxNode* fbxNode = CreateFbxNodeFromMeshData(name, m_TempVerts, m_TempIndices);

	// Convert all materials in bound
	for (int i = 0; i < numMaterials; i++)
	{
		gtaMaterialId gtaMaterial = const_cast<rage::phBoundGeometry*>(bound)->GetMaterialId(i);
		CreateFbxMaterialForBound(fbxNode, gtaMaterial);
	}

	// Not sure if there are bounds with no materials, create default one in case
	if (numMaterials == 0)
	{
		AM_WARNINGF("DrawableToFBX::ConvertBoundGeometry() -> Bound has no materials! Creating default.");
		CreateFbxMaterialForBound(fbxNode, rage::phMaterialMgr::DEFAULT_MATERIAL_ID);
	}

	// Map material to polys
	if (isSingleMaterial)
		SetFbxMeshMaterialSingle(fbxNode, 0);
	else
		SetFbxMeshMaterialsPerPolygon(fbxNode, polyMaterials);

	return fbxNode;
}

FbxNode* rageam::DrawableToFBX::ConvertBoundPrimitive(const rage::phBoundPrimitive* primitive, const Mat44V& transform)
{
	EASY_FUNCTION();

	if (primitive->GetShapeType() == rage::PH_BOUND_PLANE)
	{
		AM_TRACEF("DrawableToFBX::ConvertBoundPrimitive() -> Plane bound cannot be converted to polygonal mesh!");
		return nullptr;
	}

	m_TempVerts.Clear();
	m_TempIndices.Clear();

	ConstString nodeName;
	switch (primitive->GetShapeType())
	{
	case rage::PH_BOUND_SPHERE:
	{
		Sphere sphere = primitive->GetBoundingSphere();
		graphics::MeshGenerator::FromSphere(sphere, m_TempVerts, &m_TempIndices, transform);
		nodeName = String::FormatTemp("sphere%02d.col", m_Counters.COL_Sphere++);
		break;
	}
	case rage::PH_BOUND_CAPSULE:
	{
		rage::phBoundCapsule* capsuleBound = (rage::phBoundCapsule*)primitive;
		AABB   bb = capsuleBound->GetBoundingBox();
		Vec3V  center = bb.Center();
		Mat44V capsuleTransform = Mat44V::Translation(center) * transform;
		graphics::MeshGenerator::FromCapsule(capsuleBound->GetHalfLenght(), capsuleBound->GetRadius(), true, m_TempVerts, &m_TempIndices, capsuleTransform);
		nodeName = String::FormatTemp("capsule%02d.col", m_Counters.COL_Capsule++);
		break;
	}
	case rage::PH_BOUND_CYLINDER:
	{
		rage::phBoundCylinder* cylinderBound = (rage::phBoundCylinder*)primitive;
		AABB   bb = cylinderBound->GetBoundingBox();
		Vec3V  center = bb.Center();
		Mat44V cylinderTransform = Mat44V::Translation(center) * transform;
		graphics::MeshGenerator::FromCylinder(cylinderBound->GetHeight(), cylinderBound->GetRadius(), true, m_TempVerts, &m_TempIndices, cylinderTransform);
		nodeName = String::FormatTemp("cylinder%02d.col", m_Counters.COL_Cylinder++);
		break;
	}
	case rage::PH_BOUND_BOX:
		graphics::MeshGenerator::FromAABB(primitive->GetBoundingBox(), m_TempVerts, &m_TempIndices, transform);
		nodeName = String::FormatTemp("box%02d.col", m_Counters.COL_Box++);
		break;
	// TODO: Alexguirre - used only on vehicles
	// case rage::PH_BOUND_DISC:

	default:
		AM_WARNINGF("DrawableToFBX::ConvertBoundPrimitive() -> Primitive '%i' is not supported!", primitive->GetShapeType());
		return nullptr;
	}

	// Create mesh and set material
	FbxNode* fbxNode = CreateFbxNodeFromMeshData(nodeName, m_TempVerts, m_TempIndices);
	int fbxMaterialIndex = CreateFbxMaterialForBound(fbxNode, primitive->GetPrimitiveMaterialId());
	SetFbxMeshMaterialSingle(fbxNode, fbxMaterialIndex);

	return fbxNode;
}

FbxNode* rageam::DrawableToFBX::ConvertBoundBVH(const rage::phBoundBVH* boundBVH, const Mat44V& transform)
{
	EASY_FUNCTION();

	// We have to combine polygons in BVH into single mesh
	List<Vec3V>  triVerts;
	List<int>    triIndices;
	List<int>    triMaterials;	 // In most cases there's only single material per mesh, however it is simpler to map by poly vertex
	HashSet<int> triMaterialMap; // To remap material index from BVH space to triangle mesh index space
	HashSet<int> triIndexMap;	 // To remap BVH index space to mesh index space. Mesh excludes all other BVH primitives

	// Used for cylinder & capsule
	auto orientedTransform = [](const Vec3V& pos, const Vec3V& dir)
		{
			Vec3V up = dir;
			Vec3V right, front;
			up.TangentAndBiNormal(right, front);
			Mat34V mat;
			mat.Right = right;
			mat.Front = front;
			mat.Up = up;
			mat.Pos = pos;
			return mat.To44();
		};

	FbxNode* fbxBvhNode = CreateFbxNode(String::FormatTemp("bvh%02d.col", m_Counters.COL_BVH++));

	// Add BVH to map
	Map.SceneNodeToBound[GetCurrentNodeIndex()].Add(m_Counters.MAP_COL);
	Map.SceneNodeToBoundType[GetCurrentNodeIndex()] = graphics::PrimitiveInvalid;

	for (int i = 0; i < boundBVH->GetPrimitiveCount(); i++)
	{
		m_TempVerts.Clear();
		m_TempIndices.Clear();
		ConstString nodeName;

		graphics::PrimitiveType primitiveType = graphics::PrimitiveInvalid;

		rage::phPrimitive& primitive = boundBVH->GetPrimitive(i);
		switch (primitive.GetType())
		{
		case rage::PRIM_TYPE_POLYGON:
		{
			rage::phPolygon& polygon = primitive.GetPolygon();
			for (int k = 0; k < 3; k++)
			{
				int  vertexIndex = polygon.GetVertexIndex(k);
				int* remappedVertexIndex = triIndexMap.TryGetAt(vertexIndex);
				if (!remappedVertexIndex) // Index wasn't used before, insert new vertex and get index from it
				{
					Vec3V newVertex = boundBVH->GetVertex(vertexIndex);
					int   newVertexIndex = triVerts.GetSize();
					triVerts.Add(newVertex);
					remappedVertexIndex = &triIndexMap.InsertAt(vertexIndex, newVertexIndex);
				}
				triIndices.Add(*remappedVertexIndex);
			}
			break;
		}
		case rage::PRIM_TYPE_SPHERE:
		{
			rage::phPrimSphere& sphere = primitive.GetSphere();
			Vec3V center = boundBVH->GetVertex(sphere.GetCenterIndex());
			graphics::MeshGenerator::FromSphere(Sphere(center, sphere.GetRadius()), m_TempVerts, &m_TempIndices, transform);
			nodeName = String::FormatTemp("sphere%02d.col", m_Counters.COL_Sphere++);
			primitiveType = graphics::PrimitiveSphere;
			break;
		}
		case rage::PRIM_TYPE_CAPSULE:
		{
			rage::phPrimCapsule& capsule = primitive.GetCapsule();
			Vec3V   v0 = boundBVH->GetVertex(capsule.GetEndIndex0());
			Vec3V   v1 = boundBVH->GetVertex(capsule.GetEndIndex1());
			ScalarV len = v0.DistanceTo(v1);
			Vec3V   dir = (v1 - v0) / len;
			Vec3V   center = (v0 + v1) * rage::S_HALF;
			ScalarV halfExtent = len * rage::S_HALF;
			Mat44V capsuleTransform = orientedTransform(center, dir) * transform;
			graphics::MeshGenerator::FromCapsule(halfExtent.Get(), capsule.GetRadius(), false, m_TempVerts, &m_TempIndices, capsuleTransform);
			nodeName = String::FormatTemp("capsule%02d.col", m_Counters.COL_Capsule++);
			primitiveType = graphics::PrimitiveCapsule;
			break;
		}
		case rage::PRIM_TYPE_CYLINDER:
		{
			rage::phPrimCylinder& cylinder = primitive.GetCylinder();
			Vec3V   v0 = boundBVH->GetVertex(cylinder.GetEndIndex0());
			Vec3V   v1 = boundBVH->GetVertex(cylinder.GetEndIndex1());
			ScalarV len = v0.DistanceTo(v1);
			Vec3V   dir = (v1 - v0) / len;
			Vec3V   center = (v0 + v1) * rage::S_HALF;
			Mat44V cylinderTransform = orientedTransform(center, dir) * transform;
			graphics::MeshGenerator::FromCylinder(len.Get(), cylinder.GetRadius(), false, m_TempVerts, &m_TempIndices, cylinderTransform);
			nodeName = String::FormatTemp("cylinder%02d.col", m_Counters.COL_Cylinder++);
			primitiveType = graphics::PrimitiveCylinder;
			break;
		}
		case rage::PRIM_TYPE_BOX:
		{
			rage::phPrimBox& box = primitive.GetBox();
			Vec3V v0 = boundBVH->GetVertex(box.GetVertexIndex(0));
			Vec3V v1 = boundBVH->GetVertex(box.GetVertexIndex(1));
			Vec3V v2 = boundBVH->GetVertex(box.GetVertexIndex(2));
			Vec3V v3 = boundBVH->GetVertex(box.GetVertexIndex(3));
			graphics::MeshGenerator::FromOBB(v0, v1, v2, v3, m_TempVerts, &m_TempIndices, transform);
			nodeName = String::FormatTemp("box%02d.col", m_Counters.COL_Box++);
			primitiveType = graphics::PrimitiveBox;
			break;
		}

		default:
			AM_WARNINGF("DrawableToFBX::ConvertBoundBVH() -> Primitive '%i' is not supported!", primitive.GetType());
			continue;
		}

		// Polygons are converted later
		if (primitive.GetType() != rage::PRIM_TYPE_POLYGON)
		{
			FbxNode* fbxNode = CreateFbxNodeFromMeshData(nodeName, m_TempVerts, m_TempIndices);

			// Primitives are mapped to single material
			gtaMaterialId gtaMaterial = boundBVH->GetMaterialIdFromPartIndexAndComponent(i);
			int fbxMaterialIndex = CreateFbxMaterialForBound(fbxNode, gtaMaterial);
			SetFbxMeshMaterialSingle(fbxNode, fbxMaterialIndex);

			fbxBvhNode->AddChild(fbxNode);

			// Add BVH primitive node to map
			Map.SceneNodeToBound[GetCurrentNodeIndex()].Add(m_Counters.MAP_COL);
			Map.SceneNodeToBoundType[GetCurrentNodeIndex()] = primitiveType;
			Map.SceneNodeToBoundIsInBVH[GetCurrentNodeIndex()] = true;
		}
	}

	// Create mesh node for polygons
	if (triVerts.Any())
	{
		ConstString polyNodeName = String::FormatTemp("geometry%02d.col", m_Counters.COL_Geometry++);
		FbxNode* polyFbxNode = CreateFbxNodeFromMeshData(polyNodeName, triVerts, triIndices);
		fbxBvhNode->AddChild(polyFbxNode);

		// Convert tri materials, we have to do this after creating node for tri geometry
		for (int i = 0; i < boundBVH->GetPrimitiveCount(); i++)
		{
			rage::phPrimitive& primitive = boundBVH->GetPrimitive(i);
			if (primitive.GetType() != rage::PRIM_TYPE_POLYGON)
				continue;

			int polygonMaterialIndex = boundBVH->GetPolygonMaterial(i);
			int* existingFbxMaterialIndex = triMaterialMap.TryGetAt(polygonMaterialIndex);
			if (!existingFbxMaterialIndex)
			{
				gtaMaterialId gtaMaterial = boundBVH->GetMaterialIdFromPartIndexAndComponent(i);
				int newFbxMaterialIndex = CreateFbxMaterialForBound(polyFbxNode, gtaMaterial);
				existingFbxMaterialIndex = &triMaterialMap.InsertAt(polygonMaterialIndex, newFbxMaterialIndex);
			}
			triMaterials.Add(*existingFbxMaterialIndex);
		}

		SetFbxMeshMaterialsPerPolygon(polyFbxNode, triMaterials);

		// Add BVH tri mesh node to map
		Map.SceneNodeToBound[GetCurrentNodeIndex()].Add(m_Counters.MAP_COL);
		Map.SceneNodeToBoundType[GetCurrentNodeIndex()] = graphics::PrimitiveMesh;
		Map.SceneNodeToBoundIsInBVH[GetCurrentNodeIndex()] = true;
	}

	return fbxBvhNode;
}

void rageam::DrawableToFBX::ConvertLights()
{
	for (int i = 0; i < m_Drawable->GetLightCount(); i++)
	{
		CLightAttr* light = m_Drawable->GetLight(i);
		FbxNode* fbxLightNodeParent = m_RootNode;

		// Get position and rotation of the light
		Mat44V matrix = light->GetMatrix();
		Vec3V trans;
		QuatV rot;
		matrix.Decompose(&trans, nullptr, &rot);
		Vec3V rotEuler = rot.ToEuler();
		// For some reason FBX lights are exported -90 degrees on X axis...
		// TODO: Does that always work...?
		rotEuler += Vec3V(90, 0, 0);

		// Create FBX node for the light
		FbxNode* fbxLightNode = CreateFbxNode(String::FormatTemp("Light %i", m_Counters.Light++));
		fbxLightNode->LclTranslation = FbxDouble3(trans.X(), trans.Y(), trans.Z());
		fbxLightNode->LclRotation = FbxDouble3(rotEuler.X(), rotEuler.Y(), rotEuler.Z());

		// Link light to the bone FBX node
		rage::crSkeletonData* skeletonData = m_Drawable->GetSkeletonData().Get();
		if (skeletonData && light->BoneTag != 0)
		{
			rage::crBoneData* bone = skeletonData->GetBoneFromTag(light->BoneTag);
			if (bone)
			{
				// TODO: We should store map of bones linked objects, like nodes, and lights, and keep it if its only used for skinning!
				u16 boneIndex = bone->GetIndex();
				fbxLightNodeParent = m_FbxSkeletonNodes[boneIndex];
			}
			else
			{
				AM_WARNINGF("DrawableToFBX::ConvertLights() -> Light linked to bone with tag '%u' that doesn't exists in skeleton!", light->BoneTag);
			}
		}

		// Create light attributes and set basic parameters
		FbxLight* fbxLight = FbxLight::Create(m_FbxScene, "");
		fbxLight->Color = FbxDouble3(
			static_cast<float>(light->ColorR) / 255.0f,
			static_cast<float>(light->ColorG) / 255.0f,
			static_cast<float>(light->ColorB) / 255.0f);
		if (light->Type == LIGHT_TYPE_SPOT)
			fbxLight->LightType = FbxLight::eSpot;
		else // Capsule light are not supported in FBX
			fbxLight->LightType = FbxLight::ePoint;
		fbxLight->InnerAngle = light->ConeInnerAngle;
		fbxLight->OuterAngle = light->ConeOuterAngle;
		fbxLightNode->SetNodeAttribute(fbxLight);

		fbxLightNodeParent->AddChild(fbxLightNode);

		// Add light to drawable map
		Map.SceneNodeToLightAttr[GetCurrentNodeIndex()] = i;
		Map.LightAttrToSceneNode.Add(GetCurrentNodeIndex());
	}
}

void rageam::DrawableToFBX::Export(gtaDrawable* drawable, ConstWString drawablePath, ConstWString outFbxPath, ConstWString outTexDirectory)
{
	// Create root node in scene for our drawable, this is convinient in case if user imports multiple drawables
	file::Path drawableName;
	if (drawable->GetName()) // Might be NULL on custom models
		drawableName = drawable->GetName();
	else
		drawableName = PATH_TO_UTF8(drawablePath);
	drawableName = drawableName.GetFileNameWithoutExtension(); // Make sure to remove .#dr from name

	char blockName[256];
	sprintf_s(blockName, 256, "Export To FBX - %s", drawableName.GetCStr());
	EASY_BLOCK(blockName);

	EASY_BLOCK("FBX Init");
	Map = {};
	m_Counters = Counters();
	m_Drawable = drawable;

	m_FbxManager = FbxManager::Create();
	m_FbxScene = FbxScene::Create(m_FbxManager, "rageAm FBX Document");
	m_RootNode = CreateFbxNode(drawableName);
	m_FbxScene->GetRootNode()->AddChild(m_RootNode);

	FbxGlobalSettings& fbxSettings = m_FbxScene->GetGlobalSettings();
	fbxSettings.SetAxisSystem(FbxAxisSystem(FbxAxisSystem::eMax));
	fbxSettings.SetSystemUnit(FbxSystemUnit::m); // Meters
	EASY_END_BLOCK;

	CreateAndExportTexturesFromShaderGroup(outTexDirectory);
	CreateMaterialsAndSetBasicTextures();
	CreateSkeletonAndBoneNodes();
	ConvertLodGroup();
	ConvertBounds();
	ConvertLights();

	EASY_BLOCK("FBX Write");
	FbxExporter* exporter = FbxExporter::Create(m_FbxManager, "rageAm FBX Exporter");
	exporter->Initialize(PATH_TO_UTF8(outFbxPath)); // fileFormat '1' for ASCII
	exporter->Export(m_FbxScene);
	EASY_END_BLOCK;

	m_FbxManager->Destroy(); // Will free all FBX objects created in this function
	m_FbxManager = nullptr;
	m_FbxScene = nullptr;
	m_Drawable = nullptr;
}

void rageam::DrawableToFBX::Export(ConstWString drawablePath, ConstWString outFbxPath, ConstWString outTexDirectory)
{
	EASY_FUNCTION();

	gtaDrawable* drawable;

	EASY_BLOCK("Compile Drawable");
	bool oldKeepResourcePixelData = rage::grcTextureDX11::tl_KeepResourcePixelData;
	rage::grcTextureDX11::tl_KeepResourcePixelData = true;
	rage::pgRscBuilder::Load(&drawable, PATH_TO_UTF8(drawablePath), 165, false);
	rage::grcTextureDX11::tl_KeepResourcePixelData = oldKeepResourcePixelData;
	EASY_END_BLOCK;

	Export(drawable, drawablePath, outFbxPath, outTexDirectory);

	// TODO:
	// delete drawable;
}
