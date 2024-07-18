//
// File: drawabletofbx.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/types.h"
#include "am/graphics/color.h"
#include "drawablemap.h"

#include <fbxsdk.h>

// TODO:
// - R* don't create BVH manually, there is checkbox to export bounds as BVH
// - Supporting MAYA LOD Groups
// - Configurable LOD naming, for example unity use:
//		- ExampleMeshName_LOD0
//		- ExampleMeshName_LOD1
//		- ExampleMeshName_LOD2
//		- ExampleMeshName_LOD3
// - What will happen if we merge two geometries with different vertex layout...?
// - Export only skinned bones (Can we do it via some FBX flag...?)
// - Options to:
//		- Override format strings
//		- Flat collision hierarchy (remove composite bound nodes)
//		- No textures
//		- No root node
//		- Highest LOD (without .lod0)

// MAP? + Tune export!
// SceneMaterialToBounds

struct gtaMaterialId;

namespace rage
{
	class phBoundGeometry;
	class phBoundPrimitive;
	class crBoneData;
	class phBound;
	class phBoundBVH;
	class grmModel;
}

class gtaDrawable;

namespace rageam
{
	class DrawableToFBX
	{
		static constexpr int MAX_BONES = 128;

		struct BoundMaterial
		{
			FbxSurfaceMaterial* FbxMaterial;
			int					MaterialIndex; // Counters::MAL_Material
		};

		gtaDrawable*					m_Drawable = nullptr;
		FbxManager*						m_FbxManager = nullptr;
		FbxScene*						m_FbxScene = nullptr;
		FbxNode*						m_RootNode = nullptr;
		HashSet<FbxFileTexture*>		m_FbxTextures;	// Key is texture name hash
		List<FbxSurfaceMaterial*>		m_FbxMaterials;	// Directly maps to grmShaderGroup
		FixedArray<FbxNode*, MAX_BONES> m_FbxSkeletonNodes;
		HashSet<BoundMaterial>			m_ColMaterials; // Cache for collision materials to merge duplicates

		// Counters to maintain unique names in the scene (needed for tune file because it uses name hashes as key...)
		struct Counters
		{
			int Model;
			int Light;

			int COL_Composite;
			int COL_BVH;
			int COL_Geometry;
			int COL_Box;
			int COL_Sphere;
			int COL_Capsule;
			int COL_Cylinder;

			// Key	 - Surface Type
			// Value - Count
			HashSet<int> COL_Materials;

			// Counters for drawable map
			int MAP_COL;
			int MAP_Model[4];
			int MAP_Material;
			int MAP_Node;
		} m_Counters;

		// Reuse arrays to prevent unneeded reallocations when converting bounds
		List<Vec3V> m_TempVerts;
		List<int>   m_TempIndices;

		template<typename TGeometryElement, typename TArrayItem>
		void AppendFbxMeshElementData(u32 startIndex, u32 capacity, TGeometryElement* element, const List<TArrayItem>& array)
		{
			auto& directArray = element->GetDirectArray();
			if (directArray.GetCount() == 0) // Initialize
			{
				element->SetMappingMode(FbxLayerElement::eByControlPoint);
				element->SetReferenceMode(FbxGeometryElement::eDirect);
				directArray.SetCount(capacity, eUninitialized);
			}
			void* data = directArray.GetLocked(FbxLayerElementArray::eReadWriteLock);
			memcpy(
				(char*) data + sizeof(TArrayItem) * startIndex, 
				array.GetItems(), 
				sizeof(TArrayItem) * array.GetSize());
			directArray.Release(&data);
		}

		// NOTE: This increments node counter in the scene, nodes must be created in the same order as in the final scene
		FbxNode* CreateFbxNode(ConstString name);
		int      GetCurrentNodeIndex() const { return m_Counters.MAP_Node - 1; }
		// Same as for fbx node
		FbxSurfacePhong* CreateFbxMaterial(FbxObject* container, ConstString name);
		int				 GetCurrentMaterialIndex() const { return m_Counters.MAP_Material - 1; }

		FbxMesh* GetFbxMeshFromFbxNode(FbxNode* node) const;
		FbxNode* CreateFbxNodeFromMeshData(ConstString name, const List<Vec3V>& vertices, const List<int>& indices);
		void     SetFbxNodeTransformFromBone(FbxNode* fbxNode, const rage::crBoneData* boneData) const;
		void     SetFbxMeshMaterialsPerPolygonVertex(FbxNode* fbxNode, const List<int>& materials) const;
		void     SetFbxMeshMaterialsPerPolygon(FbxNode* fbxNode, const List<int>& materials) const;
		void     SetFbxMeshMaterialSingle(FbxNode* fbxNode, int material) const;
		void     CreateAndExportTexturesFromShaderGroup(ConstWString outTexDirectory);
		void     CreateSkeletonAndBoneNodes();
		void     CreateMaterialsAndSetBasicTextures();
		int      CreateFbxMaterialForBound(FbxNode* node, const gtaMaterialId& gtaMaterial);
		FbxNode* ConvertModel(rage::grmModel& model);
		void     ConvertLodGroup();
		void     ConvertBounds();
		void     ConvertBoundRecurse(rage::phBound* bound, FbxNode* parentNode, const Mat44V& transform, int compositeIndex);
		FbxNode* ConvertBoundGeometry(const rage::phBoundGeometry* bound, const Mat44V& transform);
		FbxNode* ConvertBoundPrimitive(const rage::phBoundPrimitive* primitive, const Mat44V& transform);
		FbxNode* ConvertBoundBVH(const rage::phBoundBVH* boundBVH, const Mat44V& transform);
		void     ConvertLights();
		
	public:
		DrawableToFBX() { m_Counters = Counters(); }

		// Tex directory might be NULL if it's not needed to export textures
		// In this function, drawablePath is only used to set root node name if name wasn't specified in rmcDrawable::m_DebugName
		void Export(gtaDrawable* drawable, ConstWString drawablePath, ConstWString outFbxPath, ConstWString outTexDirectory);
		void Export(ConstWString drawablePath, ConstWString outFbxPath, ConstWString outTexDirectory);

		struct Options
		{
			// Merges physical material that have identical properties (not only ID)
			bool MergePhysicalMaterials = true;
			// Ignores all lods lower than specified one
			int  MaxLod = 3;
		} Options = {};

		asset::DrawableAssetMap Map;
	};
}
