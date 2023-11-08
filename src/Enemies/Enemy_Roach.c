/****************************/
/*   ENEMY: ROACH.C		*/
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

static void MoveRoach(ObjNode *theNode);
static void MoveRoach_Standing(ObjNode *theNode);
static void  MoveRoach_Walking(ObjNode *theNode);
static void  MoveRoach_Death(ObjNode *theNode);
static void MoveRoachOnSpline(ObjNode *theNode);
static void LeaveGasTrail(ObjNode *theNode);
static void MoveGasTrail(ObjNode *theNode);
static void UpdateRoach(ObjNode *theNode, Boolean gas);
static void ExplodeGas(ObjNode *theNode);
static void  MoveRoach_OnButt(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_ROACHS					8

#define	ROACH_SCALE					1.7f

#define	ROACH_CHASE_DIST			900.0f
#define	ROACH_ATTACK_DIST			300.0f

#define	ROACH_TARGET_OFFSET			200.0f

#define ROACH_TURN_SPEED			2.4f
#define ROACH_WALK_SPEED			300.0f

#define	ROACH_FOOT_OFFSET			0.0f

#define	ROACH_KNOCKDOWN_SPEED		1700.0f	

#define	GAS_DIST					1200.0f


		/* ANIMS */
	

enum
{
	ROACH_ANIM_STAND,
	ROACH_ANIM_WALK,
	ROACH_ANIM_ONBUTT,
	ROACH_ANIM_DEATH
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	TargetRot		SpecialF[0]
#define	GasTimer		SpecialF[1]
#define	RotDeltaY		SpecialF[2]
#define	ButtTimer		SpecialF[3]

int32_t		gCurrentGasParticleGroup = -1;


/************************ ADD ROACH ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Roach(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

//	if (gNumEnemies >= MAX_ENEMIES)								// keep from getting absurd
//		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_ROACH] >= MAX_ROACHS)
			return(false);
	}



				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_ROACH,x,z, ROACH_SCALE);
	if (newObj == nil)
		return(false);
	newObj->TerrainItemPtr = itemPtr;

	SetSkeletonAnim(newObj->Skeleton, ROACH_ANIM_STAND);
	

				/*******************/
				/* SET BETTER INFO */
				/*******************/
			
	newObj->Coord.y 	-= ROACH_FOOT_OFFSET;			
	newObj->MoveCall 	= MoveRoach;							// set move call
	newObj->Health 		= 1.0;
	newObj->Damage 		= .1;
	newObj->Kind 		= ENEMY_KIND_ROACH;
	newObj->CType		|= CTYPE_KICKABLE|CTYPE_HURTME|CTYPE_HURTNOKNOCK;		
		
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 250,ROACH_FOOT_OFFSET,-100,100,100,-100);
	CalcNewTargetOffsets(newObj,ROACH_TARGET_OFFSET);


	

				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, 8, 8,false);
	
		
	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_ROACH]++;
	return(true);
}


/********************* MOVE ROACH **************************/

static void MoveRoach(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveRoach_Standing,
					MoveRoach_Walking,
					MoveRoach_OnButt,
					MoveRoach_Death,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	myMoveTable[theNode->Skeleton->AnimNum](theNode);	
}



/********************** MOVE ROACH: STANDING ******************************/

static void  MoveRoach_Standing(ObjNode *theNode)
{
float	dist;

				/* TURN TOWARDS ME */
				
	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, ROACH_TURN_SPEED, false);			

		

				/* SEE IF CHASE */

	dist = CalcQuickDistance(gMyCoord.x, gMyCoord.z, gCoord.x, gCoord.z);
	if (dist < ROACH_CHASE_DIST)
	{					
		MorphToSkeletonAnim(theNode->Skeleton, ROACH_ANIM_WALK, 8);
	}

			/* MOVE */
				
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);
				

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

				/* SEE IF IN LIQUID */
				
	if (theNode->StatusBits & STATUS_BIT_UNDERWATER)
	{
		KillRoach(theNode);
	}


	UpdateRoach(theNode, false);		
}




/********************** MOVE ROACH: WALKING ******************************/

static void  MoveRoach_Walking(ObjNode *theNode)
{
float		r,fps;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */
			
	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, ROACH_TURN_SPEED, false);			

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * ROACH_WALK_SPEED;
	gDelta.z = -cos(r) * ROACH_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);
		
		
				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == ROACH_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = ROACH_WALK_SPEED * .004f;
	

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;
	
				/* SEE IF IN LIQUID */
				
	if (theNode->StatusBits & STATUS_BIT_UNDERWATER)
	{
		KillRoach(theNode);
	}
	
	UpdateRoach(theNode, true);		
}


/********************** MOVE ROACH: ON BUTT ******************************/

static void  MoveRoach_OnButt(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(140.0,&gDelta);
		
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;			// add gravity

	MoveEnemy(theNode);


				/* SEE IF DONE */
			
	theNode->ButtTimer -= gFramesPerSecondFrac;
	if (theNode->ButtTimer <= 0.0)
	{
		MorphToSkeletonAnim(theNode->Skeleton, ROACH_ANIM_STAND, 3);
	}


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


	UpdateRoach(theNode, false);
}



