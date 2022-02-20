/****************************/
/*   ENEMY: BOXERFLY.C			*/
/* (c)1998 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveBoxerFly(ObjNode *theNode);
static void MoveBoxerFly_Flying(ObjNode *theNode);
static void  MoveBoxerFly_Punching(ObjNode *theNode);
static void MoveBoxerFlyOnSpline(ObjNode *theNode);
static void  MoveBoxerFly_Death(ObjNode *theNode);
static void UpdateBoxerFly(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_BOXERFLY				4

#define	BOXERFLY_CHASE_RANGE		700.0f
#define	BOXERFLY_PUNCH_RANGE		200.0f

#define BOXERFLY_TURN_SPEED			4.0f
#define BOXERFLY_CHASE_SPEED		400.0f
#define BOXERFLY_SPLINE_SPEED		180.0f

#define	MAX_BOXERFLY_RANGE			(BOXERFLY_CHASE_RANGE+1000.0f)	// max distance this enemy can go from init coord

#define	BOXERFLY_HEALTH				1.0f		
#define	BOXERFLY_DAMAGE				0.04f
#define	BOXERFLY_PUNCH_DAMAGE		.2f
#define	BOXERFLY_SCALE				.9f
#define	BOXERFLY_FLIGHT_HEIGHT		110.0f


enum
{
	BOXERFLY_ANIM_FLY,
	BOXERFLY_ANIM_PUNCH,
	BOXERFLY_ANIM_DEATH
};


enum
{
	BOXERFLY_MODE_WAITING,
	BOXERFLY_MODE_GOHOME,
	BOXERFLY_MODE_CHASE
};

#define	BoxerFlyWobbleOff		sin(theNode->Wobble += fps*15.0f) * 20.0f


#define BOXERFLY_LEFT_ELBOW_JOINT	8
#define BOXERFLY_RIGHT_ELBOW_JOINT	9

#define	BOXERFLY_KNOCKDOWN_SPEED	1400.0f		// speed ball needs to go to knock this down



/*********************/
/*    VARIABLES      */
/*********************/

#define	Wobble			SpecialF[2]
#define	PunchActive		Flag[0]				// set by anim
#define HonorRange		Flag[1]				// true = check max range


/************************ ADD BOXERFLY ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_BoxerFly(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= MAX_ENEMIES)								// keep from getting absurd
		return(false);
	if (gNumEnemyOfKind[ENEMY_KIND_BOXERFLY] >= MAX_BOXERFLY)
		return(false);

				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_BOXERFLY,x,z,BOXERFLY_SCALE);
	if (newObj == nil)
		return(false);
	newObj->TerrainItemPtr = itemPtr;

	SetSkeletonAnim(newObj->Skeleton, BOXERFLY_ANIM_FLY);
	

				/* SET BETTER INFO */
			
	newObj->Coord.y 	+= BOXERFLY_FLIGHT_HEIGHT;			
	newObj->MoveCall 	= MoveBoxerFly;							// set move call
	newObj->Health 		= BOXERFLY_HEALTH;
	newObj->Damage 		= BOXERFLY_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_BOXERFLY;
	newObj->CType		|= CTYPE_KICKABLE|CTYPE_AUTOTARGET;		// these can be kicked
	newObj->CBits		= CBITS_NOTTOP;
	
	newObj->Mode		= BOXERFLY_MODE_WAITING;
	newObj->HonorRange	= true;
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 70,-40,-40,40,40,-40);


				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, 7, 7, false);

	newObj->InitCoord = newObj->Coord;							// remember where started

	newObj->Wobble = 0;


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_BOXERFLY]++;
	return(true);
}





/********************* MOVE BOXERFLY **************************/

static void MoveBoxerFly(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveBoxerFly_Flying,
					MoveBoxerFly_Punching,
					MoveBoxerFly_Death
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}


/********************** MOVE BOXERFLY: FLYING ******************************/

