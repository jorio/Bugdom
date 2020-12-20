//
// collision.h
//
#pragma once

					/* COLLISION STRUCTURES */
struct CollisionRec
{
	Byte			baseBox,targetBox;
	unsigned short	sides;
	ObjNode			*objectPtr;			// object that collides with
};
typedef struct CollisionRec CollisionRec;



//=================================


extern	Byte HandleCollisions(ObjNode *theNode, unsigned long	cType);
extern	Boolean IsPointInTriangle(float pt_x, float pt_y, float x0, float y0, float x1, float y1, float x2, float y2);
extern	short DoSimplePointCollision(TQ3Point3D *thePoint, u_long cType);
short DoSimpleBoxCollision(float top, float bottom, float left, float right,
						float front, float back, u_long cType);


Boolean HandleFloorAndCeilingCollision(TQ3Point3D *newCoord, const TQ3Point3D *oldCoord,
									TQ3Vector3D *newDelta, const TQ3Vector3D *oldDelta,
									float footYOff, float headYOff, float speed);

Boolean DoSimpleBoxCollisionAgainstPlayer(float top, float bottom, float left, float right,
										float front, float back);
Boolean DoSimpleBoxCollisionAgainstObject(float top, float bottom, float left, float right,
										float front, float back, ObjNode *targetNode);
