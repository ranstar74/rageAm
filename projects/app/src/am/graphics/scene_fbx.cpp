#include "scene_fbx.h"

#include "am/system/enum.h"
#include "helpers/bitarray.h"

void rageam::graphics::SceneMaterialFbx::ScanTextures()
{
	file::Path textureName;
	for (ufbx_material_texture& uMatTexture : m_UMat->textures)
	{
		textureName = uMatTexture.texture->filename.data;
		textureName = textureName.GetFileNameWithoutExtension();

		ImmutableString propertyName = uMatTexture.shader_prop.data;
		if (propertyName.Contains("diff", true))
			m_DiffuseMap = textureName;
		else if (propertyName.Contains("spec", true))
			m_SpecularMap = textureName;
		else if (propertyName.Contains("bump", true) || propertyName.Contains("spec", true))
			m_NormalMap = textureName;
	}
}

rageam::graphics::SceneMaterialFbx::SceneMaterialFbx(Scene* parent, u16 index, ufbx_material* uMat) : SceneMaterial(parent, index)
{
	m_UMat = uMat;
	m_NameHash = Hash(m_UMat->name.data);
	ScanTextures();
}

ConstString rageam::graphics::SceneMaterialFbx::GetTextureName(eMaterialTexture texture) const
{
	switch (texture)
	{
	case MT_Diffuse:	return m_DiffuseMap;
	case MT_Specular:	return m_SpecularMap;
	case MT_Normal:		return m_NormalMap;
	default:
		AM_UNREACHABLE("SceneMaterialFbx::GetTextureFileName() -> Type '%s' is not supported",
			Enum::GetName(texture));
	}
}

u32 rageam::graphics::SceneGeometryFbx::ComputeVertexCount() const
{
	// We don't know total vertex count yet but it will be less or equal to index count
	u32 indexCount = m_UMesh->num_triangles * 3;
	BitArray processedVerts;
	processedVerts.Allocate(indexCount);
	
	u32 totalNumVerts = 0;
	for (u32& faceIndex : m_UMeshMat->face_indices)
	{
		ufbx_face& face = m_UMesh->faces[faceIndex];
		for (u32 i = 0; i < face.num_indices; i++)
		{
			// Faces share same vertices with other faces, we need only unique vertices
			// For example a quad will have 4 vertices and two triangles, those two
			// triangles with share 2 vertices, giving us total 6 indices
			u32 vertIndex = m_UMesh->vertex_indices[face.index_begin + i];
			if (processedVerts.IsSet(vertIndex))
				continue;
			processedVerts.Set(vertIndex);
			totalNumVerts++;
		}
	}
	return totalNumVerts;
}

