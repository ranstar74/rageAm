//
// File: meshgen.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/types.h"
#include "rage/math/mathv.h"
#include "unitshapes.h"

namespace rageam::graphics
{
#define MESH_ADD_INDEX(vi1, vi2, vi3) if (indices) { indices->Add(vi1); indices->Add(vi2); indices->Add(vi3); } MACRO_END
#define MESH_ADD_VERT(vert) verts.Add((vert).Transform(matrix))
#define MESH_ADD_VERT_MTX(vert, mtx) verts.Add((vert).Transform(mtx))
#define MESH_ARGS List<Vec3V>& verts, List<int>* indices, const Mat44V& matrix

	/**
	 * Utilities for generating triangle polygon mesh topology for primitives. Indices are optional.
	 * All generated indices are in counter clockwise order.
	 */
	class MeshGenerator
	{
	public:
		// In 4 vertices must describe two crossing diagonals
		static void FromOBB(const Vec3V& v0, const Vec3V& v1, const Vec3V& v2, const Vec3V& v3, MESH_ARGS)
		{
			// https://i.imgur.com/FmYrPUb.png
			// Indices are in format : FaceIndex | VertexIndex
			const Vec3V& v00 = v0;
			const Vec3V& v01 = v1;
			const Vec3V& v10 = v2;
			const Vec3V& v11 = v3;

			Vec3V center0 = (v00 + v01) * rage::S_HALF;
			Vec3V center1 = (v10 + v11) * rage::S_HALF;
			Vec3V normal0 = (center0 - center1).Normalized();
			Vec3V normal1 = -normal0;

			// Project vertices from opposite planes
			Vec3V v02 = ProjectOnPlane(v10, center0, normal0);
			Vec3V v03 = ProjectOnPlane(v11, center0, normal0);
			Vec3V v12 = ProjectOnPlane(v00, center1, normal1);
			Vec3V v13 = ProjectOnPlane(v01, center1, normal1);

			MESH_ADD_VERT(v00); MESH_ADD_VERT(v01); MESH_ADD_VERT(v02); MESH_ADD_VERT(v03);
			MESH_ADD_VERT(v10); MESH_ADD_VERT(v11); MESH_ADD_VERT(v12); MESH_ADD_VERT(v13);

			MESH_ADD_INDEX(0, 3, 2); MESH_ADD_INDEX(2, 3, 1);
			MESH_ADD_INDEX(5, 1, 3); MESH_ADD_INDEX(7, 1, 5);
			MESH_ADD_INDEX(6, 4, 7); MESH_ADD_INDEX(6, 7, 5);
			MESH_ADD_INDEX(0, 6, 5); MESH_ADD_INDEX(0, 5, 3);
			MESH_ADD_INDEX(2, 7, 4); MESH_ADD_INDEX(2, 1, 7);
			MESH_ADD_INDEX(6, 2, 4); MESH_ADD_INDEX(6, 0, 2);
		}

		static void FromAABB(const AABB& aabb, MESH_ARGS)
		{
			Vec3V ttl = aabb.TTL();
			Vec3V ttr = aabb.TTR();
			Vec3V tbl = aabb.TBL();
			Vec3V tbr = aabb.TBR();
			Vec3V btl = aabb.BTL();
			Vec3V btr = aabb.BTR();
			Vec3V bbl = aabb.BBL();
			Vec3V bbr = aabb.BBR();

			MESH_ADD_VERT(ttl); MESH_ADD_VERT(ttr); MESH_ADD_VERT(tbl); MESH_ADD_VERT(tbr);
			MESH_ADD_VERT(btl); MESH_ADD_VERT(btr); MESH_ADD_VERT(bbl); MESH_ADD_VERT(bbr);

			MESH_ADD_INDEX(1, 2, 0); MESH_ADD_INDEX(3, 2, 1);
			MESH_ADD_INDEX(6, 5, 4); MESH_ADD_INDEX(6, 7, 5);
			MESH_ADD_INDEX(0, 6, 4); MESH_ADD_INDEX(6, 0, 2);
			MESH_ADD_INDEX(4, 1, 0); MESH_ADD_INDEX(5, 1, 4);
			MESH_ADD_INDEX(5, 3, 1); MESH_ADD_INDEX(7, 3, 5);
			MESH_ADD_INDEX(7, 2, 3); MESH_ADD_INDEX(6, 2, 7);
		}

