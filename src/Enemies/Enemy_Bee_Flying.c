/****************************/
/*   ENEMY: FLYINGBEE.C		*/
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

static void MoveFlyingBee(ObjNode *theNode);
static void MoveFlyingBee_Flying(ObjNode *theNode);
static void  MoveFlyingBee_Diving(ObjNode *theNode);
static void UpdateFlyingBee(ObjNode *theNode);
static void  MoveFlyingBee_Fall(ObjNode *theNode);
static void  MoveFlyingBee_Dead(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_FLYINGBEE				4
#define	MAX_FLYINGBEE2				8

#define	FLYINGNBEE_KNOCKDOWN_SPEED	1100.0f

#define	FLYINGBEE_CHASE_RANGE		2000.0f
#define	FLYINGBEE_ATTACK_RANGE		450.0f
#define	FLYINGBEE_ATTACK_RANGE2		400.0f
#define	DIVE_MOVE_SPEED				500.0f
#define	DIVE_MOVE_SPEED2			1100.0f

#define	FLYINGBEE_HEALTH			1.0f		
#define	FLYINGBEE_DAMAGE			0.1f
#define	FLYINGBEE_SCALE				.8f



enum
{
	FLYINGBEE_ANIM_FLY,
	FLYINGBEE_ANIM_DIVE,
	FLYINGBEE_ANIM_FALL,
	FLYINGBEE_ANIM_DEATH
};


#define	FLYINGBEE_KNOCKDOWN_SPEED	1400.0f		// speed ball needs to go to knock this down


#define	BEE_ACCEL			1.7f
#define	BEE_CLOSEST			10.0f
#define	BEE_HEIGHT_FACTOR	0.17f
#define	BEE_MINY			170.0f
#define	BEE_MINY2			40.0f


/*********************/
/*    VARIABLES      */
/*********************/

#define	DistFromMe		SpecialF[3]

/************************ ADD FLYINGBEE ENEMY *************************/
//
// parm[0] = yoffset * 100 units or 0 = default.
//

Boolean AddEnemy_FlyingBee(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gRealLevel == LEVEL_NUM_HIVE)
	{
		if (gNumEnemyOfKind[ENEMY_KIND_FLYINGBEE] >= MAX_FLYINGBEE)						// keep from getting absurd
			return(false);
	}
	else
	{
		if (gNumEnemyOfKind[ENEMY_KIND_FLYINGBEE] >= MAX_FLYINGBEE2)						// keep from getting absurd
			return(false);
	}

		/* SEE IF KEYED ENEMY ON HIVE */
		
	if (gRealLevel == LEVEL_NUM_HIVE)
	{
		if (itemPtr->parm[3] & 1)										// see if we care
		{
			if (!gDetonatorBlown[itemPtr->parm[1]])						// see if detonator has been triggered
				return(false);		
		}
	}

				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_FLYINGBEE,x,z,FLYINGBEE_SCALE);
	if (newObj == nil)
		return(false);
	newObj->TerrainItemPtr = itemPtr;

	if (gLevelType == LEVEL_TYPE_HIVE)
	{
			newObj->Coord.y += 200.0f;			// raise off ground	
	}
	else
	{			
		if (itemPtr->parm[0] == 0)
			newObj->Coord.y += 600.0f;			// raise off ground	
		else
			newObj->Coord.y += (float)itemPtr->parm[0] * 100.0f;
	}
	
	SetSkeletonAnim(newObj->Skeleton, FLYINGBEE_ANIM_FLY);
	

				/* SET BETTER INFO */
			
	newObj->MoveCall 	= MoveFlyingBee;						// set move call
	newObj->Health 		= FLYINGBEE_HEALTH;
	newObj->Damage 		= FLYINGBEE_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_FLYINGBEE;
	newObj->CBits 		= CBITS_NOTTOP;
	
	newObj->DistFromMe	= RandomFloat() * 300.0f + 100.0f;
		 

				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 90,-50,-90,90,90,-90);


				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, 8, 8, false);


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_FLYINGBEE]++;
	return(true);
}


