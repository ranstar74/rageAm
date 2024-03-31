#include "optimizedbvh.h"

#include "common/logger.h"
#include "rage/atl/string.h"

int rage::phOptimizedBvh::CalculateSplittingIndex(int startIndex, int endIndex, int maxPrimitivesPerNode) const
{
	// One option for picking our initial splitting index is simply to chop it in half (splitting at the median)
	int splitIndex = (startIndex + endIndex) / 2;

	if (endIndex - splitIndex > splitIndex - startIndex)
	{
		// The second half is larger than the first half
		int oldFirstHalfSize = splitIndex - startIndex;
		int secondHalfSizeAdjustment = (maxPrimitivesPerNode - (oldFirstHalfSize % maxPrimitivesPerNode)) % maxPrimitivesPerNode;
		splitIndex += secondHalfSizeAdjustment;
	}
	else
	{
		// The first half is no smaller than the second half
		int oldSecondHalfSize = endIndex - splitIndex;
		int firstHalfSizeAdjustment = (maxPrimitivesPerNode - (oldSecondHalfSize % maxPrimitivesPerNode)) % maxPrimitivesPerNode;
		splitIndex -= firstHalfSizeAdjustment;
	}

	return splitIndex;
}

int rage::phOptimizedBvh::CalculateSplittingAxis(phBvhPrimitiveData* primitives, int startIndex, int endIndex, int& middleAxis) const
{
	Vec3V   sum;
	Vec3V   sumSquares;
	ScalarV numIndices;

	for (int i = startIndex; i < endIndex; i++)
	{
		// This is slightly faster than UnQuantize because it doesn't add m_AABB center
		// We keep this for compatability with native implementation
		static auto fastUnQuantize = [](s16 in[3]) { return Vec3V(in[0], in[1], in[2]) / 256.0f; };

		spdAABB bb(fastUnQuantize(primitives[i].AABBMin), fastUnQuantize(primitives[i].AABBMax));
		Vec3V   center = bb.Center();

		sum += center;
		sumSquares += center * center;
		numIndices += S_ONE;
	}

	sum *= m_InvQuantize;
	sumSquares *= m_InvQuantize * m_InvQuantize;

	// Compute the variance as the "mean of the square minus the square of the mean"
	// This really just tells us 'how much' the coordinates on an axis are different from each other
	Vec3V scaledVariance = numIndices * sumSquares - sum * sum;

	// Pick the axis based on the variance
	u32 index = 0;
	if (scaledVariance.X() > scaledVariance.Y()) index |= 1 << 0;
	if (scaledVariance.Y() > scaledVariance.Z()) index |= 1 << 1;
	if (scaledVariance.Z() > scaledVariance.X()) index |= 1 << 2;
	constexpr int extremeIndices[] = { 0, 0, 1, 0, 2, 2, 1, 2 };
	int minAxis = extremeIndices[7 - index];
	int maxAxis = extremeIndices[index];
	middleAxis = 3 - minAxis - maxAxis;
	return maxAxis;
}

void rage::phOptimizedBvh::SortPrimitivesAlongAxes(phBvhPrimitiveData* primitives, int startIndex, int endIndex, int axis1, int axis2) const
{
	// Sort based on the centroid of the AABB
	std::sort(primitives + startIndex, primitives + endIndex, [axis1, axis2] (const phBvhPrimitiveData& lhs, const phBvhPrimitiveData& rhs)
		{
			s32 center1 = s32(lhs.AABBMin[axis1]) + s32(lhs.AABBMax[axis1]);
			s32 center2 = s32(rhs.AABBMin[axis1]) + s32(rhs.AABBMax[axis1]);
			s32 otherCenter1 = s32(lhs.AABBMin[axis2]) + s32(lhs.AABBMax[axis2]);
			s32 otherCenter2 = s32(rhs.AABBMin[axis2]) + s32(rhs.AABBMax[axis2]);
			return center1 == center2 ? otherCenter1 > otherCenter2 : center1 > center2;
		});
}

