//
// File: buffereditor.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "DirectXTex.h"
#include "scene.h"
#include "vertexdeclaration.h"
#include "am/system/enum.h"
#include "rage/grcore/fvf.h"

namespace rageam::graphics
{
	class VertexBufferEditor
	{
		using CharArray = char*;

		VertexDeclaration	m_Decl;
		u32					m_VertexCount = 0;
		CharArray			m_Buffer = nullptr;
		bool				m_OwnBufferMemory = false;
		u8					m_Semantics[MAX_SEMANTIC]{}; // To check if attribute was set

		void AllocateBuffer(u32 vertexCount);
		void SetBuffer(char* buffer, u32 vertexCount);

		// Marks semantic as 'set' (meaning initialized & value was set)
		void SetSemantic(VertexSemantic semantic, u32 semanticIndex)
		{
			u8 bit = 1 << static_cast<u8>(semantic);
			m_Semantics[semanticIndex] |= bit;
		}

		// For matching data formats
		void SetData(u32 vertexIndex, const VertexAttribute* attribute, DXGI_FORMAT format, pVoid in) const;

		// 'toFormat' is the format of color attribute in this vertex buffer,
		// 'inFormat' is format of input color that we have to convert to 'toFormat' and set
		void ConvertAndSetColor(u32 vertexIndex, const VertexAttribute* attribute, DXGI_FORMAT toFormat, DXGI_FORMAT inFormat, pVoid in) const
		{
			if (inFormat == toFormat)
			{
				SetData(vertexIndex, attribute, inFormat, in);
				return;
			}

			// float[4] to u32
			if (inFormat == DXGI_FORMAT_R32G32B32A32_FLOAT && toFormat == DXGI_FORMAT_R8G8B8A8_UNORM)
			{
				rage::Vector4 in_Vec4 = *static_cast<rage::Vector4*>(in);
				u32 out_u32 =
					(u32)(in_Vec4.X * 255) << 0 |
					(u32)(in_Vec4.Y * 255) << 8 |
					(u32)(in_Vec4.Z * 255) << 16 |
					(u32)(in_Vec4.W * 255) << 24;
				SetAttributeAt(vertexIndex, attribute->Offset, out_u32);
				return;
			}

			// u32 to float[4]
			if (inFormat == DXGI_FORMAT_R8G8B8A8_UNORM && toFormat == DXGI_FORMAT_R32G32B32A32_FLOAT)
			{
				u32 in_u32 = *static_cast<u32*>(in);
				rage::Vector4 out_Vec4 =
				{
					(float)(in_u32 >> 0 & 0xF) / 255.0f,
					(float)(in_u32 >> 8 & 0xF) / 255.0f,
					(float)(in_u32 >> 16 & 0xF) / 255.0f,
					(float)(in_u32 >> 24 & 0xF) / 255.0f,
				};
				SetAttributeAt(vertexIndex, attribute->Offset, out_Vec4);
				return;
			}

			// u16[4] to float[4]
			if (inFormat == DXGI_FORMAT_R16G16B16A16_UINT && toFormat == DXGI_FORMAT_R32G32B32A32_FLOAT)
			{
				u16* in_VecU16 = static_cast<u16*>(in);
				rage::Vector4 out_Vec4 =
				{
					((float)in_VecU16[0] / (float)UINT16_MAX),
					((float)in_VecU16[1] / (float)UINT16_MAX),
					((float)in_VecU16[2] / (float)UINT16_MAX),
					((float)in_VecU16[3] / (float)UINT16_MAX),
				};
				SetAttributeAt(vertexIndex, attribute->Offset, out_Vec4);
				return;
			}
		}

		void ConvertAndSetBlendWeights(u32 vertexIndex, const VertexAttribute* attribute, DXGI_FORMAT toFormat, DXGI_FORMAT inFormat, pVoid in) const
		{
			if (inFormat == toFormat)
			{
				SetData(vertexIndex, attribute, inFormat, in);
				return;
			}
		}