/********************** MOVE ROACH: DEATH ******************************/

static void  MoveRoach_Death(ObjNode *theNode)
{
			/* SEE IF GONE */
			
	if (theNode->StatusBits & STATUS_BIT_ISCULLED)		// if was culled on last frame and is far enough away, then delete it
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) > 1000.0f)
		{
			DeleteEnemy(theNode);
			return;
		}		
	}


				/* MOVE IT */
				
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)		// if on ground, add friction
		ApplyFrictionToDeltas(60.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;		// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEATH_ENEMY_COLLISION_CTYPES))
		return;


				/* UPDATE */
			
	UpdateRoach(theNode, false);		

}


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME ROACH ENEMY *************************/

Boolean PrimeEnemy_Roach(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj,*shadowObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_ROACH,x,z, ROACH_SCALE);
	if (newObj == nil)
		return(false);
		
	DetachObject(newObj);										// detach this object from the linked list
		
	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;
	
	SetSkeletonAnim(newObj->Skeleton, ROACH_ANIM_WALK);
		

				/* SET BETTER INFO */
			
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= ROACH_FOOT_OFFSET;			
	newObj->SplineMoveCall 	= MoveRoachOnSpline;				// set move call
	newObj->Health 			= 1.0;
	newObj->Damage 			= 0;
	newObj->Kind 			= ENEMY_KIND_ROACH;
	newObj->CType			|= CTYPE_KICKABLE|CTYPE_HURTNOKNOCK|CTYPE_HURTME;	// these can be kicked
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 70,ROACH_FOOT_OFFSET,-70,70,70,-70);
	CalcNewTargetOffsets(newObj,ROACH_TARGET_OFFSET);


	newObj->InitCoord = newObj->Coord;							// remember where started

				/* MAKE SHADOW */
				
	shadowObj = AttachShadowToObject(newObj, 8, 8, false);
	DetachObject(shadowObj);									// detach this object from the linked list


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	AddToSplineObjectList(newObj);

	return(true);
}


/******************** MOVE ROACH ON SPLINE ***************************/

static void MoveRoachOnSpline(ObjNode *theNode)
{
Boolean isVisible; 

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 60);
	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/* UPDATE STUFF IF VISIBLE */
			
	if (isVisible)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);		

		theNode->Coord.y = GetTerrainHeightAtCoord(theNode->Coord.x, theNode->Coord.z, FLOOR) - ROACH_FOOT_OFFSET;	// calc y coord
		UpdateObjectTransforms(theNode);																// update transforms
		CalcObjectBoxFromNode(theNode);																	// update collision box
		UpdateShadow(theNode);	
		
		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gMyCoord.x, gMyCoord.z) < GAS_DIST)
			LeaveGasTrail(theNode);

				/* DO SOME COLLISION CHECKING */
				
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY))					// just do this to see if explosions hurt
			return;
	}
	
			/* HIDE SOME THINGS SINCE INVISIBLE */
	else
	{
//		if (theNode->ShadowNode)						// hide shadow
//			theNode->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;	
	}
	
}




#pragma mark -

/******************* BALL HIT ROACH *********************/
//
// OUTPUT: true if enemy gets killed
//
// Worker bees cannot be knocked down.  Instead, it just pisses them off.
//

Boolean BallHitRoach(ObjNode *me, ObjNode *enemy)
{
Boolean	killed = false;

				/************************/
				/* HURT & KNOCK ON BUTT */
				/************************/
				
	if (me->Speed > ROACH_KNOCKDOWN_SPEED)
	{	
				/*****************/
				/* KNOCK ON BUTT */
				/*****************/
					
		killed = KnockRoachOnButt(enemy, me->Delta.x * .8f, me->Delta.y * .8f + 250.0f, me->Delta.z * .8f, .5);

		PlayEffect_Parms3D(EFFECT_POUND, &gCoord, kMiddleC+2, 2.0);
	}
	
	return(killed);
}


/****************** KNOCK ROACH ON BUTT ********************/
//
// INPUT: dx,y,z = delta to apply to fireant 
//

Boolean KnockRoachOnButt(ObjNode *enemy, float dx, float dy, float dz, float damage)
{
	if (enemy->Skeleton->AnimNum == ROACH_ANIM_ONBUTT)		// see if already in butt mode
		return(false);
		
		/* IF ON SPLINE, DETACH */
		
	DetachEnemyFromSpline(enemy, MoveRoach);


			/* GET IT MOVING */
		
	enemy->Delta.x = dx;
	enemy->Delta.y = dy;
	enemy->Delta.z = dz;

	MorphToSkeletonAnim(enemy->Skeleton, ROACH_ANIM_ONBUTT, 9.0);
	enemy->ButtTimer = 2.0;
	

		/* SLOW DOWN PLAYER */
		
	gDelta.x *= .2f;
	gDelta.y *= .2f;
	gDelta.z *= .2f;


		/* HURT & SEE IF KILLED */
			
	if (EnemyGotHurt(enemy, damage))
		return(true);
		
	return(false);
}


