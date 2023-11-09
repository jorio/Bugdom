/*******************************/
/*   	ME BUG.C			   */
/* (c)1999 Pangea Software     */
/* By Brian Greenstone         */
/*******************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void UpdatePlayer_Bug(void);
static void MovePlayer_Bug(ObjNode *theNode);
static void MovePlayerBug_Stand(void);
static void MovePlayerBug_Walk(void);
static void DoPlayerControl_Bug(float slugFactor);
static void MovePlayerBug_RollUp(void);
static void MovePlayerBug_UnRoll(void);
static void MovePlayerBug_Kick(void);
static void MovePlayerBug_Jump(void);
static void MovePlayerBug_Fall(void);
static void MovePlayerBug_Land(void);
static void MovePlayerBug_Swim(void);
static void MovePlayerBug_FallOnButt(void);
static void MovePlayerBug_RideWaterBug(void);
static void MovePlayerBug_RideDragonFly(void);
static void MovePlayerBug_BeingEaten(void);
static void MovePlayerBug_Death(void);
static void MovePlayerBug_BloodSuck(void);
static void MovePlayerBug_Webbed(void);
static void MovePlayerBug_RopeSwing(void);
static void MovePlayerBug_Carried(void);

static void DoBugKick(void);
static void	AimAtClosestKickableObject(void);
static void	AimAtClosestBoppableObject(void);
static void PlayerLeaveRootSwing(void);
static void DrownInLiquid(void);
static void TorchPlayer(void);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	PLAYER_BUG_SCALE			1.7f 


#define	PLAYER_BUG_ACCEL			30.0f
#define	PLAYER_BUG_FRICTION_ACCEL	600.0f
#define PLAYER_BUG_SUPERFRICTION	2000.0f
#define	PLAYER_MAX_SPEED_BUG		700.0f

#define	PLAYER_BUG_JUMPFORCE		2000.0f

#define LIMB_NUM_RIGHT_TOE_TIP		3					// joint # of right toe tip

#define	MY_KICK_ENEMY_DAMAGE		.4f


/*********************/
/*    VARIABLES      */
/*********************/

#define	KickNow			Flag[0]				// set during kick anim

#define RippleTimer		SpecialF[0]

static TQ3Point3D	gRopeSwingOffset = {0, -100, 25};

Boolean				gTorchPlayer;
static	float		gTorchTimer = 0;

/*************************** INIT PLAYER: BUG ****************************/
//
// Creates an ObjNode for the player in the Bug form.
//
// INPUT:	oldObj = old objNode to base some parameters on.  nil = no old player obj
//			where = floor coord where to init the player.
//			rotY = rotation to assign to player if oldObj is nil.
//

ObjNode *InitPlayer_Bug(ObjNode *oldObj, TQ3Point3D *where, float rotY, int animNum)
{
ObjNode	*newObj;
					
	if (oldObj)
		rotY = oldObj->Rot.y;
						
			/* CREATE MY CHARACTER */	
	
	gNewObjectDefinition.type 		= SKELETON_TYPE_ME;
	gNewObjectDefinition.animNum 	= animNum;
	gNewObjectDefinition.coord.x 	= where->x;
	if (oldObj)
		gNewObjectDefinition.coord.y = where->y - PLAYER_BALL_FOOTOFFSET;
	else
		gNewObjectDefinition.coord.y = where->y;
	gNewObjectDefinition.coord.z 	= where->z;
	gNewObjectDefinition.slot 		= PLAYER_SLOT;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.moveCall 	= MovePlayer_Bug;
	gNewObjectDefinition.rot 		= rotY;
	gNewObjectDefinition.scale 		= PLAYER_BUG_SCALE;
	newObj 							= MakeNewSkeletonObject(&gNewObjectDefinition);	

	
				/* SET COLLISION INFO */

	newObj->BoundingSphere.radius	=	PLAYER_RADIUS;			
	newObj->CType = CTYPE_PLAYER;
	newObj->CBits = CBITS_TOUCHABLE;
	
		/* note: box must be same for both bug & ball to avoid collison fallthru against solids */

	SetObjectCollisionBounds(newObj,PLAYER_BUG_HEADOFFSET, -PLAYER_BUG_FOOTOFFSET,
							-42, 42, 42, -42);


			/*******************************************/
			/* COPY SOME STUFF FROM OLD OBJ & NUKE OLD */
			/*******************************************/
			
	if (oldObj)
	{
		newObj->Damage 			= oldObj->Damage;
		newObj->InvincibleTimer = oldObj->InvincibleTimer;
		newObj->Delta 			= oldObj->Delta;
		newObj->Speed			= oldObj->Speed;
		newObj->Rot.x = newObj->Rot.z = 0;
		
				/* COPY OLD COLLISION BOX */

		newObj->OldCoord 		= oldObj->OldCoord;

		GAME_ASSERT(newObj->NumCollisionBoxes > 0);
		GAME_ASSERT(oldObj->NumCollisionBoxes > 0);
		newObj->OldCollisionBoxes[0] = oldObj->OldCollisionBoxes[0];

		
		DeleteObject(oldObj);
		oldObj = nil;
	}	
	
				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, 4.0, 4.0, true);

	
	
				/* SET GLOBALS */
		
	gPlayerObj 		= newObj;		
	gPlayerMode 	= PLAYER_MODE_BUG;
	gPlayerMaxSpeed = PLAYER_MAX_SPEED_BUG; 
	gPrevRope = nil;
	gNitroParticleGroup = -1;
	
	gInfobarUpdateBits |= UPDATE_HANDS;	
	
	return(newObj);
}


   
/******************** MOVE PLAYER: BUG ***********************/

