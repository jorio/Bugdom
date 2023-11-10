/****************************/
/*   	MYGUY.C    			*/
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

static void PlayerBopEnemy(ObjNode *me, ObjNode *enemy);
static void MoveMyBuddy(ObjNode *theNode);
static void BuddyFollowsMe(ObjNode *theNode);
static Boolean MoveBuddyTowardEnemy(ObjNode *theNode);
static void SplatterBuddy(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	BUDDY_DIST_FROM_ME	120.0f
#define	BUDDY_ACCEL			4.0f
#define	BUDDY_CLOSEST		50.0f
#define	BUDDY_HEIGHT_FACTOR	0.3f
#define	BUDDY_MINY			160.0f
#define	BUDDY_ATTACK_SPEED	1200.0f

enum
{
	BUDDY_MODE_LIKESME,
	BUDDY_MODE_ATTACK
};


/*********************/
/*    VARIABLES      */
/*********************/

long		gMyStartX,gMyStartZ;
TQ3Point3D	gMyCoord = {0,0,0};
ObjNode		*gPlayerObj,*gMyBuddy;
float		gShieldTimer = 0,gShieldRingTick = 0;
int32_t		gShieldParticleGroupA,gShieldParticleGroupB;
short		gShieldChannel = -1;
float		gShieldRot = 0;

Byte		gMyStartAim;

Byte		gPlayerMode;						// ball or bug

float		gPlayerMaxSpeed;					// current max speed player can travel

float		gPlayerCurrentWaterY;
Byte		gCurrentLiquidType;

short		gBestCheckPoint;
TQ3Point3D	gMostRecentCheckPointCoord;



/******************* INIT PLAYER AT START OF LEVEL ********************/

void InitPlayerAtStartOfLevel(void)
{
			/* SET SOME GLOBALS */
			
	gShieldTimer = 0;			
	gShieldRingTick = 0;	
	gShieldChannel = -1;
	gMyBuddy = nil;
	gShieldRot = 0;
	gCurrentLiquidType = LIQUID_WATER;
	gTorchPlayer = false;
	
	gShieldParticleGroupA = gShieldParticleGroupB = -1;

	gMyCoord.x = gMyStartX;
	gMyCoord.y = GetTerrainHeightAtCoord(gMyStartX,gMyStartZ,FLOOR);
	gMyCoord.z = gMyStartZ;

	gMostRecentCheckPointCoord.y = gMyCoord.y;			// set y (x & z) were already set

	InitPlayer_Bug(nil, &gMyCoord, (float)gMyStartAim * (PI2/8), PLAYER_ANIM_STAND);
	
	gCheckPointRot = (float)(gMyStartAim/2) * (PI2/4);

	gPlayerObj->Damage 			= .5;
	gPlayerObj->InvincibleTimer = 0;
}


/********************** RESET PLAYER **********************/
//
// Called after player dies and time to reincarnate.
//

void ResetPlayer(void)
{
	gCurrentEatingFish = NULL;

		/* RETURN PLAYER TO STANDING MODE */
		
	if (gPlayerMode == PLAYER_MODE_BALL)				// see if turn into bug
		InitPlayer_Bug(gPlayerObj, &gMostRecentCheckPointCoord, gCheckPointRot, PLAYER_ANIM_STAND);
	else
	if (gPlayerMode == PLAYER_MODE_BUG)
	{
		SetSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_STAND);
		gPlayerObj->Coord = gMostRecentCheckPointCoord;
		gPlayerObj->Coord.y = GetTerrainHeightAtCoord(gMostRecentCheckPointCoord.x,
													gMostRecentCheckPointCoord.z,
													FLOOR) + PLAYER_BUG_FOOTOFFSET;
		gPlayerObj->Rot.y = gCheckPointRot;
		UpdateObjectTransforms(gPlayerObj);
	}
	gPlayerObj->HurtTimer = 0;
	gPlayerObj->InvincibleTimer = INVINCIBILITY_DURATION_DEATH;	// make me invincible for a while	

	gPlayerObj->CType = CTYPE_PLAYER;						// make sure this gets reset
	
	gPlayerObj->Delta.x =									// stop motion
	gPlayerObj->Delta.y =
	gPlayerObj->Delta.z = 0;
	

				/* RESET SOME GLOBAL INFO */
					
	gMyCoord 				= gMostRecentCheckPointCoord;
	gPlayerGotKilledFlag 	= false;		
	gMyHealth				= 1.0;							// reset health	
	gInfobarUpdateBits 		|= UPDATE_HEALTH;
	gShieldTimer			= 0;
	gTorchPlayer			= false;
	
	if (gShieldChannel != -1)
	{
		StopAChannel(&gShieldChannel);
	}
	
			/* SEE IF KILL BUDDY */
			
	if (gMyBuddy)
	{
		DeleteObject(gMyBuddy);
		gMyBuddy = nil;
	}
}



