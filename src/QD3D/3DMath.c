/*******************************/
/*     		3D MATH.C		   */
/* (c)1996-98 Pangea Software  */
/* By Brian Greenstone         */
/*******************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static inline float MaskAngle(float angle);


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	VectorComponent_X,
	VectorComponent_Y,
	VectorComponent_Z
};


/*********************/
/*    VARIABLES      */
/*********************/




/********************* MATRIX MULTIPLY FAST *******************/
//
// Multiply matrix Mat1 by matrix Mat2, return result in Result.
//
// NOTE: a or b cannot == result!!!
//

void MatrixMultiplyFast(TQ3Matrix4x4 *a, TQ3Matrix4x4 *b, TQ3Matrix4x4 *result)
{
Byte	i,j;

	GAME_ASSERT(a != result);
	GAME_ASSERT(b != result);

			/* DO IT */

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			result->value[i][j] = 	a->value[i][0] * b->value[0][j] +
									a->value[i][1] * b->value[1][j] +
								 	a->value[i][2] * b->value[2][j] +
								 	a->value[i][3] * b->value[3][j];
		}
	}
}





/******************** FAST NORMALIZE VECTOR ***********************/
// Source port note: original started with PowerPC's frsqrte and refined it with Newton-Raphson.
// Changed to sqrtf.

void FastNormalizeVector(float vx, float vy, float vz, TQ3Vector3D *outV)
{
	if ((vx == 0.0f) && (vy == 0.0f) && (vz == 0.0f))		// check for zero length vectors
	{
		outV->x = outV->y = outV->z = 0;
		return;
	}

	float invmag = 1.0f / (sqrtf(vx*vx + vy*vy + vz*vz) + FLT_MIN);
	outV->x = vx * invmag;
	outV->y = vy * invmag;
	outV->z = vz * invmag;
}

/******************** FAST NORMALIZE VECTOR 2D ***********************/
// Source port note: original started with PowerPC's frsqrte and refined it with Newton-Raphson.
// Changed to sqrtf.

void FastNormalizeVector2D(float vx, float vy, TQ3Vector2D *outV)
{
	if ((vx == 0.0f) && (vy == 0.0f))			// check for zero length vectors
	{
		outV->x = outV->y = 0;
		return;
	}

	float invmag = 1.0f / (sqrtf(vx*vx + vy*vy) + FLT_MIN);
	outV->x = vx * invmag;
	outV->y = vy * invmag;
}


/************* CALC QUICK DISTANCE ****************/
//
// Does cheezeball quick distance calculation on 2 2D points.
//

float CalcQuickDistance(float x1, float y1, float x2, float y2)
{
float	diffX,diffY;

	diffX = fabsf(x1-x2);
	diffY = fabsf(y1-y2);

	if (diffX > diffY)
	{
		return(diffX + (0.375f*diffY));			// same as (3*diffY)/8
	}
	else
	{
		return(diffY + (0.375f*diffX));
	}

}


/******************* CALC PLANE EQUATION OF TRIANGLE ********************/
//
// input points should be clockwise!
//

void CalcPlaneEquationOfTriangle(TQ3PlaneEquation *plane, const TQ3Point3D *p3, const TQ3Point3D *p2, const TQ3Point3D *p1)
{
float	pq_x,pq_y,pq_z;
float	pr_x,pr_y,pr_z;
float	p1x,p1y,p1z;
float	x,y,z;

	p1x = p1->x;									// get point #1
	p1y = p1->y;
	p1z = p1->z;

	pq_x = p1x - p2->x;								// calc vector pq
	pq_y = p1y - p2->y;
	pq_z = p1z - p2->z;

	pr_x = p1->x - p3->x;							// calc vector pr
	pr_y = p1->y - p3->y;
	pr_z = p1->z - p3->z;


			/* CALC CROSS PRODUCT FOR THE FACE'S NORMAL */
			
	x = (pq_y * pr_z) - (pq_z * pr_y);				// cross product of edge vectors = face normal
	y = ((pq_z * pr_x) - (pq_x * pr_z));
	z = (pq_x * pr_y) - (pq_y * pr_x);

	FastNormalizeVector(x, y, z, &plane->normal);
	

		/* CALC DOT PRODUCT FOR PLANE CONSTANT */

	plane->constant = 	((plane->normal.x * p1x) +
						(plane->normal.y * p1y) +
						(plane->normal.z * p1z));
}