static void MovePlayer_Bug(ObjNode *theNode)
{
	static void(*myMoveTable[])(void) =
	{
		MovePlayerBug_Stand,
		MovePlayerBug_Walk,
		MovePlayerBug_RollUp,
		MovePlayerBug_UnRoll,
		MovePlayerBug_Kick,
		MovePlayerBug_Jump,
		MovePlayerBug_Fall,
		MovePlayerBug_Land,
		MovePlayerBug_Swim,
		MovePlayerBug_FallOnButt,
		MovePlayerBug_RideWaterBug,
		MovePlayerBug_RideDragonFly,
		MovePlayerBug_BeingEaten,
		MovePlayerBug_Death,
		MovePlayerBug_BloodSuck,
		MovePlayerBug_Webbed,
		MovePlayerBug_RopeSwing,
		MovePlayerBug_Carried
	};

	gPlayerCanMove = true;						// assume can move
	GetObjectInfo(theNode);

	
			/* UPDATE INVINCIBILITY */
			
	if (theNode->InvincibleTimer > 0.0f)
	{
		theNode->InvincibleTimer -= gFramesPerSecondFrac;
		if (theNode->InvincibleTimer < 0.0f)
			theNode->InvincibleTimer = 0;
	}
		
	theNode->Rot.x = theNode->Rot.z = 0;		// hack to fix a problem when ball hits water and player gets stuck at strange rotation.
	
		
			/* JUMP TO HANDLER */
				
	myMoveTable[theNode->Skeleton->AnimNum]();
	
}




/******************** MOVE PLAYER BUG: STAND ***********************/

static void MovePlayerBug_Stand()
{
	gPrevRope = nil;
	
			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_FRICTION_ACCEL);
	if (DoPlayerMovementAndCollision(false))
		goto update;


			/* SEE IF SHOULD WALK */
			
	if (gPlayerObj->Skeleton->AnimNum == PLAYER_ANIM_STAND)
	{
		if (gPlayerObj->Speed > 1.0f)
			MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_WALK, 9);
	}

			/* DO CONTROL */
			//
			// do this last since we want any jump command to work smoothly
			//

	DoPlayerControl_Bug(1.0);



	
			/* UPDATE IT */
			
update:			
	UpdatePlayer_Bug();
}



/******************** MOVE PLAYER BUG: WALK ***********************/

static void MovePlayerBug_Walk(void)
{		
	gPrevRope = nil;

			/* DO CONTROL */

	DoPlayerControl_Bug(1.0);


			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_FRICTION_ACCEL);
	if (DoPlayerMovementAndCollision(false))
		goto update;


			/* SEE IF SHOULD STAND */
			
	if (gPlayerObj->Skeleton->AnimNum == PLAYER_ANIM_WALK)
	{
		if (gPlayerObj->Speed < 1.0f)
			MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_STAND, 6.0);
		else
			gPlayerObj->Skeleton->AnimSpeed = gPlayerObj->Speed * .006f;
	}


	
			/* UPDATE IT */
			
update:			
	UpdatePlayer_Bug();
}


/******************** MOVE PLAYER BUG: ROLLUP ***********************/

static void MovePlayerBug_RollUp(void)
{
	gPlayerCanMove = false;						// cant move while doing this
	
		/* SEE IF DONE */
		
	if (gPlayerObj->Skeleton->AnimHasStopped)
	{
		InitPlayer_Ball(gPlayerObj, &gCoord);
		return;	
	}		
		
			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_SUPERFRICTION);						// do super friction to stop during rollup
	if (DoPlayerMovementAndCollision(true))
		goto update;
	
			/* UPDATE IT */
			
update:			
	UpdatePlayer_Bug();
}


/******************** MOVE PLAYER BUG: UNROLL ***********************/

static void MovePlayerBug_UnRoll(void)
{
	gPlayerCanMove = false;						// cant move while doing this

		/* SEE IF DONE */
		
	if (gPlayerObj->Skeleton->AnimHasStopped)
	{
		SetSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_STAND);
	}		
		
			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_SUPERFRICTION);						// do super friction to stop during rollup
	if (DoPlayerMovementAndCollision(true))
		goto update;
	
			/* UPDATE IT */
			
update:			
	UpdatePlayer_Bug();
}



/******************** MOVE PLAYER BUG: KICK ***********************/

static void MovePlayerBug_Kick(void)
{
	gPlayerCanMove = false;						// cant move while doing this

			/* AIM AT CLOSEST KICKABLE ITEM */
			
	AimAtClosestKickableObject();
	

			/* SEE IF PERFORM KICK */
			
	if (gPlayerObj->KickNow)
	{
		gPlayerObj->KickNow = false;
		DoBugKick();
	}
	
			/* SEE IF ANIM IS DONE */

	if (gPlayerObj->Skeleton->AnimHasStopped)
	{
		SetSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_STAND);
	}		

		
			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_SUPERFRICTION*2);						// do super friction to stop
	if (DoPlayerMovementAndCollision(true))
		goto update;
	
			/* UPDATE IT */
			
update:			
	UpdatePlayer_Bug();
}


/******************** MOVE PLAYER BUG: JUMP ***********************/

static void MovePlayerBug_Jump(void)
{

			/* AIM AT CLOSEST BOPPABLE ITEM */
			
	AimAtClosestBoppableObject();
		
			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_FRICTION_ACCEL);
	if (DoPlayerMovementAndCollision(false))
		goto update;

			/* SEE IF HIT GROUND */
			
	if (gPlayerObj->StatusBits & STATUS_BIT_ONGROUND)
	{
		gDelta.y = 0;
		if (gPlayerObj->Skeleton->AnimNum == PLAYER_ANIM_JUMP)						// make sure still in jump anim
			MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_LAND, 7); 
	}
	
			/* SEE IF FALLING YET */
	else			
	if (gDelta.y < 900.0f)
	{
		if (gPlayerObj->Skeleton->AnimNum == PLAYER_ANIM_JUMP)						// make sure still in jump anim
			MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_FALL, 4);
	}
	
				/* LET PLAYER HAVE SOME CONTROL */
				
	DoPlayerControl_Bug(1.0);

	
			/* UPDATE IT */
			
