/****************************/
/*   ENEMY: ANT.C			*/
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

static void MoveAnt(ObjNode *theNode);
static void MoveAnt_Standing(ObjNode *theNode);
static void  MoveAnt_Throw(ObjNode *theNode);
static void  MoveAnt_Walking(ObjNode *theNode);
static void  MoveAnt_PickUp(ObjNode *theNode);
static void  MoveAnt_FallOnButt(ObjNode *theNode);
static void  MoveAnt_GetOffButt(ObjNode *theNode);
static void  MoveAnt_Death(ObjNode *theNode);
static void  MoveAnt_ThrowRock(ObjNode *theNode);

static void MoveAntOnSpline(ObjNode *theNode);
static void GiveAntASpear(ObjNode *theNode);
static void UpdateAntSpear(ObjNode *theEnemy);
static void AntThrowSpear(ObjNode *theEnemy);
static void MoveAntSpear(ObjNode *theNode);
static void  MoveAnt_PickupRock(ObjNode *theNode);

static void MakeGhostAnt(ObjNode *deadAnt, TQ3Point3D *where);
static ObjNode *MakeAntObject(float x, float z, Boolean hasShadow, Boolean rockThrower);

static void UpdateAnt(ObjNode *theNode);
static void GiveAntARock(ObjNode *theNode);
static void UpdateAntRock(ObjNode *theEnemy);
static void AntThrowRock(ObjNode *theEnemy);
static void MoveAntRock(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_ANTS				5						// max # allowed active at once

#define	ANT_SCALE				1.4f

#define	ANT_TARGET_OFFSET		200.0f


#define ANT_TURN_SPEED			2.4f
#define ANT_WALK_SPEED			400.0f
#define	ANT_KNOCKDOWN_SPEED		1400.0f					// speed ball needs to go to knock this down

#define	ANT_DAMAGE				0.1f

#define	ANT_FOOT_OFFSET			-150.0f
#define	ANT_HEAD_OFFSET			80.0f	

#define	ANT_LEAVE_SPLINE_DIST	600.0f
#define	ANT_AGGRESSIVE_STOP_DIST (ANT_LEAVE_SPLINE_DIST-200.0f)




		/* SPEAR STUFF */

#define	ANT_HOLDING_LIMB		4						// joint # of limb holding an item		
#define	ANT_HEAD_LIMB			1						// joint # of head

#define	SPEAR_SCALE					1.0f
#define	SPEAR_ATTACK_DIST			900.0f
#define	SPEAR_THROW_MIN_ANGLE		0.03f
#define	DIST_TO_RETRIVE_SPEAR		200.0f
#define SPEAR_DAMAGE				.12f



		/* MODES */
		
enum
{
	ANT_MODE_NONE,
	ANT_MODE_WATCHSPEAR,
	ANT_MODE_GETSPEAR
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	ThrowSpear		Flag[0]					// set by animation when spear should be thrown
#define PickUpNow		Flag[0]					// set by anim when pickup should occur
#define	HasSpear		Flag[1]					// true if this guy has a spear
#define Dying			Flag[2]					// set during butt fall to indicate death after fall
#define	Aggressive		Flag[3]					// set if ant should walk after player
#define	RockThrower		Flag[5]		

#define	ThrownSpear			SpecialPtr[0]		// objnode of thrown spear
#define	ButtTimer			SpecialF[0]			// timer for on butt
#define	DeathTimer			SpecialF[1]			// amount of time has been dead
#define	MadeGhost			Flag[4]				// true after ghost has been made



		/* SPEAR */
		
#define SpearOwner			SpecialPtr[0]		// objnode of ant who threw spear
#define	SpearIsInGround		Flag[0]				// set when spear is stuck in ground



/********************** IS THROWN SPEAR VALID *********************/

static Boolean IsThrownSpearValid(ObjNode* owner, ObjNode* spearObj)
{
	return spearObj
		&& (spearObj->CType != INVALID_NODE_FLAG)
		&& (spearObj->SpearOwner == owner);
}


/************************ ADD ANT ENEMY *************************/
//
// parm[0] = attack type:  0 = spear, 1 = rock
// parm[3]: bit0 = aggressive, walk after player
//

Boolean AddEnemy_Ant(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;
Boolean	rockThrower;

	if (gNumEnemies >= MAX_ENEMIES)								// keep from getting absurd
		return(false);
	if (gNumEnemyOfKind[ENEMY_KIND_ANT] >= MAX_ANTS)
		return(false);


			/* MAKE ANT */
			
	rockThrower = itemPtr->parm[0] == 1;						// see if rock thrower
			
	newObj = MakeAntObject(x, z, true, rockThrower);
	newObj->TerrainItemPtr = itemPtr;

	newObj->RockThrower = rockThrower;
	newObj->Aggressive = itemPtr->parm[3] & 1;					// see if aggressive
	
	return(true);
}


/***************** MAKE GHOST ANT *******************/

static void MakeGhostAnt(ObjNode *deadAnt, TQ3Point3D *where)
{
ObjNode	*ghost;

				/* MAKE NEW ENEMY */
				
	ghost = MakeAntObject(where->x, where->z, false, deadAnt->RockThrower);


				/* MAKE GHOSTLY GLOW */
				
	ghost->StatusBits |= STATUS_BIT_GLOW|STATUS_BIT_NOFOG|STATUS_BIT_NOZWRITE|STATUS_BIT_NOTRICACHE;


				/* MATCH PARAMETERS FROM DEAD ANT */

	ghost->Aggressive = deadAnt->Aggressive;
	ghost->RockThrower = deadAnt->RockThrower;
				
	SetSkeletonAnim(ghost->Skeleton, deadAnt->Skeleton->AnimNum);
	ghost->Skeleton->CurrentAnimTime = deadAnt->Skeleton->CurrentAnimTime;
	ghost->Skeleton->LoopBackTime = deadAnt->Skeleton->LoopBackTime;
	ghost->Skeleton->AnimDirection = deadAnt->Skeleton->AnimDirection;
	UpdateSkeletonAnimation(ghost);			// call this to set data

	ghost->Coord = *where;
	ghost->CType = 0;						// nothing can touch these



				/* SEND INTO STAND */
				
	MorphToSkeletonAnim(ghost->Skeleton, ANT_ANIM_STAND, 4);

	UpdateObjectTransforms(ghost);
}


/***************** MAKE ANT OBJECT *******************/

static ObjNode *MakeAntObject(float x, float z, Boolean hasShadow, Boolean rockThrower)
{
ObjNode	*newObj;

				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_ANT,x,z, ANT_SCALE);
	GAME_ASSERT(newObj);

	newObj->HasSpear = false;									// assume no spear
	

				/*******************/
				/* SET BETTER INFO */
				/*******************/
			
	newObj->Coord.y 	-= ANT_FOOT_OFFSET;			
	newObj->MoveCall 	= MoveAnt;								// set move call
	newObj->Health 		= 1.0;
	newObj->Damage 		= ANT_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_ANT;
	newObj->CType		|= CTYPE_KICKABLE|CTYPE_AUTOTARGET;		// these can be kicked
	newObj->CBits		= CBITS_NOTTOP;
	newObj->Mode		= ANT_MODE_NONE;
	
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, ANT_HEAD_OFFSET,ANT_FOOT_OFFSET,-70,70,70,-70);
	CalcNewTargetOffsets(newObj,ANT_TARGET_OFFSET);


				/* MAKE SHADOW */
				
	if (hasShadow)
		AttachShadowToObject(newObj, 8, 8, false);
	
	
			/* GIVE ANT THE SPEAR */

	if (!rockThrower)			
		GiveAntASpear(newObj);
	
	
	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_ANT]++;

	return(newObj);
}