rage::phOptimizedBvhNode* rage::phOptimizedBvh::BuildTree(phBvhPrimitiveData* primitives, int startIndex, int endIndex, int maxPrimitivesPerNode, int lastSortAxis)
{
	AM_ASSERT(m_NumNodesInUse < m_NumNodes, "phOptimizedBvh::BuildTree() -> Not enough nodes to build the tree. Most likely number of primitives has grown since last time.");
	int currentIndex = m_NumNodesInUse++;
	phOptimizedBvhNode& node = m_ContiniousNodes[currentIndex];

	int primitiveCount = endIndex - startIndex;

	// We don't need to split any further, node can already fit all primitives
	if (primitiveCount <= maxPrimitivesPerNode)
	{
		// In original implementation this was a build parameter, but it is not possible to build a tree with multiple primitives per node
		// without reordering primitives (because of the way they're indexed in node, start + count)
		bool needRemapPrimitives = maxPrimitivesPerNode > 0;

		int primitiveIndex = needRemapPrimitives ? startIndex : primitives[startIndex].PrimitiveIndex;
		node.SetPrimitiveCount(primitiveCount);
		node.SetPrimitiveIndex(primitiveIndex);

		// Compute total AABB for all primitives
		phBvhPrimitiveData& firstPrimitive = primitives[startIndex];
		s16 minX = firstPrimitive.AABBMin[0];
		s16 minY = firstPrimitive.AABBMin[1];
		s16 minZ = firstPrimitive.AABBMin[2];
		s16 maxX = firstPrimitive.AABBMax[0];
		s16 maxY = firstPrimitive.AABBMax[1];
		s16 maxZ = firstPrimitive.AABBMax[2];
		for (int i = startIndex; i < endIndex; i++)
		{
			phBvhPrimitiveData& primitive = primitives[i];
			minX = Min(minX, primitive.AABBMin[0]);
			minY = Min(minY, primitive.AABBMin[1]);
			minZ = Min(minZ, primitive.AABBMin[2]);
			maxX = Max(maxX, primitive.AABBMax[0]);
			maxY = Max(maxY, primitive.AABBMax[1]);
			maxZ = Max(maxZ, primitive.AABBMax[2]);
		}
		node.AABBMin[0] = minX; node.AABBMin[1] = minY; node.AABBMin[2] = minZ;
		node.AABBMax[0] = maxX; node.AABBMax[1] = maxY; node.AABBMax[2] = maxZ;

		return &node;
	}

	// Calculate two axis we'll use to sort primitives along
	int secondAxis;
	int splitAxis = CalculateSplittingAxis(primitives, startIndex, endIndex, secondAxis);
	if (splitAxis != lastSortAxis)
	{
		SortPrimitivesAlongAxes(primitives, startIndex, endIndex, splitAxis, secondAxis);
	}

	int splitIndex = CalculateSplittingIndex(startIndex, endIndex, maxPrimitivesPerNode);

	// Build the sub-trees for our left and right children
	phOptimizedBvhNode* leftChild = BuildTree(primitives, startIndex, splitIndex, maxPrimitivesPerNode, splitAxis);
	phOptimizedBvhNode* rightChild = BuildTree(primitives, splitIndex, endIndex, maxPrimitivesPerNode, splitAxis);

	node.SetEscapeIndex(m_NumNodesInUse - currentIndex);

	constexpr int MAX_SUBTREE_SIZE = 128 * sizeof phOptimizedBvhNode;

	// The escape index tells us by how many nodes to skip ahead, thereby telling us the size of the subtree
	int leftSubTreeMaxCount = leftChild->GetEscapeIndex();
	int rightSubTreeMaxCount = rightChild->GetEscapeIndex();
	int leftSubTreeSize = leftSubTreeMaxCount * sizeof phOptimizedBvhNode;
	int rightSubTreeSize = rightSubTreeMaxCount * sizeof phOptimizedBvhNode;
	if (leftSubTreeSize + rightSubTreeSize >= MAX_SUBTREE_SIZE)
	{
		auto createSubtreeInfo = [this] (phOptimizedBvhNode& fromNode)
			{
				phOptimizedBvhSubtreeInfo& info = m_SubtreeHeaders[m_CurSubtreeHeaderIndex++];
				info.SetAABBFromNode(fromNode);
				info.RootNodeIndex = static_cast<u16>(std::distance(m_ContiniousNodes, &fromNode));
				info.EndIndex = info.RootNodeIndex + fromNode.GetEscapeIndex();
			};

		if (leftSubTreeSize <= MAX_SUBTREE_SIZE)
			createSubtreeInfo(*leftChild);

		if (rightSubTreeSize <= MAX_SUBTREE_SIZE)
			createSubtreeInfo(*rightChild);
	}

	node.CombineAABBs(*leftChild, *rightChild);

	return &node;
}

