#include "scene_gl.h"

#include "am/file/fileutils.h"
#include "rage/crypto/joaat.h"
#include "rage/math/math.h"
#include "rage/math/vec.h"

DXGI_FORMAT rageam::graphics::GlTypeToDxgiFormat(cgltf_type type, cgltf_component_type componentType)
{
	switch (type) // NOLINT(clang-diagnostic-switch-enum)
	{
	case cgltf_type_scalar:
		switch (componentType) // NOLINT(clang-diagnostic-switch-enum)
		{
		case cgltf_component_type_r_8:		return DXGI_FORMAT_R8_SINT;
		case cgltf_component_type_r_8u:		return DXGI_FORMAT_R8_UINT;
		case cgltf_component_type_r_16:		return DXGI_FORMAT_R16_SINT;
		case cgltf_component_type_r_16u:	return DXGI_FORMAT_R16_UINT;
		case cgltf_component_type_r_32u:	return DXGI_FORMAT_R32_UINT;
		case cgltf_component_type_r_32f:	return DXGI_FORMAT_R32_FLOAT;
		default:							break;
		} break;

	case cgltf_type_vec2:
		switch (componentType) // NOLINT(clang-diagnostic-switch-enum)
		{
		case cgltf_component_type_r_8:		return DXGI_FORMAT_R8G8_SINT;
		case cgltf_component_type_r_8u:		return DXGI_FORMAT_R8G8_UINT;
		case cgltf_component_type_r_16:		return DXGI_FORMAT_R16G16_SINT;
		case cgltf_component_type_r_16u:	return DXGI_FORMAT_R16G16_UINT;
		case cgltf_component_type_r_32u:	return DXGI_FORMAT_R32G32_UINT;
		case cgltf_component_type_r_32f:	return DXGI_FORMAT_R32G32_FLOAT;
		default:							break;
		} break;

	case cgltf_type_vec3:
		switch (componentType) // NOLINT(clang-diagnostic-switch-enum)
		{
			// We can't really use those without providing element stride
		//case cgltf_component_type_r_8:		return DXGI_FORMAT_R8G8B8A8_SINT;
		//case cgltf_component_type_r_8u:		return DXGI_FORMAT_R8G8B8A8_UINT;
		//case cgltf_component_type_r_16:		return DXGI_FORMAT_R16G16B16A16_SINT;
		//case cgltf_component_type_r_16u:	return DXGI_FORMAT_R16G16B16A16_UINT;
		case cgltf_component_type_r_32u:	return DXGI_FORMAT_R32G32B32_UINT;
		case cgltf_component_type_r_32f:	return DXGI_FORMAT_R32G32B32_FLOAT;
		default:							break;
		} break;

	case cgltf_type_vec4:
		switch (componentType) // NOLINT(clang-diagnostic-switch-enum)
		{
		case cgltf_component_type_r_8:		return DXGI_FORMAT_R8G8B8A8_SINT;
		case cgltf_component_type_r_8u:		return DXGI_FORMAT_R8G8B8A8_UINT;
		case cgltf_component_type_r_16:		return DXGI_FORMAT_R16G16B16A16_SINT;
		case cgltf_component_type_r_16u:	return DXGI_FORMAT_R16G16B16A16_UINT;
		case cgltf_component_type_r_32u:	return DXGI_FORMAT_R32G32B32A32_UINT;
		case cgltf_component_type_r_32f:	return DXGI_FORMAT_R32G32B32A32_FLOAT;
		default:							break;
		} break;

	default: break;
	}
	AM_UNREACHABLE("GlTypeToDxgiFormat() -> Type %s with component type %s is unsupported.",
		Enum::GetName(type), Enum::GetName(componentType));
}