/******************** DO PLAYER COLLISION DETECT **************************/
//
// Standard collision handler for player
//
// OUTPUT: true = disabled/killed
//

Boolean DoPlayerCollisionDetect(void)
{
short		i;
ObjNode		*hitObj;
unsigned long	ctype;
u_char		sides;
	
			/* LITTLE ERROR CHECK HACK TO FIX A BUG WHERE SPEED BECOMES NAN */
			
	if ((gPlayerObj->Speed >= 0.0) && (gPlayerObj->Speed < 10000000.0f))
	{
	}
	else
		gPlayerObj->Speed = 0;
	
	
			/* DETERMINE CTYPE BITS TO CHECK FOR */
			
	if (gPlayerGotKilledFlag)
		ctype = CTYPE_MISC;						// when dead, only check solid things
	else
	{
		if (gCurrentDragonFly)					// if on dragonfly, then only collide against enemies
			ctype = CTYPE_ENEMY;
		else
			ctype = PLAYER_COLLISION_CTYPE;		// normal check
	}


			/***************************************/
			/* AUTOMATICALLY HANDLE THE GOOD STUFF */
			/***************************************/
			//
			// this also sets the ONGROUND status bit if on a solid object.
			//

	sides = HandleCollisions(gPlayerObj, ctype);
		

			/******************************/
			/* SCAN FOR INTERESTING STUFF */
			/******************************/
			
	gPlayerObj->StatusBits &= ~STATUS_BIT_UNDERWATER;			// assume not in water volume
	gPlayerObj->StatusBits &= ~STATUS_BIT_INVISCOUSTRAP;		// assume not in viscous volume
	gPlayerObj->MPlatform = nil;								// assume not on MPlatform

	for (i=0; i < gNumCollisions; i++)						
	{
			hitObj = gCollisionList[i].objectPtr;				// get ObjNode of this collision
			
			if (hitObj->CType == INVALID_NODE_FLAG)			// see if has since become invalid
				continue;
			
			ctype = hitObj->CType;								// get collision ctype from hit obj
					
					
			/**************************/
			/* CHECK FOR IMPENETRABLE */
			/**************************/
			
			if ((ctype & CTYPE_IMPENETRABLE) && (!(ctype & CTYPE_IMPENETRABLE2)))
			{
				if (!(gCollisionList[i].sides & SIDE_BITS_BOTTOM))	// dont do this if we landed on top of it
				{
					gCoord.x = gPlayerObj->OldCoord.x;					// dont take any chances, just move back to original safe place
					gCoord.z = gPlayerObj->OldCoord.z;
				}
			}
			
			/*****************************/
			/* CHECK FOR MOVING PLATFORM */
			/*****************************/
				
			if (ctype & CTYPE_MPLATFORM)
			{
					/* ONLY IF BOTTOM HIT IT */
					
				if (gCollisionList[i].sides & SIDE_BITS_BOTTOM)
				{
					gPlayerObj->MPlatform = hitObj;				
				}
			}
		
				/*******************/
				/* CHECK FOR ENEMY */
				/*******************/
				
			if (ctype & CTYPE_ENEMY)
			{
					/* SEE IF BOPPABLE ENEMY */
					
				if ((ctype & CTYPE_BOPPABLE) && (gCollisionList[i].sides & SIDE_BITS_BOTTOM)) // bottom must have hit for it to be a bop
				{
					PlayerBopEnemy(gPlayerObj, hitObj);
				}		
				
					/* COLLISION WASNT A BOP */	
				else
				{
					PlayerHitEnemy(hitObj);
				}

			}
			
				/**********************/
				/* SEE IF HURTME ITEM */
				/**********************/
				
			if (ctype & CTYPE_HURTME)
			{
				if (ctype & CTYPE_HURTNOKNOCK)
					PlayerGotHurt(hitObj, hitObj->Damage,false,true,false,INVINCIBILITY_DURATION);
				else
					PlayerGotHurt(hitObj, hitObj->Damage,false,true,true,INVINCIBILITY_DURATION);
			}			

				/**************************/
				/* SEE IF DRAIN BALL TIME */
				/**************************/
				
			if (ctype & CTYPE_DRAINBALLTIME)
			{
				LoseBallTime(hitObj->Damage * gFramesPerSecondFrac);
			}

			
				/**********************/
				/* SEE IF WATER PATCH */
				/**********************/
				//
				// Give other solid items priority over the water by checking
				// if the bottom collision bit is set.  This way jumping on lily pads
				// will be more reliable.
				//
			
			if ((ctype & CTYPE_LIQUID) && (!(sides&CBITS_BOTTOM)))	// only check for water if no bottom collision
			{
				if (GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR) < hitObj->Coord.y)		// make sure didnt hit liquid thru solid floor
				{
					gPlayerObj->StatusBits |= STATUS_BIT_UNDERWATER;
					gPlayerCurrentWaterY = hitObj->CollisionBoxes[0].top;
					gCurrentLiquidType = hitObj->Kind;
				}
			}				

				/******************/
				/* SEE IF VISCOUS */
				/******************/
			
			if (ctype & CTYPE_VISCOUS)
			{
				gPlayerObj->StatusBits |= STATUS_BIT_INVISCOUSTRAP;
			}
	}

	return(gPlayerGotKilledFlag);
}