/************* CALC DISTANCE ****************/
//
// Uses PowerPC fsqrt instruction to get value
//

float CalcDistance(float x1, float y1, float x2, float y2)
{
float	diffX,diffY;

	diffX = fabsf(x1-x2);
	diffY = fabsf(y1-y2);

	return sqrtf(diffX*diffX + diffY*diffY);
}



/***************** CALC FACE NORMAL *********************/
//
// Returns the normal vector off the face defined by 3 points.
//

void CalcFaceNormal(TQ3Point3D *p1, TQ3Point3D *p2, TQ3Point3D *p3, TQ3Vector3D *normal)
{
float		dx1,dx2,dy1,dy2,dz1,dz2;
float		x,y,z;

	dx1 = (p1->x - p3->x);
	dy1 = (p1->y - p3->y);
	dz1 = (p1->z - p3->z);
	dx2 = (p2->x - p3->x);
	dy2 = (p2->y - p3->y);
	dz2 = (p2->z - p3->z);


			/* DO CROSS PRODUCT */
			
	x =   dy1 * dz2 - dy2 * dz1;
	y = -(dx1 * dz2 - dx2 * dz1);
	z =   dx1 * dy2 - dx2 * dy1;
			
			/* NORMALIZE IT */

	FastNormalizeVector(x,y, z, normal);
}


/********************* CALC X ANGLE FROM POINT TO POINT **********************/

float CalcXAngleFromPointToPoint(float fromY, float fromZ, float toY, float toZ)
{
float	xangle,zdiff;

	zdiff = toZ-fromZ;									// calc diff in Z
	zdiff = fabs(zdiff);								// get abs value

	xangle = atan2(-zdiff,toY-fromY) + (PI/2.0f);
	
			/* KEEP BETWEEN 0 & 2*PI */
			
	return(MaskAngle(xangle));
}


/********************* CALC Y ANGLE FROM POINT TO POINT **********************/
//
// Returns an unsigned result from 0 -> 6.28
//

float	CalcYAngleFromPointToPoint(float oldRot, float fromX, float fromZ, float toX, float toZ)
{
float	yangle;
float	dx, dz;

	dx = fromX-toX;
	dz = fromZ-toZ;

	if ((fabs(dx) < .0001f) && (fabs(dz) < .0001f))			// see if delta to small to be accurate
		return(oldRot);

	yangle = PI2 - MaskAngle(atan2(dz,dx) - (PI/2.0f));
	
	if (yangle >= (PI2-.0001f))
		return(0);

	return(yangle);
}


/**************** CALC Y ANGLE BETWEEN VECTORS ******************/
//
// This simply returns the angle BETWEEN 2 3D vectors, NOT the angle FROM one point to
// another.
//

float	CalcYAngleBetweenVectors(TQ3Vector3D *v1, TQ3Vector3D *v2)
{
float	dot,angle;

	v1->y = v2->y = 0;

	Q3Vector3D_Normalize(v1,v1);	// make sure they're normalized
	Q3Vector3D_Normalize(v2,v2);

	dot = Q3Vector3D_Dot(v1,v2);	// the dot product is the cosine of the angle between the 2 vectors
	angle = acos(dot);				// get arc-cosine to get the angle out of it
	return(angle);
}


/**************** CALC ANGLE BETWEEN VECTORS 2D ******************/

float	CalcAngleBetweenVectors2D(TQ3Vector2D *v1, TQ3Vector2D *v2)
{
float		dot,angle;
TQ3Vector2D	v1b,v2b;

	Q3Vector2D_Normalize(v1,&v1b);	// make sure they're normalized
	Q3Vector2D_Normalize(v2,&v2b);

	dot = Q3Vector2D_Dot(&v1b,&v2b);// the dot product is the cosine of the angle between the 2 vectors
	angle = acos(dot);				// get arc-cosine to get the angle out of it
	return(angle);
}