void rageam::graphics::SceneGeometryFbx::TriangulateAndBuildAttributes()
{
	ufbx_skin_deformer* skin = nullptr;
	if (m_UMesh->skin_deformers.count > 0) // Has skinning
	{
		skin = m_UMesh->skin_deformers.data[0];
		if (skin->max_weights_per_vertex > 4)
		{
			// TODO: This will crash because SceneMeshFbx will report that skinning is present
			AM_WARNINGF(
				"SceneGeometryFbx::TriangulateAndBuildAttributes() -> "
				"Given mesh uses more than 4 bone influences per vertex (%llu), skinning will be ignored.",
				skin->max_weights_per_vertex);
			skin = nullptr;
		}
	}

	m_PositionBound = { rage::S_MAX, rage::S_MIN };

	// In this geometry
	u32 localNumTris = m_UMeshMat->num_triangles;
	u32 localNumIndices = localNumTris * 3;
	u32 localNumVerts = ComputeVertexCount();

	// In whole mesh
	u32 totalNumVerts = m_UMesh->num_vertices;

	m_IndexCount = localNumIndices;
	m_VertexCount = localNumVerts;
	m_UseShortIndices = localNumIndices < UINT16_MAX;
	if (m_UseShortIndices)	m_Indices16 = amUniquePtr<u16[]>(new u16[localNumIndices]);
	else					m_Indices32 = amUniquePtr<u32[]>(new u32[localNumIndices]);

	// Allocate buffers for every geometry vertex attribute
	if (m_UMesh->vertex_position.exists) m_Positions = amUniquePtr<rage::Vector3[]>(new rage::Vector3[localNumVerts]);
	if (m_UMesh->vertex_normal.exists) m_Normals = amUniquePtr<rage::Vector3[]>(new rage::Vector3[localNumVerts]);
	for (ufbx_uv_set& uvSet : m_UMesh->uv_sets)
	{
		if (uvSet.index < rage::TEXCOORD_MAX)
		{
			if (uvSet.vertex_uv.exists) m_Texcoords[uvSet.index] = amUniquePtr<rage::Vector2[]>(new rage::Vector2[localNumVerts]);
		}
		if (uvSet.index < rage::TANGENT_MAX)
		{
			if (uvSet.vertex_tangent.exists) m_Tangents[uvSet.index] = amUniquePtr<rage::Vector4[]>(new rage::Vector4[localNumVerts]);
		}
	}
	for (ufbx_color_set& colorSet : m_UMesh->color_sets)
	{
		if (colorSet.index < rage::COLOR_MAX)
		{
			m_Colors[colorSet.index] = amUniquePtr<rage::Vector4[]>(new rage::Vector4[localNumVerts]);
		}
	}
	if (skin)
	{
		m_BlendIndices = amUniquePtr<u32[]>(new u32[localNumVerts]);
		m_BlendWeights = amUniquePtr<rage::Vector4[]>(new rage::Vector4[localNumVerts]);
	}

	// Mesh vertex index to index in m_Positions or any other attribute
	auto vertexToLocal = amUniquePtr<u32[]>(new u32[totalNumVerts]{}); // TODO: Can we share it between geometries during building?
	for (u32 i = 0; i < totalNumVerts; i++)
		vertexToLocal[i] = u32(-1);

	// Maximum number of triangles in a mesh face, for quad plane mesh with 1 face and 4 vertices it will be 2
	size_t maxFaceTris = m_UMesh->max_face_triangles;
	// Long enough to fit any triangulated face, for 2 triangles we need 2 * 3 = 6 indices
	size_t maxFaceTriIndices = maxFaceTris * 3;
	auto triIndices = amUniquePtr<u32[]>(new u32[maxFaceTriIndices]);

	// Triangulate every face in mesh
	u32 currentVertex = 0;
	u32 currentIndex = 0;
	for (u32& faceIndex : m_UMeshMat->face_indices)
	{
		ufbx_face& face = m_UMesh->faces[faceIndex];

		u32 numTris = ufbx_triangulate_face(triIndices.get(), maxFaceTriIndices, m_UMesh, face);
		u32 numVerts = numTris * 3;

		// Fill the attribute data from the vertex
		for (u32 i = 0; i < numVerts; i++)
		{
			u32 vertIndex = triIndices[i];					// Index of vertex index in m_UMesh->vertex_indices! A bit confusing
			u32 vert = m_UMesh->vertex_indices[vertIndex]; // Actual vertex index
			u32 localVert;

			bool isNewVertex = vertexToLocal[vert] == u32(-1);
			if (isNewVertex)
			{
				vertexToLocal[vert] = currentVertex;
				localVert = currentVertex;
			}
			else
			{
				// Retrieve unique local vertex index
				localVert = vertexToLocal[vert];
			}

			if (m_UseShortIndices)	m_Indices16[currentIndex] = static_cast<u16>(localVert);
			else					m_Indices32[currentIndex] = localVert;
			currentIndex++;

			// We add only unique vertices to buffer, indices are added regardless
			if (!isNewVertex)
				continue;

			// Copy vertex attributes

			// Position
			if (m_UMesh->vertex_position.exists)
			{
				rage::Vec3V pos = UVecToV(ufbx_get_vertex_vec3(&m_UMesh->vertex_position, vertIndex));
				// pos /= 100.0f; // Centimeters - Meters
				m_PositionBound = m_PositionBound.AddPoint(pos); // Extend bounding box to new position
				m_Positions[currentVertex] = pos;
			}

			// Normals
			if (m_UMesh->vertex_normal.exists)
			{
				m_Normals[currentVertex] = UVecToS(ufbx_get_vertex_vec3(&m_UMesh->vertex_normal, vertIndex));
			}

			// UV / Tangents
			for (ufbx_uv_set& uvSet : m_UMesh->uv_sets)
			{
				if (uvSet.index < rage::TEXCOORD_MAX)
				{
					if (uvSet.vertex_uv.exists) m_Texcoords[uvSet.index][currentVertex] = UVecToS(ufbx_get_vertex_vec2(&uvSet.vertex_uv, vertIndex));
				}

				if (uvSet.index < rage::TANGENT_MAX)
				{
					if (uvSet.vertex_tangent.exists)
					{
						rage::Vector3 tangent = UVecToS(ufbx_get_vertex_vec3(&uvSet.vertex_tangent, vertIndex));
						m_Tangents[uvSet.index][currentVertex] = rage::Vector4(tangent.X, tangent.Y, tangent.Z, 1.0f);
					}
				}
			}

			// Colors
			for (ufbx_color_set& colorSet : m_UMesh->color_sets)
			{
				if (colorSet.index < rage::COLOR_MAX)
					m_Colors[colorSet.index][currentVertex] = UVecToS(ufbx_get_vertex_vec4(&colorSet.vertex_color, vertIndex));
			}

			// Skinning
			if (skin)
			{
				ufbx_skin_vertex& skinVertex = skin->vertices[vert];

				u32 blendIndices = 0;
				rage::Vector4 blendWeights(0.0f);
				for (int k = 0; k < static_cast<int>(skinVertex.num_weights); k++)
				{
					u32 skinWeightIndex = skinVertex.weight_begin + k;
					ufbx_skin_weight& skinWeight = skin->weights[skinWeightIndex];
					blendWeights[k] = skinWeight.weight;
					blendIndices |= skinWeight.cluster_index << (k * 8);
				}
				m_BlendIndices[currentVertex] = blendIndices;
				m_BlendWeights[currentVertex] = blendWeights;
			}

			currentVertex++;
		}
	}
}

