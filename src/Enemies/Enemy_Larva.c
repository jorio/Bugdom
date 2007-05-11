/****************************/
/*   ENEMY: LARVA.C			*/
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode					*gCurrentNode;
extern	SplineDefType			**gSplineList;
extern	TQ3Point3D				gCoord,gMyCoord;
extern	short					gNumEnemies;
extern	float					gFramesPerSecondFrac;
extern	TQ3Vector3D			gDelta;
extern	signed char			gNumEnemyOfKind[];
extern	ObjNode				*gPlayerObj;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveLarva(ObjNode *theNode);
static void MoveLarvaOnSpline(ObjNode *theNode);
static void  MoveLarva_Walk(ObjNode *theNode);
static void  MoveLarva_Wait(ObjNode *theNode);
static void  MoveLarva_Squished(ObjNode *theNode);
static void  MoveLarva_Dead(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_LARVA				5

#define	LARVA_CHASE_RANGE		700.0f

#define LARVA_TURN_SPEED		3.0f
#define LARVA_CHASE_SPEED		100.0f
#define	LARVA_CHASE_RANGE2		80.0f
#define LARVA_SPLINE_SPEED		70.0f

#define	MAX_LARVA_RANGE			(LARVA_CHASE_RANGE+1000.0f)	// max distance this enemy can go from init coord

#define	LARVA_HEALTH			1.0f		
#define	LARVA_DAMAGE			0.1f
#define	LARVA_SCALE				.5f


enum
{
	LARVA_ANIM_WAIT,
	LARVA_ANIM_WALK,
	LARVA_ANIM_SQUISHED,
	LARVA_ANIM_DEAD
};


/*********************/
/*    VARIABLES      */
/*********************/


/************************ ADD LARVA ENEMY *************************/

Boolean AddEnemy_Larva(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= MAX_ENEMIES)								// keep from getting absurd
		return(false);

	if (gNumEnemyOfKind[ENEMY_KIND_LARVA] >= MAX_LARVA)			
		return(false);


				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_LARVA,x,z,LARVA_SCALE);
	if (newObj == nil)
		return(false);
	newObj->TerrainItemPtr = itemPtr;
	

				/* SET BETTER INFO */
			
	newObj->MoveCall 	= MoveLarva;							// set move call
	newObj->Health 		= LARVA_HEALTH;
	newObj->Damage 		= LARVA_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_LARVA;
	
	
				/* SET COLLISION INFO */
				
	newObj->CType		|= CTYPE_AUTOTARGET|CTYPE_AUTOTARGETJUMP|CTYPE_BOPPABLE|CTYPE_SPIKED|CTYPE_KICKABLE; 
	SetObjectCollisionBounds(newObj, 70,0,-40,40,40,-40);


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_LARVA]++;
	return(true);
}


/************************ MAKE LARVA ENEMY *************************/

ObjNode *MakeLarvaEnemy(float x, float z)
{
ObjNode	*newObj;

	if (gNumEnemyOfKind[ENEMY_KIND_LARVA] >= 5)					// only allow n
		return(nil);


				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_LARVA,x,z,LARVA_SCALE);
	if (newObj == nil)
		return(nil);	

				/* SET BETTER INFO */
			
	newObj->MoveCall 	= MoveLarva;							// set move call
	newObj->Health 		= LARVA_HEALTH;
	newObj->Damage 		= LARVA_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_LARVA;
	
	newObj->Rot.y 		= RandomFloat()*PI2;					// random aim
	
				/* SET COLLISION INFO */
				
	newObj->CType		|= CTYPE_AUTOTARGET|CTYPE_AUTOTARGETJUMP|CTYPE_BOPPABLE|CTYPE_SPIKED|CTYPE_KICKABLE; 
	SetObjectCollisionBounds(newObj, 70,0,-40,40,40,-40);


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_LARVA]++;
	return(newObj);
}



/********************* MOVE LARVA **************************/

static void MoveLarva(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveLarva_Wait,
					MoveLarva_Walk,
					MoveLarva_Squished,
					MoveLarva_Dead
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}


/********************** MOVE LARVA: WALK ******************************/

static void  MoveLarva_Walk(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	r,aim;

	theNode->Skeleton->AnimSpeed = 2.0f;

			/* MOVE TOWARD PLAYER */
			
	aim = TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, LARVA_TURN_SPEED, false);			

	r = theNode->Rot.y;
	gDelta.x = sin(r) * -LARVA_CHASE_SPEED;
	gDelta.z = cos(r) * -LARVA_CHASE_SPEED;
	
	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR);	// calc y coord

			

				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

	UpdateEnemy(theNode);		
	
}




