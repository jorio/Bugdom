/****************************/
/*   ENEMY: FIREANT.C		*/
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

static void MoveFireAnt(ObjNode *theNode);
static void MoveFireAnt_Standing(ObjNode *theNode);
static void  MoveFireAnt_Fly(ObjNode *theNode);
static void  MoveFireAnt_Walking(ObjNode *theNode);
static void  MoveFireAnt_FallOnButt(ObjNode *theNode);
static void  MoveFireAnt_GetOffButt(ObjNode *theNode);
static void  MoveFireAnt_Death(ObjNode *theNode);
static void  MoveFireAnt_StandBreathFire(ObjNode *theNode);

static void MoveFireAntOnSpline(ObjNode *theNode);

static void UpdateFireAnt(ObjNode *theNode, Boolean updateFlame);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	FIREANT_HEAD_LIMB			1						// joint # of head

#define	FIREANT_SCALE				1.2f

#define	FIREANT_START_FLY_DIST		500.0f
#define	FIREANT_END_FLY_DIST		1000.0f
#define	FIREANT_ATTACK_DIST			900.0f
#define	FIREANT_TARGET_OFFSET		200.0f


#define FIREANT_TURN_SPEED			2.4f
#define FIREANT_WALK_SPEED			400.0f
#define FIREANT_FLY_SPEED			200.0f
#define	FIREANT_KNOCKDOWN_SPEED		1400.0f					// speed ball needs to go to knock this down

#define	FIREANT_DAMAGE				0.2f

#define	FIREANT_FOOT_OFFSET			-130.0f

#define	TIME_TO_BREATH				4.0f					// # seconds to breath fire
#define	FIREANT_FLY_HEIGHT			320.0f



		/* MODES */
		
enum
{
	FIREANT_MODE_NONE
};


/*********************/
/*    VARIABLES      */
/*********************/

#define Dying			Flag[2]					// set during butt fall to indicate death after fall
#define BreathTimer		SpecialF[2]				// timer for breathing fire
#define BreathRegulator	SpecialF[3]				// timer for fire spewing regulation

#define	ButtTimer			SpecialF[0]			// timer for on butt
#define FireParticleMagicNum SpecialL[2]
#define FireTimer 			SpecialF[1]



/************************ ADD FIREANT ENEMY *************************/
//
// parm[3]:bit0 = always add
//

Boolean AddEnemy_FireAnt(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

				/* SEE IF TOO MANY */
				
//	if (gNumEnemies >= MAX_ENEMIES)								// keep from getting absurd
//		return(false);
	
	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_FIREANT] > MAX_ENEMIES)		// only care if too many of this kind
			return(false);
	}
	
	
				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_FIREANT,x,z, FIREANT_SCALE);
	if (newObj == nil)
		return(false);
	newObj->TerrainItemPtr = itemPtr;

	SetSkeletonAnim(newObj->Skeleton, FIREANT_ANIM_STAND);
	

				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->Coord.y 	-= FIREANT_FOOT_OFFSET;			
	newObj->MoveCall 	= MoveFireAnt;							// set move call
	newObj->Health 		= 1.0;
	newObj->Damage 		= FIREANT_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_FIREANT;
	newObj->CType		|= CTYPE_KICKABLE|CTYPE_AUTOTARGET;		// these can be kicked
	newObj->CBits		= CBITS_NOTTOP;
	
	newObj->Mode		= FIREANT_MODE_NONE;
	
	newObj->BreathTimer 		= 0;
	newObj->BreathParticleGroup = -1;
	newObj->BreathRegulator 	= 0;
	
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 70,FIREANT_FOOT_OFFSET,-70,70,70,-70);
	CalcNewTargetOffsets(newObj,FIREANT_TARGET_OFFSET);


				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, 8, 8,false);

	newObj->InitCoord = newObj->Coord;							// remember where started
	
		
	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_FIREANT]++;
	return(true);
}




/********************* MOVE FIREANT **************************/

static void MoveFireAnt(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveFireAnt_Standing,
					MoveFireAnt_Walking,
					MoveFireAnt_StandBreathFire,
					MoveFireAnt_Fly,
					MoveFireAnt_FallOnButt,
					MoveFireAnt_GetOffButt,
					MoveFireAnt_Death,		
				};


	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
	
}




/********************** MOVE FIREANT: STANDING ******************************/

