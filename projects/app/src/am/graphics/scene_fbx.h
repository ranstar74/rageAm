//
// File: scene_fbx.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "scene.h"
#include <ufbx.h>

#include "rage/math/math.h"

namespace rageam::graphics
{
	class SceneGeometryFbx;
	class SceneNodeFbx;
	class SceneMeshFbx;
	class SceneFbx;

	// Scalar

	inline rage::Vector2 UVecToS(const ufbx_vec2& vec) { return rage::Vector2(vec.x, vec.y); }
	inline rage::Vector3 UVecToS(const ufbx_vec3& vec) { return rage::Vector3(vec.x, vec.y, vec.z); }
	inline rage::Vector4 UVecToS(const ufbx_vec4& vec) { return rage::Vector4(vec.x, vec.y, vec.z, vec.w); }

	// Vectorized

	inline rage::Vec3V UVecToV(const ufbx_vec3& vec) { return rage::Vec3V(vec.x, vec.y, vec.z); }
	inline rage::QuatV UQuatToV(const ufbx_quat& quat) { return rage::QuatV(quat.x, quat.y, quat.z, quat.w); }
	inline rage::Mat44V UTransformToMat44(const ufbx_transform& transform)
	{
		rage::Vec3V scale = UVecToV(transform.scale);
		rage::QuatV rotation = UQuatToV(transform.rotation);
		rage::Vec3V translation = UVecToV(transform.translation);
		return rage::Mat44V::Transform(scale, rotation, translation);
	}

	class SceneMaterialFbx : public SceneMaterial
	{
		ufbx_material* m_UMat;
		u32 m_NameHash;

		// Names of located textures

		file::Path m_DiffuseMap;
		file::Path m_SpecularMap;
		file::Path m_NormalMap;

		void ScanTextures();
	public:
		SceneMaterialFbx(Scene* parent, u16 index, ufbx_material* uMat);

		ConstString GetName() const override { return m_UMat->name.data; }
		u32 GetNameHash() const override { return m_NameHash; }

		ConstString GetTextureName(eMaterialTexture texture) const override;
	};

	class SceneGeometryFbx : public SceneGeometry
	{
		ufbx_mesh* m_UMesh;
		ufbx_mesh_material* m_UMeshMat;

		amUniquePtr<u32[]>				m_Indices32;
		amUniquePtr<u16[]>				m_Indices16;
		amUniquePtr<rage::Vector3[]>	m_Positions;
		amUniquePtr<rage::Vector3[]>	m_Normals;
		amUniquePtr<rage::Vector4[]>	m_Tangents[rage::TANGENT_MAX];
		amUniquePtr<rage::Vector2[]>	m_Texcoords[rage::TEXCOORD_MAX];
		amUniquePtr<rage::Vector4[]>	m_Colors[rage::COLOR_MAX];
		amUniquePtr<u32[]>				m_BlendIndices;
		amUniquePtr<rage::Vector4[]>	m_BlendWeights;
		u32								m_VertexCount;
		u32								m_IndexCount;
		u16								m_MaterialIndex;
		bool							m_UseShortIndices;
		rage::spdAABB					m_PositionBound;

		// Computes num vertices used by material
		u32 ComputeVertexCount() const;
		void TriangulateAndBuildAttributes();

	public:
		SceneGeometryFbx(SceneMesh* parent, u16 index, u16 materialIndex, ufbx_mesh* uMesh, ufbx_mesh_material* uMeshMat);

		u16 GetMaterialIndex() const override { return m_MaterialIndex; }
		u32 GetVertexCount() const override { return m_VertexCount; }
		u32 GetIndexCount() const override { return m_IndexCount; }

		void GetIndices(SceneData& data) const override;
		bool GetAttribute(SceneData& data, VertexSemantic semantic, u32 semanticIndex) const override;

		const rage::spdAABB& GetAABB() const override { return m_PositionBound; }
	};

