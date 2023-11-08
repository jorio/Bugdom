/****************************/
/*   ENEMY: KINGANT.C		*/
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

static void MoveKingAnt(ObjNode *theNode);
static void MoveKingAnt_Waiting(ObjNode *theNode);
static void  MoveKingAnt_Walk(ObjNode *theNode);
static void  MoveKingAnt_Death(ObjNode *theNode);
static void UpdateKingAnt(ObjNode *theNode);
static void  MoveKingAnt_OnButt(ObjNode *theNode);
static void  MoveKingAnt_Attack(ObjNode *theNode);

static void GiveKingAStaff(ObjNode *theNode);
static void UpdateKingStaff(ObjNode *king);
static void MakeStaffFlame(ObjNode *theNode);
static void ShootStaff(ObjNode *staff);
static void ExplodeStaffBullet(ObjNode *theNode);
static void MoveStaffBullet(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	KINGANT_HEAD_LIMB	1

#define	KINGANT_KNOCKDOWN_SPEED	1400.0f					// speed ball needs to go to knock this down

#define	KINGANT_SCALE			1.5f

#define	KINGANT_CHASE_DIST		1600.0f
#define	KINGANT_ATTACK_DIST		600.0f


#define KINGANT_TURN_SPEED		2.0f
#define KINGANT_WALK_SPEED		500.0f

#define	KINGANT_FOOT_OFFSET		-220.0f



enum
{
	KINGANT_MODE_APPEAR,
	KINGANT_MODE_ATTACK,
	KINGANT_MODE_VANISH
};


		/* ANIMS */
enum
{
	KINGANT_ANIM_WAIT,
	KINGANT_ANIM_WALK,
	KING_ANIM_ATTACK,
	KINGANT_ANIM_ONBUTT,
	KINGANT_ANIM_DEATH
};


#define	STAFF_SCALE			.2f
#define	KING_HOLDING_LIMB	14

#define	BULLET_SPEED	800.0f

/*********************/
/*    VARIABLES      */
/*********************/

ObjNode	*gAntKingObj;

static float		gStaffCharge;

#define	WetTimer	SpecialF[0]
#define	ButtTimer	SpecialF[1]
#define	DeathTimer	SpecialF[1]
#define	FireTimer	SpecialF[2]

#define	SparkTimer	SpecialF[0]

#define	PGroupA		SpecialL[0]
#define	PGroupB		SpecialL[1]



/************************ ADD KINGANT ENEMY *************************/

Boolean AddEnemy_KingAnt(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (itemPtr->parm[3])			// if any bits set then this isnt the real Queen
		return(true);
	

				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/
		
	gAntKingObj = newObj = MakeEnemySkeleton(SKELETON_TYPE_KINGANT,x,z, KINGANT_SCALE);
	if (newObj == nil)
		return(false);
	newObj->TerrainItemPtr = itemPtr;

	SetSkeletonAnim(newObj->Skeleton, KINGANT_ANIM_WAIT);
	

				/*******************/
				/* SET BETTER INFO */
				/*******************/
			
	newObj->Coord.y 	-= KINGANT_FOOT_OFFSET;			
	newObj->MoveCall 	= MoveKingAnt;							// set move call
	newObj->Health 		= ANTKING_HEALTH;									// LOTS of health!
	newObj->Damage 		= 0;
	newObj->Kind 		= ENEMY_KIND_KINGANT;
	newObj->CType		|= CTYPE_KICKABLE;
	
	newObj->Mode		= KINGANT_MODE_APPEAR;
	
	newObj->WetTimer	= 0;									// not wet yet
	
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 100,KINGANT_FOOT_OFFSET,-110,110,110,-110);


				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, 8, 8,false);
	
		
				/* GIVE KING STAFF */
				
	GiveKingAStaff(newObj);
		
	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_KINGANT]++;
	
	return(true);
}


/********************* MOVE KINGANT **************************/

static void MoveKingAnt(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveKingAnt_Waiting,
					MoveKingAnt_Walk,
					MoveKingAnt_Attack,
					MoveKingAnt_OnButt,
					MoveKingAnt_Death,
				};

	/* note: we dont track the queen since she is always active and never goes away! */

	GetObjectInfo(theNode);
	myMoveTable[theNode->Skeleton->AnimNum](theNode);	
}



