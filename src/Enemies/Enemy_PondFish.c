/****************************/
/*   ENEMY: PONDFISH.C		*/
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

static void MovePondFish(ObjNode *theNode);
static void MovePondFish_Waiting(ObjNode *theNode);
static void  MovePondFish_JumpAttack(ObjNode *theNode);
static void UpdatePondFish(ObjNode *theNode);
static void SeeIfFishEatsPlayer(ObjNode *fish);
static void PondFish_ContinueEatingPlayer(ObjNode *fish);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_PONDFISH			3

#define	PONDFISH_Y				(WATER_Y-240.0f)

#define	PONDFISH_CHASE_DIST		1900.0f
#define	PONDFISH_CHASESPEED		700.0f

#define PONDFISH_ATTACK_DIST	400.0f
#define PONDFISH_JUMPSPEED		2400.0f

#define PONDFISH_TURN_SPEED		1.8f
#define	PONDFISH_TARGET_SCALE	120.0f

#define	PONDFISH_HEALTH		1.0f		
#define	PONDFISH_DAMAGE		0.04f

#define	PONDFISH_SCALE		2.0f


enum
{
	PONDFISH_MODE_WAIT,
	PONDFISH_MODE_CHASE
};


/*********************/
/*    VARIABLES      */
/*********************/

#define RippleTimer		SpecialF[0]
#define	AttackTimer		SpecialF[1]
#define	RandomJumpTimer	SpecialF[2]
#define	EatenTimer		SpecialF[3]
#define	JumpNow			Flag[0]
#define	IsJumping		Flag[1]
#define	EatPlayer		Flag[2]
#define	TweakJumps		Flag[3]

const TQ3Point3D gPondFishMouthOff = {0,-17,-40};

ObjNode	*gCurrentEatingFish;


/************************ ADD PONDFISH ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_PondFish(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= MAX_ENEMIES)					// keep from getting absurd
		return(false);

	if (gNumEnemyOfKind[ENEMY_KIND_PONDFISH] >= MAX_PONDFISH)
		return(false);


				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_PONDFISH,x,z,PONDFISH_SCALE+RandomFloat()*.3f);
	if (newObj == nil)
		return(false);
	newObj->TerrainItemPtr = itemPtr;
	

				/* SET BETTER INFO */
			
		
	newObj->Coord.y 	= PONDFISH_Y;			
	newObj->MoveCall 	= MovePondFish;							// set move call
	newObj->Health 		= PONDFISH_HEALTH;
	newObj->Damage 		= PONDFISH_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_PONDFISH;
	newObj->Mode		= PONDFISH_MODE_WAIT;
	
				/* SET COLLISION INFO */
				//
				// NOTE:  nothing ever collides against the fish
				//

	newObj->CType = 0;
	newObj->CBits = 0;
	CalcNewTargetOffsets(newObj,PONDFISH_TARGET_SCALE);


	newObj->InitCoord = newObj->Coord;							// remember where started

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_PONDFISH]++;
	return(true);
}



/********************* MOVE PONDFISH **************************/

static void MovePondFish(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
		{
			MovePondFish_Waiting,
			MovePondFish_JumpAttack,
			MovePondFish_Waiting						// mouthfull
		};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		if (theNode == gCurrentEatingFish)				// yikes, don't let player deref a stale object
		{
			gCurrentEatingFish = NULL;
			KillPlayer(false);
		}
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	
	theNode->AttackTimer -= gFramesPerSecondFrac;		// dec this timer


			/* GIVE UP CHASE MODE IF PLAYER EATEN BY ANOTHER FISH */

	if (gCurrentEatingFish && theNode != gCurrentEatingFish)
	{
		theNode->Mode = PONDFISH_MODE_WAIT;
	}


	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}


/********************** MOVE PONDFISH: WAITING ******************************/