#pragma mark -


/********************* MOVE ANT **************************/

static void MoveAnt(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveAnt_Standing,
					MoveAnt_Walking,
					MoveAnt_Throw,
					MoveAnt_PickUp,
					MoveAnt_FallOnButt,
					MoveAnt_GetOffButt,
					MoveAnt_Death,
					nil,
					MoveAnt_PickupRock,
					MoveAnt_ThrowRock		
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
	
}


/********************** MOVE ANT: STANDING ******************************/

static void  MoveAnt_Standing(ObjNode *theNode)
{
float	angleToTarget;
ObjNode	*spearObj;

	switch(theNode->Mode)
	{
				/***********************************/
				/* HANDLE WAITING FOR SPEAR TO HIT */
				/***********************************/				
				
		case	ANT_MODE_WATCHSPEAR:
		
						/* VERIFY SPEAR */
						
				spearObj = (ObjNode *)theNode->ThrownSpear;												// get objnode of spear
				if (!IsThrownSpearValid(theNode, spearObj))													// see if isnt valid anymore
				{
					theNode->Mode = ANT_MODE_NONE;
					theNode->ThrownSpear = nil;
					break;				
				}
						
					/* SEE IF SPEAR HAS IMPACTED */
					
				if (spearObj->SpearIsInGround)
				{
					theNode->Mode = ANT_MODE_GETSPEAR;
					SetSkeletonAnim(theNode->Skeleton, ANT_ANIM_WALK);
				}
				break;
	
	
				/**************************/
				/* HANDLE ALL OTHER MODES */
				/**************************/
				
		default:
							/* TURN TOWARDS ME */
							
				angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, ANT_TURN_SPEED, false);			


							/*****************/
							/* SEE IF ATTACK */
							/*****************/

						/* SEE IF THROW ROCK */
						
				if (theNode->RockThrower)
				{
					if (angleToTarget < SPEAR_THROW_MIN_ANGLE)
					{
						if (CalcQuickDistance(gMyCoord.x, gMyCoord.z, gCoord.x, gCoord.z) < SPEAR_ATTACK_DIST)
						{					
							MorphToSkeletonAnim(theNode->Skeleton, ANT_ANIM_PICKUPROCK, 5);
							theNode->ThrowSpear = false;
						}
					}
				}
				
						/* SEE IF THROW SPEAR */
				else
				if (theNode->HasSpear)
				{
					if (angleToTarget < SPEAR_THROW_MIN_ANGLE)
					{
						if (CalcQuickDistance(gMyCoord.x, gMyCoord.z, gCoord.x, gCoord.z) < SPEAR_ATTACK_DIST)
						{					
							SetSkeletonAnim(theNode->Skeleton, ANT_ANIM_THROWSPEAR);
							theNode->ThrowSpear = false;
						}
					}
				}
	}			

				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;
		
				/* SEE IF IN LIQUID */
				
	if (theNode->StatusBits & STATUS_BIT_UNDERWATER)
	{
		KillAnt(theNode);
	}

	UpdateAnt(theNode);
}

