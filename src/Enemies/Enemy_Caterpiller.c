/****************************/
/*   ENEMY: CATERPILLER.C			*/
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

static void MoveCaterpillerOnSpline(ObjNode *theNode);
static void SetCaterpillerCollisionInfo(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	CATERPILLER_HEALTH		1.0f		
#define	CATERPILLER_DAMAGE		0.2f

#define	CATERPILLER_STRETCH		40
#define	CATERPILLER_SCALE		3.0f
#define	CATERPILLER_SPEED		45.0f

enum
{
	CATERPILLER_ANIM_INCH,
	CATERPILLER_ANIM_STILL
};

#define	CATERPILLER_FOOT_OFFSET			30.0f

#define	CATERPILLER_COLLISIONBOX_SIZE	90.0f


/*********************/
/*    VARIABLES      */
/*********************/


/************************ PRIME CATERPILLER ENEMY *************************/

Boolean PrimeEnemy_Caterpiller(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_CATERPILLER,x,z, CATERPILLER_SCALE);
	if (newObj == nil)
		return(false);
		
	DetachObject(newObj);									// detach this object from the linked list
	
	Q3Matrix4x4_SetIdentity(&newObj->BaseTransformMatrix);	// we are going to do some manual transforms on the skeleton joints
	newObj->Skeleton->JointsAreGlobal = true;
		
	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;
	
	SetSkeletonAnim(newObj->Skeleton, CATERPILLER_ANIM_INCH);

				/* SET BETTER INFO */
			
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		+= CATERPILLER_FOOT_OFFSET;			
	newObj->SplineMoveCall 	= MoveCaterpillerOnSpline;				// set move call
	newObj->Health 			= CATERPILLER_HEALTH;
	newObj->Damage 			= CATERPILLER_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_CATERPILLER;
	newObj->BoundingSphere.radius *= 1.5f;						// Source port fix: enlarge bounding sphere, otherwise tail gets culled early
	
				/* SET COLLISION INFO */

	newObj->CType			|= CTYPE_SPIKED|CTYPE_BLOCKCAMERA;
	SetCaterpillerCollisionInfo(newObj);

	newObj->InitCoord = newObj->Coord;							// remember where started


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	AddToSplineObjectList(newObj);
	return(true);
}


/******************* SET CATERPILLER COLLISION INFO ******************/

static void SetCaterpillerCollisionInfo(ObjNode *theNode)
{
SkeletonObjDataType	*skeleton = theNode->Skeleton;
SkeletonDefType		*skeletonDef;

	skeletonDef = skeleton->skeletonDefinition;

	theNode->CBits = CBITS_TOUCHABLE;
	
		/* ALLOCATE MEMORY FOR THE COLLISION BOXES */
				
	AllocateCollisionBoxMemory(theNode, skeletonDef->NumBones);	
}


/******************** MOVE CATERPILLER ON SPLINE ***************************/

static void MoveCaterpillerOnSpline(ObjNode *theNode)
{
Boolean isVisible; 

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, CATERPILLER_SPEED);
	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/
			
	if (isVisible)
	{
	
			/* UPDATE SKELETON & COLLISION INFO */
			
		SetCrawlingEnemyJointTransforms(theNode,
			CATERPILLER_STRETCH,
			CATERPILLER_FOOT_OFFSET, CATERPILLER_COLLISIONBOX_SIZE,
			CATERPILLER_SCALE, &theNode->SpecialF[0]);
	}
}