/******************** PLAYER HIT ENEMY ***********************/
//
// OUTPUT: 	true = enemy was deleted (objnode is invalid now)
//
// NOTE: just because an enemy was killed DOES NOT mean it was deleted, so be careful!
//

void PlayerHitEnemy(ObjNode *enemy)
{

			/* SEE IF ENEMY IS 'SPIKED' */
			
	if (enemy->CType & CTYPE_SPIKED)
	{
		PlayerGotHurt(enemy, enemy->Damage, false, true,true,INVINCIBILITY_DURATION);	
		
		if (enemy->Kind == ENEMY_KIND_FLYINGBEE)			// flying bees die when they sting me
		{
			KillFlyingBee(enemy,0,0,0);
			return;
		}
	}

				/*******************/
				/* HANDLE FOR BALL */
				/*******************/
				
	if (gPlayerMode == PLAYER_MODE_BALL)
	{
		switch(enemy->Kind)
		{	
			case	ENEMY_KIND_BOXERFLY:
					BallHitBoxerFly(gPlayerObj, enemy);
					break;
					
			case	ENEMY_KIND_MOSQUITO:
					BallHitMosquito(gPlayerObj, enemy);
					break;
					
			case	ENEMY_KIND_ANT:
					BallHitAnt(gPlayerObj, enemy);
					break;

			case	ENEMY_KIND_FIREANT:
					BallHitFireAnt(gPlayerObj, enemy);
					break;

			case	ENEMY_KIND_SPIDER:
					BallHitSpider(gPlayerObj, enemy);
					break;

			case	ENEMY_KIND_WORKERBEE:
					BallHitWorkerBee(gPlayerObj, enemy);
					break;

			case	ENEMY_KIND_FLYINGBEE:
					BallHitFlyingBee(gPlayerObj, enemy);
					break;

			case	ENEMY_KIND_QUEENBEE:
					BallHitQueenBee(gPlayerObj, enemy);
					break;

			case	ENEMY_KIND_ROACH:
					BallHitRoach(gPlayerObj, enemy);
					break;

			case	ENEMY_KIND_KINGANT:
					BallHitKingAnt(gPlayerObj, enemy);
					break;
		}
	}
}