update:			
	UpdatePlayer_Bug();
}


/******************** MOVE PLAYER BUG: FALL ***********************/

static void MovePlayerBug_Fall(void)
{		

			/* AIM AT CLOSEST BOPPABLE ITEM */
			
	if (gPrevRope == nil)								// dont if just now getting off a rope swing
		AimAtClosestBoppableObject();
	

			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_FRICTION_ACCEL);
	if (DoPlayerMovementAndCollision(false))
		goto update;

			/* SEE IF HIT GROUND */
			
	if (gPlayerObj->Skeleton->AnimNum == PLAYER_ANIM_FALL)			// make sure still in this anim
		if (gPlayerObj->StatusBits & STATUS_BIT_ONGROUND)
		{
			MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_LAND, 9); 
			gDelta.x *= .5f;									// slow when landing like this
			gDelta.z *= .5f;
		}
	
				/* LET PLAYER HAVE SOME CONTROL */
				
	DoPlayerControl_Bug(1.0);
	
			/* UPDATE IT */
			
update:			
	UpdatePlayer_Bug();
}


/******************** MOVE PLAYER BUG: LAND ***********************/

static void MovePlayerBug_Land(void)
{
	gPlayerCanMove = false;						// cant move while doing this
		
			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_FRICTION_ACCEL);
	if (DoPlayerMovementAndCollision(true))
		goto update;

	
			/* SEE IF DONE */
		
	if (!gPlayerObj->Skeleton->IsMorphing)
	{
		MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_STAND, 6);
	}		
	
	
			/* UPDATE IT */
			
update:			
	UpdatePlayer_Bug();
}


/******************** MOVE PLAYER BUG: SWIM ***********************/

static void MovePlayerBug_Swim(void)
{		
		/* SEE IF LIQUID KILLS */
		
	if (!gLiquidCheat)
	{
		if (gCurrentLiquidType != LIQUID_WATER)
		{
			if (gCurrentLiquidType == LIQUID_LAVA)
				gTorchPlayer = true;
				
			DrownInLiquid();
			return;
		}	
	}
	
	if (gLevelType == LEVEL_TYPE_ANTHILL)
	{
		// need to figure out if on lava or water!
	
	}

	gPlayerObj->Skeleton->AnimSpeed = 1.5;					// set speed

			/* DO CONTROL */

		DoPlayerControl_Bug(.6);


			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_FRICTION_ACCEL/6);
	if (DoPlayerMovementAndCollision(false))
		goto update;


		/* ADD WATER RIPPLE */
			
	gPlayerObj->RippleTimer += gFramesPerSecondFrac;
	if (gPlayerObj->RippleTimer > .25f)
	{
		gPlayerObj->RippleTimer = 0;
		MakeRipple(gCoord.x, gCoord.y+gLiquidCollisionTopOffset[LIQUID_WATER], gCoord.z, 3.0f);
	}



			/* SEE IF OUT OF WATER */
			
	if (!(gPlayerObj->StatusBits & STATUS_BIT_UNDERWATER))
	{
		if (gPlayerObj->Skeleton->AnimNum == PLAYER_ANIM_SWIM)						// if still swimming, then go into stand
			MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_STAND, 5.0);
			
		if (gPlayerObj->Skeleton->AnimNum == PLAYER_ANIM_JUMP)						// if jump out, then splash
			MakeSplash(gCoord.x, gPlayerCurrentWaterY, gCoord.z, .2, 3);
	}

	
			/* UPDATE IT */
			
update:			
	UpdatePlayer_Bug();
}


/********************* DROWN IN LIQUID ********************/

static void DrownInLiquid(void)
{
//float	ty;

	gPlayerGotKilledFlag = true;					// let kill timer in main.c do the 4 second sink


	gPlayerObj->Skeleton->AnimSpeed = .7;			// set speed

		/* SLOWLY SINK */

//	ty = gPlayerCurrentWaterY - 90.0f;				// calc sunken target y


	gCoord.y -= gFramesPerSecondFrac * 30.0f;
//	if (gCoord.y < ty)								// see if completely under
//		KillPlayer(false);


			/* UPDATE IT */

	UpdatePlayer_Bug();
}


/******************** MOVE PLAYER BUG: FALL ON BUTT ***********************/

static void MovePlayerBug_FallOnButt(void)
{
	gPlayerCanMove = false;						// cant move while doing this
	
			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_SUPERFRICTION);
	if (DoPlayerMovementAndCollision(true))
		goto update;


			/* SEE IF DONE */
		
	if (gPlayerObj->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_STAND, 6);
	}		
	
	
			/* UPDATE IT */
			
update:			
	UpdatePlayer_Bug();
}

/******************** MOVE PLAYER BUG: RIDE WATER BUG ***********************/
//
// NOTE: most of the updating and stuff is handled in the waterbug code.
//