void rageam::graphics::SceneMaterialGl::ScanTextures()
{
	// Diffuse
	cgltf_texture* metallicRoughness = m_Material->pbr_metallic_roughness.base_color_texture.texture;
	cgltf_texture* specularGlosiness = m_Material->pbr_specular_glossiness.diffuse_texture.texture;
	if (metallicRoughness) m_DiffuseMap = GetGlTextureName(metallicRoughness);
	if (specularGlosiness) m_DiffuseMap = GetGlTextureName(specularGlosiness);

	// Specular
	cgltf_texture* specularColor = m_Material->specular.specular_color_texture.texture;
	cgltf_texture* specular = m_Material->specular.specular_texture.texture;
	if (specularColor) m_SpecularMap = GetGlTextureName(specularColor);
	if (specular) m_SpecularMap = GetGlTextureName(specular);

	// Normal
	cgltf_texture* normal = m_Material->normal_texture.texture;
	if (normal) m_NormalMap = GetGlTextureName(normal);
}

rageam::graphics::SceneMaterialGl::SceneMaterialGl(SceneGl* parent, u16 index, cgltf_material* material)
	: SceneMaterial(parent, index)
{
	m_Material = material;
	m_NameHash = rage::joaat(material->name);

	ScanTextures();
}

ConstString rageam::graphics::SceneMaterialGl::GetName() const
{
	return m_Material->name;
}

u32 rageam::graphics::SceneMaterialGl::GetNameHash() const
{
	return m_NameHash;
}

ConstString rageam::graphics::SceneMaterialGl::GetTextureName(eMaterialTexture texture) const
{
	switch (texture)
	{
	case MT_Diffuse: return m_DiffuseMap;
	case MT_Specular: return m_SpecularMap;
	case MT_Normal: return m_NormalMap;
	default:
		AM_UNREACHABLE("SceneMaterialGl::GetTextureFileName() -> Type '%s' is not supported",
			Enum::GetName(texture));
	}
}

int rageam::graphics::SceneGeometryGl::FindGlAttributeIndex(cgltf_attribute_type type) const
{
	for (cgltf_size i = 0; i < m_Primitive->attributes_count; i++)
	{
		if (m_Primitive->attributes[i].type == type)
			return static_cast<int>(i);
	}
	return -1;
}

void rageam::graphics::SceneGeometryGl::ComputeBB()
{
	int posAttributeIdx = FindGlAttributeIndex(cgltf_attribute_type_position);
	if (posAttributeIdx == -1)
	{
		m_BoundingBox = rage::spdAABB::Empty();
		return;
	}

	float minX = FLT_MAX;
	float minY = FLT_MAX;
	float minZ = FLT_MAX;
	float maxX = FLT_MIN;
	float maxY = FLT_MIN;
	float maxZ = FLT_MIN;

	cgltf_attribute& posAttribute = m_Primitive->attributes[posAttributeIdx];
	cgltf_accessor* posData = posAttribute.data;
	cgltf_buffer_view* posBufferView = posData->buffer_view;

	// Use existing min&max data if exists or compute manually
	if (posData->has_min && posData->has_max)
	{
		minX = posData->min[0];
		minY = posData->min[1];
		minZ = posData->min[2];
		maxX = posData->max[0];
		maxY = posData->max[1];
		maxZ = posData->max[2];
	}
	else
	{
		char* buffer = (char*)posBufferView->buffer->data + posBufferView->offset;
		rage::Vector3* vecs = (rage::Vector3*)buffer;
		for (cgltf_size i = 0; i < posData->count; i++)
		{
			rage::Vector3& vec = vecs[i];
			minX = rage::Math::Min(minX, vec.X);
			minY = rage::Math::Min(minY, vec.Y);
			minZ = rage::Math::Min(minZ, vec.Z);
			maxX = rage::Math::Max(maxX, vec.X);
			maxY = rage::Math::Max(maxY, vec.Y);
			maxZ = rage::Math::Max(maxZ, vec.Z);
		}
	}

	m_BoundingBox = rage::spdAABB(rage::Vec3V(minX, minY, minZ), rage::Vec3V(maxX, maxY, maxZ));
}