		static void FromSphere(const Sphere& sphere, MESH_ARGS)
		{
			Mat44V sphereMatrix = Mat44V::Transform(sphere.GetRadius(), rage::QUAT_IDENTITY, sphere.GetCenter()) * matrix;

			for (int i = 0; i < std::size(UNIT_SPHERE_VERTICES); i += 3)
			{
				MESH_ADD_VERT_MTX(Vec3V(UNIT_SPHERE_VERTICES[i], UNIT_SPHERE_VERTICES[i + 1], UNIT_SPHERE_VERTICES[i + 2]), sphereMatrix);
			}

			for (int i = 0; i < std::size(UNIT_SPHERE_INDICES); i += 3)
			{
				MESH_ADD_INDEX(UNIT_SPHERE_INDICES[i] - 1, UNIT_SPHERE_INDICES[i + 1] - 1, UNIT_SPHERE_INDICES[i + 2] - 1);
			}
		}

		static void FromCapsule(float halfExtent, float radius, bool yAxisUp, MESH_ARGS)
		{
			Mat44V capsuleMatrix = Mat44V::Identity();
			if (yAxisUp)
				capsuleMatrix *= Mat44V::LookAt(rage::VEC_ORIGIN, rage::VEC_RIGHT, rage::VEC_FRONT);
			capsuleMatrix *= matrix;

			Vec3V   offsetUpper = rage::VEC_UP * halfExtent;
			Vec3V   offsetLower = -offsetUpper;
			ScalarV scale = radius;
			for (int i = 0; i < std::size(UNIT_CAPSULE_VERTICES); i += 3)
			{
				bool upperHalf = i < CAPSULE_UPPER_HALF_MAX_VERTEX_INDEX;
				const Vec3V& offset = upperHalf ? offsetUpper : offsetLower;
				MESH_ADD_VERT_MTX(Vec3V(UNIT_CAPSULE_VERTICES[i], UNIT_CAPSULE_VERTICES[i + 1], UNIT_CAPSULE_VERTICES[i + 2]) * scale + offset, capsuleMatrix);
			}

			for (int i = 0; i < std::size(UNIT_CAPSULE_INDICES); i += 3)
			{
				MESH_ADD_INDEX(UNIT_CAPSULE_INDICES[i] - 1, UNIT_CAPSULE_INDICES[i + 1] - 1, UNIT_CAPSULE_INDICES[i + 2] - 1);
			}
		}

		static void FromCylinder(float height, float radius, bool yAxisUp, MESH_ARGS)
		{
			Mat44V cylinderMatrix = Mat44V::Identity();
			if (yAxisUp)
				cylinderMatrix *= Mat44V::LookAt(rage::VEC_ORIGIN, rage::VEC_RIGHT, rage::VEC_FRONT);
			cylinderMatrix *= matrix;

			float   halfHeight = height * 0.5f;
			Vec3V   offsetUpper = rage::VEC_UP * halfHeight;
			Vec3V   offsetLower = -offsetUpper;
			ScalarV scale = radius;
			for (int i = 0; i < std::size(UNIT_CYLINDER_VERTICES); i += 3)
			{
				bool upperHalf = i < CYLINDER_UPPER_HALF_MAX_VERTEX_INDEX;
				Vec3V offset = upperHalf ? offsetUpper : offsetLower;
				MESH_ADD_VERT_MTX(Vec3V(UNIT_CYLINDER_VERTICES[i], UNIT_CYLINDER_VERTICES[i + 1], UNIT_CYLINDER_VERTICES[i + 2]) * scale + offset, cylinderMatrix);
			}

			for (int i = 0; i < std::size(UNIT_CYLINDER_INDICES); i += 3)
			{
				MESH_ADD_INDEX(UNIT_CYLINDER_INDICES[i] - 1, UNIT_CYLINDER_INDICES[i + 1] - 1, UNIT_CYLINDER_INDICES[i + 2] - 1);
			}
		}
	};

#undef MESH_ARGS
#undef MESH_ADD_INDEX
#undef MESH_ADD_VERT
}