static void  MoveBoxerFly_Flying(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	r,aim;

	switch(theNode->Mode)
	{
			/*****************************/
			/* JUST HOVERING AND WAITING */
			/*****************************/
			
		case	BOXERFLY_MODE_WAITING:
				TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, BOXERFLY_TURN_SPEED, false);			
				
					/* SEE IF CLOSE ENOUGH TO ATTACK */
					
				if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < BOXERFLY_CHASE_RANGE)
					theNode->Mode = BOXERFLY_MODE_CHASE;				
				break;
	
				
				/*****************/
				/* FLY BACK HOME */
				/*****************/
				
		case	BOXERFLY_MODE_GOHOME:
		
						/* MOVE TOWARD HOME */
						
				TurnObjectTowardTarget(theNode, &gCoord, theNode->InitCoord.x, theNode->InitCoord.z, BOXERFLY_TURN_SPEED, false);

				r = theNode->Rot.y;
				gDelta.x = sin(r) * -BOXERFLY_CHASE_SPEED;
				gDelta.z = cos(r) * -BOXERFLY_CHASE_SPEED;
				
				gCoord.x += gDelta.x * fps;
				gCoord.z += gDelta.z * fps;

						/* SEE IF THERE */
						
				if (CalcQuickDistance(gCoord.x, gCoord.z, theNode->InitCoord.x, theNode->InitCoord.z) < 400.0f)
					theNode->Mode = BOXERFLY_MODE_WAITING;
				break;
				
		
		
				/**************/
				/* CHASE MODE */
				/**************/
				
		default:
		
						/* MOVE TOWARD PLAYER */
						
				aim = TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, BOXERFLY_TURN_SPEED, false);			

				r = theNode->Rot.y;
				gDelta.x = sin(r) * -BOXERFLY_CHASE_SPEED;
				gDelta.z = cos(r) * -BOXERFLY_CHASE_SPEED;
				
				gCoord.x += gDelta.x * fps;
				gCoord.z += gDelta.z * fps;
				
				/* SEE IF CLOSE ENOUGH TO PUNCH */
					
				if (aim < (PI/3))										// must be aiming at me	
				{
					if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < BOXERFLY_PUNCH_RANGE)
					{
						MorphToSkeletonAnim(theNode->Skeleton, BOXERFLY_ANIM_PUNCH, 3.0);
						theNode->PunchActive = false;
					}
				}
				
					/* SEE IF TOO FAR AWAY AND NEED TO FLY BACK HOME */
					
				if (theNode->HonorRange)								// see if we care about range
				{
					if (CalcQuickDistance(theNode->InitCoord.x, theNode->InitCoord.z, gCoord.x, gCoord.z) > MAX_BOXERFLY_RANGE)
						theNode->Mode = BOXERFLY_MODE_GOHOME;				
				}
	}


				/* DO WOBBLE */

	gCoord.y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR) + BOXERFLY_FLIGHT_HEIGHT;	// calc y coord
	gCoord.y += BoxerFlyWobbleOff;


				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

	UpdateBoxerFly(theNode);		
	
}


/********************** MOVE BOXERFLY: PUNCHING ******************************/

static void  MoveBoxerFly_Punching(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	r;
static const TQ3Point3D	gloveOffset = {-35,0,-100};
TQ3Point3D glovePt;
int		i;

				/**********************/
				/* MOVE TOWARD PLAYER */
				/**********************/
						
	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, BOXERFLY_TURN_SPEED, false);			

	r = theNode->Rot.y;
	gDelta.x = sin(r) * -BOXERFLY_CHASE_SPEED;
	gDelta.z = cos(r) * -BOXERFLY_CHASE_SPEED;

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR) + BOXERFLY_FLIGHT_HEIGHT;	// calc y coord				
	gCoord.y += BoxerFlyWobbleOff;


				/* SEE IF DONE WITH ANIM */
				
	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, BOXERFLY_ANIM_FLY,3.0);
		theNode->PunchActive = false;
	}

			/****************************/
			/* SEE IF GLOVES HIT PLAYER */
			/****************************/

	if (theNode->PunchActive)
	{
		for (i = 0; i < 2; i++)
		{
			FindCoordOnJoint(theNode, BOXERFLY_LEFT_ELBOW_JOINT+i, &gloveOffset, &glovePt);
			if (DoSimpleBoxCollisionAgainstPlayer(glovePt.y+35, glovePt.y-35, glovePt.x-25, glovePt.x+25,
												glovePt.z+25, glovePt.z-25))
			{
					/* HURT THE PLAYER VIA NORMAL METHODS */
					
				PlayerGotHurt(nil, BOXERFLY_PUNCH_DAMAGE, false, false, true, INVINCIBILITY_DURATION);
				
			
					/* ALSO SEND PLAYER FLYING FROM PUNCH */
				
				gPlayerObj->Delta.x = gDelta.x * 4.0f;
				gPlayerObj->Delta.z = gDelta.z * 4.0f;
				gPlayerObj->Delta.y = 1400.0f;
				
				theNode->Mode = BOXERFLY_MODE_GOHOME;				// fly should go home now
			}	
		}
	}

		

				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

	UpdateBoxerFly(theNode);		
	
}


/********************** MOVE BOXERFLY: DEATH ******************************/

static void  MoveBoxerFly_Death(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ISCULLED)		// if was culled on last frame and is far enough away, then delete it
	{
		DeleteEnemy(theNode);
		return;
	}


				/* MOVE IT */
				
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(60.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;			// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEATH_ENEMY_COLLISION_CTYPES))
		return;


				/* UPDATE */
			
	UpdateBoxerFly(theNode);		
}

/********************* UPDATE BOXERFLY ****************************/

static void UpdateBoxerFly(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimNum != BOXERFLY_ANIM_DEATH)
	{
		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect3D(EFFECT_BUZZ, &theNode->Coord);
		else
			Update3DSoundChannel(EFFECT_BUZZ, &theNode->EffectChannel, &theNode->Coord);
	}

	UpdateEnemy(theNode);		
}

#pragma mark -

/************************ PRIME BOXERFLY ENEMY *************************/