/******************** PLAYER GOT HURT ***************************/
//
// INPUT:	what	=	the objnode (or nil) of object that cause me to get hurt
//			damage	=	how much damage to inflict
//			playerIsCurrent = true if player is current active ObjNode

void PlayerGotHurt(ObjNode *what, float damage, Boolean overrideShield, Boolean playerIsCurrent,
					Boolean canKnockOnButt, float invincibleDuration)
{
TQ3Vector3D	delta;

	if (damage == 0.0f)
		return;
		
	if (!overrideShield)
		if (gShieldTimer > 0.0f)								// see if shielded
			return;
		
	if (gPlayerGotKilledFlag)									// cant get hurt if already dead
		return;
		
	
	if (gPlayerObj->InvincibleTimer > 0.0f)						// cant be harmed if invincible
		return;

	if (gPlayerObj->InvincibleTimer < invincibleDuration)
		gPlayerObj->InvincibleTimer = invincibleDuration;	// make me invincible for a while
	

			/* LOSE HEALTH & SEE IF WAS KILLED */
			
	LoseHealth(damage);
	
	
				/*****************/
				/* KNOCK ON BUTT */
				/*****************/				
				
	if ((!gPlayerGotKilledFlag) && canKnockOnButt)
	{	
					/* CALC KNOCK VECTOR */
					
		if (what)									
		{
			delta.x = what->Delta.x;
			delta.z = what->Delta.z;
			delta.y = 800;
		}
		else
		{
			delta.x = delta.y = delta.z = 0;	
		}

		
				/* IF NOT KILLED, SEE IF KNOCK ON BUTT */
				
		KnockPlayerBugOnButt(&delta, false, playerIsCurrent);
	}


			/* PLAY OUCH SOUND */
			
	PlayEffect3D(EFFECT_OUCH, &gPlayerObj->Coord);	
}


/******************** KILL PLAYER *****************************/
//
// INPUT:	changeAnims	= true if need to do death anim, otherwise, just set flags
//
// OUTPUT:	new objNode in case it changed during morph
//

void KillPlayer(Boolean changeAnims)
{
	if (!gPlayerGotKilledFlag)
	{
		if (changeAnims)
		{
			if (gPlayerMode == PLAYER_MODE_BALL)
				InitPlayer_Bug(gPlayerObj, &gPlayerObj->Coord, gPlayerObj->Rot.y, PLAYER_ANIM_DEATH);
			else
				SetSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_DEATH);
		}	
		gPlayerGotKilledFlag = true;
	}
}


/********************* PLAYER BOP ENEMY *********************/
//
// Called when collision detect sees that I've landed on a boppable enemy
//

static void PlayerBopEnemy(ObjNode *me, ObjNode *enemy)
{
	(void) me;

	switch(enemy->Kind)
	{
		case	ENEMY_KIND_LARVA:
				LarvaGotBopped(enemy);
				break;
				
		case	ENEMY_KIND_TICK:
				TickGotBopped(enemy);
				break;
	}
}


#pragma mark -


/********************** CREATE MY BUDDY ****************************/
//
// Called when the buddy powerup nut is cracked open
//

void CreateMyBuddy(float x, float z)
{
ObjNode *newObj;

	gNewObjectDefinition.type 		= SKELETON_TYPE_BUDDY;
	gNewObjectDefinition.animNum 	= 0;							
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR) + 30;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;	
	gNewObjectDefinition.slot 		= PLAYER_SLOT+1;
	gNewObjectDefinition.moveCall 	= MoveMyBuddy;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .3;
	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	GAME_ASSERT(newObj);

	newObj->Mode = BUDDY_MODE_LIKESME;

	SetObjectCollisionBounds(newObj, 40,-40,-40,40,40,-40);


	AttachShadowToObject(newObj, 1, 1, true);

	gMyBuddy = newObj;

}