rage::phOptimizedBvh::phOptimizedBvh()
{
	m_ContiniousNodes = nullptr;
	m_SubtreeHeaders = nullptr;
	m_Pad = 0;
	m_NumNodes = 0;
	m_NumNodesInUse = 0;
	m_NumSubtreeHeaders = 0;
	m_CurSubtreeHeaderIndex = 0;
}

// ReSharper disable once CppPossiblyUninitializedMember
rage::phOptimizedBvh::phOptimizedBvh(const datResource& rsc)
{
	rsc.Fixup(m_ContiniousNodes);
	rsc.Fixup(m_SubtreeHeaders);
}

rage::phOptimizedBvh::~phOptimizedBvh()
{
	Destroy();
}

void rage::phOptimizedBvh::Destroy()
{
	delete m_ContiniousNodes;
	delete m_SubtreeHeaders;

	m_NumNodes = 0;
	m_NumNodesInUse = 0;
	m_NumSubtreeHeaders = 0;
	m_CurSubtreeHeaderIndex = 0;
	m_ContiniousNodes = nullptr;
	m_SubtreeHeaders = nullptr;
}

void rage::phOptimizedBvh::SetExtents(const spdAABB& bb)
{
	m_AABB = bb;
	m_AABBCenter = bb.Center();
	
	Vec3V size = bb.Max - bb.Min;
	m_Quantize = Vec3V(65534.0f) / size;
	m_InvQuantize = S_ONE / m_Quantize;
}

void rage::phOptimizedBvh::QuantizeMin(s16 out[3], const Vec3V& in) const
{
	Vec3V v = (in - m_AABBCenter) * m_Quantize;
	v.Min(Vec3V(32767.0f));
	v.Max(Vec3V(-32768.0f));
	out[0] = static_cast<s16>(floorf(v.X()));
	out[1] = static_cast<s16>(floorf(v.Y()));
	out[2] = static_cast<s16>(floorf(v.Z()));
}

void rage::phOptimizedBvh::QuantizeMax(s16 out[3], const Vec3V& in) const
{
	Vec3V v = (in - m_AABBCenter) * m_Quantize;
	v.Min(Vec3V(32767.0f));
	v.Max(Vec3V(-32768.0f));
	out[0] = static_cast<s16>(ceilf(v.X()));
	out[1] = static_cast<s16>(ceilf(v.Y()));
	out[2] = static_cast<s16>(ceilf(v.Z()));
}

void rage::phOptimizedBvh::QuantizeClosest(s16 out[3], const Vec3V& in) const
{
	Vec3V v = (in - m_AABBCenter) * m_Quantize;
	v.Min(Vec3V(32767.0f));
	v.Max(Vec3V(-32768.0f));
	out[0] = static_cast<s16>(round(v.X()));
	out[1] = static_cast<s16>(round(v.Y()));
	out[2] = static_cast<s16>(round(v.Z()));
}

rage::Vec3V rage::phOptimizedBvh::UnQuantize(const s16 in[3]) const
{
	return m_AABBCenter + Vec3V(in[0], in[1], in[2]) * m_InvQuantize;
}

void rage::phOptimizedBvh::BuildFromPrimitiveDataNoAllocate(phBvhPrimitiveData* primitives, int count, int maxPrimitivesPerNode)
{
	AM_ASSERT(IS_POWER_OF_TWO(maxPrimitivesPerNode), "phOptimizedBvh::BuildFromPrimitiveDataNoAllocate() -> Max primitive count must be power of two!");

	m_NumNodesInUse = 0;
	m_CurSubtreeHeaderIndex = 0;
	for (int i = 0; i < m_NumNodes; i++)
		m_ContiniousNodes[i].Reset();

	if (count == 0)
		return;

	BuildTree(primitives, 0, count, maxPrimitivesPerNode);

	// Tree was too small to generate any subtree headers
	if (m_CurSubtreeHeaderIndex == 0)
	{
		m_SubtreeHeaders[0].SetAABBFromNode(m_ContiniousNodes[0]);
		m_SubtreeHeaders[0].RootNodeIndex = 0;
		m_SubtreeHeaders[0].EndIndex = m_NumNodesInUse;
		m_CurSubtreeHeaderIndex = 1;
	}
}