static void MovePlayerBug_RideWaterBug(void)
{		
TQ3Matrix4x4	m,m2,m3;
static const TQ3Point3D zero = {0,0,0};
float			s;

	gPlayerCanMove = false;						// cant move while doing this


			/* SEE IF HOP OFF BUG */
			
	if (GetNewKeyState(kKey_Jump))
	{
		gCurrentWaterBug->Mode = WATERBUG_MODE_COAST;
		MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_JUMP, 9);
		MorphToSkeletonAnim(gCurrentWaterBug->Skeleton, WATERBUG_ANIM_OUTOFSERVICE, 5);		
		gDelta.y = PLAYER_BUG_JUMPFORCE;
		MovePlayerBug_Jump();						// reroute to the jump function now
		return;
	}


			/* DRIVE WATER BUG */
			
	DriveWaterBug(gCurrentWaterBug, gPlayerObj);


		/**********************************/
		/* UPDATE MY PLACEMENT ON THE BUG */
		/**********************************/
			
	s = PLAYER_BUG_SCALE / gCurrentWaterBug->Scale.x;		// compensate for bug's scale
	Q3Matrix4x4_SetScale(&m, s,s,s);			
	FindJointFullMatrix(gCurrentWaterBug, 0, &m2);			// get matrix of waterbug
	MatrixMultiplyFast(&m,&m2,&m3);			
	
	Q3Matrix4x4_SetTranslate(&m, 0, 30, 40);				// set offset translation matrix for point on waterbug's back
	MatrixMultiplyFast(&m,&m3,&gPlayerObj->BaseTransformMatrix);			

			/* CALC PLAYER'S COORD */
			
	Q3Point3D_Transform(&zero, &gPlayerObj->BaseTransformMatrix, &gMyCoord);
	gPlayerObj->Coord = gMyCoord;	


			/* HIDE MY SHADOW WHILE RIDING */

	if (gPlayerObj->ShadowNode)
	{
		gPlayerObj->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;
	}
}


/******************** MOVE PLAYER BUG: RIDE DRAGONFLY ***********************/

static void MovePlayerBug_RideDragonFly(void)
{		
static const TQ3Point3D	zero = {0,0,0};
TQ3Matrix4x4	m,m2,m3;
float			s;

	gPlayerCanMove = false;						// cant move while doing this


			/* SEE IF HOP OFF BUG */
			
	if (GetNewKeyState(kKey_Jump))
	{
hop_off:	
		if (!gPlayerGotKilledFlag)
			MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_JUMP, 9);	// if not dead then goto jump anim
		PlayerOffDragonfly();
		gDelta.y = PLAYER_BUG_JUMPFORCE;
		gDelta.x = gDelta.z = 0;
		MovePlayerBug_Jump();						// reroute to the jump function now
		return;
	}

			/* DRIVE BUG */
			
	DriveDragonFly(gCurrentDragonFly, gPlayerObj);


			/* UPDATE MY PLACEMENT ON THE BUG */
			
	s = PLAYER_BUG_SCALE / gCurrentDragonFly->Scale.x;					// compensate for bug's scale
	Q3Matrix4x4_SetScale(&m, s,s,s);			
	FindJointFullMatrix(gCurrentDragonFly, DRAGONFLY_JOINT_TAIL, &m2);	// get matrix
	MatrixMultiplyFast(&m,&m2,&m3);			
	
	Q3Matrix4x4_SetTranslate(&m, 0, 8, 55);								// set offset translation matrix for point on waterbug's back
	MatrixMultiplyFast(&m,&m3,&gPlayerObj->BaseTransformMatrix);			

			
		/* CALC WORLD COORD & DELTA OF BUG */
		
	Q3Point3D_Transform(&zero, &gPlayerObj->BaseTransformMatrix, &gMyCoord);
	gPlayerObj->Coord = gCoord = gMyCoord;
	gDelta.x = gCoord.x - gPlayerObj->OldCoord.x;
	gDelta.y = gCoord.y - gPlayerObj->OldCoord.y;
	gDelta.z = gCoord.z - gPlayerObj->OldCoord.z;
	gPlayerObj->Delta = gDelta;
	
	
			/* DO COLLISION AGAINST ENEMIES */
	
	DoPlayerCollisionDetect();
	if (gNumCollisions > 0)
		goto hop_off;
	
			/*************/
			/* UPDATE IT */
			/*************/
			
	
		/* UPDATE COLLISION BOX */
		
	CalcObjectBoxFromNode(gPlayerObj);


		/* HIDE MY SHADOW WHILE RIDING */

	if (gPlayerObj->ShadowNode)
	{
		gPlayerObj->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;
	}
}



/******************** MOVE PLAYER BUG: BEING EATEN ***********************/
//
// This keeps the player in the fish or bat mouth
//

static void MovePlayerBug_BeingEaten(void)
{		
TQ3Matrix4x4	m,m2,m3;
float			s;
static const TQ3Point3D	zero = {0,0,0};
ObjNode	*enemy;
TQ3Point3D off;
int		j;

	gPlayerCanMove = false;						// cant move while doing this

			/* MAKE SURE PLAYER IS IN CORRECT ANIM */
			
	if (gPlayerObj->Skeleton->AnimNum != PLAYER_ANIM_BEINGEATEN)
		MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_BEINGEATEN, 7);

	
			/****************/
			/* UPDATE COORD */
			/****************/
		
	switch(gLevelType)
	{
		case	LEVEL_TYPE_POND:
				enemy = gCurrentEatingFish;
				off = gPondFishMouthOff;
				j = PONDFISH_JOINT_HEAD;
				break;
				
		case	LEVEL_TYPE_FOREST:
				enemy = gCurrentEatingBat;
				off = gBatMouthOff;
				j = BAT_JOINT_HEAD;
				break;
				
		default:
				DoFatalAlert("MovePlayerBug_BeingEaten: how did we get here?");
				return;
	}


			/* BAIL JUST IN CASE ENEMY REFERENCE WENT INVALID */

	if (!enemy || enemy->CType == INVALID_NODE_FLAG)
	{
		KillPlayer(false);
		return;
	}


	s = PLAYER_BUG_SCALE / enemy->Scale.x;			// compensate for enemy's scale
	Q3Matrix4x4_SetScale(&m, s,s,s);
	FindJointFullMatrix(enemy,j,&m2);						// get matrix
	MatrixMultiplyFast(&m,&m2,&m3);		

	off.x *= enemy->Scale.x;								// scale offset
	off.y *= enemy->Scale.y;
	off.z *= enemy->Scale.z;
	Q3Matrix4x4_SetTranslate(&m, off.x, off.y, off.z);  // get mouth offset	
	MatrixMultiplyFast(&m,&m3,&gPlayerObj->BaseTransformMatrix);		

			/* ON FLIGHT LEVEL, THE CAMERA TRACKS BAT */
			
	if (gLevelType == LEVEL_TYPE_FOREST)
	{
		Q3Point3D_Transform(&zero, &gPlayerObj->BaseTransformMatrix, &gMyCoord);
		gPlayerObj->Coord = gMyCoord;
	}
}