/********************* MOVE MY BUDDY ************************/

static void MoveMyBuddy(ObjNode *theNode)
{

	GetObjectInfo(theNode);

	switch(theNode->Mode)
	{
		case	BUDDY_MODE_LIKESME:
				BuddyFollowsMe(theNode);
				if (GetNewKeyState(kKey_BuddyAttack) && gPlayerCanMove)
				{
					gMyBuddy = nil;							// buddy isnt attached to me anymore
					theNode->Mode = BUDDY_MODE_ATTACK;					
					PlayEffect3D(EFFECT_BUDDYLAUNCH, &theNode->Coord);
					break;
				}
				break;
		
		case	BUDDY_MODE_ATTACK:
				if (MoveBuddyTowardEnemy(theNode))
					return;
				break;
	}

		/**********/
		/* UPDATE */
		/**********/

	if (theNode->EffectChannel == -1)
		theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_BUZZ, &gCoord, kMiddleC+5, .1f);
	else
		Update3DSoundChannel(EFFECT_BUZZ, &theNode->EffectChannel, &gCoord);


	UpdateObject(theNode);
}


/****************** BUDDY FOLLOWS ME ***********************/

static void BuddyFollowsMe(ObjNode *theNode)
{

TQ3Point3D	from,target;
float		distX,distZ,dist;
TQ3Vector2D	pToC;
float		myX,myY,myZ;
float		fps = gFramesPerSecondFrac;


			/*************************/
			/* UPDATE BUDDY POSITION */
			/*************************/

	myX = gMyCoord.x;
	myY = gMyCoord.y + gPlayerObj->BottomOff;
	myZ = gMyCoord.z;

	pToC.x = gCoord.x - myX;									// calc player->buddy vector
	pToC.y = gCoord.z - myZ;
	FastNormalizeVector2D(pToC.x, pToC.y, &pToC);				// normalize it
	
	target.x = myX + (pToC.x * BUDDY_DIST_FROM_ME);				// target is appropriate dist based on buddy's current coord
	target.z = myZ + (pToC.y * BUDDY_DIST_FROM_ME);


			/* MOVE BUDDY TOWARDS POINT */
					
	distX = target.x - gCoord.x;
	distZ = target.z - gCoord.z;
	
	if (distX > 500.0f)											// pin max accel factor
		distX = 500.0f;
	else
	if (distX < -500.0f)
		distX = -500.0f;
	if (distZ > 500.0f)
		distZ = 500.0f;
	else
	if (distZ < -500.0f)
		distZ = -500.0f;
		
	from.x = gCoord.x+(distX * (fps * BUDDY_ACCEL));
	from.z = gCoord.z+(distZ * (fps * BUDDY_ACCEL));


		/* CALC FROM Y */

	dist = CalcQuickDistance(from.x, from.z, myX, myZ) - BUDDY_CLOSEST;
	if (dist < 0.0f)
		dist = 0.0f;
	
	target.y = myY + (dist*BUDDY_HEIGHT_FACTOR) + BUDDY_MINY;	// calc desired y based on dist and height factor


		/* MOVE ABOVE ANY SOLID OBJECT */

	if (DoSimpleBoxCollision(target.y + 100.0f, target.y - 100.0f,
							target.x - 100.0f, target.x + 100.0f,
							target.z + 100.0f, target.z - 100.0f,
							CTYPE_MISC|CTYPE_ENEMY|CTYPE_TRIGGER))
	{
		ObjNode *obj = gCollisionList[0].objectPtr;					// get collided object
		if (obj)
		{
			CollisionBoxType *coll = obj->CollisionBoxes;			// get object's collision box
			if (coll)
			{
				target.y = coll->top + 100.0f;						// set target on top of object			
			}
		}
	}

	dist = (target.y - gCoord.y)*BUDDY_ACCEL;						// calc dist from current y to desired y
	from.y = gCoord.y+(dist*fps);
	
	if (gDoCeiling)
	{
				/* MAKE SURE NOT ABOVE CEILING */
		
		dist = GetTerrainHeightAtCoord(from.x, from.z, CEILING) - 30.0f;
		if (from.y > dist)
			from.y = dist;
	}

			/* MAKE SURE NOT UNDERGROUND */
			
	dist = GetTerrainHeightAtCoord(from.x, from.z, FLOOR) + 50.0f;
	if (from.y < dist)
		from.y = dist;

	gCoord = from;
	
	
				/* AIM HIM AT ME */
				
	TurnObjectTowardTarget(theNode, &from, myX, myZ, 3.0, false);	
	
	
	/* MATCH UNDERWATER BITS SO SHADOW WILL GO AWAY AUTOMATICALLY */

	if (gPlayerObj->StatusBits & STATUS_BIT_UNDERWATER)
		theNode->StatusBits |= STATUS_BIT_UNDERWATER;
	else
		theNode->StatusBits &= ~STATUS_BIT_UNDERWATER;
	
}