Boolean PrimeEnemy_BoxerFly(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj,*shadowObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_BOXERFLY,x,z, BOXERFLY_SCALE);
	if (newObj == nil)
		return(false);
		
	DetachObject(newObj);									// detach this object from the linked list
		
	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;
	
	SetSkeletonAnim(newObj->Skeleton, BOXERFLY_ANIM_FLY);
	

				/* SET BETTER INFO */
			
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		+= BOXERFLY_FLIGHT_HEIGHT;			
	newObj->SplineMoveCall 	= MoveBoxerFlyOnSpline;				// set move call
	newObj->Health 			= BOXERFLY_HEALTH;
	newObj->Damage 			= BOXERFLY_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_BOXERFLY;
	newObj->CType			|= CTYPE_KICKABLE|CTYPE_AUTOTARGET;	// these can be kicked
	newObj->CBits			= CBITS_NOTTOP;

	newObj->Wobble = 0;

	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 70,-40,-40,40,40,-40);


				/* MAKE SHADOW */
				
	shadowObj = AttachShadowToObject(newObj, 7, 7, false);
	DetachObject(shadowObj);									// detach this object from the linked list


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	AddToSplineObjectList(newObj);

	return(true);
}

/******************** MOVE BOXERFLY ON SPLINE ***************************/
//
// NOTE: This is called for every BoxerFly on the spline in the entire level.
//		The isVisible flag determines if this particular one is in visible range.
//		It actually means the enemy is on an active supertile.
//

static void MoveBoxerFlyOnSpline(ObjNode *theNode)
{
Boolean isVisible; 
float	fps = gFramesPerSecondFrac;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode,BOXERFLY_SPLINE_SPEED);
	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/
			
	if (isVisible)
	{
			/* START/UPDATE BUZZ */
	
		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect3D(EFFECT_BUZZ, &theNode->Coord);
		else
			Update3DSoundChannel(EFFECT_BUZZ, &theNode->EffectChannel, &theNode->Coord);
	
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);		

		theNode->Coord.y = GetTerrainHeightAtCoord(theNode->Coord.x, theNode->Coord.z, FLOOR) + BOXERFLY_FLIGHT_HEIGHT;	// calc y coord
		theNode->Coord.y += BoxerFlyWobbleOff;															// do wobble
		UpdateObjectTransforms(theNode);																// update transforms
		CalcObjectBoxFromNode(theNode);																	// update collision box
		UpdateShadow(theNode);																			// update shadow		
		
				/*********************************/
				/* SEE IF CLOSE ENOUGH TO ATTACK */
				/*********************************/
				
		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gMyCoord.x, gMyCoord.z) < BOXERFLY_CHASE_RANGE)
		{
					/* REMOVE FROM SPLINE */
					
			DetachEnemyFromSpline(theNode, MoveBoxerFly);

			theNode->Mode		= BOXERFLY_MODE_CHASE;
			theNode->HonorRange = false;
		}		
	}
	
			/* NOT VISIBLE */
	else
	{
		StopObjectStreamEffect(theNode);
	
//		if (theNode->ShadowNode)									// hide shadow
//			theNode->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;	
	}
}


#pragma mark -


/******************* BALL HIT BOXERFLY *********************/
//
// OUTPUT: true if enemy gets killed (but not necessarily deleted!)
//

Boolean BallHitBoxerFly(ObjNode *me, ObjNode *enemy)
{
Boolean	killed = false;

				/************************/
				/* HURT & KNOCK ON BUTT */
				/************************/
				
	if (me->Speed > BOXERFLY_KNOCKDOWN_SPEED)
	{	
		KillBoxerFly(enemy, me->Delta.x*.5f, 400, me->Delta.z*.5f);
   		killed = true;
		PlayEffect_Parms3D(EFFECT_POUND, &gCoord, kMiddleC+2, 2.0);
	}
	
	return(killed);
}


/****************** KILL BOXERFLY *********************/
//
// OUTPUT: true if ObjNode was deleted
//

Boolean KillBoxerFly(ObjNode *theNode, float dx, float dy, float dz)
{
		/* STOP BUZZ */
		
	if (theNode->EffectChannel != -1)
		StopAChannel(&theNode->EffectChannel);

	
		/* IF ON SPLINE, DETACH */
		
	DetachEnemyFromSpline(theNode, MoveBoxerFly);


			/* DEACTIVATE */
			
	theNode->TerrainItemPtr = nil;									// dont ever come back
	theNode->CType = CTYPE_MISC;
	theNode->BottomOff = 0;
	
			/* NUKE SHADOW */
			
	if (theNode->ShadowNode)
	{
		DeleteObject(theNode->ShadowNode);
		theNode->ShadowNode = nil;	
	}
	
		/* DO DEATH ANIM */
			
	if (theNode->Skeleton->AnimNum != BOXERFLY_ANIM_DEATH)
		SetSkeletonAnim(theNode->Skeleton, BOXERFLY_ANIM_DEATH);
		
	theNode->Delta.x = dx;	
	theNode->Delta.y = dy;	
	theNode->Delta.z = dz;	
	
	return(false);
}