/**************** CALC ANGLE BETWEEN VECTORS 3D ******************/

float	CalcAngleBetweenVectors3D(TQ3Vector3D *v1, TQ3Vector3D *v2)
{
float	dot,angle;

	Q3Vector3D_Normalize(v1,v1);	// make sure they're normalized
	Q3Vector3D_Normalize(v2,v2);

	dot = Q3Vector3D_Dot(v1,v2);	// the dot product is the cosine of the angle between the 2 vectors
	angle = acos(dot);				// get arc-cosine to get the angle out of it
	return(angle);
}




/******************** MASK ANGLE ****************************/
//
// Given an arbitrary angle, it limits it to between 0 and 2*PI
//

static inline float MaskAngle(float angle)
{
int		n;
Boolean	neg;

	neg = angle < 0.0f;
	n = angle * (1.0f/PI2);						// see how many times it wraps fully
	angle -= ((float)n * PI2);					// subtract # wrappings to get just the remainder
	
	if (neg)
		angle += PI2;

	return(angle);
}


/************ TURN OBJECT TOWARD TARGET ****************/
//
// INPUT:	theNode = object
//			from = from pt or uses theNode->Coord
//			x/z = target coords to aim towards
//			turnSpeed = turn speed, 0 == just return angle between
//
// OUTPUT:  remaining angle to target rot
//

float TurnObjectTowardTarget(ObjNode *theNode, TQ3Point3D *from, float x, float z, float turnSpeed, Boolean useOffsets)
{
float	desiredRotY,diff;
float	currentRotY;
float	adjustedTurnSpeed;
float	fromX,fromZ;

	if (useOffsets)
	{
		x += theNode->TargetOff.x;										// offset coord
		z += theNode->TargetOff.y;
	}

	if (from)
	{
		fromX = from->x;
		fromZ = from->z;
	}
	else
	{
		fromX = theNode->Coord.x;
		fromZ = theNode->Coord.z;
	}

	if ((fabs(fromX - x) < 1.0f) &&									// see if nowhere
		(fabs(fromZ - z) < 1.0f))
		return(0);

	desiredRotY = CalcYAngleFromPointToPoint(theNode->Rot.y, fromX, fromZ, x, z);	// calc angle directly at target
	currentRotY = theNode->Rot.y;									// get current angle

	if (turnSpeed != 0.0f)
	{
		adjustedTurnSpeed = turnSpeed * gFramesPerSecondFrac;		// calc actual turn speed

		diff = desiredRotY - currentRotY;							// calc diff from current to desired
		if (diff > adjustedTurnSpeed)								// (+) rotation
		{
			if (diff > PI)											// see if shorter to go backward
			{
			   currentRotY -= adjustedTurnSpeed;
			}
			else
			{
			   	currentRotY += adjustedTurnSpeed;
			}
		}
		else
		if (diff < -adjustedTurnSpeed)
		{
			if (diff < -PI)											// see if shorter to go forward
			{
			   currentRotY += adjustedTurnSpeed;
			}
			else
			{
			   currentRotY -= adjustedTurnSpeed;
			}
		}
		else
		{
			currentRotY = desiredRotY;
		}
		currentRotY = MaskAngle(currentRotY);						// keep from looping too bad

		theNode->Rot.y = currentRotY;								// update node
	}

	return(fabs(desiredRotY - currentRotY));
}


/************ TURN OBJECT TOWARD TARGET ****************/
//
// Same as above but rotates on flat z axis
//

