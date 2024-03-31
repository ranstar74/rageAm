// File: optimizedbvh.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/asserts.h"
#include "rage/paging/resource.h"
#include "rage/spd/aabb.h"
#include "common/types.h"

#include <functional>

namespace rage
{
	// NOTE: Native implementation uses term 'Polygon' for simplest shapes, we use 'Primitive' instead,
	// to prevent confusion with phPolygon

	/**
	 * \brief Used when building BVH tree.
	 */
	struct phBvhPrimitiveData
	{
		s16 AABBMin[3];
		s16 Centroid[3];
		u16 Pad;
		u16 PrimitiveIndex;
		s16 AABBMax[3];

		phBvhPrimitiveData& operator=(const phBvhPrimitiveData&) = default;
	};

	struct phOptimizedBvhNode
	{
		s16 AABBMin[3];
		s16 AABBMax[3];
		// Polygon start index if the node is a leaf node, and an escape index if it's not a leaf node
		u16 NodeData;
		u8  PrimitiveCount;

		// Whether this node is last one in tree and holds primitives
		bool IsLeafNode() const { return PrimitiveCount != 0; }
		int  GetPrimitiveIndex() const { AM_ASSERTS(IsLeafNode()); return NodeData; }
		int  GetPrimitiveCount() const { AM_ASSERTS(IsLeafNode()); return PrimitiveCount; }
		void SetPrimitiveCount(int primitiveCount) { PrimitiveCount = primitiveCount; }
		void SetPrimitiveIndex(int primitiveIndex) { AM_ASSERTS(IsLeafNode()); NodeData = primitiveIndex; }

		// Escape index is number of sub-nodes in tree, add escape index to index of current node
		// to get index of the next node on this depth level
		int  GetEscapeIndex() const { /*AM_ASSERTS(!IsLeafNode()); return NodeData;*/return IsLeafNode() ? 0 : NodeData; }
		void SetEscapeIndex(int escapeIndex) { AM_ASSERTS(!IsLeafNode()); NodeData = escapeIndex; }

		void CombineAABBs(const phOptimizedBvhNode& left, const phOptimizedBvhNode& right)
		{
			AABBMin[0] = Min(left.AABBMin[0], right.AABBMin[0]);
			AABBMin[1] = Min(left.AABBMin[1], right.AABBMin[1]);
			AABBMin[2] = Min(left.AABBMin[2], right.AABBMin[2]);
			AABBMax[0] = Max(left.AABBMax[0], right.AABBMax[0]);
			AABBMax[1] = Max(left.AABBMax[1], right.AABBMax[1]);
			AABBMax[2] = Max(left.AABBMax[2], right.AABBMax[2]);
		}

		void Reset()
		{
			SetPrimitiveCount(0);
			SetEscapeIndex(1); // Link to the next node
		}
	};

	struct phOptimizedBvhSubtreeInfo
	{
		s16 AABBMin[3];
		s16 AABBMax[3];
		u16 RootNodeIndex;
		u8  EndIndex;

		void SetAABBFromNode(const phOptimizedBvhNode& node)
		{
			memcpy(AABBMin, node.AABBMin, sizeof AABBMin);
			memcpy(AABBMax, node.AABBMax, sizeof AABBMax);
		}
	};

	/**
	 * \brief An Octree.
	 */
	class phOptimizedBvh
	{
		phOptimizedBvhNode*        m_ContiniousNodes;
		int                        m_NumNodesInUse; // Num valid nodes (Count)
		int                        m_NumNodes;		// Num allocated nodes (Capacity)
		int                        m_Pad;
		spdAABB                    m_AABB;
		Vec3V                      m_AABBCenter;
		Vec3V                      m_Quantize;
		Vec3V                      m_InvQuantize;
		phOptimizedBvhSubtreeInfo* m_SubtreeHeaders;
		u16                        m_NumSubtreeHeaders;
		u16                        m_CurSubtreeHeaderIndex;

		// Determines the axis along with the variance of the center points of the nodes is greatest
		int CalculateSplittingIndex(int startIndex, int endIndex, int maxPrimitivesPerNode) const;
		int CalculateSplittingAxis(phBvhPrimitiveData* primitives, int startIndex, int endIndex, int& middleAxis) const;
		void SortPrimitivesAlongAxes(phBvhPrimitiveData* primitives, int startIndex, int endIndex, int axis1, int axis2) const;
		phOptimizedBvhNode* BuildTree(phBvhPrimitiveData* primitives, int startIndex, int endIndex, int maxPrimitivesPerNode, int lastSortAxis = -1);

	public:
		phOptimizedBvh();
		phOptimizedBvh(const datResource& rsc);
		~phOptimizedBvh();

		void Destroy();

		// Extents must be set BEFORE building the tree
		void SetExtents(const spdAABB& bb);

		// After quantization we'll loose some precision. Those functions floor/ceil quantized values to make sure that AABB will never shrink-in
		void QuantizeMin(s16 out[3], const Vec3V& in) const;
		void QuantizeMax(s16 out[3], const Vec3V& in) const;
		void QuantizeClosest(s16 out[3], const Vec3V& in) const;
		Vec3V UnQuantize(const s16 in[3]) const;

		// NOTE: During tree build we have to sort primitives (in case if there are more than 1 primitive per node allowed)
		// because node stores only first index of the primitive and count.
		// You have to remap all primitives by hand after building the tree.

		// Used when BVH needs to be updated without re-allocating tree nodes (meaning number of elements didn't change)
		void BuildFromPrimitiveDataNoAllocate(phBvhPrimitiveData* primitives, int count, int maxPrimitivesPerNode);
		// Builds the tree from scratch, destroying existing one
		void BuildFromPrimitiveData(phBvhPrimitiveData* primitives, int count, int maxPrimitivesPerNode);

		int GetNodeCount() const { return m_NumNodesInUse; }
		phOptimizedBvhNode& GetNode(int index) const;
		spdAABB GetNodeAABB(const phOptimizedBvhNode& node) const;

		// Iterates all nodes as they're ordered in array (from first to end), but also keeps up depth level
		void IterateTree(const std::function<void (phOptimizedBvhNode& node, int depth)>& delegate) const;
		// How many levels are there in this tree
		int ComputeDepth() const;
		// Debug function to print octree to the console, use this to visually see how tree is structured
		void PrintInfo();
	};
}