/********************** MOVE KINGANT: WAITING ******************************/

static void  MoveKingAnt_Waiting(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
	
			/* AIM AT PLAYER */
			
	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, KINGANT_TURN_SPEED, false);			


				/* MOVE */
				
	gDelta.y -= ENEMY_GRAVITY*fps;
	MoveEnemy(theNode);
				
				
			/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

			/* SEE IF CHASE */
				
	if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < KINGANT_CHASE_DIST)
	{
		MorphToSkeletonAnim(theNode->Skeleton, KINGANT_ANIM_WALK, 5);	
	}



	UpdateKingAnt(theNode);		
}


/********************** MOVE KINGANT: WALK ******************************/

static void  MoveKingAnt_Walk(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	r;
	
			/* AIM AT PLAYER */
			
	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, KINGANT_TURN_SPEED, false);			


				/* MOVE */
				
	r = theNode->Rot.y;
	gDelta.x = -sin(r) * KINGANT_WALK_SPEED;
	gDelta.z = -cos(r) * KINGANT_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				
	MoveEnemy(theNode);
				
				
			/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

			/* SEE IF ATTACK */
			
	if (theNode->WetTimer <= 0.0f)								// dont attack if wet
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < KINGANT_ATTACK_DIST)
		{
			MorphToSkeletonAnim(theNode->Skeleton, KING_ANIM_ATTACK, 5);	
			gStaffCharge = 0;
		}
	}


	UpdateKingAnt(theNode);		
}

/********************** MOVE KINGANT: ATTACK ******************************/

static void  MoveKingAnt_Attack(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
	
			/* AIM AT PLAYER */
			
	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, KINGANT_TURN_SPEED, false);			


				/* MOVE */
				
	gDelta.y -= ENEMY_GRAVITY*fps;
	ApplyFrictionToDeltas(60.0,&gDelta);
	MoveEnemy(theNode);
				
				
			/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


		/* SEE IF READY TO SHOOT */
		
	if (theNode->WetTimer > 0.0f)							// if wet, then abort attack
	{
		MorphToSkeletonAnim(theNode->Skeleton, KINGANT_ANIM_WALK, 5);
	}
	else
	{
		gStaffCharge += fps;
		if (gStaffCharge > 2.0f)
		{
			ShootStaff(theNode->ChainNode);
			MorphToSkeletonAnim(theNode->Skeleton, KINGANT_ANIM_WAIT, 3.0);	
		}
	}
	
	UpdateKingAnt(theNode);		
}




/********************** MOVE KINGANT: ON BUTT ******************************/

static void  MoveKingAnt_OnButt(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

				/* MOVE IT */
				
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)		// if on ground, add friction
		ApplyFrictionToDeltas(60.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*fps;						// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

	theNode->ButtTimer -= fps;
	if (theNode->ButtTimer <= 0.0f)
	{
		MorphToSkeletonAnim(theNode->Skeleton, KINGANT_ANIM_WAIT, 6);
	}

				/* UPDATE */
			
	UpdateKingAnt(theNode);		
}


/********************** MOVE KINGANT: DEATH ******************************/