void rage::phOptimizedBvh::BuildFromPrimitiveData(phBvhPrimitiveData* primitives, int count, int maxPrimitivesPerNode)
{
	Destroy();
	if (count == 0)
		return;

	// Approximate number of nodes to allocate, this is may produce 1-2 more nodes than actually needed
	m_NumNodes = 2 * count / maxPrimitivesPerNode + 1;
	m_NumSubtreeHeaders = Max(m_NumNodes / 2, 1);
	m_ContiniousNodes = new phOptimizedBvhNode[m_NumNodes];
	m_SubtreeHeaders = new phOptimizedBvhSubtreeInfo[m_NumSubtreeHeaders];

	BuildFromPrimitiveDataNoAllocate(primitives, count, maxPrimitivesPerNode);
}

rage::phOptimizedBvhNode& rage::phOptimizedBvh::GetNode(int index) const
{
	AM_ASSERTS(index >= 0 && index < m_NumNodesInUse);
	return m_ContiniousNodes[index];
}

rage::spdAABB rage::phOptimizedBvh::GetNodeAABB(const phOptimizedBvhNode& node) const
{
	return spdAABB(UnQuantize(node.AABBMin), UnQuantize(node.AABBMax));
}

void rage::phOptimizedBvh::IterateTree(const std::function<void(phOptimizedBvhNode& node, int depth)>& delegate) const
{
	if (m_NumNodesInUse == 0)
		return;

	int depth = 0;

	std::function<void(int start, int end)> iterateRecurse;
	iterateRecurse = [&](int start, int end)
		{
			// Until we reach the end of the tree (or sub-tree)
			while (start != end)
			{
				phOptimizedBvhNode& node = m_ContiniousNodes[start];
				delegate(node, depth);
				// Iterate subtree
				if (!node.IsLeafNode())
				{
					depth++;
					iterateRecurse(start + 1, start + node.GetEscapeIndex());
					depth--;

					// Move to the next node on this depth level
					start += node.GetEscapeIndex();
				}
				else
				{
					// Move to the next adjacent leaf
					start++;
				}
			}
		};
	iterateRecurse(0, m_NumNodesInUse);
}

int rage::phOptimizedBvh::ComputeDepth() const
{
	int maxDepth = 0;
	IterateTree([&maxDepth](phOptimizedBvhNode& node, int depth)
		{
			maxDepth = Max(maxDepth, depth);
		});
	return maxDepth;
}

void rage::phOptimizedBvh::PrintInfo()
{
	AM_TRACEF("== phOptimizedBVH: %p", this);
	AM_TRACEF("NumNodesInUse: %u", m_NumNodesInUse);
	AM_TRACEF("NumNodes: %u", m_NumNodes);
	AM_TRACEF("NumSubtreeHeaders: %u", m_NumSubtreeHeaders);
	AM_TRACEF("CurSubTreeHeaderIndex: %u", m_CurSubtreeHeaderIndex);
	AM_TRACEF("Depth: %u", ComputeDepth());
	AM_TRACEF("== Nodes:");
	int i = 0;
	atString indent;
	IterateTree([&](const phOptimizedBvhNode& node, int depth)
	{
		indent = "";
		for (int k = 0; k < depth; k++)
			indent += "    ";

		spdAABB bb = GetNodeAABB(node);
		AM_TRACEF("%s[NODE: %i]", indent.GetCStr(), i);
		AM_TRACEF("%sAABBMin: %.02f %.02f %.02f", indent.GetCStr(), bb.Min.X(), bb.Min.Y(), bb.Min.Z());
		AM_TRACEF("%sAABBMax: %.02f %.02f %.02f", indent.GetCStr(), bb.Max.X(), bb.Max.Y(), bb.Max.Z());
		if (node.IsLeafNode())
		{
			AM_TRACEF("%sPrimIndex: %u", indent.GetCStr(), node.GetPrimitiveIndex());
			AM_TRACEF("%sPrimCount: %u", indent.GetCStr(), node.GetPrimitiveCount());
		}
		else
		{
			AM_TRACEF("%sEscapeIndex: %u", indent.GetCStr(), node.GetEscapeIndex());
		}
		i++;
	});
}