float TurnObjectTowardTargetZ(ObjNode *theNode, float x, float y, float turnSpeed)
{
float	desiredRotZ,diff;
float	currentRotZ;
float	adjustedTurnSpeed;

	desiredRotZ = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->Coord.x, -theNode->Coord.y,x, -y);		// calc angle directly at target
	currentRotZ = theNode->Rot.z;															// get current angle

	if (turnSpeed != 0.0f)
	{
		adjustedTurnSpeed = turnSpeed * gFramesPerSecondFrac;		// calc actual turn speed


		diff = desiredRotZ - currentRotZ;							// calc diff from current to desired
		if (diff > adjustedTurnSpeed)								// (+) rotation
		{
			if (diff > PI)											// see if shorter to go backward
			{
			   currentRotZ -= adjustedTurnSpeed;
			}
			else
			{
			   	currentRotZ += adjustedTurnSpeed;
			}
		}
		else
		if (diff < -adjustedTurnSpeed)
		{
			if (diff < -PI)											// see if shorter to go forward
			{
			   currentRotZ += adjustedTurnSpeed;
			}
			else
			{
			   currentRotZ -= adjustedTurnSpeed;
			}
		}
		else
		{
			currentRotZ = desiredRotZ;
		}
		currentRotZ = MaskAngle(currentRotZ);						// keep from looping too bad

		theNode->Rot.z = currentRotZ;								// update node
	}

	return(fabs(desiredRotZ - currentRotZ));
}




/******************** CALC POINT ON OBJECT *************************/
//
// Uses the input ObjNode's BaseTransformMatrix to transform the input point.
//

void CalcPointOnObject(ObjNode *theNode, TQ3Point3D *inPt, TQ3Point3D *outPt)
{
	Q3Point3D_Transform(inPt, &theNode->BaseTransformMatrix, outPt);
}



/******************* SET QUICK XYZ-ROTATION MATRIX ************************/
//
// Does a quick precomputation to calculate an XYZ rotation matrix
//
//
// cy*cz					cy*sz					-sy			0
// (sx*sy*cz)+(cx*-sz)		(sx*sy*sz)+(cx*cz)		sx*cy		0
// (cx*sy*cz)+(-sx*-sz)		(cx*sy*sz)+(-sx*cz)		cx*cy		0
// 0						0						0			1
//

void SetQuickRotationMatrix_XYZ(TQ3Matrix4x4 *m, float rx, float ry, float rz)
{
float	sx,cx,sy,sz,cy,cz,sxsy,cxsy;

	sx = sin(rx);
	sy = sin(ry);
	sz = sin(rz);
	cx = cos(rx);
	cy = cos(ry);
	cz = cos(rz);
	
	sxsy = sx*sy;
	cxsy = cx*sy;
	
	m->value[0][0] = cy*cz;					m->value[0][1] = cy*sz; 				m->value[0][2] = -sy; 	m->value[0][3] = 0;
	m->value[1][0] = (sxsy*cz)+(cx*-sz);	m->value[1][1] = (sxsy*sz)+(cx*cz);		m->value[1][2] = sx*cy;	m->value[1][3] = 0;
	m->value[2][0] = (cxsy*cz)+(-sx*-sz);	m->value[2][1] = (cxsy*sz)+(-sx*cz);	m->value[2][2] = cx*cy;	m->value[2][3] = 0;
	m->value[3][0] = 0;						m->value[3][1] = 0;						m->value[3][2] = 0;		m->value[3][3] = 1;
}







/************************** VECTORS ARE CLOSE ENOUGH ****************************/

Boolean VectorsAreCloseEnough(TQ3Vector3D *v1, TQ3Vector3D *v2)
{
	if (fabs(v1->x - v2->x) < 0.02f)			// WARNING: If change, must change in BioOreoPro also!!
		if (fabs(v1->y - v2->y) < 0.02f)
			if (fabs(v1->z - v2->z) < 0.02f)
				return(true);

	return(false);
}

/************************** POINTS ARE CLOSE ENOUGH ****************************/

Boolean PointsAreCloseEnough(TQ3Point3D *v1, TQ3Point3D *v2)
{
	if (fabs(v1->x - v2->x) < 0.001f)		// WARNING: If change, must change in BioOreoPro also!!
		if (fabs(v1->y - v2->y) < 0.001f)
			if (fabs(v1->z - v2->z) < 0.001f)
				return(true);

	return(false);
}




#pragma mark ....intersect line & plane....

/******************** INTERSECT PLANE & LINE SEGMENT ***********************/
//
// Returns true if the input line segment intersects the plane.
//