rageam::graphics::SceneGeometryGl::SceneGeometryGl(SceneMeshGl* parent, u16 index, cgltf_primitive* primitive, const cgltf_data* data)
	: SceneGeometry(parent, index)
{
	m_Primitive = primitive;

	if (primitive->material)
		m_MaterialIndex = std::distance(data->materials, primitive->material);
	else
		m_MaterialIndex = DEFAULT_MATERIAL;

	ComputeBB();
}

u32 rageam::graphics::SceneGeometryGl::GetIndexCount() const
{
	return static_cast<u32>(m_Primitive->indices->count);
}

u32 rageam::graphics::SceneGeometryGl::GetVertexCount() const
{
	return static_cast<u32>(m_Primitive->attributes[0].data->count);
}

u16 rageam::graphics::SceneGeometryGl::GetMaterialIndex() const
{
	return m_MaterialIndex;
}

void rageam::graphics::SceneGeometryGl::GetIndices(SceneData& data) const
{
	cgltf_accessor* indices = m_Primitive->indices;

	char* indexBuffer =
		(char*)indices->buffer_view->buffer->data + indices->buffer_view->offset;

	data.Buffer = indexBuffer;
	data.Format = GlTypeToDxgiFormat(indices->type, indices->component_type);
}

bool rageam::graphics::SceneGeometryGl::GetAttribute(SceneData& data, VertexSemantic semantic, u32 semanticIndex) const
{
	int attributeIndex = -1;
	for (int i = 0; i < static_cast<int>(m_Primitive->attributes_count); i++)
	{
		cgltf_attribute& attribute = m_Primitive->attributes[i];
		VertexSemantic attributeSemantic = GlSemanticToAm[attribute.type];

		// Semantic name (TexCoord) must match with semantic index (TexCoord0, Texcoord1 - (1; 2) etc)
		if (attributeSemantic == semantic && static_cast<u32>(attribute.index) == semanticIndex)
		{
			attributeIndex = i;
			break;
		}
	}

	if (attributeIndex == -1)
		return false;

	cgltf_attribute& attribute = m_Primitive->attributes[attributeIndex];
	cgltf_accessor* attributeData = attribute.data;

	char* attributeBuffer =
		(char*)attributeData->buffer_view->buffer->data + attributeData->buffer_view->offset;

	data.Buffer = attributeBuffer;
	data.Format = GlTypeToDxgiFormat(attributeData->type, attributeData->component_type);

	return true;
}

rageam::graphics::SceneMeshGl::SceneMeshGl(Scene* scene, SceneNode* parent, cgltf_mesh* mesh, const cgltf_data* data) : SceneMesh(scene, parent)
{
	m_Geometries.Reserve(static_cast<u16>(mesh->primitives_count));
	for (cgltf_size i = 0; i < mesh->primitives_count; i++)
	{
		cgltf_primitive& primitive = mesh->primitives[i];

		SceneGeometryGl* geometry = new SceneGeometryGl(this, i, &primitive, data);
		m_Geometries.Construct(geometry);
	}
}

u16 rageam::graphics::SceneMeshGl::GetGeometriesCount() const
{
	return m_Geometries.GetSize();
}

rageam::graphics::SceneGeometry* rageam::graphics::SceneMeshGl::GetGeometry(u16 index) const
{
	return m_Geometries[index].get();
}

rageam::graphics::SceneNodeGl::SceneNodeGl(SceneGl* scene, u16 index, SceneNodeGl* parent, cgltf_node* glNode, const cgltf_data* glData)
	: SceneNode(scene, parent, index)
{
	m_Node = glNode;
	m_Parent = parent;

	m_Name = glNode->name;

	if (glNode->mesh)
		m_Mesh = std::make_unique<SceneMeshGl>(scene, this, glNode->mesh, glData);

	if (glNode->skin)
	{
		//cgltf_size jointCount = glNode->skin->joints_count;
		//cgltf_buffer_view* bufferView = m_Node->skin->inverse_bind_matrices->buffer_view;
		//auto matrices = (rage::Mat44V*)((char*)bufferView->buffer->data + bufferView->offset);

		//m_InverseBindMatrices.Reserve(jointCount);
		//for (cgltf_size i = 0; i < jointCount; i++)
		//	m_InverseBindMatrices.Add(matrices[i]);
	}
}