/********************** MOVE ANT: WALKING ******************************/

static void  MoveAnt_Walking(ObjNode *theNode)
{
float		r,fps;
ObjNode		*spearObj;

	fps = gFramesPerSecondFrac;
		
	theNode->Skeleton->AnimSpeed = ANT_WALK_SPEED * .0032f;
	

	switch(theNode->Mode)
	{
				/***************************/
				/* HANDLE RETREIVING SPEAR */
				/***************************/				
				
		case	ANT_MODE_GETSPEAR:
		
						/* VERIFY SPEAR */
						
				spearObj = (ObjNode *)theNode->ThrownSpear;												// get objnode of spear
				if (!IsThrownSpearValid(theNode, spearObj))													// see if isnt valid anymore
				{
					theNode->ThrownSpear = nil;
					theNode->Mode = ANT_MODE_NONE;
					break;				
				}
		
						/* MOVE TOWARD SPEAR */
						
				TurnObjectTowardTarget(theNode, &gCoord, spearObj->Coord.x, spearObj->Coord.z,
											ANT_TURN_SPEED, false);			
		
				r = theNode->Rot.y;
				gDelta.x = -sin(r) * ANT_WALK_SPEED;
				gDelta.z = -cos(r) * ANT_WALK_SPEED;
				gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity

				MoveEnemy(theNode);
		
		
						/* SEE IF GET SPEAR */
						
				if (CalcQuickDistance(gCoord.x, gCoord.z, spearObj->Coord.x, spearObj->Coord.z) < DIST_TO_RETRIVE_SPEAR)
				{
					theNode->Mode = ANT_MODE_NONE;
					theNode->PickUpNow = false;
					SetSkeletonAnim(theNode->Skeleton, ANT_ANIM_PICKUP);
				}
		
				break;
				
				
			/********************************/
			/* HANDLE NORMAL WALKING AROUND */
			/********************************/				
				
		default:
						/* MOVE TOWARD PLAYER */
						
				TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, ANT_TURN_SPEED, false);			

				r = theNode->Rot.y;
				gDelta.x = -sin(r) * ANT_WALK_SPEED;
				gDelta.z = -cos(r) * ANT_WALK_SPEED;
				gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
				MoveEnemy(theNode);


						/* SEE IF CLOSE ENOUGH TO ATTACK */
						
				if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < ANT_AGGRESSIVE_STOP_DIST)
				{
					MorphToSkeletonAnim(theNode->Skeleton, ANT_ANIM_STAND, 2.0);				
				}
	}
	
	
				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;
	
				/* SEE IF IN LIQUID */
				
	if (theNode->StatusBits & STATUS_BIT_UNDERWATER)
	{
		KillAnt(theNode);
	}
	
	UpdateAnt(theNode);
}