/********************** MOVE LARVA: WAIT ******************************/

static void  MoveLarva_Wait(ObjNode *theNode)
{

	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, LARVA_TURN_SPEED, false);			
	
		/* SEE IF CLOSE ENOUGH TO ATTACK */
		
	if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < LARVA_CHASE_RANGE)
	{
		MorphToSkeletonAnim(theNode->Skeleton, LARVA_ANIM_WALK, 7);	
	}

				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

	UpdateEnemy(theNode);		
	
}


/********************** MOVE LARVA: SQUISHED ******************************/

static void  MoveLarva_Squished(ObjNode *theNode)
{
	UpdateEnemy(theNode);		
	
}

/********************** MOVE LARVA: DEAD ******************************/

static void  MoveLarva_Dead(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ISCULLED)		// if was culled on last frame and is far enough away, then delete it
	{
		DeleteEnemy(theNode);
		return;
	}
}



#pragma mark -

/************************ PRIME LARVA ENEMY *************************/

Boolean PrimeEnemy_Larva(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_LARVA,x,z, LARVA_SCALE);
	if (newObj == nil)
		return(false);
		
	DetachObject(newObj);									// detach this object from the linked list
		
	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;
	
	SetSkeletonAnim(newObj->Skeleton, LARVA_ANIM_WALK);
	

				/* SET BETTER INFO */
			
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveLarvaOnSpline;				// set move call
	newObj->Health 			= LARVA_HEALTH;
	newObj->Damage 			= LARVA_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_LARVA;
	newObj->CType			|= CTYPE_AUTOTARGET|CTYPE_AUTOTARGETJUMP|CTYPE_BOPPABLE|CTYPE_SPIKED; 

	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 70,0,-40,40,40,-40);



			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	AddToSplineObjectList(newObj);

	return(true);
}

/******************** MOVE LARVA ON SPLINE ***************************/
//
// NOTE: This is called for every Larva on the spline in the entire level.
//		The isVisible flag determines if this particular one is in visible range.
//		It actually means the enemy is on an active supertile.
//

static void MoveLarvaOnSpline(ObjNode *theNode)
{
Boolean isVisible; 


	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, LARVA_SPLINE_SPEED);
	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/
			
	if (isVisible)
	{
		theNode->Skeleton->AnimSpeed = 2.0f;
	
			/* START/UPDATE BUZZ */
	
		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect3D(EFFECT_BUZZ, &theNode->Coord);
		else
			Update3DSoundChannel(EFFECT_BUZZ, &theNode->EffectChannel, &theNode->Coord);
	
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);		

		theNode->Coord.y = GetTerrainHeightAtCoord(theNode->Coord.x, theNode->Coord.z, FLOOR);	// calc y coord
		UpdateObjectTransforms(theNode);																// update transforms
		CalcObjectBoxFromNode(theNode);																	// update collision box
		
				/*********************************/
				/* SEE IF CLOSE ENOUGH TO ATTACK */
				/*********************************/
				
		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gMyCoord.x, gMyCoord.z) < LARVA_CHASE_RANGE2)
		{
					/* REMOVE FROM SPLINE */
					
			DetachEnemyFromSpline(theNode, MoveLarva);
		}		
	}
	
			/* NOT VISIBLE */
	else
	{
		StopObjectStreamEffect(theNode);
	}
}


#pragma mark -

/****************** LARVA GOT BOPPED ************************/
//
// Called during player collision handler.
//

void LarvaGotBopped(ObjNode *enemy)
{
		/* IF ON SPLINE, DETACH */
		
	DetachEnemyFromSpline(enemy, MoveLarva);
	
	SetSkeletonAnim(enemy->Skeleton, LARVA_ANIM_SQUISHED);
	
		/* FLATTEN */
		
	enemy->Scale.y *= .2f;	
	UpdateObjectTransforms(enemy);	
	
	enemy->CType = 0;
}

/******************* KILL LARVA **********************/

Boolean KillLarva(ObjNode *theNode)
{
	theNode->TerrainItemPtr = nil;				// dont ever come back
	MorphToSkeletonAnim(theNode->Skeleton, LARVA_ANIM_DEAD, 3.0);
	theNode->CType = CTYPE_MISC;
	return(false);
}