/************************ MAKE FLYINGBEE ENEMY *************************/

Boolean MakeFlyingBee(TQ3Point3D *where)
{
ObjNode	*newObj;

	if (gNumEnemyOfKind[ENEMY_KIND_FLYINGBEE] >= MAX_FLYINGBEE)						// keep from getting absurd
		return(false);

				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_FLYINGBEE,where->x,where->z,FLYINGBEE_SCALE);
	if (newObj == nil)
		return(false);
			
	newObj->Coord.y = where->y;		
	
	SetSkeletonAnim(newObj->Skeleton, FLYINGBEE_ANIM_FLY);
	

				/* SET BETTER INFO */
			
	newObj->MoveCall 	= MoveFlyingBee;						// set move call
	newObj->Health 		= FLYINGBEE_HEALTH;
	newObj->Damage 		= FLYINGBEE_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_FLYINGBEE;
	newObj->CType		|= CTYPE_SPIKED;
	newObj->CBits 		= CBITS_NOTTOP;
	
	newObj->DistFromMe	= RandomFloat() * 300.0f + 100.0f;
		 

				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 90,-50,-90,90,90,-90);


				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, 8, 8, false);

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_FLYINGBEE]++;
	return(true);
}



/********************* MOVE FLYINGBEE **************************/

static void MoveFlyingBee(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveFlyingBee_Flying,
					MoveFlyingBee_Diving,
					MoveFlyingBee_Fall,
					MoveFlyingBee_Dead
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}


/********************** MOVE FLYINGBEE: FLYING ******************************/