Boolean IntersectionOfLineSegAndPlane(TQ3PlaneEquation *plane, float v1x, float v1y, float v1z,
								 float v2x, float v2y, float v2z, TQ3Point3D *outPoint)
{
int		a,b;
float	r;
float	nx,ny,nz,planeConst;
float	vBAx, vBAy, vBAz, dot, lam;

	nx = plane->normal.x;
	ny = plane->normal.y;
	nz = plane->normal.z;
	planeConst = plane->constant;
	
	
		/* DETERMINE SIDENESS OF VERT1 */
		
	r = -planeConst;
	r += (nx * v1x) + (ny * v1y) + (nz * v1z);
	a = (r < 0.0f) ? 1 : 0;

		/* DETERMINE SIDENESS OF VERT2 */
		
	r = -planeConst;
	r += (nx * v2x) + (ny * v2y) + (nz * v2z);
	b = (r < 0.0f) ? 1 : 0;

		/* SEE IF LINE CROSSES PLANE (INTERSECTS) */

	if (a == b)
	{
		return(false);
	}


		/****************************************************/
		/* LINE INTERSECTS, SO CALCULATE INTERSECTION POINT */
		/****************************************************/
			
				/* CALC LINE SEGMENT VECTOR BA */
				
	vBAx = v2x - v1x;
	vBAy = v2y - v1y;
	vBAz = v2z - v1z;
	
			/* DOT OF PLANE NORMAL & LINE SEGMENT VECTOR */
			
	dot = (nx * vBAx) + (ny * vBAy) + (nz * vBAz);
	
			/* IF VALID, CALC INTERSECTION POINT */
			
	if (dot)
	{
		lam = planeConst;
		lam -= (nx * v1x) + (ny * v1y) + (nz * v1z);		// calc dot product of plane normal & 1st vertex
		lam /= dot;											// div by previous dot for scaling factor
		
		outPoint->x = v1x + (lam * vBAx);					// calc intersect point
		outPoint->y = v1y + (lam * vBAy);
		outPoint->z = v1z + (lam * vBAz);
		return(true);
	}
	
		/* IF DOT == 0, THEN LINE IS PARALLEL TO PLANE THUS NO INTERSECTION */
		
	else
		return(false);
}




/*********** INTERSECTION OF Y AND PLANE FUNCTION ********************/
//
// INPUT:	
//			x/z		:	xz coords of point
//			p		:	ptr to the plane
//
//
// *** IMPORTANT-->  This function does not check for divides by 0!! As such, there should be no
//					"vertical" polygons (polys with normal->y == 0).
//

float IntersectionOfYAndPlane_Func(float x, float z, TQ3PlaneEquation *p)
{
	return	((p->constant - ((p->normal.x * x) + (p->normal.z * z))) / p->normal.y);
}



/********************* MATRIX MULTIPLY *******************/
//
// Multiply matrix Mat1 by matrix Mat2, return result in Result
//

void MatrixMultiply(TQ3Matrix4x4 *a, TQ3Matrix4x4 *b, TQ3Matrix4x4 *result)
{
TQ3Matrix4x4	*r2,temp;
unsigned short	i,j;

		/* CODE TO ALLOW RESULT TO BE A SOURCE MATRIX */
		
	if (result == a)
	{
		r2 = a;
		result = &temp;
	}
	else
	if (result == b)
	{
		r2 = b;
		result = &temp;
	}
	else
		r2 = nil;

			/* DO IT */
			
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			result->value[i][j] = 	a->value[i][0] * b->value[0][j] +
									a->value[i][1] * b->value[1][j] +
								 	a->value[i][2] * b->value[2][j] +
								 	a->value[i][3] * b->value[3][j];
		}
	}
				
	if (r2)								// see if copy over input matrix
		*r2 = temp;			
}


/********************* SET LOOKAT MATRIX ********************/