/********************** MOVE ANT: THROW ******************************/

static void  MoveAnt_Throw(ObjNode *theNode)
{
float	angleToTarget;

	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, ANT_TURN_SPEED, false);			

			/***************************/
			/* SEE IF LAUNCH THE SPEAR */
			/***************************/
			
	if (theNode->ThrowSpear)
	{
		theNode->ThrowSpear = false;
	
				/* MAKE SURE STILL AIMED AT ME */
					
		if (angleToTarget < SPEAR_THROW_MIN_ANGLE)
			AntThrowSpear(theNode);
	}

	
			/* SEE IF DONE WITH ANIM */
			
	if (theNode->Skeleton->AnimHasStopped)
	{
		SetSkeletonAnim(theNode->Skeleton, ANT_ANIM_STAND);	
	}


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

	UpdateAnt(theNode);
}



/********************** MOVE ANT: PICKUP ******************************/

static void  MoveAnt_PickUp(ObjNode *theNode)
{
ObjNode	*spearObj;

	gDelta.x = gDelta.z = 0;						// make sure not moving during this anim

			/************************/
			/* SEE IF GET THE SPEAR */
			/************************/
			
	if (theNode->PickUpNow)
	{
		theNode->PickUpNow = false;
		
				/* VERIFY SPEAR */
				
		spearObj = (ObjNode *)theNode->ThrownSpear;												// get objnode of spear
		if (IsThrownSpearValid(theNode, spearObj))													// make sure spear obj is valid
		{
			DeleteObject(spearObj);																// delete the old spear
			theNode->ThrownSpear = nil;
		}

		GiveAntASpear(theNode);																// give it a new spear (even if old is invalid)
	}

	
			/* SEE IF DONE WITH ANIM */
			
	if (theNode->Skeleton->AnimHasStopped)
	{
		if (theNode->Aggressive)
		{
			SetSkeletonAnim(theNode->Skeleton, ANT_ANIM_WALK);	
			theNode->Mode = ANT_MODE_NONE;
		}
		else
			SetSkeletonAnim(theNode->Skeleton, ANT_ANIM_STAND);	
	}


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

	UpdateAnt(theNode);
}


/********************** MOVE ANT: FALLONBUTT ******************************/

static void  MoveAnt_FallOnButt(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(140.0,&gDelta);
		
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity

	MoveEnemy(theNode);


				/* SEE IF DONE */
			
	theNode->ButtTimer -= gFramesPerSecondFrac;
	if (theNode->ButtTimer <= 0.0)
	{
		if (theNode->Dying)									// see if die now
		{
			SetSkeletonAnim(theNode->Skeleton, ANT_ANIM_DIE);
			theNode->DeathTimer = 0;
			theNode->MadeGhost = false;			
			goto update;
		}
		else
			SetSkeletonAnim(theNode->Skeleton, ANT_ANIM_GETOFFBUTT);
	}


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


update:
	UpdateAnt(theNode);
}


/********************** MOVE ANT: DEATH ******************************/

static void  MoveAnt_Death(ObjNode *theNode)
{	
float	fps = gFramesPerSecondFrac;

			/***************/
			/* SEE IF GONE */
			/***************/
			
	if (theNode->StatusBits & STATUS_BIT_ISCULLED)		// if was culled on last frame and is far enough away, then delete it
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) > 600.0f)
		{
			DeleteEnemy(theNode);
			return;
		}
	}

				/***********/
				/* MOVE IT */
				/***********/
				
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)		// if on ground, add friction
		ApplyFrictionToDeltas(60.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*fps;						// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode, DEATH_ENEMY_COLLISION_CTYPES | CTYPE_LIQUID))
		return;

				/*********************/
				/* SEE IF MAKE GHOST */
				/*********************/

	if (gLevelType == LEVEL_TYPE_ANTHILL
		&& !theNode->MadeGhost
		&& !(theNode->StatusBits & STATUS_BIT_UNDERWATER))		// avoid endless respawn if underwater (ghosts also die in water)
	{
		theNode->DeathTimer += fps;
		if (theNode->DeathTimer > .5f)
		{
			theNode->MadeGhost = true;
			MakeGhostAnt(theNode,&gCoord);
		}
	}

				/* UPDATE */
			
	UpdateAnt(theNode);

}


