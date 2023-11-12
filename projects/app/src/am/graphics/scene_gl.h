//
// File: scene_gl.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "cgltf.h"
#include "scene.h"
#include "am/file/fileutils.h"
#include "am/system/enum.h"
#include "am/system/ptr.h"
#include "rage/atl/array.h"

namespace rageam::graphics
{
	class SceneGeometryGl;
	class SceneNodeGl;
	class SceneMeshGl;
	class SceneGl;

	// cgltf_attribute_type -> VertexSemantic
	static inline VertexSemantic GlSemanticToAm[] =
	{
		INVALID_SEMANTIC,	// cgltf_attribute_type_invalid
		POSITION,
		NORMAL,
		TANGENT,
		TEXCOORD,
		COLOR,
		BLENDINDICES,
		BLENDWEIGHT,
		INVALID_SEMANTIC,	// cgltf_attribute_type_custom
	};

	inline rage::Vec3V GlVecToVec3V(const cgltf_float* vec)
	{
		return rage::Vec3V(vec[0], vec[1], vec[2]);
	}

	inline rage::QuatV GlQuatToQuatV(const cgltf_float* quat)
	{
		return rage::QuatV(quat[0], quat[1], quat[2], quat[3]);
	}

	// We choose DXGI format here because rage::grcFormat covers only small subset of it
	DXGI_FORMAT GlTypeToDxgiFormat(cgltf_type type, cgltf_component_type componentType);

	class SceneMaterialGl : public SceneMaterial
	{
		cgltf_material* m_Material;
		u32 m_NameHash;

		// Names of located textures

		ConstString m_DiffuseMap = nullptr;
		ConstString m_SpecularMap = nullptr;
		ConstString m_NormalMap = nullptr;

		ConstString GetGlTextureName(const cgltf_texture* texture) const { return texture->image->name; }

		void ScanTextures();

	public:
		SceneMaterialGl(SceneGl* parent, u16 index, cgltf_material* material);

		ConstString GetName() const override;
		u32 GetNameHash() const override;

		ConstString GetTextureName(eMaterialTexture texture) const override;
	};

	class SceneGeometryGl : public SceneGeometry
	{
		cgltf_primitive* m_Primitive;
		rage::spdAABB m_BoundingBox;
		u32 m_MaterialIndex;

		int FindGlAttributeIndex(cgltf_attribute_type type) const;
		void ComputeBB();

	public:
		SceneGeometryGl(SceneMeshGl* parent, u16 index, cgltf_primitive* primitive, const cgltf_data* data);

		u32 GetIndexCount() const override;
		u32 GetVertexCount() const override;

		u16 GetMaterialIndex() const override;

		void GetIndices(SceneData& data) const override;
		bool GetAttribute(SceneData& data, VertexSemantic semantic, u32 semanticIndex) const override;

		const rage::spdAABB& GetAABB() const override;
	};

	class SceneMeshGl : public SceneMesh
	{
		rage::atArray<amPtr<SceneGeometryGl>> m_Geometries;

	public:
		SceneMeshGl(Scene* scene, SceneNode* parent, const cgltf_mesh* mesh, const cgltf_data* data);

		u16 GetGeometriesCount() const override;
		SceneGeometry* GetGeometry(u16 index) const override;
	};

	class SceneLightGl : public SceneLight
	{
		ColorU32 m_Color;
		eSceneLightType m_Type;
		const cgltf_light* m_LightGl;

	public:
		SceneLightGl(Scene* scene, SceneNode* parent, const cgltf_light* light);

		ColorU32 GetColor() override { return m_Color; }
		eSceneLightType GetType() override { return m_Type; }
		float GetOuterConeAngle() override;
		float GetInnerConeAngle() override;
	};

	class SceneNodeGl : public SceneNode
	{
		friend class SceneGl;

		ConstString m_Name; // From m_Node->name, for easier debugging
		cgltf_node* m_Node;
		SceneNodeGl* m_Parent;
		amUniquePtr<SceneMeshGl> m_Mesh;
		amUniquePtr<SceneLightGl> m_Light;

		// Copied from skin->inverse_bind_matrices, gl ones are unaligned and cause access violation
		//rage::atArray<rage::Mat44V> m_InverseBindMatrices;

		//SceneNode* m_SkinRoot;	// Scene node that have this node as joint in skin
		//u16 m_SkinJointIndex;	// Joint index of this bone in m_SkinRoot joints

	public:
		SceneNodeGl(SceneGl* scene, u16 index, SceneNodeGl* parent, cgltf_node* glNode, const cgltf_data* glData);

		ConstString GetName() const override { return m_Name; }
		SceneMesh* GetMesh() const override { return m_Mesh.get(); }
		SceneLight* GetLight() const override { return m_Light.get(); }

		bool HasSkin() const override { return m_Node->skin != nullptr; }
		u16 GetBoneCount() const override;
		SceneNode* GetBone(u16 index) override;
		//const rage::Mat44V& GetInverseBoneMatrix(u16 index) override;
		const rage::Mat44V& GetWorldBoneTransform(u16 boneIndex) override;

		bool HasTranslation() const override { return m_Node->has_translation; }
		bool HasRotation() const override { return m_Node->has_rotation; }
		bool HasScale() const override { return m_Node->has_scale; }

		rage::Vec3V GetTranslation() const override { return GlVecToVec3V(m_Node->translation); }
		rage::QuatV GetRotation() const override { return GlQuatToQuatV(m_Node->rotation); }
		rage::Vec3V GetScale() const override { return GlVecToVec3V(m_Node->scale); }
	};

	class SceneGl : public Scene
	{
		// In case of .glb file must be loaded always because cgltf buffers point to it
		file::FileBytes m_FileData;

		cgltf_data* m_Data = nullptr;

		rage::atArray<u16> m_NodeGlToSceneNode; // Maps cgltf_node* index to SceneNode index
		rage::atArray<amUniquePtr<SceneNode>> m_Nodes;
		rage::atArray<amUniquePtr<SceneMaterialGl>> m_Materials;

		SceneNode* m_FirstNode = nullptr;

		cgltf_size GetGlNodeIndex(cgltf_node* glNode) const;

		bool VerifyResult(cgltf_result result) const { return result == cgltf_result_success; }
		ConstString GetResultString(cgltf_result result) const { return Enum::GetName(result); }

		bool TryLoadGl(const cgltf_options& options, ConstWString path);
		bool LoadGl(ConstWString path);

		// Returns the first added child, if any
		SceneNodeGl* AddNodesRecursive(cgltf_node** glNodes, cgltf_size nodeCount, SceneNodeGl* parent = nullptr);

		bool ConstructScene();

	public:
		~SceneGl() override;

		bool Load(ConstWString path, SceneLoadOptions& loadOptions) override;

		u16 GetNodeCount() const override;
		SceneNode* GetNode(u16 index) const override;
		SceneNode* GetFirstNode() const override { return m_FirstNode; }

		u16 GetMaterialCount() const override;
		SceneMaterial* GetMaterial(u16 index) const override;

		// Gets corresponding SceneNodeGl from gltf node
		SceneNode* GetNodeFromGl(cgltf_node* glNode) const;

		ConstString GetTypeName() override { return "GL"; }
	};
}
