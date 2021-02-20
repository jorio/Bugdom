// FRUSTUM CULLING.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom

#include <QD3D.h>
#include <math.h>
#include <stdbool.h>
#include "frustumculling.h"

static TQ3RationalPoint4D gFrustumPlanes[6];

/*************** FRUSTUM CALCS ***************/
// Planes 0,1: X axis. Right, left
// Planes 2,3: Y axis. Top, bottom
// Planes 4,5: Z axis. Near, far

void UpdateFrustumPlanes(const struct TQ3Matrix4x4* worldToFrustum)
{
#define M(a,b) worldToFrustum->value[a][b]

	for (int plane = 0; plane < 6; plane++)
	{
		int axis = plane >> 1u;							// 0=X, 1=Y, 2=Z
		float sign = plane%2 == 0 ? -1.0f : 1.0f;		// To pick either frustum plane on the axis.

		float x = M(0,3) + sign * M(0,axis);
		float y = M(1,3) + sign * M(1,axis);
		float z = M(2,3) + sign * M(2,axis);
		float w = M(3,3) + sign * M(3,axis);

		float t = sqrtf(x*x + y*y + z*z);				// Normalize
		gFrustumPlanes[plane].x = x/t;
		gFrustumPlanes[plane].y = y/t;
		gFrustumPlanes[plane].z = z/t;
		gFrustumPlanes[plane].w = w/t;
	}

#undef M
}

static inline bool IsSphereFacingFrustumPlane(const TQ3Point3D* worldPt, float radius, int plane)
{
	float planeDot =
			worldPt->x * gFrustumPlanes[plane].x +
			worldPt->y * gFrustumPlanes[plane].y +
			worldPt->z * gFrustumPlanes[plane].z +
			gFrustumPlanes[plane].w;

	return planeDot > -radius;
}

bool IsSphereInFrustum_XZ(const TQ3Point3D* worldPt, float radius)
{
	return IsSphereFacingFrustumPlane(worldPt, radius, kFrustumPlaneRight)
		&& IsSphereFacingFrustumPlane(worldPt, radius, kFrustumPlaneLeft)
		&& IsSphereFacingFrustumPlane(worldPt, radius, kFrustumPlaneNear)
		&& IsSphereFacingFrustumPlane(worldPt, radius, kFrustumPlaneFar);
}

bool IsSphereInFrustum_XYZ(const TQ3Point3D* worldPt, float radius)
{
	return IsSphereFacingFrustumPlane(worldPt, radius, kFrustumPlaneRight)
		&& IsSphereFacingFrustumPlane(worldPt, radius, kFrustumPlaneLeft)
		&& IsSphereFacingFrustumPlane(worldPt, radius, kFrustumPlaneTop)
		&& IsSphereFacingFrustumPlane(worldPt, radius, kFrustumPlaneBottom)
		&& IsSphereFacingFrustumPlane(worldPt, radius, kFrustumPlaneNear)
		&& IsSphereFacingFrustumPlane(worldPt, radius, kFrustumPlaneFar);
}