/********************** MOVE ANT: GET OFF BUTT ******************************/

static void  MoveAnt_GetOffButt(ObjNode *theNode)
{
	ApplyFrictionToDeltas(60.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity

	MoveEnemy(theNode);


				/* SEE IF DONE */
			
	if (theNode->Skeleton->AnimHasStopped)
	{
		theNode->TopOff = ANT_HEAD_OFFSET;

		switch(theNode->Mode)
		{
			case	ANT_MODE_GETSPEAR:
					SetSkeletonAnim(theNode->Skeleton, ANT_ANIM_WALK);
					break;

			case	ANT_MODE_WATCHSPEAR:
					theNode->Mode = ANT_MODE_GETSPEAR;
					SetSkeletonAnim(theNode->Skeleton, ANT_ANIM_WALK);
					break;
					
			default:
					SetSkeletonAnim(theNode->Skeleton, ANT_ANIM_STAND);
		}
	}


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


	UpdateAnt(theNode);
}


/********************** MOVE ANT: PICKUP ROCK ******************************/

static void  MoveAnt_PickupRock(ObjNode *theNode)
{
	gDelta.x = gDelta.z = 0;						// make sure not moving during this anim

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;
		
			/***********************/
			/* SEE IF GET THE ROCK */
			/***********************/
			
	if (theNode->PickUpNow)
	{
		theNode->PickUpNow = false;
		GiveAntARock(theNode);
	}

	
			/* SEE IF DONE WITH ANIM */
			
	if (theNode->Skeleton->AnimHasStopped)
	{
		SetSkeletonAnim(theNode->Skeleton, ANT_ANIM_THROWROCK);	
	}
		

	UpdateAnt(theNode);
}

/********************** MOVE ANT: THROW ROCK ******************************/

static void  MoveAnt_ThrowRock(ObjNode *theNode)
{

	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, ANT_TURN_SPEED, false);			

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


			/* SEE IF LAUNCH THE SPEAR */
			
	if (theNode->ThrowSpear)
	{
		theNode->ThrowSpear = false;
		AntThrowRock(theNode);
	}

	
			/* SEE IF DONE WITH ANIM */
			
	if (theNode->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(theNode->Skeleton, ANT_ANIM_STAND, 3);	


	UpdateAnt(theNode);
}





/***************** UPDATE ANT **********************/

static void UpdateAnt(ObjNode *theNode)
{
	UpdateEnemy(theNode);		
	
	if (theNode->RockThrower)
		UpdateAntRock(theNode);
	else
		UpdateAntSpear(theNode);	
}


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME ANT ENEMY *************************/

Boolean PrimeEnemy_Ant(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj,*shadowObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_ANT,x,z, ANT_SCALE);
	if (newObj == nil)
		return(false);
		
		
	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;
	
	SetSkeletonAnim(newObj->Skeleton, ANT_ANIM_WALK);
	
	newObj->HasSpear = false;									// assume no spear
	

				/* SET BETTER INFO */
			
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= ANT_FOOT_OFFSET;			
	newObj->SplineMoveCall 	= MoveAntOnSpline;				// set move call
	newObj->Health 			= 1.0;
	newObj->Damage 			= ANT_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_ANT;
	newObj->CType			|= CTYPE_KICKABLE|CTYPE_AUTOTARGET;	// these can be kicked & bopped
	newObj->CBits			= CBITS_NOTTOP;
	
	newObj->Mode		= ANT_MODE_NONE;
	newObj->Aggressive	= true;
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 70,ANT_FOOT_OFFSET,-70,70,70,-70);
	CalcNewTargetOffsets(newObj,ANT_TARGET_OFFSET);


	newObj->InitCoord = newObj->Coord;							// remember where started

				/* MAKE SHADOW */
				
	shadowObj = AttachShadowToObject(newObj, 8, 8, false);


			/* GIVE ANT THE SPEAR */
			
	GiveAntASpear(newObj);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	AddToSplineObjectList(newObj);


			/* DETACH FROM LINKED LIST */
			
	DetachObject(newObj);										// detach enemy
	DetachObject(newObj->ChainNode);							// detach spear (if any)
	DetachObject(shadowObj);									// detach shadow

	return(true);
}


/******************** MOVE ANT ON SPLINE ***************************/

