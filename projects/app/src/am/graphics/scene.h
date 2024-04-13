//
// File: scene.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "vertexdeclaration.h"
#include "am/system/ptr.h"
#include "rage/atl/string.h"
#include "rage/math/mtxv.h"
#include "rage/math/quatv.h"
#include "rage/spd/aabb.h"
#include "color.h"

// TODO: Progress reporting

namespace rageam::graphics
{
	class SceneNode;
	class SceneGeometry;
	class SceneMesh;
	class Scene;

	// Used as placeholder for unsupported semantics in loader types
	static constexpr auto SCENE_INVALID_SEMANTIC = VertexSemantic(-1);

	// This really should not be used by model files! Reserved identifier.
	static constexpr ConstString SCENE_DEFAULT_MATERIAL_NAME = "AM_DEFAULT_MATERIAL";

	// Case-sensitive atStringHash
	inline HashValue SceneHashFn(ConstString str) { return rage::atStringHash(str, false); }

	/**
	 * \brief Packed vertex channel data, (for e.g. 'Position' or 'Normal') or vertex indices.
	 */
	struct SceneData
	{
		char* Buffer;
		DXGI_FORMAT Format;

		template<typename T>
		T* GetBufferAs() { return reinterpret_cast<T*>(Buffer); }
	};

	class SceneHandle
	{
	protected:
		u16 m_Index;

	public:
		SceneHandle(u16 index) { m_Index = index; }

		virtual ~SceneHandle() = default;

		u16 GetIndex() const { return m_Index; }
	};

	enum eMaterialTexture
	{ // We can extend this with PBR set when time comes...
		MT_Diffuse,
		MT_Specular,
		MT_Normal,

		MT_Unknown = -1,
	};

	/**
	 * \brief Material properties of scene geometry.
	 */
	class SceneMaterial : public SceneHandle
	{
		Scene* m_Scene;

	public:
		SceneMaterial(Scene* parent, u16 index) : SceneHandle(index)
		{
			m_Scene = parent;
		}

		virtual ConstString GetName() const = 0;
		virtual u32         GetNameHash() const = 0;

		virtual ConstString GetTextureName(eMaterialTexture texture) const = 0;

		virtual bool IsDefault() const { return false; }
	};
	struct SceneMaterialHashFn
	{
		u32 operator()(const SceneMaterial* mat) const { return SceneHashFn(mat->GetName()); }
	};

	/**
	 * \brief Dummy default material.
	 */
	class SceneMaterialDefault : public SceneMaterial
	{
	public:
		SceneMaterialDefault(Scene* parent, u16 index) : SceneMaterial(parent, index) { }

		ConstString GetName() const override { return "Default"; }
		u32         GetNameHash() const override { return Hash("Default"); }

		ConstString GetTextureName(eMaterialTexture texture) const override { return ""; }

		bool IsDefault() const override { return true; }
	};

	/**
	 * \brief Represents single geometry primitive of 3D model.
	 */
	class SceneGeometry : public SceneHandle
	{
		SceneMesh* m_Mesh;

	public:
		SceneGeometry(SceneMesh* parent, u16 index) : SceneHandle(index)
		{
			m_Mesh = parent;
		}

		// This always gives an VALID index!
		// For cases when there's no material specified in the model file, default material is created.
		virtual u16 GetMaterialIndex() const = 0;

		virtual u32 GetVertexCount() const = 0;
		virtual u32 GetIndexCount() const = 0;

		virtual void GetIndices(SceneData& data) const = 0;
		virtual bool GetAttribute(SceneData& data, VertexSemantic semantic, u32 semanticIndex) const = 0;

		virtual const rage::spdAABB& GetAABB() const = 0;

		bool HasSkin() const;
		SceneMesh* GetParentMesh() const { return m_Mesh; }
		SceneNode* GetParentNode() const;
	};

	/**
	 * \brief Represents a 3D model (mesh) in scene.
	 */
	class SceneMesh
	{
		Scene* m_Scene;
		SceneNode* m_Parent;