		void ConvertAndSetBlendIndices(u32 vertexIndex, const VertexAttribute* attribute, DXGI_FORMAT toFormat, DXGI_FORMAT inFormat, pVoid in) const
		{
			if (inFormat == toFormat)
			{
				SetData(vertexIndex, attribute, inFormat, in);
				return;
			}

			// See comment in CanConvertSetBlendIndices
			if (toFormat == DXGI_FORMAT_R32G32B32A32_FLOAT && inFormat == DXGI_FORMAT_R8G8B8A8_UINT)
			{
				u32 in_u32 = *static_cast<u32*>(in);
				rage::Vector4 out_Vec4 =
				{
					(float)(in_u32 >> 0 & 0xF) / 255.0f,
					(float)(in_u32 >> 8 & 0xF) / 255.0f,
					(float)(in_u32 >> 16 & 0xF) / 255.0f,
					(float)(in_u32 >> 24 & 0xF) / 255.0f,
				};
				SetAttributeAt(vertexIndex, attribute->Offset, out_Vec4);
				return;
			}
		}

		bool CanConvertColor(DXGI_FORMAT toFormat, DXGI_FORMAT inFormat) const;
		bool CanConvertSetBlendWeight(DXGI_FORMAT toFormat, DXGI_FORMAT inFormat) const;
		bool CanConvertSetBlendIndices(DXGI_FORMAT toFormat, DXGI_FORMAT inFormat) const;

		void Destroy();

	public:
		VertexBufferEditor(const VertexDeclaration& decl);
		VertexBufferEditor(const rage::grcVertexFormatInfo& vtxInfo);
		~VertexBufferEditor();

		// Computes total vertex buffer in bytes
		u64 ComputeBufferSize() const { return (u64)m_VertexCount * (u64)m_Decl.Stride; }

		// If buffer is not specified, allocates internal one
		void Init(u32 vertexCount, char* buffer = nullptr);

		// Gets pointer to internal vertex buffer
		void* GetBuffer() const { return m_Buffer; }

		// Returns buffer pointer and moves ownership to caller,
		// buffer editor is considered as destructed afterwards
		void* MoveBuffer()
		{
			AM_ASSERT(m_OwnBufferMemory, "VertexBufferEditor::MoveBuffer() -> Can't give ownership on not-owned buffer.");

			void* buffer = m_Buffer;
			m_OwnBufferMemory = false;
			Destroy();
			return buffer;
		}

		// Sets all existing attributes from given geometry except for blend indices because
		// we have to re-map the mto generated skeleton
		void SetFromGeometry(const SceneGeometry* geometry);

		// Computes attribute (element ) offset in given vertex within the vertex buffer
		u64 GetBufferOffset(u32 vertexIndex, u64 vertexOffset) const;

		// Gets whether given semantic was set using SetTexcoord/SetPosition etc functions
		bool IsSet(VertexSemantic semantic, u32 semanticIndex) const
		{
			u8 bit = 1 << static_cast<u8>(semantic);
			return m_Semantics[semanticIndex] & bit;
		}

		// Gets reference to attribute at given position
		template<typename T>
		T& Attribute(u32 vertexIndex, u64 vertexOffset) const
		{
			return *(T*)(m_Buffer + GetBufferOffset(vertexIndex, vertexOffset));
		}

		// Sets element value at given position
		template<typename T>
		void SetAttributeAt(u32 vertexIndex, u64 vertexOffset, const T& value) const
		{
			Attribute<T>(vertexIndex, vertexOffset) = value;
		}

		// Sets single texcoord coordinate for all vertices
		void SetTexcordSingle(u32 semanticIndex, const rage::Vector2& uv)
		{
			const VertexAttribute* attr = m_Decl.FindAttribute(TEXCOORD, semanticIndex);
			for (u32 i = 0; i < m_VertexCount; i++)
				SetAttributeAt(i, attr->Offset, uv);
			SetSemantic(TEXCOORD, semanticIndex);
		}