static void MoveAntOnSpline(ObjNode *theNode)
{
Boolean isVisible; 

	isVisible = IsSplineItemVisible(theNode);			// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 100);
	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/
			
	if (isVisible)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,	// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);		

		theNode->Coord.y = GetTerrainHeightAtCoord(theNode->Coord.x, theNode->Coord.z, FLOOR) - ANT_FOOT_OFFSET;	// calc y coord
		UpdateObjectTransforms(theNode);				// update transforms
		CalcObjectBoxFromNode(theNode);					// update collision box
		UpdateShadow(theNode);							// update shadow
		
		if (theNode->RockThrower)
			UpdateAntRock(theNode);
		else
			UpdateAntSpear(theNode);	
		
			/* SEE IF CLOSE ENOUGH TO DETACH FROM SPLINE */
		
		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gMyCoord.x, gMyCoord.z) < ANT_LEAVE_SPLINE_DIST)
		{
			DetachEnemyFromSpline(theNode, MoveAnt);
			theNode->Mode = ANT_MODE_NONE;
		}
		
	}
	
			/************************************/
			/* HIDE SOME THINGS SINCE INVISIBLE */
			/************************************/
	else
	{
//		if (theNode->ShadowNode)						// hide shadow
//			theNode->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;	
//			
//		if (theNode->HasSpear)							// hide spear
//			if (theNode->ChainNode)
//				theNode->ChainNode->StatusBits |= STATUS_BIT_HIDDEN;
	}
}


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================

#pragma mark -


/***************************** GIVE ANT A SPEAR *********************************/

static void GiveAntASpear(ObjNode *theNode)
{
ObjNode	*spearObj;

			/* MAKE SPEAR OBJECT */

	gNewObjectDefinition.group 		= GLOBAL1_MGroupNum_Spear;	
	gNewObjectDefinition.type 		= GLOBAL1_MObjType_Spear;	
	gNewObjectDefinition.coord		= theNode->Coord;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= theNode->Slot+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= SPEAR_SCALE;
	spearObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (spearObj == nil)
		return;

			/* ATTACH SPEAR TO ENEMY */
	
	theNode->ChainNode = spearObj;
	spearObj->ChainHead = theNode;

	theNode->HasSpear = true;
}


/************************* UPDATE ANT SPEAR ************************/
//
// Updated when is being held by ant.
//

static void UpdateAntSpear(ObjNode *theEnemy)
{
ObjNode					*spearObj;
TQ3Matrix4x4			m,m3;
static const TQ3Point3D	zero = {0,0,0};


			/* VERIFY */
			
	if (!theEnemy->HasSpear)
		return;
	if (!theEnemy->ChainNode)
		return;
		
	spearObj = theEnemy->ChainNode;

//	spearObj->StatusBits &= ~STATUS_BIT_HIDDEN;							// make sure not hidden
	
	
	Q3Matrix4x4_SetTranslate(&m3, 21, -80, -33);
	FindJointFullMatrix(theEnemy, ANT_HOLDING_LIMB, &m);
	MatrixMultiplyFast(&m3, &m, &spearObj->BaseTransformMatrix);


			/* SET REAL POINT FOR CULLING */
			
	Q3Point3D_Transform(&zero, &spearObj->BaseTransformMatrix, &spearObj->Coord);
}


/********************* THROW SPEAR ************************/

static void AntThrowSpear(ObjNode *theEnemy)
{
ObjNode		 			*spearObj;
static const TQ3Point3D zero = {0,0,0};
float					rot,speed;

	theEnemy->HasSpear = false;							// dont have it anymore

	spearObj = theEnemy->ChainNode;						// get spear obj
	if (spearObj == nil)
		return;


		/* SETUP NEW LINKS TO REMEMBER SPEAR */
		
	theEnemy->ThrownSpear = spearObj;			// remember the ObjNode to the spear so I can go get it
	spearObj->SpearOwner = theEnemy;			// remember node of ant


		/* DETACH FROM CHAIN */
		
	theEnemy->ChainNode = nil;
	spearObj->ChainHead = nil;
	spearObj->MoveCall = MoveAntSpear;


			/* CALC THROW START COORD */
			
	Q3Point3D_Transform(&zero, &spearObj->BaseTransformMatrix, &spearObj->Coord);
			
			
		/* CALC THROW VECTOR */
			
	speed = CalcQuickDistance(gMyCoord.x, gMyCoord.z, spearObj->Coord.x, spearObj->Coord.z) * 1.6f;
			
	spearObj->Rot.x = PI/2;
	spearObj->Rot.y = rot = theEnemy->Rot.y + 0.1f;		// get aim & offset a tad
	spearObj->Delta.x = -sin(rot) * speed;
	spearObj->Delta.z = -cos(rot) * speed;
	spearObj->Delta.y = 0;
	
	spearObj->SpearIsInGround = false;					// not in ground yet
	
	
			/* GIVE SPEAR COLLISION INFO */
			
	spearObj->CType  = CTYPE_HURTME;
	spearObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(spearObj, 30,-30,-30,30,30,-30);
	spearObj->Damage = SPEAR_DAMAGE;
			

	theEnemy->Mode = ANT_MODE_WATCHSPEAR;			// the ant is watching the spear go
	

			/* MAKE A SHADOW */
				
	AttachShadowToObject(spearObj, 1.7, 4.5, false);


			/* PLAY SOUND */

	PlayEffect3D(EFFECT_THROWSPEAR, &spearObj->Coord);
}