rageam::graphics::SceneGeometryFbx::SceneGeometryFbx(
	SceneMesh* parent, u16 index, u16 materialIndex, ufbx_mesh* uMesh, ufbx_mesh_material* uMeshMat)
	: SceneGeometry(parent, index)
{
	m_UMesh = uMesh;
	m_UMeshMat = uMeshMat;

	// We told ufbx to allow null material to make splitting geometries easier
	if (m_UMeshMat->material)
		m_MaterialIndex = materialIndex;
	else
		m_MaterialIndex = parent->GetScene()->GetMaterialDefault()->GetIndex();

	TriangulateAndBuildAttributes();
}

void rageam::graphics::SceneGeometryFbx::GetIndices(SceneData& data) const
{
	data.Buffer = m_UseShortIndices ?
		(char*)m_Indices16.get() :
		(char*)m_Indices32.get();
	data.Format = m_UseShortIndices ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
}

bool rageam::graphics::SceneGeometryFbx::GetAttribute(SceneData& data, VertexSemantic semantic, u32 semanticIndex) const
{
	data.Buffer = nullptr;

	switch (semantic)
	{
	case POSITION:
	{
		data.Buffer = (char*)m_Positions.get();
		data.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		break;
	}
	case NORMAL:
	{
		data.Buffer = (char*)m_Normals.get();
		data.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		break;
	}
	case TEXCOORD:
	{
		if (semanticIndex >= rage::TEXCOORD_MAX)
			return false;
		data.Buffer = (char*)m_Texcoords[semanticIndex].get();
		data.Format = DXGI_FORMAT_R32G32_FLOAT;
		break;
	}
	case TANGENT:
	{
		if (semanticIndex >= rage::TANGENT_MAX)
			return false;
		data.Buffer = (char*)m_Tangents[semanticIndex].get();
		data.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	}
	case COLOR:
	{
		if (semanticIndex >= rage::COLOR_MAX)
			return false;
		data.Buffer = (char*)m_Colors[semanticIndex].get();
		data.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	}
	case BLENDINDICES:
	{
		data.Buffer = (char*)m_BlendIndices.get();
		data.Format = DXGI_FORMAT_R8G8B8A8_UINT;
		break;
	}
	case BLENDWEIGHT:
	{
		data.Buffer = (char*)m_BlendWeights.get();
		data.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	}
	default: return false;
	}
	return data.Buffer != nullptr;
}