static void  MoveFireAnt_Standing(ObjNode *theNode)
{
float	angleToTarget,dist;

				/* TURN TOWARDS ME */
				
	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, FIREANT_TURN_SPEED, false);			


		/* IF I'M TOO CLOSE, THEN GO INTO FLY MODE */
		
	dist = CalcQuickDistance(gMyCoord.x, gMyCoord.z, gCoord.x, gCoord.z);

	if (dist < FIREANT_START_FLY_DIST)
	{
		MorphToSkeletonAnim(theNode->Skeleton, FIREANT_ANIM_FLY, 7);
		gDelta.y = 0;
	}

				/* SEE IF ATTACK */

	else
	if (angleToTarget < .03f)
	{
		if (dist < FIREANT_ATTACK_DIST)
		{					
			MorphToSkeletonAnim(theNode->Skeleton, FIREANT_ANIM_STANDFIRE, 8);
			theNode->BreathTimer = 0;
			theNode->BreathParticleGroup = -1;
			theNode->BreathRegulator = 0;
		}
	}

			/* MOVE */
				
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);

				

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


	UpdateFireAnt(theNode, true);		
}


/********************** MOVE FIREANT: BREATH FIRE ******************************/

static void  MoveFireAnt_StandBreathFire(ObjNode *theNode)
{
float	dist;
					/* TURN TOWARDS ME */
					
	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, FIREANT_TURN_SPEED, false);			


					/* BREATH FIRE */
					
	FireAntBreathFire(theNode);
	theNode->BreathTimer += gFramesPerSecondFrac;
	if (theNode->BreathTimer > TIME_TO_BREATH)
	{
		MorphToSkeletonAnim(theNode->Skeleton, FIREANT_ANIM_STAND, 4);	
	}

			/* MOVE */
				
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


			/* IF I'M TOO CLOSE, THEN GO INTO FLY MODE */
			
	dist = CalcQuickDistance(gMyCoord.x, gMyCoord.z, gCoord.x, gCoord.z);
	if (dist < FIREANT_START_FLY_DIST)
	{
		MorphToSkeletonAnim(theNode->Skeleton, FIREANT_ANIM_FLY, 7);
		gDelta.y = 0;
	}


	UpdateFireAnt(theNode, false);		
}


/********************** MOVE FIREANT: WALKING ******************************/

static void  MoveFireAnt_Walking(ObjNode *theNode)
{
float		r,fps;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */
			
	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, FIREANT_TURN_SPEED, false);			

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * FIREANT_WALK_SPEED;
	gDelta.z = -cos(r) * FIREANT_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity

	MoveEnemy(theNode);
	
	
		
				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == FIREANT_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = FIREANT_WALK_SPEED * .0032f;
	

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;
	
	UpdateFireAnt(theNode, true);		
}


/********************** MOVE FIREANT: FLY ******************************/

static void  MoveFireAnt_Fly(ObjNode *theNode)
{
float		r,fps,y;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */
			
	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, FIREANT_TURN_SPEED, false);			

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * FIREANT_FLY_SPEED;
	gDelta.z = -cos(r) * FIREANT_FLY_SPEED;
	gCoord.x += gDelta.x*fps;
	gCoord.z += gDelta.z*fps;
	
			/* HOVER ABOVE GROUND */
			
	y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR) + FIREANT_FLY_HEIGHT;		// get target Y
	if (gCoord.y < y)
	{
		gDelta.y += 1000.0f*fps;
		gCoord.y += gDelta.y*fps;				
		if (gCoord.y >= y)
		{
			gCoord.y = y;
			gDelta.y = 0;
		}
	}
	else
	if (gCoord.y > y)
	{
		gDelta.y -= 1000.0f*fps;				
		gCoord.y += gDelta.y*fps;				
		if (gCoord.y <= y)
		{
			gCoord.y = y;
			gDelta.y = 0;
		}
	}
	else
		gDelta.y = 0;	


					/* BREATH FIRE */
					
	FireAntBreathFire(theNode);

	
				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;
		

			/* IF FAR AWAY, STOP FLYING*/
			
	if (CalcQuickDistance(gMyCoord.x, gMyCoord.z, gCoord.x, gCoord.z) > FIREANT_END_FLY_DIST)
	{
		MorphToSkeletonAnim(theNode->Skeleton, FIREANT_ANIM_STAND, 7);
		gDelta.x = gDelta.z = gDelta.y = 0;
	}

	UpdateFireAnt(theNode, false);		
}



/********************** MOVE FIREANT: FALLONBUTT ******************************/