		// Unpacks texcoord coordinates from given array to TEXCOORD with specified semantic index
		void SetTexcords(u32 semanticIndex, const rage::Vector2* uvs)
		{
			const VertexAttribute* attr = m_Decl.FindAttribute(TEXCOORD, semanticIndex);
			for (u32 i = 0; i < m_VertexCount; i++)
				SetAttributeAt(i, attr->Offset, uvs[i]);
			SetSemantic(TEXCOORD, semanticIndex);
		}

		// Unpacks position coordinates from given array to POSITION0
		void SetPositions(const rage::Vector3* positions)
		{
			const VertexAttribute* attr = m_Decl.FindAttribute(POSITION, 0);
			for (u32 i = 0; i < m_VertexCount; i++)
				SetAttributeAt(i, attr->Offset, positions[i]);
			SetSemantic(POSITION, 0);
		}

		// Unpacks position coordinates from given vectorized array to POSITION0
		void SetPositions(const rage::Vec3V* positions)
		{
			const VertexAttribute* attr = m_Decl.FindAttribute(POSITION, 0);
			for (u32 i = 0; i < m_VertexCount; i++)
			{
				rage::Vector3 position = positions[i]; // Convert vectorized to scalar
				SetAttributeAt(i, attr->Offset, position);
			}
			SetSemantic(POSITION, 0);
		}

		// Unpacks normal coordinates from given array to NORMAL0
		void SetNormals(const rage::Vector3* normals)
		{
			const VertexAttribute* attr = m_Decl.FindAttribute(NORMAL, 0);
			for (u32 i = 0; i < m_VertexCount; i++)
				SetAttributeAt(i, attr->Offset, normals[i]);
			SetSemantic(NORMAL, 0);
		}

		// Unpacks normal coordinates from given vectorized array to NORMAL0
		void SetNormals(const rage::Vec3V* normals)
		{
			const VertexAttribute* attr = m_Decl.FindAttribute(NORMAL, 0);
			for (u32 i = 0; i < m_VertexCount; i++)
			{
				rage::Vector3 normal = normals[i]; // Convert vectorized to scalar
				SetAttributeAt(i, attr->Offset, normal);
			}
			SetSemantic(NORMAL, 0);
		}

		// Unpacks tangent coordinates from given array to TANGENT0
		void SetTangents(const rage::Vector4* tangents)
		{
			const VertexAttribute* attr = m_Decl.FindAttribute(TANGENT, 0);
			for (u32 i = 0; i < m_VertexCount; i++)
				SetAttributeAt(i, attr->Offset, tangents[i]);
			SetSemantic(TANGENT, 0);
		}

		// Sets single color for all vertices
		void SetColorSingle(u32 semanticIndex, u32 color);

		// Unpacks & converts colors to COLOR with specified semantic
		void SetColor(u32 semanticIndex, pVoid colors, DXGI_FORMAT inFormat);

		// Unpacks & converts skin blend weights to BLENDWEIGHT with semantic 0
		void SetBlendWeights(pVoid weights, DXGI_FORMAT inFormat);

		// Unpacks & converts skin blend indices to BLENDINDICES with semantic 0
		void SetBlendIndices(pVoid indices, DXGI_FORMAT inFormat);

		// Computes min & max values from POSITION0 coordinates
		void ComputeMinMax_Position(rage::Vec3V& min, rage::Vec3V& max) const
		{
			min = rage::S_MAX;
			max = rage::S_MIN;

			u32 offset = m_Decl.FindAttribute(POSITION, 0)->Offset;

			for (u32 i = 0; i < m_VertexCount; i++)
			{
				const rage::Vector3& pos = Attribute<rage::Vector3>(i, offset);

				rage::Vec3V posV(pos);
				min = posV.Min(min);
				max = posV.Max(max);
			}
		}
	};
}