void SetLookAtMatrix(TQ3Matrix4x4 *m, const TQ3Vector3D *upVector, const TQ3Point3D *from, const TQ3Point3D *to)
{
TQ3Vector3D	lookAt,theXAxis;

			/* CALC LOOK-AT VECTOR */

	FastNormalizeVector(from->x - to->x, from->y - to->y, from->z - to->z, &lookAt);

	m->value[2][0] = lookAt.x;
	m->value[2][1] = lookAt.y;
	m->value[2][2] = lookAt.z;
	

			/* CALC UP VECTOR */

	m->value[1][0] = upVector->x;
	m->value[1][1] = upVector->y;
	m->value[1][2] = upVector->z;


		/* CALC THE X-AXIS VECTOR */
		
	theXAxis.x = 	upVector->y * lookAt.z - lookAt.y * upVector->z;		// calc cross product
	theXAxis.y =  -(upVector->x	* lookAt.z - lookAt.x * upVector->z);
	theXAxis.z = 	upVector->x * lookAt.y - lookAt.x * upVector->y;		
		
	m->value[0][0] = theXAxis.x;
	m->value[0][1] = theXAxis.y;
	m->value[0][2] = theXAxis.z;
	
	
			/* SET OTHER THINGS */
	m->value[0][3] =
	m->value[1][3] = 
	m->value[2][3] =
	m->value[3][0] = m->value[3][1] = m->value[3][2] = 0;
	m->value[3][3] = 1;
}


/********************* SET LOOKAT MATRIX AND TRANSLATE ********************/

void SetLookAtMatrixAndTranslate(TQ3Matrix4x4 *m, const TQ3Vector3D *upVector, const TQ3Point3D *from, const TQ3Point3D *to)
{
TQ3Vector3D	lookAt;

			/* CALC LOOK-AT VECTOR */

	FastNormalizeVector(from->x - to->x, from->y - to->y, from->z - to->z, &lookAt);

	m->value[2][0] = lookAt.x;
	m->value[2][1] = lookAt.y;
	m->value[2][2] = lookAt.z;
	

			/* CALC UP VECTOR */

	m->value[1][0] = upVector->x;
	m->value[1][1] = upVector->y;
	m->value[1][2] = upVector->z;


		/* CALC THE X-AXIS VECTOR */

	m->value[0][0] = 	upVector->y * lookAt.z - lookAt.y * upVector->z;		// calc cross product
	m->value[0][1] =  -(upVector->x	* lookAt.z - lookAt.x * upVector->z);
	m->value[0][2] = 	upVector->x * lookAt.y - lookAt.x * upVector->y;		
	
	
			/* SET OTHER THINGS */
			
	m->value[0][3] =
	m->value[1][3] = 
	m->value[2][3] = 0;
	
	m->value[3][0] = from->x;					// set translate
	m->value[3][1] = from->y;
	m->value[3][2] = from->z;
	m->value[3][3] = 1;
}



#pragma mark -

/*===========================================================================*\
 *
 *	Routine:	EiPoint3D_InsideTriangle()
 *
 *	Comments:	Is the point, which lies on the triangle plane, inside the
 *				triangle. If coords != NULL also returns the affine coordinates
 *				of this point. Return coordinate even if point is outside 
 *				triangle because of instability at borderline cases.
 *
 * 				Reference: Graphics Gems (the first one), page 390.
 *
\*===========================================================================*/

#define kQ3RealZero			(FLT_EPSILON)
#define	EmIsZero(x)		((x >= -kQ3RealZero) && (x <= kQ3RealZero))
#define	EcEpsilon3D		0.0001f


/********************** IS POINT IN TRIANGLE 3D **************************/
//
// INPUT:	point3D			= the point to test
//			trianglePoints	= triangle's 3 points
//			normal			= triangle's normal
//