static void  MovePondFish_Waiting(ObjNode *theNode)
{
float	dist,r,aim;

				/*****************************/
				/* JUST HOVERING AND WAITING */
				/*****************************/

	if (theNode->Mode == PONDFISH_MODE_WAIT)
	{
		TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, PONDFISH_TURN_SPEED, false);			

				/* SEE IF DO RANDOM JUMP */
				
		if ((theNode->RandomJumpTimer -= gFramesPerSecondFrac) < 0.0f)
		{
			theNode->RandomJumpTimer = 2.0f + RandomFloat()*2.0f;
			theNode->TweakJumps = false;
			goto attack;
		}


			/* START CHASING IF PLAYER IS STILL IN WATER, NOT BEING EATEN, AND CLOSE */

		if (gPlayerObj->StatusBits & STATUS_BIT_UNDERWATER
			&& !gCurrentEatingFish)										// make sure not already being eaten
		{
			dist = CalcQuickDistance(gMyCoord.x, gMyCoord.z, gCoord.x, gCoord.z);
			if (dist < PONDFISH_CHASE_DIST)
			{
				theNode->Mode = PONDFISH_MODE_CHASE;			
			}
		}
	}
				/***********/
				/* CHASING */
				/***********/
	else
	{
		aim = TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, PONDFISH_TURN_SPEED, true);			

		
				/* MOVE FORWARD */
				
		r = theNode->Rot.y;
		gDelta.x = -sin(r) * PONDFISH_CHASESPEED;
		gDelta.z = -cos(r) * PONDFISH_CHASESPEED;		
		MoveEnemy(theNode);


				/* MAKE SURE NOT TOO SHALLOW */
				
		if ((GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR) + 350.0f) > WATER_Y)
		{
			gCoord.x = theNode->OldCoord.x;		
			gCoord.z = theNode->OldCoord.z;		
		}


			/* ATTACK IF PLAYER IS STILL IN WATER, NOT BEING EATEN, AND CLOSE */

		if (gPlayerObj->StatusBits & STATUS_BIT_UNDERWATER
			&& !gCurrentEatingFish)									// make sure not already being eaten
		{
			dist = CalcQuickDistance(gMyCoord.x, gMyCoord.z, gCoord.x, gCoord.z);
			if (dist > PONDFISH_CHASE_DIST)
			{
				theNode->Mode = PONDFISH_MODE_WAIT;			
			}
					/* ALSO SEE IF CLOSE ENOUGH TO ATTACK */
			else
			if (dist < PONDFISH_ATTACK_DIST
				&& aim < .8f							// aiming at player
				&& theNode->AttackTimer < 0)
			{
				theNode->TweakJumps = true;
attack:					
				MorphToSkeletonAnim(theNode->Skeleton, PONDFISH_ANIM_JUMPATTACK, 6.0);
				theNode->JumpNow = false;
				theNode->IsJumping = false;
			}
		}
	}

			/* UPDATE POND FISH */
			
	UpdatePondFish(theNode);	
}


/********************** MOVE PONDFISH: JUMP ATTACK ******************************/