u16 rageam::graphics::SceneNodeGl::GetBoneCount() const
{
	cgltf_skin* skinGl = m_Node->skin;
	if (skinGl)
		return skinGl->joints_count;

	return 0;
}

rageam::graphics::SceneNode* rageam::graphics::SceneNodeGl::GetBone(u16 index)
{
	AM_ASSERT(index < GetBoneCount(), "SceneNodeGl::GetBone() -> Index '%u' is out of bounds.", index);
	SceneGl* sceneGl = (SceneGl*)m_Scene;
	return sceneGl->GetNodeFromGl(m_Node->skin->joints[index]);
}
//
//const rage::Mat44V& rageam::graphics::SceneNodeGl::GetInverseBoneMatrix(u16 index)
//{
//	return m_InverseBindMatrices[index];
//}

const rage::Mat44V& rageam::graphics::SceneNodeGl::GetWorldBoneTransform(u16 boneIndex)
{
	// https://stackoverflow.com/questions/64745393/gltf-are-bone-matrices-specified-in-local-or-model-space
	// jointMatrix(j) =
	//	globalTransformOfNodeThatTheMeshIsAttachedTo ^ -1 *
	//	globalTransformOfJointNode(j) *
	//	inverseBindMatrixForJoint(j);

	// Compute 
	/*for(u16 i = 0; i < index; i++)
	{

	}*/

	/*rage::Mat44V nodeMatrix = m_LocalMatrix;
	if (m_Parent)
	{

	}*/

	//DirectX::XMMATRIX inverseBind = m_InverseBindMatrices[boneIndex];
	/*DirectX::XMMATRIX nodeForm = GetBone(boneIndex)->GetWorldTransform();
	DirectX::XMMATRIX boneForm = XMMatrixMultiply(inverseBind, nodeForm);*/

	//rage::Vec4V scale, rot, trans;
	//XMMatrixDecompose(&scale.M, &rot.M, &trans.M, boneForm);

	//DirectX::XMVECTOR scale, rot, trans;
	//XMMatrixDecompose(&scale, &rot, &trans, m_InverseBindMatrices[boneIndex]);


	// XMMatrixInverse(NULL, m_InverseBindMatrices[boneIndex]);
	//DirectX::XMMATRIX yzSwap = DirectX::XMMatrixSet
	//(
	//	1, 0, 0, 0,
	//	0, 0, 1, 0,
	//	0, -1, 0, 0,
	//	0, 0, 0, 1
	//);


	DirectX::XMMATRIX boneMatrix = GetBone(boneIndex)->GetLocalTransform();
//	DirectX::XMMATRIX boneMatrix = GetBone(boneIndex)->GetWorldTransform();


	//boneMatrix.r[0] = rage::Vec3V(boneMatrix.r[0]).XZY();
	//boneMatrix.r[1] = rage::Vec3V(boneMatrix.r[1]).XZY();
	//boneMatrix.r[2] = rage::Vec3V(boneMatrix.r[2]).XZY();
	//boneMatrix.r[3] = rage::Vec3V(boneMatrix.r[3]).XZY();

	//rage::Vec4V scale, rot, trans;
	//XMMatrixDecompose(&scale.M, &rot.M, &trans.M, boneMatrix);

	//rage::QuatV			m_DefaultRotation;
	//rage::Vec3V			m_DefaultTranslation;
	//rage::Vec3V			m_DefaultScale;
	//rage::Mat44V(boneMatrix).Decompose(&m_DefaultTranslation, &m_DefaultScale, &m_DefaultRotation);

	//return {};


	//return rage::Mat44V::Identity();

	return rage::Mat44V(boneMatrix);
}