/******************* MOVE ANT SPEAR ****************/

static void MoveAntSpear(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	groundY;

			/* SEE IF ITS STILL AROUND */
			
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	if (theNode->SpearIsInGround)					// if stuck in ground, then dont need to do anything
		return;
		
	GetObjectInfo(theNode);
	
	
				/* MOVE IT */
				
	gDelta.y -= 1100.0f * fps;						// gravity	
	gCoord.x += gDelta.x * fps;						// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;
		
				/* AIM DOWN */
	
	if (theNode->Rot.x > -PI)
		theNode->Rot.x -= fps * .8f;


				/* SEE IF IMPACT GROUND */

	groundY	= GetTerrainHeightAtCoord(gCoord.x,gCoord.z,FLOOR);
	if (gCoord.y <= groundY)
	{
		gCoord.y = groundY;
		gDelta.x = 0;
		gDelta.y = 0;
		gDelta.z = 0;
		theNode->SpearIsInGround = true;
		
		theNode->CType = CTYPE_MISC;			// when  in ground it cant hurt me, its just solid
		
		PlayEffect3D(EFFECT_HITDIRT, &gCoord);
	}
			
	UpdateObject(theNode);
}


#pragma mark -

/******************* BALL HIT ANT *********************/
//
// OUTPUT: true if enemy gets killed
//

Boolean BallHitAnt(ObjNode *me, ObjNode *enemy)
{
Boolean	killed = false;

				/************************/
				/* HURT & KNOCK ON BUTT */
				/************************/
				
	if (me->Speed > ANT_KNOCKDOWN_SPEED)
	{	
				/*****************/
				/* KNOCK ON BUTT */
				/*****************/
					
		killed = KnockAntOnButt(enemy, me->Delta.x * .8f, me->Delta.y * .8f + 250.0f, me->Delta.z * .8f, .5);
		PlayEffect_Parms3D(EFFECT_POUND, &gCoord, kMiddleC+2, 2.0);
	}
	
	return(killed);
}


/****************** KNOCK ANT ON BUTT ********************/
//
// INPUT: dx,y,z = delta to apply to fireant 
//

Boolean KnockAntOnButt(ObjNode *enemy, float dx, float dy, float dz, float damage)
{
	if (enemy->Skeleton->AnimNum == ANT_ANIM_FALLONBUTT)		// see if already in butt mode
		return(false);
		
		/* IF ON SPLINE, DETACH */
		
	DetachEnemyFromSpline(enemy, MoveAnt);


		/* IF HAVE ROCK, NUKE IT */
	
	if (enemy->RockThrower)
	{
		if (enemy->ChainNode)
		{
			DeleteObject(enemy->ChainNode);
			enemy->ChainNode = nil;
		}
	}


			/* GET IT MOVING */
		
	enemy->Delta.x = dx;
	enemy->Delta.y = dy;
	enemy->Delta.z = dz;

	MorphToSkeletonAnim(enemy->Skeleton, ANT_ANIM_FALLONBUTT, 9.0);
	enemy->ButtTimer = 2.0;

	enemy->TopOff = ANT_HEAD_OFFSET/2;
	

		/* SLOW DOWN PLAYER */
		
	gDelta.x *= .2f;
	gDelta.y *= .2f;
	gDelta.z *= .2f;


		/* HURT & SEE IF KILLED */
			
	if (EnemyGotHurt(enemy, damage))
		return(true);
		
	return(false);
}


/****************** KILL FIRE ANT *********************/
//
// OUTPUT: true if ObjNode was deleted
//

