//
// File: meshsplitter.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/ptr.h"
#include "common/types.h"
#include "rage/atl/array.h"

namespace rageam::graphics
{
	struct MeshChunk
	{
		amUniquePtr<char>	Vertices;
		amUniquePtr<u16>	Indices;

		u32 VertexCount, IndexCount;
	};

	/**
	 * \brief Utility for splitting large meshes (65k+ indices) on chunks that can fit into 16 byte indices,
	 * this is required by rage vertex buffer as it uses UINT16_t as index type.
	 */
	class MeshSplitter
	{
	public:
		static rage::atArray<MeshChunk> Split(pVoid vertices, u32 vtxStride, const u32* indices, u32 idxCount)
		{
			static constexpr u16 MAX_INDEX = UINT16_MAX;

			u32 totalIndex = 0;

			rage::atArray<MeshChunk> chunks;
			while (totalIndex < idxCount)
			{
				HashSet<u16> oldIndexToNewIndex;

				// We can't use atArray so we have to simulate it's grow behaviour here
				u32 chunkVerticesCapacity = 4096; // Mesh with 32 bit indices will most likely have a lot of vertices
				u32 chunkVerticesCount = 0;
				char* chunkVertices = new char[static_cast<size_t>(chunkVerticesCapacity) * vtxStride];

				rage::atArray<u16, u32> chunkIndices;

				// Pack each chunk until we got to max index
				u16 chunkIndexCount = 0; // We use it as new index in current chunk
				u16 chunkIndex = 0;
				while (totalIndex < idxCount && chunkIndexCount < MAX_INDEX)
				{
					u32 oldIndex = indices[totalIndex];
					u16* existingIndex = oldIndexToNewIndex.TryGetAt(oldIndex);
					if (existingIndex)
					{
						// We already have this index vertex added, simply remap it to next index
						chunkIndices.Add(*existingIndex);
					}
					else
					{
						// We got new index unseen before, we have to add both vertex and index
						chunkIndices.Add(chunkIndex);
						oldIndexToNewIndex.InsertAt(oldIndex, chunkIndex);

						char* src = static_cast<char*>(vertices) + static_cast<size_t>(oldIndex) * vtxStride;
						char* dst = chunkVertices + static_cast<size_t>(chunkVerticesCount) * vtxStride;
						memcpy(dst, src, vtxStride);

						// Grow vertices buffer if out of space
						chunkVerticesCount++;
						if (chunkVerticesCount == chunkVerticesCapacity)
						{
							chunkVerticesCapacity *= 2;
							char* newChunkVertices = new char[static_cast<size_t>(chunkVerticesCapacity) * vtxStride];
							memcpy(newChunkVertices, chunkVertices, static_cast<size_t>(chunkVerticesCount) * vtxStride);
							delete[] chunkVertices;
							chunkVertices = newChunkVertices;
						}
						chunkIndex++;
					}

					chunkIndexCount++;
					totalIndex++;
				}

				MeshChunk chunk(
					amUniquePtr<char>(chunkVertices),
					amUniquePtr<u16>(chunkIndices.MoveBuffer()),
					chunkVerticesCount, chunkIndexCount);
				chunks.Emplace(std::move(chunk));

				//AM_DEBUGF("MeshSplitter::Split -> Packed chunk with %u vertices; %u indices; %u indices left",
				//	chunkVerticesCount, chunkIndexCount, idxCount - totalIndex);
			}

			return chunks;
		}
	};
}