const rage::spdAABB& rageam::graphics::SceneGeometryGl::GetAABB() const
{
	return m_BoundingBox;
}

cgltf_size rageam::graphics::SceneGl::GetGlNodeIndex(cgltf_node* glNode) const
{
	return std::distance(m_Data->nodes, glNode);
}

bool rageam::graphics::SceneGl::LoadGl(ConstWString path)
{
	file::FileBytes file;
	ReadAllBytes(path, file);

	cgltf_options options{};
	cgltf_result result = cgltf_parse(&options, file.Data.get(), file.Size, &m_Data);
	if (!AM_VERIFY(VerifyResult(result),
		L"SceneGl::LoadGl() -> Failed to load model at path %ls, error: %hs", path, GetResultString(result)))
	{
		return false;
	}

	cgltf_load_buffers(&options, m_Data, NULL);
	return true;
}

rageam::graphics::SceneNodeGl* rageam::graphics::SceneGl::AddNodesRecursive(cgltf_node** glNodes, cgltf_size nodeCount, SceneNodeGl* parent)
{
	SceneNodeGl* previousNode = nullptr;
	SceneNodeGl* firstNode = nullptr;
	for (cgltf_size i = 0; i < nodeCount; i++)
	{
		cgltf_node* glNode = glNodes[i];
		cgltf_size glNodeIndex = GetGlNodeIndex(glNode);

		u16 nodeIndex = m_Nodes.GetSize();
		SceneNodeGl* node = new SceneNodeGl(this, nodeIndex, parent, glNode, m_Data);
		m_Nodes.Construct(node);
		m_NodeGlToSceneNode[glNodeIndex] = nodeIndex;

		node->m_FirstChild = AddNodesRecursive(glNode->children, glNode->children_count, node);
		// Set next sibling as current node to previous node
		if (previousNode)
			previousNode->m_NextSibling = node;

		previousNode = node;
		if (!firstNode) firstNode = node;
	}
	return firstNode;
}

void rageam::graphics::SceneGl::ConstructScene()
{
	cgltf_scene* scene = m_Data->scene;

	// Note that we aren't using scene->node_count because it will give us node count
	// on the first layer of depth, but we need total num of nodes
	m_Nodes.Reserve(m_Data->nodes_count);
	m_NodeGlToSceneNode.Resize(m_Data->nodes_count);

	// Convert node hierarchy
	m_FirstNode = AddNodesRecursive(scene->nodes, scene->nodes_count);

	// Convert materials
	m_Materials.Reserve(static_cast<u16>(m_Data->materials_count));
	for (cgltf_size i = 0; i < m_Data->materials_count; i++)
	{
		cgltf_material* cmat = &m_Data->materials[i];
		SceneMaterialGl* mat = new SceneMaterialGl(this, static_cast<u16>(i), cmat);
		m_Materials.Construct(mat);
	}
}

rageam::graphics::SceneGl::~SceneGl()
{
	cgltf_free(m_Data);
	m_Data = nullptr;
}

bool rageam::graphics::SceneGl::Load(ConstWString path)
{
	if (!LoadGl(path))
		return false;

	ConstructScene();
	return true;
}

u16 rageam::graphics::SceneGl::GetNodeCount() const
{
	return m_Nodes.GetSize();
}

rageam::graphics::SceneNode* rageam::graphics::SceneGl::GetNode(u16 index) const
{
	return m_Nodes[index].get();
}

u16 rageam::graphics::SceneGl::GetMaterialCount() const
{
	return m_Materials.GetSize();
}

rageam::graphics::SceneMaterial* rageam::graphics::SceneGl::GetMaterial(u16 index) const
{
	if (index == u16(-1))
		return nullptr;
	return m_Materials[index].get();
}

rageam::graphics::SceneNode* rageam::graphics::SceneGl::GetNodeFromGl(cgltf_node* glNode) const
{
	u16 nodeIndex = m_NodeGlToSceneNode[GetGlNodeIndex(glNode)];
	return GetNode(nodeIndex);
}