/******************** MOVE PLAYER BUG: DEATH ***********************/

static void MovePlayerBug_Death(void)
{		
	gPlayerCanMove = false;						// cant move while doing this

	gPlayerObj->CType = 0;								// no collision on me during this
	
			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_FRICTION_ACCEL*5);
	if (DoPlayerMovementAndCollision(true))
		goto update;

	
			/* UPDATE IT */
			
update:			
	UpdatePlayer_Bug();
}


/******************** MOVE PLAYER BUG: BLOODSUCK ***********************/

static void MovePlayerBug_BloodSuck(void)
{		
	gPlayerCanMove = false;						// cant move while doing this

	gPlayerObj->AccelVector.x = 
	gPlayerObj->AccelVector.y = 0;
	gPlayerObj->CType = 0;								// no collision on me during this
	
			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_FRICTION_ACCEL*5);
	if (DoPlayerMovementAndCollision(true))
		goto update;

	
			/* UPDATE IT */
			
update:			
	LoseBallTime(.1f * gFramesPerSecondFrac);			// lose ball time while being sucked
	UpdatePlayer_Bug();
}


/******************** MOVE PLAYER BUG: WEBBED ***********************/

static void MovePlayerBug_Webbed(void)
{		
	gPlayerCanMove = false;						// cant move while doing this

	gPlayerObj->AccelVector.x = 
	gPlayerObj->AccelVector.y = 0;
	gPlayerObj->CType = 0;								// no collision on me during this
	
			/* MOVE PLAYER */

	DoFrictionAndGravity(PLAYER_BUG_FRICTION_ACCEL*10);
	if (DoPlayerMovementAndCollision(true))
		goto update;

	
			/* UPDATE IT */
			
update:			
	UpdatePlayer_Bug();
}


/******************** MOVE PLAYER BUG: ROPE SWING ***********************/

static void MovePlayerBug_RopeSwing(void)
{		
TQ3Matrix4x4	m,m2,m3;
float			s;
float			dx,dy;

	gPlayerCanMove = false;						// cant move while doing this

			/* SEE IF HOP OFF BUG */
			
	if (GetNewKeyState(kKey_Jump))
	{
		PlayerLeaveRootSwing();
		MovePlayerBug_Fall();									// reroute to the fall function now
		return;
	}
	
			/* SEE IF ROTATE */
			
	GetMouseDelta(&dx, &dy);								// get mouse or key info			
	gPlayerObj->Rot.y += dx * .008f;

	

			/* UPDATE MY PLACEMENT ON THE ROPE */
			
	s = PLAYER_BUG_SCALE / gCurrentRope->Scale.x;				// compensate for rope's scale
	Q3Matrix4x4_SetScale(&m, s,s,s);			
	FindJointFullMatrix(gCurrentRope, gCurrentRopeJoint, &m2);	// get matrix
	MatrixMultiplyFast(&m,&m2,&m3);			
	
	Q3Matrix4x4_SetRotate_Y(&m, gPlayerObj->Rot.y);				// also rotate to correct y angle
	MatrixMultiplyFast(&m,&m3,&m2);								// concat rotation into matrix
	
	Q3Matrix4x4_SetTranslate(&m, gRopeSwingOffset.x,			// set offset translation matrix for point on rope
								 gRopeSwingOffset.y,
								 gRopeSwingOffset.z);	
	MatrixMultiplyFast(&m,&m2,&gPlayerObj->BaseTransformMatrix);			

			/* UPDATE IT */
			
	FindCoordOnJoint(gCurrentRope, gCurrentRopeJoint, &gRopeSwingOffset, &gMyCoord);	// est. coord of joint	
	gPlayerObj->Coord = gMyCoord;
	gPlayerObj->Delta.x = (gMyCoord.x - gPlayerObj->OldCoord.x) * gFramesPerSecond;
	gPlayerObj->Delta.y = (gMyCoord.y - gPlayerObj->OldCoord.y) * gFramesPerSecond;
	gPlayerObj->Delta.z = (gMyCoord.z - gPlayerObj->OldCoord.z) * gFramesPerSecond;
		
	CalcObjectBoxFromNode(gPlayerObj);

	UpdatePlayerShield();
}


/******************** MOVE PLAYER BUG: CARRIED ***********************/
//
// carried by firefly.
//