Boolean KillAnt(ObjNode *theNode)
{
	
		/* IF ON SPLINE, DETACH */
		
	DetachEnemyFromSpline(theNode, MoveAnt);


			/* DEACTIVATE */
			
	theNode->TerrainItemPtr = nil;				// dont ever come back
	theNode->CType = CTYPE_MISC;
	theNode->Dying = true;						// after butt-fall, make it die
	
	if (theNode->Skeleton->AnimNum != ANT_ANIM_FALLONBUTT)			// make fall on butt first
	{
		SetSkeletonAnim(theNode->Skeleton, ANT_ANIM_FALLONBUTT);	
		theNode->ButtTimer = 2.0;
	}
	
	return(false);
}


#pragma mark -

/***************************** GIVE ANT A ROCK *********************************/

static void GiveAntARock(ObjNode *theNode)
{
ObjNode	*spearObj;

	if (theNode->ChainNode)				// see if already have something
		return;

			/* MAKE SPEAR OBJECT */

	gNewObjectDefinition.group 		= GLOBAL1_MGroupNum_Rock;	
	gNewObjectDefinition.type 		= GLOBAL1_MObjType_ThrowRock;	
	gNewObjectDefinition.coord		= theNode->Coord;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= theNode->Slot+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1;
	spearObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (spearObj == nil)
		return;

			/* ATTACH SPEAR TO ENEMY */
	
	theNode->ChainNode = spearObj;
	spearObj->ChainHead = theNode;
}


/************************* UPDATE ANT ROCK ************************/
//
// Updated when is being held by ant.
//

static void UpdateAntRock(ObjNode *theEnemy)
{
ObjNode					*rock;
TQ3Matrix4x4			m,m3;
static const TQ3Point3D	zero = {0,0,0};


			/* VERIFY */
			
	rock = theEnemy->ChainNode;
	if (!rock)
		return;	
	
	Q3Matrix4x4_SetTranslate(&m3, 30, 0, -50);
	FindJointFullMatrix(theEnemy, ANT_HOLDING_LIMB, &m);
	MatrixMultiplyFast(&m3, &m, &rock->BaseTransformMatrix);


			/* SET REAL POINT FOR CULLING */
			
	Q3Point3D_Transform(&zero, &rock->BaseTransformMatrix, &rock->Coord);
}


/********************* THROW ROCK ************************/

static void AntThrowRock(ObjNode *theEnemy)
{
ObjNode		 			*rock;
static const TQ3Point3D zero = {0,0,0};
float					rot,speed;

	rock = theEnemy->ChainNode;						// get spear obj
	if (rock == nil)
		return;


		/* DETACH FROM CHAIN */
		
	theEnemy->ChainNode = nil;
	rock->MoveCall 		= MoveAntRock;


			/* CALC THROW START COORD */
			
	Q3Point3D_Transform(&zero, &rock->BaseTransformMatrix, &rock->Coord);
			
			
		/* CALC THROW VECTOR */
			
	speed = CalcQuickDistance(gMyCoord.x, gMyCoord.z, rock->Coord.x, rock->Coord.z) * 1.6f;
			
	rot = theEnemy->Rot.y;
	rock->Delta.x = -sin(rot) * speed;
	rock->Delta.z = -cos(rot) * speed;
	rock->Delta.y = 300;
	
	
			/* GIVE SPEAR COLLISION INFO */
			
	rock->CType  = CTYPE_HURTME;
	rock->CBits  = CBITS_ALLSOLID;
	SetObjectCollisionBounds(rock, 40,-40,-40,40,40,-40);
	rock->Damage = SPEAR_DAMAGE;
	

			/* MAKE A SHADOW */
				
	AttachShadowToObject(rock, 2, 2, false);


}


/******************* MOVE ANT ROCK ****************/

static void MoveAntRock(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	groundY;

		/* SEE IF ITS STILL AROUND */
			
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);
	
	
			/* MOVE IT */
				
	gDelta.y -= 1300.0f * fps;						// gravity	
	gCoord.x += gDelta.x * fps;						// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;
		
	theNode->Rot.x += fps * 3.2f;
	theNode->Rot.y += fps * 1.2f;


		/* SEE IF IMPACT GROUND */

	groundY	= GetTerrainHeightAtCoord(gCoord.x,gCoord.z,FLOOR);
	if (gCoord.y <= groundY)
	{
		PlayEffect3D(EFFECT_HITDIRT, &gCoord);
	
		QD3D_ExplodeGeometry(theNode, 500.0f, 0, 1, .3);
		DeleteObject(theNode);
		return;
	}

	UpdateObject(theNode);
}



