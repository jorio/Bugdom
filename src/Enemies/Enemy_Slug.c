/****************************/
/*   ENEMY: SLUG.C			*/
/* By Brian Greenstone      */
/* (c)1998 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveSlugOnSpline(ObjNode *theNode);
static void SetSlugCollisionInfo(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	SLUG_HEALTH		1.0f		
#define	SLUG_DAMAGE		0.1f

#define	SLUG_SCALE		3.0f
#define SLUG_STRETCH	30

enum
{
	SLUG_ANIM_INCH,
	SLUG_ANIM_STILL
};

#define	SLUG_FOOT_OFFSET		0.0f
#define	SLUG_COLLISIONBOX_SIZE	40.0f


/*********************/
/*    VARIABLES      */
/*********************/


/************************ PRIME SLUG ENEMY *************************/

Boolean PrimeEnemy_Slug(long splineNum, SplineItemType *itemPtr)
{
SplineDefType*	spline;
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	spline = &(*gSplineList)[splineNum];
	placement = itemPtr->placement;
	GetCoordOnSpline(spline, placement, &x, &z);



				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_SLUG,x,z, SLUG_SCALE);
	if (newObj == nil)
		return(false);
		
	DetachObject(newObj);									// detach this object from the linked list
	
	Q3Matrix4x4_SetIdentity(&newObj->BaseTransformMatrix);	// we are going to do some manual transforms on the skeleton joints
	newObj->Skeleton->JointsAreGlobal = true;
		
	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;
	
	SetSkeletonAnim(newObj->Skeleton, SLUG_ANIM_INCH);

				/* SET BETTER INFO */
			
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= SLUG_FOOT_OFFSET;
	newObj->SplineMoveCall 	= MoveSlugOnSpline;				// set move call
	newObj->Health 			= SLUG_HEALTH;
	newObj->Damage 			= SLUG_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_SLUG;
	newObj->BoundingSphere.radius *= 1.5f;					// Source port fix: enlarge bounding sphere, otherwise tail gets culled early
	
				/* SET COLLISION INFO */

	newObj->CType			|= CTYPE_SPIKED|CTYPE_BLOCKCAMERA;
	SetSlugCollisionInfo(newObj);

	newObj->InitCoord = newObj->Coord;							// remember where started


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	AddToSplineObjectList(newObj);
	return(true);
}


/******************* SET SLUG COLLISION INFO ******************/

static void SetSlugCollisionInfo(ObjNode *theNode)
{
SkeletonObjDataType	*skeleton = theNode->Skeleton;
SkeletonDefType		*skeletonDef;

	skeletonDef = skeleton->skeletonDefinition;

	theNode->CBits = CBITS_TOUCHABLE;
	
		/* ALLOCATE MEMORY FOR THE COLLISION BOXES */
				
	AllocateCollisionBoxMemory(theNode, skeletonDef->NumBones);	
}


/******************** MOVE SLUG ON SPLINE ***************************/

static void MoveSlugOnSpline(ObjNode *theNode)
{
Boolean isVisible; 

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 90);

	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);

			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/

	if (isVisible)
	{
			/* UPDATE SKELETON & COLLISION INFO */

		SetCrawlingEnemyJointTransforms(theNode,
			SLUG_STRETCH,
			SLUG_FOOT_OFFSET, SLUG_COLLISIONBOX_SIZE,
			SLUG_SCALE, &theNode->SpecialF[0]);
	}
}


/****************** SET JOINT TRANSFORMS ON CRAWLING ENEMY ******************/
//
// Also used in Enemy_Caterpiller.c
//

void SetCrawlingEnemyJointTransforms(
	ObjNode* theNode,
	int splineIndexDelta,
	float footOffset,
	float collisionBoxSize,
	float undulateScale,
	float* undulatePhase)
{
	if (!theNode->Skeleton)
		return;

	const TQ3Vector3D up = { 0,1,0 };
	TQ3Matrix4x4 m, m2, m3;

	CollisionBoxType* boxPtr = theNode->CollisionBoxes;

	SplineDefType* spline = &(*gSplineList)[theNode->SplineNum];

	const float splinePlacementDelta = (float)splineIndexDelta / spline->numPoints;

	float splineWalk = theNode->SplinePlacement;
	TQ3Point3D thisJointPos;		// world-space coords of current joint (start at my head)
	TQ3Point3D nextJointPos;		// world-space coords of next joint, closer to my tail (for LookAt rotation calculation)

			/* PRIME SPLINE WALK - COMPUTE POSITION OF HEAD JOINT */

	GetCoordOnSpline(spline, splineWalk, &nextJointPos.x, &nextJointPos.z);
	nextJointPos.y = GetTerrainHeightAtCoord(nextJointPos.x, nextJointPos.z, FLOOR) - footOffset;

			/***************************************/
			/* TRANSFORM EACH JOINT TO WORLD-SPACE */
			/***************************************/

	for (int jointNum = 0; jointNum < theNode->Skeleton->skeletonDefinition->NumBones; jointNum++)
	{
				/* WALK SPLINE TOWARDS MY TAIL */

		thisJointPos = nextJointPos;			// previous iteration's next joint is now current joint

		splineWalk -= splinePlacementDelta;		// walk backwards on spline, towards my tail
		if (splineWalk < 0)
			splineWalk += 1.0f;

				/* COMPUTE POSITION OF NEXT JOINT */

		GetCoordOnSpline(spline, splineWalk, &nextJointPos.x, &nextJointPos.z);
		nextJointPos.y = GetTerrainHeightAtCoord(nextJointPos.x, nextJointPos.z, FLOOR) - footOffset;

				/* UPDATE THIS COINCIDING COLLISION BOX */

		boxPtr[jointNum].left 	= thisJointPos.x - collisionBoxSize;
		boxPtr[jointNum].right 	= thisJointPos.x + collisionBoxSize;
		boxPtr[jointNum].top 	= thisJointPos.y + collisionBoxSize;
		boxPtr[jointNum].bottom = thisJointPos.y - collisionBoxSize;
		boxPtr[jointNum].front 	= thisJointPos.z + collisionBoxSize;
		boxPtr[jointNum].back 	= thisJointPos.z - collisionBoxSize;

				/* TRANSFORM JOINT'S MATRIX TO WORLD COORDS */

		// get joint's transform matrix which was set during skeletal animation
		TQ3Matrix4x4* jointMatrix = &theNode->Skeleton->jointTransformMatrix[jointNum];

		Q3Matrix4x4_SetScale(&m, undulateScale, undulateScale, undulateScale);
		SetLookAtMatrix(&m2, &up, &nextJointPos, &thisJointPos);

		MatrixMultiplyFast(&m, &m2, &m3);

		Q3Matrix4x4_SetTranslate(&m, thisJointPos.x, thisJointPos.y, thisJointPos.z);
		MatrixMultiplyFast(&m3, &m, jointMatrix);

				/* UNDULATE */

		*undulatePhase -= gFramesPerSecondFrac * .8f;
		undulateScale += sin(*undulatePhase + (float)jointNum*1.6f) * .3f;
	}
}