Boolean IsPointInTriangle3D(const TQ3Point3D *point3D,	const TQ3Point3D *trianglePoints, TQ3Vector3D *normal)
{
Byte					maximalComponent;
unsigned long			skip;
float					*tmp;
float 					xComp, yComp, zComp;
float					pX,pY;
float					verts0x,verts0y;
float					verts1x,verts1y;
float					verts2x,verts2y;
	
	tmp = (float *)trianglePoints;
	skip = sizeof(TQ3Point3D) / sizeof(float);
	
		
			/*****************************************/
			/* DETERMINE LONGEST COMPONENT OF NORMAL */
			/*****************************************/
				
//	EiVector3D_MaximalComponent(normal,&maximalComponent,&maximalComponentNegative);

	xComp = fabs(normal->x);
	yComp = fabs(normal->y);
	zComp = fabs(normal->z);
	
	if (xComp > yComp)
	{
		if (xComp > zComp)
			maximalComponent = VectorComponent_X;
		else
			maximalComponent = VectorComponent_Z;
	}
	else
	{
		if (yComp > zComp)
			maximalComponent = VectorComponent_Y;
		else
			maximalComponent = VectorComponent_Z;
	}

	
	
				/* PROJECT 3D POINTS TO 2D */

	switch(maximalComponent)
	{
		TQ3Point3D	*point;
		
		case	VectorComponent_X:
				pX = point3D->y;
				pY = point3D->z;
				
				point = (TQ3Point3D *)tmp;
				verts0x = point->y;
				verts0y = point->z;

				point = (TQ3Point3D *)(tmp + skip);
				verts1x = point->y;
				verts1y = point->z;

				point = (TQ3Point3D *)(tmp + skip + skip);
				verts2x = point->y;
				verts2y = point->z;
				break;

		case	VectorComponent_Y:
				pX = point3D->z;
				pY = point3D->x;
				
				point = (TQ3Point3D *)tmp;
				verts0x = point->z;
				verts0y = point->x;

				point = (TQ3Point3D *)(tmp + skip);
				verts1x = point->z;
				verts1y = point->x;

				point = (TQ3Point3D *)(tmp + skip + skip);
				verts2x = point->z;
				verts2y = point->x;
				break;

		case	VectorComponent_Z:
				pX = point3D->x;
				pY = point3D->y;
				
				point = (TQ3Point3D *)tmp;
				verts0x = point->x;
				verts0y = point->y;

				point = (TQ3Point3D *)(tmp + skip);
				verts1x = point->x;
				verts1y = point->y;

				point = (TQ3Point3D *)(tmp + skip + skip);
				verts2x = point->x;
				verts2y = point->y;
				break;

		default:
				DoFatalAlert("IsPointInTriangle3D: unknown component");
				return false;
	}
	
	
			/* NOW DO 2D POINT-IN-TRIANGLE CHECK */
				
	return ((Boolean)IsPointInTriangle(pX, pY, verts0x, verts0y, verts1x, verts1y, verts2x, verts2y));	
}

#pragma mark -


/******************** INTERSECT LINE SEGMENTS ************************/
//
// OUTPUT: x,y = coords of intersection
//			true = yes, intersection occured
//

Boolean IntersectLineSegments(double x1, double y1, double x2, double y2,
		                     double x3, double y3, double x4, double y4,
                             double *x, double *y)
{
double 	a1, a2, b1, b2, c1, c2; 				// Coefficients of line eqns. 
long 	r1, r2, r3, r4;         				// 'Sign' values 
double 	denom, offset, num;     				// Intermediate values 

     /* Compute a1, b1, c1, where line joining points 1 and 2
      * is "a1 x  +  b1 y  +  c1  =  0".
      */

	a1 = y2 - y1;
	b1 = x1 - x2;
	c1 = (x2 * y1) - (x1 * y2);


     /* Compute r3 and r4. */
     
	r3 = (a1 * x3) + (b1 * y3) + c1;
	r4 = (a1 * x4) + (b1 * y4) + c1;


     /* Check signs of r3 and r4.  If both point 3 and point 4 lie on
      * same side of line 1, the line segments do not intersect.
      */

	if ((r3 != 0) && (r4 != 0) && (!((r3^r4)&0x80000000)))
         return(false);

     /* Compute a2, b2, c2 */

	a2 = y4 - y3;
	b2 = x3 - x4;
	c2 = x4 * y3 - x3 * y4;

     /* Compute r1 and r2 */

	r1 = a2 * x1 + b2 * y1 + c2;
	r2 = a2 * x2 + b2 * y2 + c2;

     /* Check signs of r1 and r2.  If both point 1 and point 2 lie
      * on same side of second line segment, the line segments do
      * not intersect.
      */

	if ((r1 != 0) && (r2 != 0) && (!((r1^r2)&0x80000000)))
		return (false);


     /* Line segments intersect: compute intersection point. */
   
	denom = (a1 * b2) - (a2 * b1);
	if (denom == 0.0)
	{
		*x = x1;
		*y = y1;
		return (true);									// collinear
	}

	offset = denom < 0.0 ? - denom * .5 : denom * .5;

     /* The denom/2 is to get rounding instead of truncating.  It
      * is added or subtracted to the numerator, depending upon the
      * sign of the numerator.
      */

	if (denom != 0.0)
		denom = 1.0/denom;

 	num = b1 * c2 - b2 * c1;
     *x = ( num < 0 ? num - offset : num + offset ) * denom;

	num = a2 * c1 - a1 * c2;
	*y = ( num < 0 ? num - offset : num + offset ) * denom;

	return (true);
}




