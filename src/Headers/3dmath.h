//
// 3DMath.h
//

#pragma once

extern	float CalcXAngleFromPointToPoint(float fromY, float fromZ, float toY, float toZ);
float	CalcYAngleFromPointToPoint(float oldRot, float fromX, float fromZ, float toX, float toZ);
float TurnObjectTowardTarget(ObjNode *theNode, TQ3Point3D *from, float x, float z, float turnSpeed, Boolean useOffsets);
extern	float	CalcYAngleBetweenVectors(TQ3Vector3D *v1, TQ3Vector3D *v2);
extern	float	CalcAngleBetweenVectors2D(TQ3Vector2D *v1, TQ3Vector2D *v2);
extern	float	CalcAngleBetweenVectors3D(TQ3Vector3D *v1, TQ3Vector3D *v2);
extern	void CalcPointOnObject(ObjNode *theNode, TQ3Point3D *inPt, TQ3Point3D *outPt);
extern	void SetQuickRotationMatrix_XYZ(TQ3Matrix4x4 *m, float rx, float ry, float rz);
extern	Boolean IntersectionOfLineSegAndPlane(TQ3PlaneEquation *plane, float v1x, float v1y, float v1z,
								 float v2x, float v2y, float v2z, TQ3Point3D *outPoint);

extern	Boolean VectorsAreCloseEnough(TQ3Vector3D *v1, TQ3Vector3D *v2);
extern	Boolean PointsAreCloseEnough(TQ3Point3D *v1, TQ3Point3D *v2);
extern	float IntersectionOfYAndPlane_Func(float x, float z, TQ3PlaneEquation *p);

extern	Boolean IsPointInTriangle3D(const TQ3Point3D *point3D,	const TQ3Point3D *trianglePoints, TQ3Vector3D *normal);
void MatrixMultiply(TQ3Matrix4x4 *a, TQ3Matrix4x4 *b, TQ3Matrix4x4 *result);
void SetLookAtMatrix(TQ3Matrix4x4 *m, const TQ3Vector3D *upVector, const TQ3Point3D *from, const TQ3Point3D *to);
void SetLookAtMatrixAndTranslate(TQ3Matrix4x4 *m, const TQ3Vector3D *upVector, const TQ3Point3D *from, const TQ3Point3D *to);
Boolean IntersectLineSegments(double x1, double y1, double x2, double y2,
		                     double x3, double y3, double x4, double y4,
                             double *x, double *y);

void CalcLineNormal2D(float p0x, float p0y, float p1x, float p1y,
					 float	px, float py, TQ3Vector2D *normal);
void CalcRayNormal2D(TQ3Vector2D *vec, float p0x, float p0y,
					 float	px, float py, TQ3Vector2D *normal);

void ReflectVector2D(TQ3Vector2D *theVector, const TQ3Vector2D *N);
float TurnObjectTowardTargetZ(ObjNode *theNode, float x, float y, float turnSpeed);
float CalcDistance(float x1, float y1, float x2, float y2);
void CalcFaceNormal(TQ3Point3D *p1, TQ3Point3D *p2, TQ3Point3D *p3, TQ3Vector3D *normal);
void CalcPlaneEquationOfTriangle(TQ3PlaneEquation *plane, const TQ3Point3D *p3, const TQ3Point3D *p2, const TQ3Point3D *p1);
float CalcQuickDistance(float x1, float y1, float x2, float y2);
void FastNormalizeVector(float vx, float vy, float vz, TQ3Vector3D *outV);
void FastNormalizeVector2D(float vx, float vy, TQ3Vector2D *outV);
void MatrixMultiplyFast(TQ3Matrix4x4 *a, TQ3Matrix4x4 *b, TQ3Matrix4x4 *result);





/*********** INTERSECTION OF Y AND PLANE ********************/
//
// INPUT:	
//			x/z		:	xz coords of point
//			p		:	ptr to the plane
//
//
// *** IMPORTANT-->  This function does not check for divides by 0!! As such, there should be no
//					"vertical" polygons (polys with normal->y == 0).
//

#define IntersectionOfYAndPlane(_x, _z, _p)	(((_p)->constant - (((_p)->normal.x * _x) + ((_p)->normal.z * _z))) / (_p)->normal.y)


/***************** ANGLE TO VECTOR ******************/
//
// Returns a normalized 2D vector based on a radian angle
//
// NOTE: +rotation == counter clockwise!
//

static inline void AngleToVector(float angle, TQ3Vector2D *theVector)
{
	theVector->x = -sin(angle);
	theVector->y = -cos(angle);
}



/******************** FAST RECIPROCAL **************************/
//
//   y1 = y0 + y0*(1 - num*y0), where y0 = recip_est(num)
//

#define FastReciprocal(_num,_out)		\
{										\
	float	_y0;						\
										\
	_y0 = __fres(_num);					\
	_out = _y0 * (1 - _num*_y0) + _y0;	\
}


/***************** FAST VECTOR LENGTH 3D/2D *********************/

#define FastVectorLength3D(_inX, _inY, _inZ)	sqrtf(((_inY)*(_inY)) + ((_inX)*(_inX)) + ((_inZ)*(_inZ)))

#define FastVectorLength2D(_inX, _inY)			sqrtf(((_inY)*(_inY)) + ((_inX)*(_inX)))


static inline float ClampFloat(float x, float lo, float hi)
{
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}

static inline unsigned int PositiveModulo(int value, unsigned int m)
{
	int mod = value % (int)m;
	if (mod < 0)
	{
		mod += m;
	}
	return mod;
}

