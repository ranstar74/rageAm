#include "shapetest.h"

bool rageam::graphics::ShapeTest::RayIntersectsSphere(
	const rage::Vec3V& fromPos, const rage::Vec3V& dir,
	const rage::Vec3V& spherePos, const rage::ScalarV& sphereRadius,
	rage::ScalarV* outDistance)
{
	if (outDistance) *outDistance = rage::S_ZERO;

	rage::Vec3V rayToSphere = fromPos - spherePos;

	rage::ScalarV b = rayToSphere.Dot(dir);
	rage::ScalarV c = rayToSphere.Dot(rayToSphere) - sphereRadius * sphereRadius;

	if (c > rage::S_ZERO && b > rage::S_ZERO)
		return false;

	rage::ScalarV discriminant = b * b - c;

	if (discriminant < rage::S_ZERO)
		return false;

	if (outDistance)
	{
		rage::ScalarV distance = -b - discriminant.Sqrt();
		if (distance < rage::S_ZERO)
			distance = rage::S_ZERO;

		*outDistance = distance;
	}

	return true;
}

bool rageam::graphics::ShapeTest::RayIntersectsTriangle(
	const rage::Vec3V& rayPos, const rage::Vec3V& rayDir,
	const rage::Vec3V& vertex1, const rage::Vec3V& vertex2, const rage::Vec3V& vertex3,
	float& outDistance)
{
	//Source: Fast Minimum Storage Ray / Triangle Intersection
	//Reference: http://www.cs.virginia.edu/~gfx/Courses/2003/ImageSynthesis/papers/Acceleration/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf

	outDistance = 0.0f;

	//Compute vectors along two edges of the triangle.
	rage::Vec3V edge1 = vertex2 - vertex1;
	rage::Vec3V edge2 = vertex3 - vertex1;

	//Cross product of ray direction and edge2 - first part of determinant.
	rage::Vec3V directioncrossedge2 = rayDir.Cross(edge2);

	//Compute the determinant.
	//Dot product of edge1 and the first part of determinant.
	rage::ScalarV determinant = edge1.Dot(directioncrossedge2);

	//If the ray is parallel to the triangle plane, there is no collision.
	//This also means that we are not culling, the ray may hit both the
	//back and the front of the triangle.
	static const rage::ScalarV EPSILON = { 0.000001f };
	if (determinant.Abs() < EPSILON)
	{
		return false;
	}

	rage::ScalarV inversedeterminant = determinant.Reciprocal();

	//Calculate the U parameter of the intersection point.
	rage::Vec3V distanceVector = rayPos - vertex1;

	rage::ScalarV triangleU;
	triangleU = distanceVector.Dot(directioncrossedge2);
	triangleU *= inversedeterminant;

	//Make sure it is inside the triangle.
	if (triangleU < 0.0f || triangleU > 1.0f)
	{
		return false;
	}

	//Calculate the V parameter of the intersection point.
	rage::Vec3V distancecrossedge1 = distanceVector.Cross(edge1);

	rage::ScalarV triangleV;
	triangleV = rayDir.Dot(distancecrossedge1);
	triangleV *= inversedeterminant;

	//Make sure it is inside the triangle.
	if (triangleV < 0.0f || triangleU + triangleV > 1.0f)
	{
		return false;
	}

	//Compute the distance along the ray to the triangle.
	rage::ScalarV rayDistance;
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

bool rageam::graphics::ShapeTest::RayIntersectsPlane(
	const rage::Vec3V& rayPos, const rage::Vec3V& rayDir,
	const rage::Vec3V& planePos, const rage::Vec3V& planeNormal,
	rage::ScalarV* outDistance)
{
	if (outDistance) *outDistance = rage::S_ZERO;

	rage::ScalarV direction = planeNormal.Dot(rayDir);
	if (direction.AlmostEqual(rage::S_ZERO))
		return false;

	rage::ScalarV position = planeNormal.Dot(rayPos);
	rage::ScalarV planeD = planeNormal.Dot(planePos);
	rage::ScalarV distance = (planeD - position) / direction;

	if (distance < rage::S_ZERO)
		return false;

	if (outDistance) *outDistance = distance;
	return true;
}

bool rageam::graphics::ShapeTest::RayIntersectsPlane(
	const rage::Vec3V& rayPos, const rage::Vec3V& rayDir,
	const rage::Vec3V& planePos, const rage::Vec3V& planeNormal, 
	rage::Vec3V& outPoint)
{
	rage::ScalarV hitDistance;
	if (!RayIntersectsPlane(rayPos, rayDir, planePos, planeNormal, &hitDistance))
		return false;

	outPoint = rayPos + rayDir * hitDistance;
	return true;
}

bool rageam::graphics::ShapeTest::RayIntersectsCircle(
	const rage::Vec3V& rayPos, const rage::Vec3V& rayDir,
	const rage::Vec3V& circlePos, const rage::Vec3V& circleNormal,
	const rage::ScalarV& circleRadius,
	rage::ScalarV* outDistance, rage::Vec3V* outPoint)
{
	rage::ScalarV distance;
	if (!RayIntersectsPlane(rayPos, rayDir, circlePos, circleNormal, &distance))
		return false;

	rage::Vec3V point = rayPos + rayDir * distance;
	rage::ScalarV radiusSq = circleRadius * circleRadius;
	if (point.DistanceToSquared(circlePos) > radiusSq)
		return false;

	if (outDistance) *outDistance = distance;
	if (outPoint) *outPoint = point;

	return true;
}

bool rageam::graphics::ShapeTest::RayIntersectsCapsule(
	const rage::Vec3V& rayPos, const rage::Vec3V& rayDir,
	const rage::Vec3V& extentFrom, const rage::Vec3V& extentTo, 
	const rage::ScalarV& radius, 
	rage::ScalarV* outDistance)
{
	if (outDistance) *outDistance = rage::S_ZERO;

	rage::Vec3V ba = extentTo - extentFrom;
	rage::Vec3V oa = rayPos - extentFrom;
	rage::ScalarV baba = ba.Dot(ba);
	rage::ScalarV bard = ba.Dot(rayDir);
	rage::ScalarV baoa = ba.Dot(oa);
	rage::ScalarV rdoa = rayDir.Dot(oa);
	rage::ScalarV oaoa = oa.Dot(oa);
	rage::ScalarV a = baba - bard * bard;
	rage::ScalarV b = baba * rdoa - baoa * bard;
	rage::ScalarV c = baba * oaoa - baoa * baoa - radius * radius * baba;
	rage::ScalarV h = b * b - a * c;
	if (h >= 0.0)
	{
		rage::ScalarV t = (-b - h.Sqrt()) / a;
		rage::ScalarV y = baoa + t * bard;
		// body
		if (y > 0.0 && y < baba)
		{
			if (outDistance) *outDistance = t;
			return true;
		}
		// caps
		rage::Vec3V oc = (y <= 0.0) ? oa : rayPos - extentTo;
		b = rayDir.Dot(oc);
		c = oc.Dot(oc) - radius * radius;
		h = b * b - c;
		if (h > 0.0)
		{
			if (outDistance) *outDistance = -b - h.Sqrt();
			return true;
		}
	}
	return false;
}