void rageam::graphics::SceneMeshFbx::SplitMeshOnGeometries()
{
	for (size_t i = 0; i < m_UMesh->materials.count; i++)
	{
		ufbx_mesh_material& meshMaterial = m_UMesh->materials[i];
		if (meshMaterial.num_faces == 0)
			continue;

		u16 geometryIndex = m_Geometries.GetSize();
		u16 materialIndex = static_cast<u16>(i);
		auto geometry = new SceneGeometryFbx(this, geometryIndex, materialIndex, m_UMesh, &meshMaterial);
		m_Geometries.Construct(geometry);
	}
}

rageam::graphics::SceneMeshFbx::SceneMeshFbx(Scene* scene, SceneNode* parent, ufbx_mesh* uMesh)
	: SceneMesh(scene, parent)
{
	m_UMesh = uMesh;
	SplitMeshOnGeometries();
}

rageam::graphics::eSceneLightType rageam::graphics::SceneLightFbx::GetType()
{
	switch (m_ULight->type)
	{
	case UFBX_LIGHT_SPOT:	return SceneLight_Spot;
	case UFBX_LIGHT_POINT:
	default:				return SceneLight_Point;
	}
}

rageam::graphics::SceneNodeFbx::SceneNodeFbx(Scene* scene, SceneNode* parent, u16 index, ufbx_node* uNode)
	: SceneNode(scene, parent, index)
{
	m_UNode = uNode;

	//m_LocalMatrix = UTransformToMat44(m_UNode->local_transform);
	m_WorldMatrix = UTransformToMat44(m_UNode->world_transform);

	m_Translation = UVecToV(m_UNode->local_transform.translation);
	m_Scale = UVecToV(m_UNode->local_transform.scale);
	m_Rotation = UQuatToV(m_UNode->local_transform.rotation);

	m_HasTranslation = !m_Translation.AlmostEqual(rage::S_ZERO);
	m_HasScale = !m_Scale.AlmostEqual(rage::S_ONE);
	m_HasRotation = !m_Rotation.AlmostEqual(rage::QUAT_IDENTITY);

	if (m_UNode->mesh)
		m_Mesh = std::make_unique<SceneMeshFbx>(scene, this, m_UNode->mesh);

	if (m_UNode->light)
		m_Light = std::make_unique<SceneLightFbx>(scene, this, m_UNode->light);

	m_NameHash = Hash(m_UNode->name.data);
}

bool rageam::graphics::SceneNodeFbx::HasSkin() const
{
	return m_UNode->mesh && m_UNode->mesh->skin_deformers.count > 0;
}

u16 rageam::graphics::SceneNodeFbx::GetBoneCount() const
{
	if (!HasSkin()) return 0;
	return m_UNode->mesh->skin_deformers.data[0]->clusters.count;
}

rageam::graphics::SceneNode* rageam::graphics::SceneNodeFbx::GetBone(u16 index)
{
	ufbx_skin_deformer* skin = m_UNode->mesh->skin_deformers.data[0];
	AM_ASSERT(index < skin->clusters.count, "SceneNodeFbx::GetBone() -> Index %u is out of bounds.", index);
	ufbx_node* boneNode = skin->clusters.data[index]->bone_node;
	return ((SceneFbx*)m_Scene)->GetNodeFromUNode(boneNode);
}

rage::Mat44V rageam::graphics::SceneNodeFbx::GetWorldBoneTransform(u16 index)
{
	return GetBone(index)->GetWorldTransform();
}

