#include "intersection.h"

bool rage::RayIntersectsTriangle(const Vec3V& rayPos, const Vec3V& rayDir, const Vec3V& vertex1, const Vec3V& vertex2, const Vec3V& vertex3, float& outDistance)
{
	//Source: Fast Minimum Storage Ray / Triangle Intersection
	//Reference: http://www.cs.virginia.edu/~gfx/Courses/2003/ImageSynthesis/papers/Acceleration/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf

	outDistance = 0.0f;

	//Compute vectors along two edges of the triangle.
	Vec3V edge1 = vertex2 - vertex1;
	Vec3V edge2 = vertex3 - vertex1;

	//Cross product of ray direction and edge2 - first part of determinant.
	Vec3V directioncrossedge2 = rayDir.Cross(edge2);

	//Compute the determinant.
	//Dot product of edge1 and the first part of determinant.
	ScalarV determinant = edge1.Dot(directioncrossedge2);

	//If the ray is parallel to the triangle plane, there is no collision.
	//This also means that we are not culling, the ray may hit both the
	//back and the front of the triangle.
	static const ScalarV EPSILON = { 0.000001f };
	if (determinant.Abs() < EPSILON)
	{
		return false;
	}

	ScalarV inversedeterminant = determinant.Reciprocal();

	//Calculate the U parameter of the intersection point.
	Vec3V distanceVector = rayPos - vertex1;

	ScalarV triangleU;
	triangleU = distanceVector.Dot(directioncrossedge2);
	triangleU *= inversedeterminant;

	//Make sure it is inside the triangle.
	if (triangleU < 0.0f || triangleU > 1.0f)
	{
		return false;
	}

	//Calculate the V parameter of the intersection point.
	Vec3V distancecrossedge1 = distanceVector.Cross(edge1);

	ScalarV triangleV;
	triangleV = rayDir.Dot(distancecrossedge1);
	triangleV *= inversedeterminant;

	//Make sure it is inside the triangle.
	if (triangleV < 0.0f || triangleU + triangleV > 1.0f)
	{
		return false;
	}

	//Compute the distance along the ray to the triangle.
	ScalarV rayDistance;
	rayDistance = edge2.Dot(distancecrossedge1);
	rayDistance *= inversedeterminant;

	//Is the triangle behind the ray origin?
	if (rayDistance < 0.0f)
	{
		return false;
	}

	outDistance = rayDistance.Get();

	return true;
}