/******************* MOVE BUDDY TOWARD ENEMY *************************/
//
// Returns true if buddy was deleted
//

static Boolean MoveBuddyTowardEnemy(ObjNode *theNode)
{
ObjNode	*enemy;
float	fps = gFramesPerSecondFrac;
float	dist;
	
			/* FIND CLOSEST ENEMY */
			
	enemy = FindClosestEnemy(&gCoord, &dist);
	
	
			/* AIM AT ENEMY */
			
	if (enemy)
	{
		TurnObjectTowardTarget(theNode, &gCoord, enemy->Coord.x, enemy->Coord.z, 4.0, false);	

		if (gCoord.y < (enemy->Coord.y + enemy->BottomOff))
			gDelta.y += 400.0f * fps;
		else
		if (gCoord.y > (enemy->Coord.y + enemy->TopOff))
			gDelta.y -= 400.0f * fps;
		else
			gDelta.y *= .5f;
	}


			/* CALC DELTA & MOVE */
			
	gDelta.x = -sin(theNode->Rot.y) * BUDDY_ATTACK_SPEED;
	gDelta.z = -cos(theNode->Rot.y) * BUDDY_ATTACK_SPEED;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


	
		/* SEE IF HIT FLOOR OR CEILING */
		
	if ((gCoord.y < GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR)) ||
		(gCoord.y > GetTerrainHeightAtCoord(gCoord.x, gCoord.z, CEILING)))
	{
explode:	
		SplatterBuddy(theNode);
		return(true);
	}

		/* SEE IF HIT SOLID */
		
	if (DoSimplePointCollision(&gCoord, CTYPE_MISC))
		goto explode;


		/********************/
		/* SEE IF HIT ENEMY */
		/********************/

	HandleCollisions(theNode, CTYPE_ENEMY);					// collide against enemies
	if (gNumCollisions)
	{
		EnemyGotHurt(gCollisionList[0].objectPtr, 1.1);		// cause massive damage
		goto explode;
	}
	
		/***************/
		/* SEE IF GONE */
		/***************/
			
	if (TrackTerrainItem_Far(theNode, 500))	
	{
		DeleteObject(theNode);
		return(true);
	}
	

	return(false);
}


/********************* SPLATTER BUDDY **************************/