static void MovePlayerBug_Carried(void)
{
	gPlayerCanMove = false;						// cant move while doing this

			/* SEE IF PLAYER WAS DROPPED */
			
	if (gCurrentCarryingFireFly == nil)
	{
		gDelta.x = gDelta.y = gDelta.z = 0;
		MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_FALL, 5);
		MovePlayerBug_Fall();
		return;
	}
	
			/* UPDATE MY LOCATION */
			
	gCoord.x = gCurrentCarryingFireFly->Coord.x;
	gCoord.y = gCurrentCarryingFireFly->Coord.y - 180.0f;
	gCoord.z = gCurrentCarryingFireFly->Coord.z;
	
	gPlayerObj->Rot.y = gCurrentCarryingFireFly->Rot.y;
	
	gMyCoord = gCoord;
	
	UpdateObject(gPlayerObj);
	UpdatePlayerShield();
}



#pragma mark -

/************************ UPDATE PLAYER: BUG ***************************/

static void UpdatePlayer_Bug(void)
{

		/* CALC Y-ROTATION BASED ON DELTA VECTOR */
	
	if ((!gPlayerUsingKeyControl) || (!gGamePrefs.playerRelativeKeys))
		TurnObjectTowardTarget(gPlayerObj, &gCoord, gCoord.x+gDelta.x, gCoord.z+gDelta.z, gPlayerObj->Speed*.013f, false);


			/* UPDATE THE NODE */
			
	gMyCoord = gCoord;
	UpdateObject(gPlayerObj);	

		/* UPDATE SHIELD */
		
	UpdatePlayerShield();
	
	
		/* SEE IF TORCHED */
		
	if (gTorchPlayer)
	{
		TorchPlayer();	
	}
}




/**************** DO PLAYER CONTROL: BUG ***************/
//
// Moves a player based on its control bit settings.
// These settings are already set either by keyboard interpretation or reading off the network.
//
// INPUT:	theNode = the node of the player
//			slugFactor = how much of acceleration to apply (varies if jumping et.al)
//


static void DoPlayerControl_Bug(float slugFactor)
{
float	mouseDX, mouseDY;
float	dx, dy;
int		anim;
Boolean	isOnGround,isSwimming;

			/* GET SOME INFO */
			
	anim = gPlayerObj->Skeleton->AnimNum;				// get anim #
	
	if ((gPlayerObj->StatusBits & STATUS_BIT_ONGROUND) || (gMyDistToFloor < 5.0f))
		isOnGround = true;
	else
		isOnGround = false;
		
	isSwimming = (anim == PLAYER_ANIM_SWIM);


			/********************/
			/* GET MOUSE DELTAS */
			/********************/
			
	GetMouseDelta(&dx, &dy);
	
	mouseDX = dx * .08f;
	mouseDY = dy * .08f;

	if (!isOnGround)				// if not on ground, then lessen the mouse effect
	{
		mouseDX *= .5f;
		mouseDY *= .5f;	
	}

	if (gPlayerUsingKeyControl && gGamePrefs.playerRelativeKeys)
	{
		gPlayerObj->AccelVector.y = 
		gPlayerObj->AccelVector.x = 0;	
	}
	else
	{
		gPlayerObj->AccelVector.y = mouseDY * PLAYER_BUG_ACCEL * slugFactor;
		gPlayerObj->AccelVector.x = mouseDX * PLAYER_BUG_ACCEL * slugFactor;	
	}



			/***************/
			/* SEE IF KICK */
			/***************/

	if (isOnGround)
	{
		if ((anim == PLAYER_ANIM_STAND) || (anim == PLAYER_ANIM_WALK))
		{
			if (GetNewKeyState(kKey_Kick))
			{
				MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_KICK, 9);	
				gPlayerObj->KickNow = false;
			}		
		}
	}
			/***************/
			/* SEE IF JUMP */
			/***************/

	if (((!isOnGround) && isSwimming) ||									// if swimming...
		((anim == PLAYER_ANIM_STAND) || (anim == PLAYER_ANIM_WALK) ||	// or in one of these anims & on the ground
		(anim == PLAYER_ANIM_SWIM)))
	{
		if (GetNewKeyState(kKey_Jump))
		{
			MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_JUMP, 9);
			if (isSwimming)
				gDelta.y = PLAYER_BUG_JUMPFORCE/1.4f;
			else
				gDelta.y = PLAYER_BUG_JUMPFORCE;
				
			if (!isSwimming)									// play jump sound if not swimming
				PlayEffect3D(EFFECT_JUMP, &gCoord);
		}
	}
}


/******************** DO BUG KICK **********************/