static void  MoveKingAnt_Death(ObjNode *theNode)
{
				/* MOVE IT */
				
	ApplyFrictionToDeltas(60.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;		// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEATH_ENEMY_COLLISION_CTYPES))
		return;

	
	theNode->DeathTimer -= gFramesPerSecondFrac;
	if (theNode->DeathTimer < 0.0f)		
		gAreaCompleted = true;


				/* UPDATE */
			
	UpdateKingAnt(theNode);		
}



//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/******************* BALL HIT KINGANT *********************/
//
// OUTPUT: true if enemy gets killed
//
// Worker bees cannot be knocked down.  Instead, it just pisses them off.
//

Boolean BallHitKingAnt(ObjNode *me, ObjNode *enemy)
{
Boolean	killed = false;

	if (enemy->WetTimer > 0.0f)						// king must be wet for any damage to occur
	{
		if (me->Speed > KINGANT_KNOCKDOWN_SPEED)
		{				
					/* KNOCK ON BUTT */
						
			killed = KnockKingAntOnButt(enemy, me->Delta.x * .3f, me->Delta.z * .3f, .6);
			PlayEffect_Parms3D(EFFECT_POUND, &gCoord, kMiddleC-2, 2.0);
		}
	}
	else
		PlayEffect_Parms3D(EFFECT_KINGLAUGH, &enemy->Coord, kMiddleC, 2.0);	

	return(killed);		
}


/***************** KNOCK KING ANT ON BUTT *****************/
//
// INPUT: dx,y,z = delta to apply to enemy 
//

Boolean KnockKingAntOnButt(ObjNode *enemy, float dx, float dz, float damage)
{
	if (enemy->WetTimer <= 0.0f)						// king must be wet for any damage to occur
	{
		PlayEffect_Parms3D(EFFECT_KINGLAUGH, &enemy->Coord, kMiddleC, 2.0);	
		return(false);
	}

	if (enemy->Skeleton->AnimNum == KINGANT_ANIM_ONBUTT)		// see if already in butt mode
		return(false);

		/* DO BUTT ANIM */
		
	MorphToSkeletonAnim(enemy->Skeleton, KINGANT_ANIM_ONBUTT, 8.0);
	enemy->ButtTimer = 2.0;

			/* GET IT MOVING */
		
	enemy->Delta.x = dx;
	enemy->Delta.z = dz;
	enemy->Delta.y = 600;
	

		/* SLOW DOWN PLAYER */
		
	gDelta.x *= .2f;
	gDelta.y *= .2f;
	gDelta.z *= .2f;		


		/* HURT & SEE IF KILLED */
			
	if (EnemyGotHurt(enemy, damage))
		return(true);
		
	gInfobarUpdateBits |= UPDATE_BOSS;
		
	return(false);
}




/****************** KILL FIRE KINGANT *********************/
//
// OUTPUT: true if ObjNode was deleted
//

Boolean KillKingAnt(ObjNode *theNode)
{	

		/* STOP SOUND */
		
	if (theNode->EffectChannel != -1)
		StopAChannel(&theNode->EffectChannel);

			/* DEACTIVATE */
			
	theNode->TerrainItemPtr = nil;				// dont ever come back
	theNode->CType = CTYPE_MISC;
	
	MorphToSkeletonAnim(theNode->Skeleton, KINGANT_ANIM_DEATH, 2);
			
	theNode->DeathTimer = 4.0;
			
	return(false);
}





/***************** UPDATE KINGANT ************************/

static void UpdateKingAnt(ObjNode *theNode)
{
	UpdateEnemy(theNode);
	UpdateKingStaff(theNode);
	
	
		/************************/
		/* SEE IF PUT OUT FLAME */
		/************************/
	
	if (ParticleHitObject(theNode, PARTICLE_FLAGS_EXTINGUISH))				// see if water particle hit king
	{
		if (theNode->WetTimer <= 0.0f)										// if this is new then sizzle
			PlayEffect3D(EFFECT_SIZZLE, &theNode->Coord);
		theNode->WetTimer = 5;		
	}

	
	if ((theNode->WetTimer <= 0.0f) && (theNode->Skeleton->AnimNum != KINGANT_ANIM_DEATH))
	{
			/******************/
			/* UPDATE CRACKLE */
			/******************/

		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect3D(EFFECT_KINGCRACKLE, &theNode->Coord);
		else
			Update3DSoundChannel(EFFECT_KINGCRACKLE, &theNode->EffectChannel, &theNode->Coord);
		
			/****************/
			/* UPDATE FLAME */
			/****************/

		if (!(theNode->StatusBits & STATUS_BIT_ISCULLED))		// only update fire if is not culled
		{
			theNode->FireTimer += gFramesPerSecondFrac;
			if (theNode->FireTimer > .01f)
			{
				if (!VerifyParticleGroup(theNode->ParticleGroup))
				{
					theNode->ParticleGroup = NewParticleGroup(
																	PARTICLE_TYPE_GRAVITOIDS,	// type
																	PARTICLE_FLAGS_ROOF|PARTICLE_FLAGS_HOT,		// flags
																	0,							// gravity
																	14000,						// magnetism
																	20,							// base scale
																	.6,							// decay rate
																	0,							// fade rate
																	PARTICLE_TEXTURE_FIRE);		// texture

				}

				if (theNode->ParticleGroup != -1)
				{
					TQ3Vector3D	delta;
					TQ3Point3D  pt;
					static const TQ3Point3D off = {0,100, -50};							// offset to top of head
				
					theNode->FireTimer = 0;
					
					FindCoordOnJoint(theNode, KINGANT_HEAD_LIMB, &off, &pt);			// get coord of head
					pt.x += (RandomFloat()-.5f) * 100.0f;
					pt.y += (RandomFloat()-.5f) * 40.0f;
					pt.z += (RandomFloat()-.5f) * 100.0f;
					
					delta.x = (RandomFloat()-.5f) * 70.0f;
					delta.y = (RandomFloat()-.5f) * 40.0f + 90.0f;
					delta.z = (RandomFloat()-.5f) * 70.0f;
					
					AddParticleToGroup(theNode->ParticleGroup, &pt, &delta, RandomFloat() + 1.7f, FULL_ALPHA);
				}
			}
		}
	}	
	
	
			/* DEC THE WET TIMER */
	else
	{
		theNode->WetTimer -= gFramesPerSecondFrac;
		if (theNode->WetTimer < 0.0f)
			theNode->WetTimer = 0;
			
		if (theNode->EffectChannel != -1)					// stop crackle sound if wet
			StopAChannel(&theNode->EffectChannel);
	}

	
}


#pragma mark -

/***************************** GIVE KING A SPEAR *********************************/

static void GiveKingAStaff(ObjNode *king)
{
ObjNode	*staff;

			/* MAKE STAFF OBJECT */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= ANTHILL_MObjType_Staff;	
	gNewObjectDefinition.coord		= king->Coord;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_GLOW|STATUS_BIT_NULLSHADER|STATUS_BIT_NOZWRITE;
	gNewObjectDefinition.slot 		= king->Slot+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= STAFF_SCALE;
	staff = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (staff == nil)
		return;

			/* ATTACH SPEAR TO ENEMY */
	
	king->ChainNode = staff;
	staff->ChainHead = king;
}


/************************* UPDATE KING STAFF ************************/

static void UpdateKingStaff(ObjNode *king)
{
ObjNode					*staff;
TQ3Matrix4x4			m,m2,m3;
static const TQ3Point3D	zero = {0,0,0};
float					s;

			/* VERIFY */
			
	if (!king->ChainNode)
		return;
		
	staff = king->ChainNode;	
	
	s = STAFF_SCALE / king->Scale.x;						// compensate for king's scale
	Q3Matrix4x4_SetScale(&m, s,s,s);			
	FindJointFullMatrix(king, KING_HOLDING_LIMB, &m2);
	MatrixMultiplyFast(&m,&m2,&m3);			
	
	Q3Matrix4x4_SetRotate_Z(&m, -.25);						// initial rot
	m.value[3][0] = 240;									// offset 
	m.value[3][1] = -40;
	m.value[3][2] = -130;
	MatrixMultiplyFast(&m,&m3,&staff->BaseTransformMatrix);			

			/* CALC STAFF'S COORD */
			
	Q3Point3D_Transform(&zero, &staff->BaseTransformMatrix, &staff->Coord);


			/* UPDATE FLAMES */
			
	MakeStaffFlame(staff);
}


/********************* MAKE STAFF FLAME **************************/

static void MakeStaffFlame(ObjNode *theNode)
{
int	i;
float	fps = gFramesPerSecondFrac;

	if ((theNode->ChainHead->WetTimer > 0.0f) ||	// dont update flame if wet
		(theNode->Skeleton && theNode->Skeleton->AnimNum == KINGANT_ANIM_DEATH))
	{
		theNode->ParticleGroup = -1;				// invalidate particle group during absence
	}
	
			/* UPDATE FLAME */
	else
	{
		theNode->FireTimer += fps;
		if (theNode->FireTimer > .02f)
		{
			if (!VerifyParticleGroup(theNode->ParticleGroup))
			{
new_group:
				theNode->ParticleGroup = NewParticleGroup(
														PARTICLE_TYPE_GRAVITOIDS,	// type
														PARTICLE_FLAGS_ROOF|PARTICLE_FLAGS_HOT,		// flags
														0,							// gravity
														10000,						// magnetism
														13,							// base scale
														.6,							// decay rate
														0,							// fade rate
														PARTICLE_TEXTURE_BLUEFIRE);	// texture
			}

			if (theNode->ParticleGroup != -1)
			{
				TQ3Vector3D	delta;
				TQ3Point3D  pt,tip;
				static const TQ3Point3D off = {0,370, 0};							// offset to top of staff
			
				theNode->FireTimer = 0;
				Q3Point3D_Transform(&off, &theNode->BaseTransformMatrix, &tip);
				
				for (i = 0; i < 1; i++)
				{
					pt.x = tip.x + (RandomFloat()-.5f) * 60.0f;
					pt.y = tip.y + (RandomFloat()-.5f) * 40.0f;
					pt.z = tip.z + (RandomFloat()-.5f) * 60.0f;
					
					delta.x = (RandomFloat()-.5f) * 50.0f;
					delta.y = (RandomFloat()-.5f) * 40.0f + 40.0f;
					delta.z = (RandomFloat()-.5f) * 50.0f;
					
					if (AddParticleToGroup(theNode->ParticleGroup, &pt, &delta, RandomFloat() + 1.7f, FULL_ALPHA))
						goto new_group;
				}
			}
		}
	}
}


/*************************** SHOOT STAFF *************************/

static void ShootStaff(ObjNode *staff)
{
ObjNode			*newObj;
static const TQ3Point3D off = {0,370, 0};							// offset to top of staff
TQ3Vector3D		delta;


			/* CALC COORD OF STAFF TIP */
			
	Q3Point3D_Transform(&off, &staff->BaseTransformMatrix, &gNewObjectDefinition.coord);

			/******************/
			/* MAKE NEW EVENT */
			/******************/
			
	gNewObjectDefinition.genre 		= EVENT_GENRE;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 500;
	gNewObjectDefinition.moveCall 	= MoveStaffBullet;
	newObj = MakeNewObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;
		
			/* SET COLLISION INFO */
			
	newObj->CType 			= CTYPE_HURTME;
	newObj->CBits			= CBITS_TOUCHABLE;
	SetObjectCollisionBounds(newObj,70,-70,-70,70,70,-70);
		
	newObj->Damage = .2;
	
	newObj->SparkTimer = 0;					// spark generator timer
	newObj->PGroupA = 						// no particle groups yet
	newObj->PGroupB = -1;
	
			/* CALC DELTA VECTOR */
			
	delta.x = gMyCoord.x - staff->Coord.x;
	delta.y = (gMyCoord.y + 50.0f) - staff->Coord.y;
	delta.z = gMyCoord.z - staff->Coord.z;
			
	FastNormalizeVector(delta.x, delta.y, delta.z, &delta);		// get normalized aim vector
	delta.x *= BULLET_SPEED;
	delta.y *= BULLET_SPEED;
	delta.z *= BULLET_SPEED;
	newObj->Delta = delta;
	
	
			/* PLAY EFFECT */
	
	PlayEffect3D(EFFECT_KINGSHOOT, &newObj->Coord);
}

/********************* MOVE STAFF BULLET **********************/

static void MoveStaffBullet(ObjNode *theNode)
{
int	i;
TQ3Vector3D	delta;
float	fps = gFramesPerSecondFrac;

			/****************************/
			/* MOVE THE FIREBALL OBJECT */
			/****************************/

	GetObjectInfo(theNode);
	
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;
	
			/* SEE IF HIT ANYTHING */
		
	if ((gCoord.y <= GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR)) ||
		(gCoord.y >= GetTerrainHeightAtCoord(gCoord.x, gCoord.z, CEILING)))
	{
		ExplodeStaffBullet(theNode);
		return;
	}
	
	if (DoSimpleBoxCollision(gCoord.y+20,gCoord.y-20,gCoord.x-20,gCoord.x+20,gCoord.z+20,gCoord.z-20, CTYPE_MISC|CTYPE_PLAYER))
	{
		ExplodeStaffBullet(theNode);
		return;
	}
	
	UpdateObject(theNode);


			/*********************/
			/* LEAVE SPARK TRAIL */
			/*********************/

	theNode->SparkTimer -= fps;
	if (theNode->SparkTimer <= 0.0f)
	{
		theNode->SparkTimer = .03;


				/*********************/
				/* DO FIRE PARTICLES */
				/*********************/
	
				/* SEE IF MAKE NEW GROUP */

		if (!VerifyParticleGroup(theNode->PGroupA))
		{
			theNode->PGroupA = NewParticleGroup(
												PARTICLE_TYPE_FALLINGSPARKS,// type
												PARTICLE_FLAGS_HOT,			// flags
												0,							// gravity
												0,							// magnetism
												10,							// base scale
												-10.0,						// decay rate
												2.0,						// fade rate
												PARTICLE_TEXTURE_BLUEFIRE);	// texture
		}
	
	
				/* ADD PARTICLES TO FIRE */
				
		if (theNode->PGroupA != -1)
		{
			delta.x = 
			delta.y = 
			delta.z = 0;			
			AddParticleToGroup(theNode->PGroupA, &gCoord, &delta, RandomFloat()*2.0f + 1.0, FULL_ALPHA);
		}
		
				/**********************/
				/* DO SPARK PARTICLES */
				/**********************/
	
				/* SEE IF MAKE NEW GROUP */
				
		if (theNode->PGroupB == -1)
		{
			theNode->PGroupB = NewParticleGroup(
												PARTICLE_TYPE_FALLINGSPARKS,// type
												PARTICLE_FLAGS_HOT,			// flags
												900,						// gravity
												0,							// magnetism
												10,							// base scale
												1.6,						// decay rate
												0,							// fade rate
												PARTICLE_TEXTURE_WHITE);	// texture
		}
	
	
				/* ADD PARTICLES TO SPARK */
				
		if (theNode->PGroupB != -1)
		{
			for (i = 0; i < 3; i++)
			{
				delta.x = (RandomFloat()-.5f) * 700.0f;
				delta.y = (RandomFloat()-.5f) * 700.0f;
				delta.z = (RandomFloat()-.5f) * 700.0f;
				
				AddParticleToGroup(theNode->PGroupB, &gCoord, &delta, RandomFloat()*2.0f + 2.0f, FULL_ALPHA);
			}
		}			
	}
}


/****************** EXPLODE STAFF BULLET ***********************/

static void ExplodeStaffBullet(ObjNode *theNode)
{
TQ3Vector3D		delta;

			/*******************/
			/* SPARK EXPLOSION */
			/*******************/

			/* white sparks */

	int32_t pg = NewParticleGroup(
							PARTICLE_TYPE_FALLINGSPARKS,	// type
							PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_HURTPLAYER|PARTICLE_FLAGS_ROOF|PARTICLE_FLAGS_HOT,	// flags
							400,						// gravity
							0,							// magnetism
							40,							// base scale
							0,							// decay rate
							.7,							// fade rate
							PARTICLE_TEXTURE_BLUEFIRE);		// texture

	for (int i = 0; i < 60; i++)
	{
		delta.x = (RandomFloat()-.5f) * 1400.0f;
		delta.y = (RandomFloat()-.5f) * 1400.0f;
		delta.z = (RandomFloat()-.5f) * 1400.0f;
		AddParticleToGroup(pg, &theNode->Coord, &delta, RandomFloat() + 1.0f, FULL_ALPHA);
	}


		/* MAKE SOUND */
		
	PlayEffect3D(EFFECT_KINGEXPLODE, &theNode->Coord);

	DeleteObject(theNode);


}