static void  MoveFlyingBee_Flying(ObjNode *theNode)
{
TQ3Point3D	from,target;
float		distX,distZ,dist;
TQ3Vector2D	pToC;
float		myX,myY,myZ,dy;
float		fps = gFramesPerSecondFrac;

	theNode->CType	&= ~CTYPE_SPIKED;						// not spiked now


	myX = gMyCoord.x;
	myY = gMyCoord.y + gPlayerObj->BottomOff;
	myZ = gMyCoord.z;


			/* SEE IF CLOSE ENOUGH TO CHASE */
			
	dist = CalcQuickDistance(gCoord.x, gCoord.z, myX, myZ);
	if (dist < FLYINGBEE_CHASE_RANGE)
	{
				/*******************/
				/* UPDATE POSITION */
				/*******************/

		pToC.x = gCoord.x - myX;									// calc player->buddy vector
		pToC.y = gCoord.z - myZ;
		FastNormalizeVector2D(pToC.x, pToC.y, &pToC);				// normalize it
		
		dist = theNode->DistFromMe;
		if (gCurrentDragonFly)
			dist += 100.0f;
		
		target.x = myX + (pToC.x * dist);					// target is appropriate dist based on buddy's current coord
		target.z = myZ + (pToC.y * dist);


				/* MOVE BUDDY TOWARDS POINT */
						
		distX = target.x - gCoord.x;
		distZ = target.z - gCoord.z;
		
		if (distX > 300.0f)											// pin max accel factor
			distX = 300.0f;
		else
		if (distX < -300.0f)
			distX = -300.0f;
		if (distZ > 300.0f)
			distZ = 300.0f;
		else
		if (distZ < -300.0f)
			distZ = -300.0f;
			
		from.x = gCoord.x+(distX * (fps * BEE_ACCEL));
		from.z = gCoord.z+(distZ * (fps * BEE_ACCEL));


			/* CALC FROM Y */

		dist = CalcQuickDistance(from.x, from.z, myX, myZ) - BEE_CLOSEST;
		if (dist < 0.0f)
			dist = 0.0f;
		
		if (gPlayerMode == PLAYER_MODE_BALL)
			target.y = myY + (dist*BEE_HEIGHT_FACTOR) + BEE_MINY2;		// calc desired y based on dist and height factor		
		else
			target.y = myY + (dist*BEE_HEIGHT_FACTOR) + BEE_MINY;


		dist = (target.y - gCoord.y)*BEE_ACCEL;						// calc dist from current y to desired y
		from.y = gCoord.y+(dist*fps);
		
		if (gDoCeiling)
		{
					/* MAKE SURE NOT ABOVE CEILING */
			
			dist = GetTerrainHeightAtCoord(from.x, from.z, CEILING) - 100.0f;
			if (from.y > dist)
				from.y = dist;
		}

				/* MAKE SURE NOT UNDERGROUND */
				
		dist = GetTerrainHeightAtCoord(from.x, from.z, FLOOR) + 50.0f;
		if (from.y < dist)
			from.y = dist;

		gCoord = from;
	}	
	
				/* AIM HIM AT ME */
				
	TurnObjectTowardTarget(theNode, &gCoord, myX, myZ, 3.0, false);	


				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/

	gDelta.x = gCoord.x - theNode->OldCoord.x;						// calc legit deltas
	gDelta.y = gCoord.y - theNode->OldCoord.y;				
	gDelta.z = gCoord.z - theNode->OldCoord.z;

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

			/*********************************/
			/* SEE IF CLOSE ENOUGH TO ATTACK */
			/*********************************/

	dy = gCoord.y - gMyCoord.y;

			/* PLAYER ON DRAGONFLY */
			
	if (gCurrentDragonFly)
	{
		if ((dy > 50.0f) && (dy < 900.0f))					// must be above player, but not too far above
		{
			if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < FLYINGBEE_ATTACK_RANGE2)
			{
				MorphToSkeletonAnim(theNode->Skeleton, FLYINGBEE_ANIM_DIVE, 8);
				gDelta.x *= .3f;
				gDelta.z *= .3f;
			}
		}
	}
	
			/* PLAYER NOT ON DRAGONFLY */
	else
	{
		if ((dy > 200.0f) && (dy < 800.0f))					// must be above player, but not too far above
		{
			if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < FLYINGBEE_ATTACK_RANGE)
			{
				MorphToSkeletonAnim(theNode->Skeleton, FLYINGBEE_ANIM_DIVE, 8);
				gDelta.x *= .4f;
				gDelta.z *= .4f;
			}
		}
	}

				/* UPDATE */
		
	UpdateFlyingBee(theNode);
		
}


/********************** MOVE FLYINGBEE: DIVING ******************************/

static void  MoveFlyingBee_Diving(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	speed,y;
	
	theNode->CType	|= CTYPE_SPIKED;						// spiked now
	

			/* ON DRAGONFLY */
			
	if (gCurrentDragonFly)
	{
		gDelta.y -= 800.0f * fps;			// go down
		speed = DIVE_MOVE_SPEED2 * fps;
	}
	else
	{
		gDelta.y -= 1100.0f * fps;			// go down
		speed = DIVE_MOVE_SPEED * fps;
	}

	if (gCoord.x < gMyCoord.x)	
		gDelta.x += speed;
	else
		gDelta.x -= speed;

	if (gCoord.z < gMyCoord.z)	
		gDelta.z += speed;
	else
		gDelta.z -= speed;

			
	MoveEnemy(theNode);
	
	
			/* COLLISION */
			
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


			/**********************/
			/* SEE IF DONE DIVING */
			/**********************/
			
	y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR);						// get terrain y
			
	if ((theNode->StatusBits & STATUS_BIT_ONGROUND) ||							// if hit ground or other solid
		 (gCoord.y < gMyCoord.y) ||												// if below player
		 ((gCoord.y - y) < 50.0f))												// if too close to ground
	{		
		MorphToSkeletonAnim(theNode->Skeleton, 	FLYINGBEE_ANIM_FLY, 3);
		gDelta.x *= .5f;
		gDelta.y *= .5f;
		gDelta.z *= .5f;
	}
		
	
			/* UPDATE */
			
	TurnObjectTowardTarget(theNode, &theNode->OldCoord, gCoord.x, gCoord.z, 5, false);	

	UpdateFlyingBee(theNode);
}