static void DoBugKick(void)
{
static const TQ3Point3D offsetCoord = {0,-10,-50};
TQ3Point3D	impactCoord;
int			i;


			/* GET WORLD-COORD TO TEST */
			
	FindCoordOnJoint(gPlayerObj, BUG_LIMB_NUM_PELVIS, &offsetCoord, &impactCoord);


		/* DO COLLISION DETECTION ON KICKABLE THINGS */
			
	if (DoSimpleBoxCollision(impactCoord.y + 40.0f, impactCoord.y - 40.0f,
							impactCoord.x - 40.0f, impactCoord.x + 40.0f,
							impactCoord.z + 40.0f, impactCoord.z - 40.0f,
							CTYPE_KICKABLE))
	{
		PlayEffect3D(EFFECT_POUND, &impactCoord);
	
		for (i = 0; i < gNumCollisions; i++)
		{	
			ObjNode *kickedObj;
			
			kickedObj = gCollisionList[i].objectPtr;				// get objnode of kicked object


					/* HANDLE SPECIFICS */
				
			if (kickedObj->Genre == DISPLAY_GROUP_GENRE)
			{
				switch(kickedObj->Group)
				{			
					case	MODEL_GROUP_GLOBAL1:
							switch(kickedObj->Type)
							{
								case	GLOBAL1_MObjType_Nut:			// NUT
										KickNut(gPlayerObj, kickedObj);
										break;
										
								case	GLOBAL1_MObjType_LadyBugCage:	// LADY BUG CAGE
										KickLadyBugBox(kickedObj);
										break;	
							}
							break;

					case	MODEL_GROUP_GLOBAL2:
							switch(kickedObj->Type)
							{
								case	GLOBAL2_MObjType_Tick:			// TICK ENEMY
										KillTick(kickedObj);
										break;								
							}
							break;
					
					case	MODEL_GROUP_LEVELSPECIFIC:
							switch(kickedObj->Type)
							{
								case	ANTHILL_MObjType_KingPipe:
										if (gRealLevel == LEVEL_NUM_ANTKING)
										{
											KickKingWaterPipe(kickedObj);										
										}
										break;
							
							}
							break;
				}
			}
			else
			if (kickedObj->Genre == SKELETON_GENRE)
			{
				switch(kickedObj->Type)
				{
					case	SKELETON_TYPE_ANT:
							KnockAntOnButt(kickedObj, 
												-sin(gPlayerObj->Rot.y) * 700.0f,
												700.0f,
												-cos(gPlayerObj->Rot.y) * 700.0f, MY_KICK_ENEMY_DAMAGE);
							break;

					case	SKELETON_TYPE_BOXERFLY:
							KillBoxerFly(kickedObj, -sin(gPlayerObj->Rot.y) * 700.0f,
												700.0f,
												-cos(gPlayerObj->Rot.y) * 700.0f);
							break;

					case	SKELETON_TYPE_MOSQUITO:
							KillMosquito(kickedObj, -sin(gPlayerObj->Rot.y) * 700.0f,
												700.0f,
												-cos(gPlayerObj->Rot.y) * 700.0f);
							break;
												
					case	SKELETON_TYPE_SPIDER:
							KnockSpiderOnButt(kickedObj, 
												-sin(gPlayerObj->Rot.y) * 700.0f,
												700.0f,
												-cos(gPlayerObj->Rot.y) * 700.0f , MY_KICK_ENEMY_DAMAGE);
							break;

					case	SKELETON_TYPE_FLYINGBEE:
							KillFlyingBee(kickedObj, -sin(gPlayerObj->Rot.y) * 700.0f,
												700.0f,
												-cos(gPlayerObj->Rot.y) * 700.0f);
							break;

					case	SKELETON_TYPE_QUEENBEE:
							KnockQueenBeeOnButt(kickedObj, 
												-sin(gPlayerObj->Rot.y) * 300.0f,
												-cos(gPlayerObj->Rot.y) * 300.0f,MY_KICK_ENEMY_DAMAGE);
							break;

					case	SKELETON_TYPE_ROACH:
							KnockRoachOnButt(kickedObj, 
												-sin(gPlayerObj->Rot.y) * 300.0f,
												700.0f,
												-cos(gPlayerObj->Rot.y) * 300.0f,
												MY_KICK_ENEMY_DAMAGE);
							break;

					case	SKELETON_TYPE_LARVA:
							KillLarva(kickedObj);
							break;
							
					case	SKELETON_TYPE_KINGANT:
							KnockKingAntOnButt(kickedObj, 
												-sin(gPlayerObj->Rot.y) * 300.0f,
												-cos(gPlayerObj->Rot.y) * 300.0f,MY_KICK_ENEMY_DAMAGE);
							break;

				}
			}
		}
	} // if DoSimpleBox...
}


/******************** AIM AT CLOSEST KICKABLE OBJECT **********************/

static void	AimAtClosestKickableObject(void)
{
ObjNode	*closest,*thisNodePtr;
float	minDist,dist,myX,myZ;

			/************************/
			/* SCAN OBJ LINKED LIST */
			/************************/
			
	thisNodePtr = gFirstNodePtr;
	myX = gCoord.x;
	myZ = gCoord.z;
	minDist = 10000000;
	closest = nil;
	
	while(thisNodePtr)
	{
		if (thisNodePtr->CType & CTYPE_KICKABLE)
		{			
			dist = CalcDistance(myX, myZ, thisNodePtr->Coord.x, thisNodePtr->Coord.z);	// calc dist to this obj
			if (dist < minDist)
			{
				minDist = dist;
				closest = thisNodePtr;
			}	
		}
		thisNodePtr = thisNodePtr->NextNode;
	}

				/* SEE IF ANYTHING CLOSE ENOUGH */
				
	if (minDist < 300.0f)
	{		
		TurnObjectTowardTarget(gPlayerObj, &gCoord, closest->Coord.x, closest->Coord.z,	9.0, false);			
	}
}


/******************** AIM AT CLOSEST BOPPABLE OBJECT **********************/

static void	AimAtClosestBoppableObject(void)
{
ObjNode	*closest,*thisNodePtr;
u_long	closestCType;
float	minDist,dist,myX,myZ,myY;

			/************************/
			/* SCAN OBJ LINKED LIST */
			/************************/
			
	thisNodePtr = gFirstNodePtr;
	myX = gCoord.x;
	myY = gCoord.y + gPlayerObj->TopOff;
	myZ = gCoord.z;
	minDist = 10000000;
	closest = nil;
	closestCType = 0;
	
	while(thisNodePtr)
	{
		if (thisNodePtr->CType & CTYPE_AUTOTARGET)
		{
			if (myY > (thisNodePtr->Coord.y - thisNodePtr->BoundingSphere.radius))				// player must be above it
			{
				dist = CalcQuickDistance(myX, myZ, thisNodePtr->Coord.x, thisNodePtr->Coord.z);	// calc dist to this obj
				if (dist < minDist)
				{
					minDist = dist;
					closest = thisNodePtr;
					closestCType = thisNodePtr->CType;
				}	
			}
		}
		thisNodePtr = thisNodePtr->NextNode;
	}

				/********************************/
				/* SEE IF ANYTHING CLOSE ENOUGH */
				/********************************/
				
	if (minDist < 290.0f)
	{		
		TQ3Vector2D	dir;
		
		if (minDist > 15.0f)							// this avoids rotation & motion jitter
		{
			TurnObjectTowardTarget(gPlayerObj, &gCoord, closest->Coord.x, closest->Coord.z,	9.0, false);	// aim at target
		
					/* MOVE TOWARDS THE TARGET */
		
			if (closestCType & CTYPE_AUTOTARGETJUMP)
			{			
				FastNormalizeVector2D(closest->Coord.x - gCoord.x, closest->Coord.z - gCoord.z, &dir);
				dir.x *= 500.0f;
				dir.y *= 500.0f;

				gCoord.x += dir.x * gFramesPerSecondFrac;
				gCoord.z += dir.y * gFramesPerSecondFrac;

				gDelta.x = gCoord.x - gPlayerObj->OldCoord.x;
				gDelta.z = gCoord.z - gPlayerObj->OldCoord.z;
			}
		}
	}
}