static void  MovePondFish_JumpAttack(ObjNode *theNode)
{

	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, PONDFISH_TURN_SPEED, false);			

			/*************************/
			/* SEE IF START LEAP NOW */
			/*************************/
			
	if (theNode->JumpNow)
	{
		theNode->JumpNow = false;
		theNode->IsJumping = true;
		
			/* CALC JUMP AIM DELTA */
		gDelta.x = 0;
		gDelta.y = PONDFISH_JUMPSPEED;
		gDelta.z = 0;
		MoveEnemy(theNode);
		MakeSplash(gCoord.x, WATER_Y-50, gCoord.z, 1.0, 4);
		MakeRipple(gCoord.x, WATER_Y, gCoord.z, 7.0);
		MakeRipple(gCoord.x, WATER_Y, gCoord.z, 5.0);
	}
	
			/****************/
			/* PROCESS JUMP */
			/****************/
			
	else
	if (theNode->IsJumping)
	{
		float dx,dz;
		
			/* TWEAK TOWARD PLAYER */
			
		if (theNode->TweakJumps)
		{
			TQ3Point3D	mouthPt,playerPt;
			
			FindCoordOnJoint(theNode, PONDFISH_JOINT_HEAD, &gPondFishMouthOff, &mouthPt);	// get coord of mouth

			if (gPlayerObj->Skeleton)
				FindCoordOfJoint(gPlayerObj, BUG_LIMB_NUM_PELVIS, &playerPt);				// get coord of player pelvis
			else
				playerPt = gPlayerObj->Coord;												// Source port fix: fall back to node coord if player has no skeleton (ball mode)
				
			dx = playerPt.x - mouthPt.x;
			dz = playerPt.z - mouthPt.z;
			gDelta.x = dx*15;
			gDelta.z = dz*15;
		}

		
			/* DO GRAVITY & SEE IF SPLASHDOWN */
					
		gDelta.y -= 3400.0f * gFramesPerSecondFrac;					// gravity	
		MoveEnemy(theNode);

				/* MAKE SURE NOT TOO SHALLOW */
				
		if ((GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR) + 350.0f) > WATER_Y)
		{
			gCoord.x = theNode->OldCoord.x;		
			gCoord.z = theNode->OldCoord.z;		
		}

		if ((gCoord.y < PONDFISH_Y) && (gDelta.y < 0.0f))			// see if hit water
		{
			gCoord.y = PONDFISH_Y;
			gDelta.x =
			gDelta.y =
			gDelta.z = 0;
			if (theNode->EatPlayer)
				MorphToSkeletonAnim(theNode->Skeleton, PONDFISH_ANIM_MOUTHFULL, 4);
			else
				MorphToSkeletonAnim(theNode->Skeleton, PONDFISH_ANIM_WAIT, 4);				
			theNode->IsJumping = false;
			MakeSplash(gCoord.x, WATER_Y-100.0f, gCoord.z, 1.0f, 4);
			MakeRipple(gCoord.x, WATER_Y, gCoord.z, 7.0);
			MakeRipple(gCoord.x, WATER_Y, gCoord.z, 5.0);
			theNode->AttackTimer = 2.0f + RandomFloat()*2.0f;	// dont attack for n more seconds at least
		}
	}
	
		/*****************************/
		/* WAITING FOR JUMP TO START */
		/*****************************/

	else
	{
		ApplyFrictionToDeltas(100.0,&gDelta);	
		MoveEnemy(theNode);
	}


		/*********************/
		/* SEE IF EAT PLAYER */
		/*********************/

	if (!theNode->EatPlayer)
		SeeIfFishEatsPlayer(theNode);


			/* UPDATE */
			
	UpdatePondFish(theNode);	
}


/****************** UPDATE POND FISH ********************/

static void UpdatePondFish(ObjNode *theNode)
{
	if (theNode->EatPlayer)
		PondFish_ContinueEatingPlayer(theNode);

		/* ADD WATER RIPPLE */

	if ((theNode->Mode != PONDFISH_MODE_WAIT) && (!theNode->IsJumping))
	{		
		theNode->RippleTimer += gFramesPerSecondFrac;
		if (theNode->RippleTimer > .20f)
		{
			theNode->RippleTimer = 0;
			MakeRipple(gCoord.x, WATER_Y, gCoord.z, 2.0);
		}
	}
	
	UpdateEnemy(theNode);
}

#pragma mark -



/********************** SEE IF FISH EATS PLAYER *************************/
//
// Does simply collision check to see if fish should eat the player.
//

static void SeeIfFishEatsPlayer(ObjNode *fish)
{
TQ3Point3D	pt;

	if (gPlayerMode == PLAYER_MODE_BALL)					// can only eat the bug, not the ball
		return;

	if (gCurrentEatingFish)									// bug already eaten by someone else
		return;

	
			/* GET COORD OF MOUTH */
			
	FindCoordOnJoint(fish, PONDFISH_JOINT_HEAD, &gPondFishMouthOff, &pt);


		/* SEE IF POINT IS INSIDE PLAYER */
			
			
	if (DoSimpleBoxCollisionAgainstPlayer(pt.y+15.0f, pt.y-15.0f, pt.x-15.0f, pt.x+15.0f,
										pt.z+15.0f, pt.z-15.0f))
	{
		gCurrentEatingFish = fish;
		MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_BEINGEATEN, 7);
		gPlayerObj->CType = 0;							// no more collision
		fish->EatPlayer = true;	
		fish->EatenTimer = 3;							// start timer
		PondFish_ContinueEatingPlayer(fish);
	}
}


/****************** PONDFISH: CONTINUE EATING PLAYER *********************/
//
// Called after fish has eaten player.
//

static void PondFish_ContinueEatingPlayer(ObjNode *fish)
{
		/* SEE IF EATEN TIMER HAS TIMED OUT */
		
	fish->EatenTimer -= gFramesPerSecondFrac;	
	if (fish->EatenTimer < 0.0f)
	{	
		fish->EatPlayer = false;
		KillPlayer(false);	
	}
}