/********************** MOVE FLYINGBEE: FALL ******************************/

static void  MoveFlyingBee_Fall(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
	
	theNode->CType	&= ~CTYPE_SPIKED;						// not spiked now	

	gDelta.y -= 800.0f * fps;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;
			
	MoveEnemy(theNode);
	
	
			/* COLLISION */
			
	if (DoEnemyCollisionDetect(theNode,DEATH_ENEMY_COLLISION_CTYPES))
		return;


	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
		MorphToSkeletonAnim(theNode->Skeleton, FLYINGBEE_ANIM_DEATH, 8);


	UpdateEnemy(theNode);
}


/********************** MOVE FLYINGBEE: DEATH ******************************/

static void  MoveFlyingBee_Dead(ObjNode *theNode)
{	

			/* SEE IF GONE */
			
	if (theNode->StatusBits & STATUS_BIT_ISCULLED)			// if was culled on last frame then delete it
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
			
	UpdateEnemy(theNode);		
}


#pragma mark -




/****************** KILL FLYINGBEE *********************/
//
// OUTPUT: true if ObjNode was deleted
//

Boolean KillFlyingBee(ObjNode *theNode, float dx, float dy, float dz)
{
	(void) dx;
	(void) dy;
	(void) dz;

TQ3Vector3D		delta;

	
		/* STOP BUZZ */
		
	if (theNode->EffectChannel != -1)
		StopAChannel(&theNode->EffectChannel);
	
			/* DEACTIVATE */
			
	if (gRealLevel != LEVEL_NUM_FLIGHT)				// always regenerate bees on flight attack level
		theNode->TerrainItemPtr = nil;				// dont ever come back
	
	MorphToSkeletonAnim(theNode->Skeleton, FLYINGBEE_ANIM_FALL, 5);
	
	theNode->BottomOff = 0;							// change bottom
	theNode->CType = 0;								// no more collision
	
	
			/*******************/
			/* SPARK EXPLOSION */
			/*******************/

			/* white sparks */
				
	int32_t pg = NewParticleGroup(
							PARTICLE_TYPE_FALLINGSPARKS,	// type
							PARTICLE_FLAGS_BOUNCE,		// flags
							400,						// gravity
							0,							// magnetism
							20,							// base scale
							.8,							// decay rate
							0,							// fade rate
							PARTICLE_TEXTURE_YELLOWBALL);	// texture
	
	for (int i = 0; i < 60; i++)
	{
		delta.x = (RandomFloat()-.5f) * 1400.0f;
		delta.y = (RandomFloat()-.5f) * 1400.0f;
		delta.z = (RandomFloat()-.5f) * 1400.0f;
		AddParticleToGroup(pg, &theNode->Coord, &delta, RandomFloat() + 1.0f, FULL_ALPHA);
	}
	
	
	return(false);
}


/***************** UPDATE FLYING BEE *******************/

static void UpdateFlyingBee(ObjNode *theNode)
{
	if (theNode->EffectChannel == -1)
		theNode->EffectChannel = PlayEffect3D(EFFECT_BUZZ, &gCoord);
	else
		Update3DSoundChannel(EFFECT_BUZZ, &theNode->EffectChannel, &gCoord);
		
	UpdateEnemy(theNode);
}


/******************* BALL HIT FLYING BEE *********************/
//
// OUTPUT: true if enemy gets killed
//

Boolean BallHitFlyingBee(ObjNode *me, ObjNode *enemy)
{
	if (me->Speed > FLYINGNBEE_KNOCKDOWN_SPEED)
	{	
		KillFlyingBee(enemy, gDelta.x * .3f, gDelta.y * .3f, gDelta.z * .3f);
		PlayEffect_Parms3D(EFFECT_POUND, &gCoord, kMiddleC+2, 2.0);
		return(true);		
	}
	
	return(false);
}




