static void  MoveFireAnt_FallOnButt(ObjNode *theNode)
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
			SetSkeletonAnim(theNode->Skeleton, FIREANT_ANIM_DIE);
			goto update;
		}
		else
			SetSkeletonAnim(theNode->Skeleton, FIREANT_ANIM_GETOFFBUTT);
	}


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


update:
	UpdateFireAnt(theNode, false);		
}


/********************** MOVE FIREANT: DEATH ******************************/

static void  MoveFireAnt_Death(ObjNode *theNode)
{
	
			/* SEE IF GONE */
			
	if (theNode->StatusBits & STATUS_BIT_ISCULLED)		// if was culled on last frame and is far enough away, then delete it
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) > 600.0f)
		{
			DeleteEnemy(theNode);
			return;
		}
	}


				/* MOVE IT */
				
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(60.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEATH_ENEMY_COLLISION_CTYPES))
		return;


				/* UPDATE */
			
	UpdateFireAnt(theNode, false);		

}


/********************** MOVE FIREANT: GET OFF BUTT ******************************/

static void  MoveFireAnt_GetOffButt(ObjNode *theNode)
{
	ApplyFrictionToDeltas(60.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity

	MoveEnemy(theNode);


				/* SEE IF DONE */
			
	if (theNode->Skeleton->AnimHasStopped)
	{
		SetSkeletonAnim(theNode->Skeleton, FIREANT_ANIM_STAND);
	}


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

	UpdateFireAnt(theNode, true);		
}



//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME FIREANT ENEMY *************************/

Boolean PrimeEnemy_FireAnt(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj,*shadowObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_FIREANT,x,z, FIREANT_SCALE);
	if (newObj == nil)
		return(false);
		
	DetachObject(newObj);										// detach this object from the linked list
		
	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;
	
	SetSkeletonAnim(newObj->Skeleton, FIREANT_ANIM_WALK);
		

				/* SET BETTER INFO */
			
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= FIREANT_FOOT_OFFSET;			
	newObj->SplineMoveCall 	= MoveFireAntOnSpline;				// set move call
	newObj->Health 			= 1.0;
	newObj->Damage 			= FIREANT_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_FIREANT;
	newObj->CType			|= CTYPE_KICKABLE|CTYPE_AUTOTARGET;	// these can be kicked
	newObj->CBits			= CBITS_NOTTOP;
	
	newObj->Mode		= FIREANT_MODE_NONE;
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 70,FIREANT_FOOT_OFFSET,-70,70,70,-70);
	CalcNewTargetOffsets(newObj,FIREANT_TARGET_OFFSET);


	newObj->InitCoord = newObj->Coord;							// remember where started

				/* MAKE SHADOW */
				
	shadowObj = AttachShadowToObject(newObj, 8, 8, false);
	DetachObject(shadowObj);									// detach this object from the linked list


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	AddToSplineObjectList(newObj);

	return(true);
}


/******************** MOVE FIREANT ON SPLINE ***************************/

static void MoveFireAntOnSpline(ObjNode *theNode)
{
Boolean isVisible; 

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 100);
	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/* UPDATE STUFF IF VISIBLE */
			
	if (isVisible)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);		

		theNode->Coord.y = GetTerrainHeightAtCoord(theNode->Coord.x, theNode->Coord.z, FLOOR) - FIREANT_FOOT_OFFSET;	// calc y coord
		UpdateObjectTransforms(theNode);																// update transforms
		CalcObjectBoxFromNode(theNode);																	// update collision box
		UpdateShadow(theNode);																			// update shadow		
	}
	
			/* HIDE SOME THINGS SINCE INVISIBLE */
	else
	{
//		if (theNode->ShadowNode)						// hide shadow
//			theNode->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;	
	}
	
}




#pragma mark -

/******************* BALL HIT FIREANT *********************/
//
// OUTPUT: true if enemy gets killed
//