/************** KNOCK PLAYER BUG ON BUTT *******************/
//
// INPUT:	playerIsCurrent = true if player is currently active ObjNode
//			delta	= delta to apply to me
//
// OUTPUT:	new objNode in case it changed during morph
//

void KnockPlayerBugOnButt(TQ3Vector3D *delta, Boolean allowBall, Boolean playerIsCurrent)
{
	
			/* IF WAS BALL, NOW ITS A BUG */
			
	if (gPlayerMode == PLAYER_MODE_BALL)						// see if turn into bug
	{
		if (allowBall)											// see if can do it now
			InitPlayer_Bug(gPlayerObj, &gPlayerObj->Coord, gPlayerObj->Rot.y, PLAYER_ANIM_UNROLL);
		else
		{
			gPlayerKnockOnButt = true;							// tell ball code to do this at end of Move function instead
			gPlayerKnockOnButtDelta = *delta;
			return;
		}
	}		

			/* MORPH TO BUTT POSITION */
			
	MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_FALLONBUTT, 3);
	gPlayerObj->Delta = *delta;
	if (playerIsCurrent)										// update global delta if player is current
		gDelta = *delta;
		
	gPlayerObj->AccelVector.x = gPlayerObj->AccelVector.y = 0;
	
		/* SET ROTATION */

	if ((gPlayerObj->Delta.x != 0.0f) || (gPlayerObj->Delta.y != 0.0f) || (gPlayerObj->Delta.z != 0.0f))
	{
		float	tx,tz;
		
		tx = gPlayerObj->Coord.x - gPlayerObj->Delta.x;
		tz = gPlayerObj->Coord.z - gPlayerObj->Delta.z;
	
		gPlayerObj->Rot.y = CalcYAngleFromPointToPoint(gPlayerObj->Rot.y, gPlayerObj->Coord.x, gPlayerObj->Coord.z, tx, tz);
	}
}


/******************* PLAYER GRAB ROOT SWING ********************/

void PlayerGrabRootSwing(ObjNode *root, int joint)
{
	gCurrentRope = gPrevRope = root;
	gCurrentRopeJoint = joint;
	
	MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_ROPESWING, 10);
}


/*************** PLAYER LEAVE ROOT SWING *****************/

static void PlayerLeaveRootSwing(void)
{
#if 0
float	r,dx,dz;

	r = gPlayerObj->Rot.y;
	dx = -sin(r) * 1200.0f;
	dz = -cos(r) * 1200.0f;

//	gDelta.x += dx;
//	gDelta.z += dz;
//	dy = gDelta.y;
#endif

	FastNormalizeVector(gDelta.x, gDelta.y, gDelta.z, &gDelta);

	gDelta.x *= 3000.0f;
	gDelta.z *= 3000.0f;
	gDelta.y = 200.0f; //(dy*.2f) + 

	MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_FALL, 9);
}


#pragma mark -


/********************* TORCH PLAYER ************************/

static void TorchPlayer(void)
{
	gTorchTimer += gFramesPerSecondFrac;

	if (gTorchTimer > 0.04f)							// see if time to spew fire particles
	{
		gTorchTimer = 0;								// reset timer

				/* SEE IF MAKE NEW GROUP */
				
		if (gPlayerObj->ParticleGroup == -1)
		{
new_pgroup:		
			gPlayerObj->ParticleGroup = NewParticleGroup(
														PARTICLE_TYPE_FALLINGSPARKS,// type
														PARTICLE_FLAGS_HOT,			// flags
														-100,						// gravity
														9000,						// magnetism
														25,							// base scale
														0.1,						// decay rate
														.7,							// fade rate
														PARTICLE_TEXTURE_FIRE);		// texture
		}

				/* ADD PARTICLES TO FIRE */
				
		if (gPlayerObj->ParticleGroup != -1)
		{
			TQ3Vector3D	delta;
			TQ3Point3D  pt;
			int			i;

			FindCoordOfJoint(gPlayerObj, BUG_LIMB_NUM_PELVIS, &pt);

			for (i = 0; i < 3; i++)
			{
				pt.x += (RandomFloat()-.5f) * 50.0f;
				pt.y += (RandomFloat()-.5f) * 30.0f;
				pt.z += (RandomFloat()-.5f) * 50.0f;
				
				delta.x = (RandomFloat()-.5f) * 100.0f;
				delta.y = RandomFloat() * 50.0f;
				delta.z = (RandomFloat()-.5f) * 100.0f;
				
				if (AddParticleToGroup(gPlayerObj->ParticleGroup, &pt, &delta, RandomFloat() + 2.1f, FULL_ALPHA))
					goto new_pgroup;				
			}		
		}
	}
}