/****************** KILL ROACH *********************/
//
// OUTPUT: true if ObjNode was deleted
//

Boolean KillRoach(ObjNode *theNode)
{
	
		/* IF ON SPLINE, DETACH */
		
	DetachEnemyFromSpline(theNode, MoveRoach);


			/* DEACTIVATE */
			
	theNode->TerrainItemPtr = nil;				// dont ever come back
	theNode->CType = CTYPE_MISC;

	MorphToSkeletonAnim(theNode->Skeleton, ROACH_ANIM_DEATH, 2.0);	
	
	return(false);
}





/***************** UPDATE ROACH ************************/

static void UpdateRoach(ObjNode *theNode, Boolean gas)
{
	UpdateEnemy(theNode);
	
	if (gas)
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < GAS_DIST)
			LeaveGasTrail(theNode);
	}
}


#pragma mark -

/******************** LEAVE GAS TRAIL **********************/

static void LeaveGasTrail(ObjNode *theNode)
{
ObjNode	*newObj;

	theNode->GasTimer += gFramesPerSecondFrac;
	if (theNode->GasTimer < .4f)
		return;

	theNode->GasTimer = RandomFloat()*.1f;

			/* MAKE CLOUD OBJECT */
			
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	if (gLevelType == LEVEL_TYPE_ANTHILL)
		gNewObjectDefinition.type 	= ANTHILL_MObjType_GasCloud;	
	else
		gNewObjectDefinition.type 	= NIGHT_MObjType_GasCloud;

	gNewObjectDefinition.coord.x 	= theNode->Coord.x;
	gNewObjectDefinition.coord.y 	= theNode->Coord.y + 100.0f + (RandomFloat()*120.0f);
	gNewObjectDefinition.coord.z 	= theNode->Coord.z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER | STATUS_BIT_GLOW
			| STATUS_BIT_NOZWRITE | STATUS_BIT_KEEPBACKFACES | gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 400;
	gNewObjectDefinition.moveCall 	= MoveGasTrail;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= .6;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;

			/* SET COLLISION */
			
	newObj->CType = CTYPE_DRAINBALLTIME;
	newObj->CBits = CBITS_TOUCHABLE;
	newObj->Damage = .2;
	
	SetObjectCollisionBounds(newObj,90,-90,-100,100,100,-100);

	newObj->Health = 5.0f + RandomFloat()*3.0f;

	newObj->Delta.x = (RandomFloat()-.5f) * 50.0f;
	newObj->Delta.z = (RandomFloat()-.5f) * 50.0f;
	
	newObj->RotDeltaY = (RandomFloat()-.5f) * 2.0f;
}


/******************* MOVE GAS TRAIL *********************/

static void MoveGasTrail(ObjNode *theNode)
{
float fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	theNode->Rot.y += fps * theNode->RotDeltaY;
	
	if (theNode->Scale.x < 2.0f)
		theNode->Scale.z = theNode->Scale.x += fps * 1.0f; 

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

			/* DISPERSE GAS */
			
	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}
	
	if (theNode->Health < 1.0f)
	{
		MakeObjectTransparent(theNode, theNode->Health);
		theNode->CType = 0;
	}

		/* SEE IF IGNITE */
		
	if (theNode->Health < 5.0f)
	{
		if (ParticleHitObject(theNode, PARTICLE_FLAGS_HOT))
		{
			ExplodeGas(theNode);
			return;
		}
	}


		/* UPDATE */
		
	UpdateObject(theNode);
}


/********************* EXPLODE GAS ***********************/

static void ExplodeGas(ObjNode *theNode)
{
int32_t			pg;
TQ3Vector3D		delta;

			/* SEE IF NEED TO CREATE NEW PARTICLE GROUP */

	if (!VerifyParticleGroup(gCurrentGasParticleGroup))
	{
		pg = NewParticleGroup(
								PARTICLE_TYPE_FALLINGSPARKS,	// type
								PARTICLE_FLAGS_HURTPLAYER|PARTICLE_FLAGS_HOT|PARTICLE_FLAGS_HURTENEMY,	// flags
								500,							// gravity
								0,								// magnetism
								30,								// base scale
								-1.5,							// decay rate
								1.2,							// fade rate
								PARTICLE_TEXTURE_ORANGESPOT);	// texture
		gCurrentGasParticleGroup = pg;	
	}
	else
		pg = gCurrentGasParticleGroup;
		

			/* ADD PARTICLES TO CURRENT GROUP */

	if (pg != -1)
	{
		for (int i = 0; i < 25; i++)
		{
			delta.x = (RandomFloat()-.5f) * 800.0f;
			delta.y = RandomFloat() * 600.0f;
			delta.z = (RandomFloat()-.5f) * 800.0f;
			if (AddParticleToGroup(pg, &theNode->Coord, &delta, RandomFloat() + 1.0f, FULL_ALPHA))
			{
				gCurrentGasParticleGroup = -1;
				break;
			}
		}
	}

	PlayEffect_Parms3D(EFFECT_FIRECRACKER, &theNode->Coord, kMiddleC-10, .9);

	DeleteObject(theNode);
}
