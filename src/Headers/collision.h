//
// collision.h
//
#pragma once

enum
{
	COLLISION_TYPE_OBJ,						// box
#if 0	// Source port removal: Bugdom only ever uses OBJ collisions
	COLLISION_TYPE_TRIANGLE,				// poly
	COLLISION_TYPE_TILE
#endif
};
							



					/* COLLISION STRUCTURES */
struct CollisionRec
{
	Byte			baseBox,targetBox;
	unsigned short	sides;
	Byte			type;
	ObjNode			*objectPtr;			// object that collides with (if object type)
	float			planeIntersectY;	// where intersected triangle
};
typedef struct CollisionRec CollisionRec;



//=================================


extern	Byte HandleCollisions(ObjNode *theNode, unsigned long	cType);
extern	ObjNode *IsPointInPickupCollisionSphere(TQ3Point3D *thePt);
extern	Boolean IsPointInPoly2D( float,  float ,  Byte ,  TQ3Point2D *);
extern	Boolean IsPointInTriangle(float pt_x, float pt_y, float x0, float y0, float x1, float y1, float x2, float y2);
extern	short DoSimplePointCollision(TQ3Point3D *thePoint, u_long cType);
extern	void DisposeCollisionTriangleMemory(ObjNode *theNode);
extern	void CreateCollisionTrianglesForObject(ObjNode *theNode);
extern	void DoTriangleCollision(ObjNode *theNode, unsigned long CType);
short DoSimpleBoxCollision(float top, float bottom, float left, float right,
						float front, float back, u_long cType);


Boolean HandleFloorAndCeilingCollision(TQ3Point3D *newCoord, const TQ3Point3D *oldCoord,
									TQ3Vector3D *newDelta, const TQ3Vector3D *oldDelta,
									float footYOff, float headYOff, float speed);

Boolean DoSimplePointCollisionAgainstPlayer(TQ3Point3D *thePoint);
Boolean DoSimpleBoxCollisionAgainstPlayer(float top, float bottom, float left, float right,
										float front, float back);
Boolean DoSimpleBoxCollisionAgainstObject(float top, float bottom, float left, float right,
										float front, float back, ObjNode *targetNode);