Boolean BallHitFireAnt(ObjNode *me, ObjNode *enemy)
{
long			pg,i;
TQ3Vector3D		delta;
Boolean	killed = false;

				
	if (me->Speed > FIREANT_KNOCKDOWN_SPEED)
	{	

				/*******************/
				/* SPARK EXPLOSION */
				/*******************/

				/* white sparks */
					
		pg = NewParticleGroup(	0,							// magic num
								PARTICLE_TYPE_FALLINGSPARKS,	// type
								PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_HOT,		// flags
								500,						// gravity
								0,							// magnetism
								20,							// base scale
								.9,							// decay rate
								0,							// fade rate
								PARTICLE_TEXTURE_WHITE);	// texture
		
		if (pg != -1)
		{
			for (i = 0; i < 50; i++)
			{
				delta.x = (RandomFloat()-.5f) * 1000.0f;
				delta.y = (RandomFloat()-.5f) * 1000.0f;
				delta.z = (RandomFloat()-.5f) * 1000.0f;
				AddParticleToGroup(pg, &enemy->Coord, &delta, RandomFloat() + 1.0f, FULL_ALPHA);
			}
		}
		
					/* fire sparks */
					
		pg = NewParticleGroup(	0,							// magic num
								PARTICLE_TYPE_FALLINGSPARKS,	// type
								PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_HOT,		// flags
								500,						// gravity
								0,							// magnetism
								20,							// base scale
								.7,							// decay rate
								0,							// fade rate
								PARTICLE_TEXTURE_FIRE);		// texture
		
		if (pg != -1)
		{
			for (i = 0; i < 50; i++)
			{
				delta.x = (RandomFloat()-.5f) * 500.0f;
				delta.y = (RandomFloat()-.4f) * 500.0f;
				delta.z = (RandomFloat()-.5f) * 500.0f;
				AddParticleToGroup(pg, &enemy->Coord, &delta, RandomFloat() + 1.0f, FULL_ALPHA);
			}
		}
		
				/*****************/
				/* KNOCK ON BUTT */
				/*****************/
					
		killed = KnockFireAntOnButt(enemy, me->Delta.x * .8f, me->Delta.y * .8f + 250.0f, me->Delta.z * .8f);

		PlayEffect_Parms3D(EFFECT_POUND, &gCoord, kMiddleC+2, 2.0);
	}

	return(killed);		
}


/****************** KNOCK FIREANT ON BUTT ********************/
//
// INPUT: dx,y,z = delta to apply to fireant 
//

Boolean KnockFireAntOnButt(ObjNode *enemy, float dx, float dy, float dz)
{
	if (enemy->Skeleton->AnimNum == FIREANT_ANIM_FALLONBUTT)		// see if already in butt mode
		return(false);
		
		/* IF ON SPLINE, DETACH */
		
	DetachEnemyFromSpline(enemy, MoveFireAnt);


			/* GET IT MOVING */
		
	enemy->Delta.x = dx;
	enemy->Delta.y = dy;
	enemy->Delta.z = dz;

	MorphToSkeletonAnim(enemy->Skeleton, FIREANT_ANIM_FALLONBUTT, 9.0);
	enemy->ButtTimer = 2.0;


		/* SLOW DOWN PLAYER */
		
	gDelta.x *= .2f;
	gDelta.y *= .2f;
	gDelta.z *= .2f;


		/* HURT & SEE IF KILLED */
			
	if (EnemyGotHurt(enemy, .5))
		return(true);
		
	return(false);
}


/****************** KILL FIRE FIREANT *********************/
//
// OUTPUT: true if ObjNode was deleted
//

Boolean KillFireAnt(ObjNode *theNode)
{
	
		/* IF ON SPLINE, DETACH */
		
	DetachEnemyFromSpline(theNode, MoveFireAnt);


			/* DEACTIVATE */
			
	theNode->TerrainItemPtr = nil;				// dont ever come back
	theNode->CType = CTYPE_MISC;
	theNode->Dying = true;						// after butt-fall, make it die
	
	if (theNode->Skeleton->AnimNum != FIREANT_ANIM_FALLONBUTT)			// make fall on butt first
	{
		SetSkeletonAnim(theNode->Skeleton, FIREANT_ANIM_FALLONBUTT);	
		theNode->ButtTimer = 2.0;
	}
	
	return(false);
}





/***************** UPDATE FIRE FIREANT ************************/