	public:
		SceneMesh(Scene* scene, SceneNode* parent)
		{
			m_Scene = scene;
			m_Parent = parent;
		}

		virtual ~SceneMesh() = default;

		// Number of geometric primitives this model is composed of,
		// usually primitives are split by material.
		virtual u16 GetGeometriesCount() const = 0;
		virtual SceneGeometry* GetGeometry(u16 index) const = 0;

		bool HasSkin() const;
		SceneNode* GetParentNode() const { return m_Parent; }
		Scene* GetScene() const { return m_Scene; }
	};

	enum eSceneLightType
	{
		SceneLight_Point,
		SceneLight_Spot,
	};

	/**
	 * \brief Represents a light (spot / directional etc) in the scene.
	 */
	class SceneLight
	{
		Scene* m_Scene;
		SceneNode* m_Parent;

	public:
		SceneLight(Scene* scene, SceneNode* parent)
		{
			m_Scene = scene;
			m_Parent = parent;
		}

		virtual ~SceneLight() = default;

		virtual ColorU32 GetColor() = 0;
		virtual eSceneLightType GetType() = 0;

		// -- Type: Spot --

		virtual float GetOuterConeAngle() = 0;
		virtual float GetInnerConeAngle() = 0;
	};

	/**
	 * \brief Represents single element (node) in scene. Node has name and optionally transform.
	 */
	class SceneNode : public SceneHandle
	{
		friend class Scene;

	protected:
		SceneNode* m_Parent;
		SceneNode* m_NextSibling;
		SceneNode* m_FirstChild;
		Scene* m_Scene;

		rage::Mat44V m_LocalMatrix;
		rage::Mat44V m_WorldMatrix;

	public:
		SceneNode(Scene* scene, SceneNode* parent, u16 index) : SceneHandle(index)
		{
			m_Scene = scene;
			m_Parent = parent;

			m_NextSibling = nullptr;
			m_FirstChild = nullptr;

			m_LocalMatrix = rage::Mat44V::Identity();
			m_WorldMatrix = rage::Mat44V::Identity();
		}

		virtual ConstString GetName() const = 0;
		virtual SceneMesh* GetMesh() const = 0;
		virtual SceneLight* GetLight() const = 0;

		bool HasLight() const { return GetLight() != nullptr; }

		virtual bool HasSkin() const = 0;
		// Number of skinned bones (or joints)
		virtual u16 GetBoneCount() const = 0;
		// Gets skinned bone node
		virtual SceneNode* GetBone(u16 index) = 0;
		//virtual const rage::Mat44V& GetInverseBoneMatrix(u16 index) = 0;
		virtual rage::Mat44V GetWorldBoneTransform(u16 index) = 0;

		bool HasMesh() const { return GetMesh() != nullptr; }

		// If false, local and world matrices will be computed automatically from GetScale, GetRotation and GetTranslation
		// TODO: Implement this
		// virtual bool HasLocalWorldMatrices() { return false; }

		// Called by the scene on initialization, don't use
		void ComputeMatrices();

		// So called 'XForm' in 3ds max, when model is exported without transformation applied
		// We have to create skeleton and bones for such models

		virtual bool HasTranslation() const = 0;
		virtual bool HasRotation() const = 0;
		virtual bool HasScale() const = 0;

		virtual rage::Vec3V GetTranslation() const = 0;
		virtual rage::QuatV GetRotation() const = 0;
		virtual rage::Vec3V GetScale() const = 0;

		// Translation + Rotation + Scale transformation matrix
		const rage::Mat44V& GetLocalTransform() const { return m_LocalMatrix; }
		// Combined local + parent node matrices into world transform matrix
		const rage::Mat44V& GetWorldTransform() const { return m_WorldMatrix; }

		bool HasWorldTransform() const
		{
			if (HasTransform())
				return true;

			SceneNode* parent = GetParent();
			return parent && parent->HasWorldTransform();
		}
		bool HasTransform() const { return HasTranslation() || HasRotation() || HasScale(); }
		bool HasTransformedChild() const;