/********************* CALC LINE NORMAL 2D *************************/
//
// INPUT: 	p0, p1 = 2 points on the line
//			px,py = point used to determine which way we want the normal aiming
//
// OUTPUT:	normal = normal to the line
//

void CalcLineNormal2D(float p0x, float p0y, float p1x, float p1y,
					 float	px, float py, TQ3Vector2D *normal)
{
TQ3Vector2D		normalA, normalB;
float			temp,x,y;

	
		/* CALC NORMALIZED VECTOR FROM ENDPOINT TO ENDPOINT */
			
	FastNormalizeVector2D(p0x - p1x, p0y - p1y, &normalA);
	 
	 
		/* CALC NORMALIZED VECTOR FROM REF POINT TO ENDPOINT 0 */

	FastNormalizeVector2D(px - p0x, py - p0y, &normalB);
	
	
	temp = -((normalB.x * normalA.y) - (normalA.x * normalB.y));
	x =   -(temp * normalA.y);
	y =   normalA.x * temp;

	FastNormalizeVector2D(x, y, normal);					// normalize the result
}

/********************* CALC RAY NORMAL 2D *************************/
//
// INPUT: 	vec = normalized vector of ray
//			p0 = ray origin
//			px,py = point used to determine which way we want the normal aiming
//
// OUTPUT:	normal = normal to the line
//

void CalcRayNormal2D(TQ3Vector2D *vec, float p0x, float p0y,
					 float	px, float py, TQ3Vector2D *normal)
{
TQ3Vector2D		normalB;
float			temp,x,y;

	 
		/* CALC NORMALIZED VECTOR FROM REF POINT TO ENDPOINT 0 */

	FastNormalizeVector2D(px - p0x, py - p0y, &normalB);
	
	
	temp = -((normalB.x * vec->y) - (vec->x * normalB.y));
	x =   -(temp * vec->y);
	y =   vec->x * temp;

	FastNormalizeVector2D(x, y, normal);					// normalize the result
}





/*********************** REFLECT VECTOR 2D *************************/
//
// NOTE:	This preserves the magnitude of the input vector.
//
// INPUT:	theVector = vector to reflect (not normalized)
//			N 		  = normal to reflect around (normalized)
//
// OUTPUT:	theVector = reflected vector, scaled to magnitude of original
//

void ReflectVector2D(TQ3Vector2D *theVector, const TQ3Vector2D *N)
{
float	x,y;
float	normalX,normalY;
float	dotProduct,reflectedX,reflectedY;
float	mag,oneOverM;

	x = theVector->x;
	y = theVector->y;

			/* CALC LENGTH AND NORMALIZE INPUT VECTOR */
			
	mag = sqrt(x*x + y*y);							// calc magnitude of input vector;	
	if (mag != 0.0f)
		oneOverM = 1.0f / mag;	
	else
		oneOverM = 0;
	x *= oneOverM;									// normalize
	y *= oneOverM;
	

	normalX = N->x;
	normalY = N->y;
							
	/* compute NxV */
	dotProduct = normalX * x;
	dotProduct += normalY * y;
	
	/* compute 2(NxV) */
	dotProduct += dotProduct;
	
	/* compute final vector */
	reflectedX = normalX * dotProduct - x;
	reflectedY = normalY * dotProduct - y;
	
	/* Normalize the result */
		
	FastNormalizeVector2D(reflectedX,reflectedY, theVector);
	
			/* SCALE TO ORIGINAL MAGNITUDE */
			
	theVector->x *= -mag;	
	theVector->y *= -mag;
}












