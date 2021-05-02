/****************************/
/*   ENEMY: SKIPPY.C		*/
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveSkippy(ObjNode *theNode);
static void MoveSkippy_Swimming(ObjNode *theNode);
static void  MoveSkippy_Death(ObjNode *theNode);
static void UpdateSkippy(ObjNode *theNode);
static void MoveSkippyOnSpline(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_SKIPPY				5

#define	SKIPPY_CHASE_RANGE		500.0f

#define SKIPPY_TURN_SPEED		4.5f
#define SKIPPY_SPLINE_SPEED		200.0f

#define	SKIPPY_HEALTH			.5f		
#define	SKIPPY_DAMAGE			0.05f
#define	SKIPPY_SCALE			.7f

#define	SKIPPY_Y				(WATER_Y + 6)

enum
{
	SKIPPY_ANIM_SWIM,
	SKIPPY_ANIM_DEATH
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	SpeedBoost	Flag[0]		
#define	RippleTimer	SpecialF[0]


/************************ ADD SKIPPY ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Skippy(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= MAX_ENEMIES)								// keep from getting absurd
		return(false);

				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_SKIPPY,x,z,SKIPPY_SCALE);
	if (newObj == nil)
		return(false);
	newObj->TerrainItemPtr = itemPtr;

	SetSkeletonAnim(newObj->Skeleton, SKIPPY_ANIM_SWIM);
	

				/* SET BETTER INFO */
			
	newObj->MoveCall 	= MoveSkippy;							// set move call
	newObj->Health 		= SKIPPY_HEALTH;
	newObj->Damage 		= SKIPPY_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_SKIPPY;
	newObj->CType		|= CTYPE_SPIKED;
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 30,-60,-50,50,50,-50);


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_SKIPPY]++;
	return(true);
}





/********************* MOVE SKIPPY **************************/

static void MoveSkippy(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveSkippy_Swimming,
					MoveSkippy_Death
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}


/********************** MOVE SKIPPY: SWIMMING ******************************/

static void  MoveSkippy_Swimming(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	r,s;


			/* MOVE TOWARD PLAYER */
			
	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, SKIPPY_TURN_SPEED, false);			
	r = theNode->Rot.y;
	
	
			/* SEE IF SPEED BOOST */
			
	if (theNode->SpeedBoost)
	{
		theNode->SpeedBoost = false;
		theNode->Speed = 400;	
	}
	
			/* CALC SPEED DELTAS */
			
	theNode->Speed -= fps * 500.0f;
	if (theNode->Speed < 0.0f)
		theNode->Speed  = 0.0;
	s = theNode->Speed;
	
	gDelta.x = -sin(r) * s;
	gDelta.z = -cos(r) * s;
	
	
			/* MOVE IT */
	
	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;	
	gCoord.y = SKIPPY_Y;


	
				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


			/* DONT ALLOW ON LAND */
			
	if (GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR) > WATER_Y)
	{
		gCoord = theNode->OldCoord;
		gDelta.x = gDelta.z = 0;	
	}

	UpdateSkippy(theNode);		
	
}


/********************** MOVE SKIPPY: DEATH  ******************************/

static void  MoveSkippy_Death(ObjNode *theNode)
{

	UpdateSkippy(theNode);
}



/***************** UPDATE SKIPPY ********************/

static void UpdateSkippy(ObjNode *theNode)
{
	UpdateEnemy(theNode);			

		/* ADD WATER RIPPLE */
			
	theNode->RippleTimer += gFramesPerSecondFrac;
	if (theNode->RippleTimer > .3f)
	{
		theNode->RippleTimer = 0;
		MakeRipple(gCoord.x, WATER_Y+2, gCoord.z, 2.0);
	}
}


#pragma mark -

/************************ PRIME SKIPPY ENEMY *************************/

Boolean PrimeEnemy_Skippy(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_SKIPPY,x,z, SKIPPY_SCALE);
	if (newObj == nil)
		return(false);
		
	DetachObject(newObj);									// detach this object from the linked list
		
	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;
	
	SetSkeletonAnim(newObj->Skeleton, SKIPPY_ANIM_SWIM);
	

				/* SET BETTER INFO */
			
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		= WATER_Y;			
	newObj->SplineMoveCall 	= MoveSkippyOnSpline;				// set move call
	newObj->Health 			= SKIPPY_HEALTH;
	newObj->Damage 			= SKIPPY_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_SKIPPY;
	newObj->CType			|= CTYPE_SPIKED;


	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 30,-60,-50,50,50,-50);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	AddToSplineObjectList(newObj);

	return(true);
}

/******************** MOVE SKIPPY ON SPLINE ***************************/
//
// NOTE: This is called for every Skippy on the spline in the entire level.
//		The isVisible flag determines if this particular one is in visible range.
//		It actually means the enemy is on an active supertile.
//

static void MoveSkippyOnSpline(ObjNode *theNode)
{
Boolean isVisible; 

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, SKIPPY_SPLINE_SPEED);
	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/
			
	if (isVisible)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);		


		UpdateObjectTransforms(theNode);																// update transforms
		CalcObjectBoxFromNode(theNode);																	// update collision box
		UpdateShadow(theNode);																			// update shadow		
		
				/* SEE IF CLOSE ENOUGH TO CHASE */
				
		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gMyCoord.x, gMyCoord.z) < SKIPPY_CHASE_RANGE)
		{
					/* REMOVE FROM SPLINE */
					
			DetachEnemyFromSpline(theNode, MoveSkippy);
		}		
	}
	
			/* NOT VISIBLE */
	else
	{
	}
}


#pragma mark -


/****************** KILL SKIPPY *********************/
//
// OUTPUT: true if ObjNode was deleted
//

Boolean KillSkippy(ObjNode *theNode)
{
		/* IF ON SPLINE, DETACH */
		
	DetachEnemyFromSpline(theNode, MoveSkippy);


			/* DEACTIVATE */
			
	theNode->TerrainItemPtr = nil;									// dont ever come back
	theNode->CType = CTYPE_MISC;
	
		/* DO DEATH ANIM */
			
	if (theNode->Skeleton->AnimNum != SKIPPY_ANIM_DEATH)
		SetSkeletonAnim(theNode->Skeleton, SKIPPY_ANIM_DEATH);
		
	theNode->Delta.x = theNode->Delta.y = theNode->Delta.z = 0;	
	
	return(false);
}