static void UpdateFireAnt(ObjNode *theNode, Boolean updateFlame)
{
			/****************/
			/* UPDATE FLAME */
			/****************/

	if (updateFlame)
	{
		if (!(theNode->StatusBits & STATUS_BIT_ISCULLED))		// only update fire if is not culled
		{
			if ((theNode->ParticleGroup == -1) || (!VerifyParticleGroupMagicNum(theNode->ParticleGroup, theNode->FireParticleMagicNum)))
			{
				theNode->FireParticleMagicNum = MyRandomLong();			// generate a random magic num
				
				theNode->ParticleGroup = NewParticleGroup(	theNode->FireParticleMagicNum,	// magic num
																PARTICLE_TYPE_GRAVITOIDS,	// type
																PARTICLE_FLAGS_HOT,			// flags
																0,							// gravity
																10000,						// magnetism
																15,							// base scale
																1.0,						// decay rate
																0,							// fade rate
																PARTICLE_TEXTURE_FIRE);		// texture

			}

			if (theNode->ParticleGroup != -1)
			{
				theNode->FireTimer += gFramesPerSecondFrac;
				if (theNode->FireTimer > .01f)
				{
					TQ3Vector3D	delta;
					TQ3Point3D  pt;
					static const TQ3Point3D off = {0,30, -10};							// offset to top of head
				
					theNode->FireTimer = 0;
					
					FindCoordOnJoint(theNode, FIREANT_HEAD_LIMB, &off, &pt);			// get coord of head
					pt.x += (RandomFloat()-.5f) * 50.0f;
					pt.y += (RandomFloat()-.5f) * 30.0f;
					pt.z += (RandomFloat()-.5f) * 50.0f;
					
					delta.x = (RandomFloat()-.5f) * 50.0f;
					delta.y = (RandomFloat()-.5f) * 30.0f + 40.0f;
					delta.z = (RandomFloat()-.5f) * 50.0f;
					
					AddParticleToGroup(theNode->ParticleGroup, &pt, &delta, RandomFloat() + 1.7f, FULL_ALPHA);
				}
			}
		}
	}
	
		/* UPDATE ENEMY */
		
	UpdateEnemy(theNode);


		/* UPDATE BUZZ */	
	
	if (theNode->Skeleton->AnimNum == FIREANT_ANIM_FLY)
	{
		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_BUZZ, &gCoord, kMiddleC-2, 1);
		else
			Update3DSoundChannel(EFFECT_BUZZ, &theNode->EffectChannel, &gCoord);
	}
	else
		StopAChannel(&theNode->EffectChannel);
	
}


#pragma mark -

/***************************** BREATH FIRE *************************************/

void FireAntBreathFire(ObjNode *theNode)
{
TQ3Matrix4x4	m;
static const TQ3Point3D off[2] = { {0,-8,-25}, {0,10,0} };			// offset to mouth & center of head
TQ3Point3D	p[2];
float		dx,dy,dz;
TQ3Vector3D	dir;
long		i;

	theNode->BreathRegulator += gFramesPerSecondFrac;				// see if time to spew fire
	if (theNode->BreathRegulator < 0.02f)
		return;
		
	theNode->BreathRegulator = 0;

				/* CALC COORD OF MOUTH AND CENTER OF HEAD */
				
	FindJointFullMatrix(theNode, FIREANT_HEAD_LIMB, &m);
	Q3Point3D_To3DTransformArray(&off[0], &m, &p[0], 2);//, sizeof(TQ3Point3D), sizeof(TQ3Point3D));


					/* CALC FIRE VECTOR */

	dx = p[0].x - p[1].x;
	dy = p[0].y - p[1].y;
	dz = p[0].z - p[1].z;
	FastNormalizeVector(dx,dy,dz, &dir);
		
					/* MAKE GROUP */
					
	if (theNode->BreathParticleGroup == -1)
	{
new_pgroup:	
		theNode->BreathParticleGroup = NewParticleGroup(0,							// magic num
														PARTICLE_TYPE_FALLINGSPARKS,	// type
														PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_HURTPLAYER|PARTICLE_FLAGS_HOT,		// flags
														400,						// gravity
														0,							// magnetism
														15,							// base scale
														-1.4,						// decay rate
														1.1,						// fade rate
														PARTICLE_TEXTURE_FIRE);		// texture
	}


			/**********************/
			/* ADD FIRE PARTICLES */
			/**********************/
			
	if (theNode->BreathParticleGroup != -1)
	{	
		for (i = 0; i < 4; i++)
		{
			TQ3Vector3D	dir2,delta;
			float		temp;

			temp = 400.0f + RandomFloat()*200.0f;
			dir2.x = dir.x * temp;								// speed of flames
			dir2.y = dir.y * temp;
			dir2.z = dir.z * temp;
			
					/* RANDOMLY ROTATE VECTOR IN CONE FOR GOOD SPREAD */
					
			Q3Matrix4x4_SetRotate_XYZ(&m, RandomFloat()*.15f, RandomFloat()*.15f, RandomFloat()*.15f);
			Q3Vector3D_Transform(&dir2, &m, &delta);
			
			if (AddParticleToGroup(theNode->BreathParticleGroup, &p[0], &delta, 1.5f, FULL_ALPHA))
				goto new_pgroup;
		}
	}
}



