		// Computes num of nodes on this level of depth starting from current node
		int GetNumSiblings() const;

		// Navigation

		SceneNode* GetParent() const { return m_Parent; }
		SceneNode* GetNextSibling() const { return m_NextSibling; }
		SceneNode* GetFirstChild() const { return m_FirstChild; }

		List<SceneNode*> GetAllChildren() const;
		List<SceneNode*> GetAllChildrenRecurse() const;
	};
	struct SceneNodeHashFn
	{
		u32 operator()(const SceneNode* node) const { return SceneHashFn(node->GetName()); }
	};

	struct SceneLoadOptions
	{
		// TODO: Not implemented in GLTF
		bool SkipMeshData = false; // No expensive vertex data will be loaded
	};

	/**
	 * \brief A Scene represents 3D model data loaded from file such as obj/fbx/glb.
	 */
	class Scene
	{
	protected:
		rage::atString			m_Name;
		bool					m_HasTransform;
		bool					m_NeedDefaultMaterial;
		bool					m_HasMultipleRootNodes;
		bool					m_HasSkinning;
		SmallList<SceneNode*>	m_LightNodes;
		SceneMaterial*			m_MaterialDefault = nullptr;

		HashSet<SceneNode*, SceneNodeHashFn>			m_NameToNode;
		HashSet<SceneMaterial*, SceneMaterialHashFn>	m_NameToMaterial;

		void FindSkinnedNodes();
		void FindTransformedModels();
		void ScanForMultipleRootBones();
		void ComputeNodeMatrices() const;

	public:
		Scene() = default;
		virtual ~Scene() = default;

		// Must be called after loading
		virtual void Init();
		// Loads model from file at given path
		virtual bool Load(ConstWString path, SceneLoadOptions& loadOptions) = 0;
		virtual bool ValidateScene() const;

		virtual u16 GetNodeCount() const = 0;
		virtual SceneNode* GetNode(u16 index) const = 0;
		// Returns the first node on depth 0, it still may have sibling nodes
		// Use GetFirstChild / GetNextSibling / GetParent to traverse the tree
		virtual SceneNode* GetFirstNode() const = 0;

		virtual u16 GetMaterialCount() const = 0;
		virtual SceneMaterial* GetMaterial(u16 index) const = 0;
		SceneMaterial* GetMaterial(const SceneGeometry* geometry) const
		{
			return GetMaterial(geometry->GetMaterialIndex());
		}
		// Might be NULL if default material is not used in the scene
		SceneMaterial* GetMaterialDefault() const { return m_MaterialDefault; }

		ConstString GetName() const { return m_Name; }
		void SetName(ConstString name) { m_Name = name; }

		// Whether there is at least one model with non-zero transform
		bool HasTransform() const { return m_HasTransform; }
		// Whether there is multiple nodes on root level
		bool HasMultipleRootNodes() const { return m_HasMultipleRootNodes; }
		// Whether any node in the scene use skinning
		bool HasSkinning() const { return m_HasSkinning; }
		// Light nodes count
		u16 GetLightCount() const { return m_LightNodes.GetSize(); }
		SceneNode* GetLight(u16 index) const { return m_LightNodes[index]; }

		// NOTE: Lookup is case-sensitive!
		SceneNode* GetNodeByName(ConstString name) const;
		SceneMaterial* GetMaterialByName(ConstString name) const;

		// Gets scene type such as 'GL' / 'FBX' / 'OBJ'
		virtual ConstString GetTypeName() = 0;

		// Prints scene hierarchy to console
		void DebugPrint();
	};
	using ScenePtr = amPtr<Scene>;

	class SceneFactory
	{
	public:
		static amPtr<Scene> LoadFrom(ConstWString path, SceneLoadOptions* options = nullptr);
		static bool IsSupportedFormat(ConstWString extension);
	};
}