static void SplatterBuddy(ObjNode *theNode)
{
TQ3Vector3D		delta;
TQ3Point3D		coord;

	DeleteObject(theNode);
	
				/* MAKE PARTICLE EXPLOSION */

	int32_t pg = NewParticleGroup(
							PARTICLE_TYPE_FALLINGSPARKS,	// type
							PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_HOT,		// flags
							550,						// gravity
							0,							// magnetism
							35,							// base scale
							.7,							// decay rate
							0,							// fade rate
							PARTICLE_TEXTURE_WHITE);	// texture
	
	if (pg != -1)
	{
		for (int i = 0; i < 100; i++)
		{
			delta.x = (RandomFloat()-.5f) * 800.0f;
			delta.y = (RandomFloat()-.4f) * 800.0f;
			delta.z = (RandomFloat()-.5f) * 800.0f;
			
			coord.x = gCoord.x + ((RandomFloat()-.5f) * 80.0f);
			coord.y = gCoord.y + ((RandomFloat()-.5f) * 80.0f);
			coord.z = gCoord.z + ((RandomFloat()-.5f) * 80.0f);
			
			AddParticleToGroup(pg, &coord, &delta, RandomFloat() + 1.0f, FULL_ALPHA);
		}
	}
	
	
		/* EXPLODE EFFECT */
		
	PlayEffect_Parms3D(EFFECT_FIRECRACKER, &gCoord, kMiddleC-12, 3.0);
}


#pragma mark -


/********************** UPDATE PLAYER SHIELD ***********************/
//
// Called from player update function to see if need to maintain shield.
//

void UpdatePlayerShield(void)
{
TQ3Vector3D	delta;
TQ3Point3D  pt;
float		r;
			

	if (gShieldTimer == 0.0f)
		return;
	
			/* DEC TIMER */
			
	gShieldTimer -= gFramesPerSecondFrac;
	if (gShieldTimer < 0.0f)
	{
		gShieldTimer = 0.0f;
		if (gShieldChannel != -1)
			StopAChannel(&gShieldChannel);
		gShieldParticleGroupA = gShieldParticleGroupB = -1;
	}
	
		/* UPDATE SOUND */
		
	else
	{
		if (gShieldChannel == -1)
		{
			gShieldChannel = PlayEffect_Parms(EFFECT_SHIELD, 200, 200, kMiddleC);	
		}
	}

			/************************/
			/* SPEW RINGS PARTICLES */
			/************************/


	r = gShieldRot += gFramesPerSecondFrac * 5.0f;

	gShieldRingTick += gFramesPerSecondFrac;
	if (gShieldRingTick > .01f)
	{
		gShieldRingTick = 0;

		pt = gMyCoord;
		pt.y += gPlayerObj->BottomOff + 100.0f;
		delta.y = 0;


				/* DO PARTICLE GROUP A */
				
		if (!VerifyParticleGroup(gShieldParticleGroupA))
		{
new_groupa:
			gShieldParticleGroupA = NewParticleGroup(
													PARTICLE_TYPE_FALLINGSPARKS,	// type
													0,								// flags
													0,								// gravity
													0,								// magnetism
													25,								// base scale
													1.6,							// decay rate
													0,								// fade rate
													PARTICLE_TEXTURE_WHITE);		// texture
		}

		if (gShieldParticleGroupA != -1)
		{			
			delta.x = sin(r) * 250.0f;
			delta.z = cos(r) * 250.0f;
			
			if (AddParticleToGroup(gShieldParticleGroupA, &pt, &delta, RandomFloat() + 1.5f, FULL_ALPHA))
				goto new_groupa;
		}

		
		if (gShieldTimer > 2.0f)			// when 2 seconds remain, use 1 color
		{
					/* DO PARTICLE GROUP B */
					
			if (!VerifyParticleGroup(gShieldParticleGroupB))
			{
new_groupb:
				gShieldParticleGroupB = NewParticleGroup(
														PARTICLE_TYPE_FALLINGSPARKS,	// type
														0,								// flags
														0,								// gravity
														0,								// magnetism
														25,								// base scale
														1.6,							// decay rate
														0,								// fade rate
														PARTICLE_TEXTURE_ORANGESPOT);		// texture
			}

			if (gShieldParticleGroupB != -1)
			{
				r += PI;
				delta.x = sin(r) * 250.0f;
				delta.z = cos(r) * 250.0f;
				
				if (AddParticleToGroup(gShieldParticleGroupB, &pt, &delta, RandomFloat() + 1.5f, FULL_ALPHA))
					goto new_groupb;
			}
		}

	}

}