rageam::graphics::SceneNodeFbx* rageam::graphics::SceneFbx::AddNodesRecurse(ufbx_node* uNode, SceneNodeFbx* parent)
{
	// TODO: Logic is exactly the same to SceneGl::AddNodesRecurse, can we merge them somehow?
	SceneNodeFbx* previousNode = nullptr;
	SceneNodeFbx* firstNode = nullptr;
	for (size_t i = 0; i < uNode->children.count; i++)
	{
		ufbx_node* uChildNode = uNode->children[i];

		u16 nodeIndex = m_Nodes.GetSize();
		SceneNodeFbx* node = new SceneNodeFbx(this, parent, nodeIndex, uChildNode);
		m_Nodes.Construct(node);

		node->m_FirstChild = AddNodesRecurse(uChildNode, node);
		// Set next sibling as current node to previous node
		if (previousNode)
			previousNode->m_NextSibling = node;

		previousNode = node;
		if (!firstNode) firstNode = node;
	}
	return firstNode;
}

bool rageam::graphics::SceneFbx::ConstructScene()
{
	for (size_t i = 0; i < m_UScene->materials.count; i++)
	{
		ufbx_material* uMaterial = m_UScene->materials[i];
		SceneMaterialFbx* material = new SceneMaterialFbx(this, static_cast<u16>(i), uMaterial);
		m_Materials.Construct(material);
	}

	// Add default material if needed...
	for (size_t i = 0; i < m_UScene->meshes.count; i++)
	{
		ufbx_mesh* mesh = m_UScene->meshes[i];
		for (size_t j = 0; j < mesh->materials.count; j++)
		{
			if (!mesh->materials[j].material)
			{
				SceneMaterial* material = new SceneMaterialDefault(this, m_Materials.GetSize());
				m_Materials.Construct(material);
				m_MaterialDefault = material;

				goto outsideLoop; // Quick exit...
			}
		}
	}
outsideLoop:

	m_FirstNode = AddNodesRecurse(m_UScene->root_node, nullptr);

	return true;
}

rageam::graphics::SceneFbx::~SceneFbx()
{
	ufbx_free_scene(m_UScene);
}

bool rageam::graphics::SceneFbx::Load(ConstWString path, SceneLoadOptions& loadOptions)
{
	constexpr ufbx_coordinate_axes axes =
	{
		UFBX_COORDINATE_AXIS_POSITIVE_X, // Right
		UFBX_COORDINATE_AXIS_POSITIVE_Y, // Front
		UFBX_COORDINATE_AXIS_POSITIVE_Z, // Up
	};

	ufbx_load_opts opts = { 0 };
	opts.allow_null_material = true;
	opts.target_axes = axes;
	//opts.target_camera_axes = axes;
	opts.target_light_axes = axes;
	opts.ignore_all_content = loadOptions.SkipMeshData;

	ufbx_error error;
	m_UScene = ufbx_load_file(String::ToUtf8Temp(path), &opts, &error);
	if (!m_UScene)
	{
		AM_TRACEF("SceneFbx::Load() -> %s", error.description.data);
		return false;
	}

	if (!ConstructScene())
	{
		AM_TRACEF("SceneFbx::Load() -> Failed to construct scene.");
		return false;
	}

	return true;
}

ConstString rageam::graphics::SceneFbx::GetTypeName()
{
	if (m_UScene->metadata.file_format == UFBX_FILE_FORMAT_FBX)
		return "FBX";
	if (m_UScene->metadata.file_format == UFBX_FILE_FORMAT_OBJ)
		return "OBJ";
	return "UNKNOWN";
}

rageam::graphics::SceneNodeFbx* rageam::graphics::SceneFbx::GetNodeFromUNode(const ufbx_node* uNode) const
{
	// Simply do linear search, although we can use map...
	for (amUniquePtr<SceneNodeFbx>& node : m_Nodes)
	{
		if (node->m_UNode == uNode)
			return node.get();
	}
	AM_UNREACHABLE("SceneFbx::GetNodeFromUNode() -> Given node does not belong to this scene.");
}
