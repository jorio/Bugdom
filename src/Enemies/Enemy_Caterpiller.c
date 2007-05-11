/****************************/
/*   ENEMY: CATERPILLER.C			*/
/* (c)1998 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode					*gCurrentNode;
extern	SplineDefType			**gSplineList;
extern	TQ3Point3D				gCoord,gMyCoord;
extern	short					gNumEnemies;
extern	float					gFramesPerSecondFrac;
extern	TQ3Vector3D			gDelta;
extern	signed char			gNumEnemyOfKind[];
extern	TQ3Vector3D		gRecentTerrainNormal[];

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveCaterpillerOnSpline(ObjNode *theNode);
static void SetCaterpillerJointTransforms(ObjNode *theNode, long index);
static void SetCaterpillerCollisionInfo(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	CATERPILLER_HEALTH		1.0f		
#define	CATERPILLER_DAMAGE		0.2f

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
	SetObjectTransformMatrix(newObj);
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
long	index;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, CATERPILLER_SPEED);
	index = GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/
			
	if (isVisible)
	{
	
			/* UPDATE SKELETON & COLLISION INFO */
			
		SetCaterpillerJointTransforms(theNode, index);
	}
	else
	{
	}
}


/********************* SET CATERPILLER JOINT TRANSFORMS **********************/
//
// INPUT: index = index into spline's point list of object's base coord.
//

static void SetCaterpillerJointTransforms(ObjNode *theNode, long index)
{
SkeletonObjDataType	*skeleton = theNode->Skeleton;
long				jointNum;
long				numPointsInSpline;
SkeletonDefType		*skeletonDef;
TQ3Matrix4x4		*jointMatrix;
SplinePointType		*points;
SplineDefType		*splinePtr;
TQ3Matrix4x4		m,m2,m3;
const static	TQ3Vector3D up = {0,1,0};
float				scale;
CollisionBoxType *boxPtr;

	if (skeleton == nil)
		return;

	skeletonDef = skeleton->skeletonDefinition;

	splinePtr = &(*gSplineList)[theNode->SplineNum];			// point to the spline
	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline
	points = *splinePtr->pointList;								// point to point list

	boxPtr = theNode->CollisionBoxes;

			/***************************************/
			/* TRANSFORM EACH JOINT TO WORLD-SPACE */
			/***************************************/
		
	scale = CATERPILLER_SCALE;
			
	for (jointNum = 0; jointNum < skeletonDef->NumBones; jointNum++)		
	{
		TQ3Point3D	coord,prevCoord;
		
				/* GET COORDS OF THIS SEGMENT */
				
		coord.x = points[index].x;
		coord.z = points[index].z;
		coord.y = GetTerrainHeightAtCoord(coord.x, coord.z, FLOOR) + CATERPILLER_FOOT_OFFSET;


			/* UPDATE THIS COINCIDING COLLISION BOX */

		boxPtr[jointNum].left 	= coord.x - CATERPILLER_COLLISIONBOX_SIZE;
		boxPtr[jointNum].right 	= coord.x + CATERPILLER_COLLISIONBOX_SIZE;
		boxPtr[jointNum].top 	= coord.y + CATERPILLER_COLLISIONBOX_SIZE;
		boxPtr[jointNum].bottom = coord.y - CATERPILLER_COLLISIONBOX_SIZE;
		boxPtr[jointNum].front 	= coord.z + CATERPILLER_COLLISIONBOX_SIZE;
		boxPtr[jointNum].back 	= coord.z - CATERPILLER_COLLISIONBOX_SIZE;
		

			/* GET PREVIOUS SPLINE COORD FOR ROTATION CALCULATION */
				
		if (index >= 30)
		{
			prevCoord.x = points[index-30].x;
			prevCoord.z = points[index-30].z;
		}
		else
		{
			prevCoord.x = points[numPointsInSpline-30].x;
			prevCoord.z = points[numPointsInSpline-30].z;
		}
		prevCoord.y = GetTerrainHeightAtCoord(prevCoord.x, prevCoord.z, FLOOR) + CATERPILLER_FOOT_OFFSET;

	
				/* TRANSFORM JOINT'S MATRIX TO WORLD COORDS */
	
		jointMatrix = &skeleton->jointTransformMatrix[jointNum];		// get ptr to joint's xform matrix which was set during regular animation functions

		Q3Matrix4x4_SetScale(&m, scale, scale, scale);
		SetLookAtMatrix(&m2, &up, &prevCoord, &coord);

		MatrixMultiplyFast(&m, &m2, &m3);

		Q3Matrix4x4_SetTranslate(&m, coord.x, coord.y, coord.z);
		MatrixMultiplyFast(&m3, &m, jointMatrix);
				

				/* INC INDEX FOR NEXT SEGMENT */
				
		index -= 40;
		if (index < 0)
			index += numPointsInSpline;
			
		theNode->SpecialF[0] -= gFramesPerSecondFrac * .8f;
		scale += sin(theNode->SpecialF[0] + (float)jointNum*1.6f) * .2f;
			
	}

}

