	class SceneMeshFbx : public SceneMesh
	{
		List<amUniquePtr<SceneGeometryFbx>> m_Geometries;
		ufbx_mesh* m_UMesh;

		// FBX has no mesh primitives, everything stored directly in mesh
		// We have to split mesh by material manually
		void SplitMeshOnGeometries();

	public:
		SceneMeshFbx(Scene* scene, SceneNode* parent, ufbx_mesh* uMesh);

		u16 GetGeometriesCount() const override { return m_Geometries.GetSize(); }
		SceneGeometry* GetGeometry(u16 index) const override { return m_Geometries[index].get(); }
	};

	class SceneLightFbx : public SceneLight
	{
		ufbx_light* m_ULight;

		ColorU32 m_Color;

	public:
		SceneLightFbx(Scene* scene, SceneNode* parent, ufbx_light* uLight) : SceneLight(scene, parent)
		{
			m_ULight = uLight;
			m_Color = ColorU32::FromVec3(UVecToS(m_ULight->color));
		}

		ColorU32 GetColor() override { return m_Color; }
		eSceneLightType GetType() override;

		float GetOuterConeAngle() override { return rage::DegToRad(m_ULight->outer_angle); }
		float GetInnerConeAngle() override { return rage::DegToRad(m_ULight->inner_angle); }
	};

	class SceneNodeFbx : public SceneNode
	{
		friend class SceneFbx;

		ufbx_node* m_UNode;

		amUniquePtr<SceneMeshFbx>	m_Mesh;
		amUniquePtr<SceneLightFbx>	m_Light;
		//rage::Mat44V				m_LocalMatrix;
		//rage::Mat44V				m_WorldMatrix;
		rage::Vec3V					m_Translation;
		rage::Vec3V					m_Scale;
		rage::QuatV					m_Rotation;
		bool						m_HasScale;
		bool						m_HasRotation;
		bool						m_HasTranslation;

	public:
		SceneNodeFbx(Scene* scene, SceneNode* parent, u16 index, ufbx_node* uNode);

		ConstString GetName() const override { return m_UNode->name.data; }
		SceneMesh* GetMesh() const override { return m_Mesh.get(); }
		SceneLight* GetLight() const override { return m_Light.get(); }

		bool HasSkin() const override;
		u16 GetBoneCount() const override;
		SceneNode* GetBone(u16 index) override;
		const rage::Mat44V& GetWorldBoneTransform(u16 index) override;

		//bool HasLocalWorldMatrices() override { return true; }

		bool HasScale() const override { return m_HasScale; }
		bool HasRotation() const override { return m_HasRotation; }
		bool HasTranslation() const override { return m_HasTranslation; }

		rage::Vec3V GetScale() const override { return m_Scale; }
		rage::QuatV GetRotation() const override { return m_Rotation; }
		rage::Vec3V GetTranslation() const override { return m_Translation; }
	};

	class SceneFbx : public Scene
	{
		ufbx_scene* m_UScene = nullptr;

		List<amUniquePtr<SceneMaterialFbx>> m_Materials;
		List<amUniquePtr<SceneNodeFbx>> m_Nodes;
		SceneNodeFbx* m_FirstNode = nullptr;

		SceneNodeFbx* AddNodesRecurse(ufbx_node* uNode, SceneNodeFbx* parent);
		bool ConstructScene();

	public:
		SceneFbx() = default;
		~SceneFbx() override;

		bool Load(ConstWString path, SceneLoadOptions& loadOptions) override;

		u16 GetNodeCount() const override { return m_Nodes.GetSize(); }
		SceneNode* GetNode(u16 index) const override { return m_Nodes[index].get(); }
		SceneNode* GetFirstNode() const override { return m_FirstNode; }

		u16 GetMaterialCount() const override { return m_Materials.GetSize(); }
		SceneMaterial* GetMaterial(u16 index) const override { return m_Materials[index].get(); }

		ConstString GetTypeName() override;

		// UFBX specific

		SceneNodeFbx* GetNodeFromUNode(const ufbx_node* uNode) const;
	};
}
