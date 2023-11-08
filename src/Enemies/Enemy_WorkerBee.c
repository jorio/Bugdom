/****************************/
/*   ENEMY: WORKERBEE.C		*/
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

static void MoveWorkerBee(ObjNode *theNode);
static void MoveWorkerBee_Standing(ObjNode *theNode);
static void  MoveWorkerBee_Walking(ObjNode *theNode);
static void  MoveWorkerBee_Death(ObjNode *theNode);
static void MoveWorkerBeeOnSpline(ObjNode *theNode);
static void GiveWorkerBeeStinger(ObjNode *theNode);
static void UpdateWorkerBee(ObjNode *theNode);
static void AlignStingerOnBee(ObjNode *bee);
static void  MoveWorkerBee_ShootButt(ObjNode *theNode);
static void ShootStinger(ObjNode *bee);
static void MoveStinger(ObjNode *theNode);
static void  MoveWorkerBee_Pound(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_WORKERBEES					4

#define	WORKERBEE_SPLINE_SPEED			100.0f

#define	WORKERBEE_SCALE					1.5f
#define	STINGER_SCALE					(WORKERBEE_SCALE * .25)

#define	WORKERBEE_CHASE_DIST			900.0f
#define	WORKERBEE_ATTACK_DIST			300.0f
#define	WORKERBEE_DETACH_DIST			300.0f


#define	WORKERBEE_TARGET_OFFSET			200.0f



#define WORKERBEE_TURN_SPEED			2.4f
#define WORKERBEE_WALK_SPEED			600.0f
#define	STINGER_SPEED					1000.0f

#define	WORKERBEE_FOOT_OFFSET			-152.0f

#define	WORKERBEE_JOINT_BUTT			1


		/* ANIMS */
	

enum
{
	WORKERBEE_ANIM_STAND,
	WORKERBEE_ANIM_WALK,
	WORKERBEE_ANIM_SHOOTBUTT,
	WORKERBEE_ANIM_FALLONFACE,
	WORKERBEE_ANIM_POUND
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	ShootButtFlag	Flag[0]
#define	PoundFlag		Flag[0]
#define	TargetRot		SpecialF[0]
#define	TimeUntilPound	SpecialF[1]


/************************ ADD WORKERBEE ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_WorkerBee(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= MAX_ENEMIES)								// keep from getting absurd
		return(false);
	if (gNumEnemyOfKind[ENEMY_KIND_WORKERBEE] >= MAX_WORKERBEES)
		return(false);

		/* SEE IF KEYED ENEMY ON HIVE */
		
	if (gRealLevel == LEVEL_NUM_HIVE)
	{
		if (itemPtr->parm[3] & 1)										// see if we care
		{
			if (!gDetonatorBlown[itemPtr->parm[0]])									// see if detonator has been triggered
				return(false);		
		}
	}

				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_WORKERBEE,x,z, WORKERBEE_SCALE);
	if (newObj == nil)
		return(false);
	newObj->TerrainItemPtr = itemPtr;

	SetSkeletonAnim(newObj->Skeleton, WORKERBEE_ANIM_STAND);
	

				/*******************/
				/* SET BETTER INFO */
				/*******************/
			
	newObj->Coord.y 	-= WORKERBEE_FOOT_OFFSET;			
	newObj->MoveCall 	= MoveWorkerBee;							// set move call
	newObj->Health 		= 1.0;
	newObj->Damage 		= 0;
	newObj->Kind 		= ENEMY_KIND_WORKERBEE;
	newObj->CType		|= CTYPE_KICKABLE|CTYPE_AUTOTARGET;		// these can be kicked
		
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 140,WORKERBEE_FOOT_OFFSET,-100,100,100,-100);
	CalcNewTargetOffsets(newObj,WORKERBEE_TARGET_OFFSET);


				/* ADD STINGER */
				
	GiveWorkerBeeStinger(newObj);
	

				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, 8, 8,false);
	
		
	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_WORKERBEE]++;
	return(true);
}


/********************* MOVE WORKERBEE **************************/

static void MoveWorkerBee(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveWorkerBee_Standing,
					MoveWorkerBee_Walking,
					MoveWorkerBee_ShootButt,
					MoveWorkerBee_Death,
					MoveWorkerBee_Pound	
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	myMoveTable[theNode->Skeleton->AnimNum](theNode);	
}



/********************** MOVE WORKERBEE: STANDING ******************************/

static void  MoveWorkerBee_Standing(ObjNode *theNode)
{
float	dist;

				/* TURN TOWARDS ME */
				
	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, WORKERBEE_TURN_SPEED, false);			

		

				/* SEE IF CHASE */

	dist = CalcQuickDistance(gMyCoord.x, gMyCoord.z, gCoord.x, gCoord.z);
	if (dist < WORKERBEE_CHASE_DIST)
	{					
		MorphToSkeletonAnim(theNode->Skeleton, WORKERBEE_ANIM_WALK, 8);
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
		KillWorkerBee(theNode);
	}

	UpdateWorkerBee(theNode);		
}




/********************** MOVE WORKERBEE: WALKING ******************************/

static void  MoveWorkerBee_Walking(ObjNode *theNode)
{
float		r,fps,aim,dist;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */
			
	aim = TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, WORKERBEE_TURN_SPEED, false);			

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * WORKERBEE_WALK_SPEED;
	gDelta.z = -cos(r) * WORKERBEE_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);
		
		
				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == WORKERBEE_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = WORKERBEE_WALK_SPEED * .004f;
	

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;
	
				/* SEE IF IN LIQUID */
				
	if (theNode->StatusBits & STATUS_BIT_UNDERWATER)
	{
		KillWorkerBee(theNode);
		goto update;
	}
	
	
			/*********************************/
			/* SEE IF CLOSE ENOUGH TO ATTACK */
			/**********************************/
	
				/* SHOOT STINGER */
				
	if (gPlayerMode == PLAYER_MODE_BUG)					// only shoot at bug
	{
		short anim = gPlayerObj->Skeleton->AnimNum;
		
		if ((anim == PLAYER_ANIM_STAND) || (anim == PLAYER_ANIM_WALK) ||	// only shoot if player is in one of these anims
			(anim == PLAYER_ANIM_KICK) || (anim == PLAYER_ANIM_LAND))
		{
			if (aim < 1.1f)									// must be aiming at me
			{
				dist = CalcQuickDistance(gMyCoord.x, gMyCoord.z, gCoord.x, gCoord.z);
				if (dist < WORKERBEE_ATTACK_DIST)
				{					
					MorphToSkeletonAnim(theNode->Skeleton, WORKERBEE_ANIM_SHOOTBUTT, 2);
					theNode->ShootButtFlag = false;
					theNode->TargetRot = theNode->Rot.y + PI;
					gDelta.x = gDelta.z = 0;
				}
			}		
		}
	}
	
			/* SEE IF POUND THE BALL */
			
	else
	{
		if (theNode->TimeUntilPound <= 0.0f)				// see if can go now
		{
			if (aim < 1.4f)									// must be aiming at me
			{
				dist = CalcQuickDistance(gMyCoord.x, gMyCoord.z, gCoord.x, gCoord.z);
				if (dist < 140.0f)
				{					
					MorphToSkeletonAnim(theNode->Skeleton, WORKERBEE_ANIM_POUND, 5);
					gDelta.x = gDelta.z = 0;
					theNode->PoundFlag = false;
				}
			}		
		}
	}
	
update:	
	UpdateWorkerBee(theNode);		
}


/********************** MOVE WORKERBEE: SHOOT BUTT ******************************/

static void  MoveWorkerBee_ShootButt(ObjNode *theNode)
{
float	r,fps = gFramesPerSecondFrac;

	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, WORKERBEE_TURN_SPEED, false);			

			/* MOVE */
				
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);
		

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


			/********************/	
			/* SEE IF SHOOT NOW */
			/********************/	

	if (theNode->ShootButtFlag)
	{
			/* SHOOT STINGER */
			
		ShootStinger(theNode);
		
		
		/* ENEMY FALLS ON FACE */
			
		r = theNode->Rot.y;
		gDelta.x = sin(r) * 800.0f;
		gDelta.y = 300.0f;
		gDelta.z = cos(r) * 800.0f;
		MorphToSkeletonAnim(theNode->Skeleton, WORKERBEE_ANIM_FALLONFACE, 3);
	}
				

	UpdateWorkerBee(theNode);		
}




/********************** MOVE WORKERBEE: DEATH ******************************/

static void  MoveWorkerBee_Death(ObjNode *theNode)
{
	
			/* SEE IF GONE */
			
	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
nukeit:	
		DeleteEnemy(theNode);
		return;
	}

	if (theNode->StatusBits & STATUS_BIT_ISCULLED)		// if was culled on last frame and is far enough away, then delete it
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) > 900.0f)
			goto nukeit;
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
			
	UpdateWorkerBee(theNode);		

}

/********************** MOVE WORKERBEE: POUND ******************************/

static void  MoveWorkerBee_Pound(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* MOVE */
				
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);
		

		/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


		/***********************/
		/* SEE IF TRY TO POUND */
		/***********************/
		
	if (theNode->PoundFlag)
	{
		static const TQ3Point3D off = {0,0,-100};
		TQ3Point3D	pp;
		
		FindCoordOnJoint(theNode, 0, &off, &pp);

		if (DoSimpleBoxCollisionAgainstPlayer(pp.y+10.0f, pp.y+10.0f,
										pp.x-10.0f, pp.x+10.0f,
										pp.z+10.0f, pp.z-10.0f))
		{
			PlayerGotHurt(theNode, .4, false, false, true, 1.1);
		}
	}
	

		/* SEE IF ANIM IS DONE */
		
	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, WORKERBEE_ANIM_STAND, 6);
		theNode->TimeUntilPound = 1.5;					// n seconds until can pound again
	}

	UpdateWorkerBee(theNode);		
}




//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME WORKERBEE ENEMY *************************/

Boolean PrimeEnemy_WorkerBee(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj,*shadowObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);

			/*******************************/
			/* MAKE DEFAULT SKELETON ENEMY */
			/*******************************/
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_WORKERBEE,x,z, WORKERBEE_SCALE);
	if (newObj == nil)
		return(false);
		
		
	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;
	
	SetSkeletonAnim(newObj->Skeleton, WORKERBEE_ANIM_WALK);
		

				/* SET BETTER INFO */
			
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= WORKERBEE_FOOT_OFFSET;			
	newObj->SplineMoveCall 	= MoveWorkerBeeOnSpline;				// set move call
	newObj->Health 			= 1.0;
	newObj->Damage 			= 0;
	newObj->Kind 			= ENEMY_KIND_WORKERBEE;
	newObj->CType			|= CTYPE_KICKABLE|CTYPE_AUTOTARGET;	// these can be kicked
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 70,WORKERBEE_FOOT_OFFSET,-70,70,70,-70);
	CalcNewTargetOffsets(newObj,WORKERBEE_TARGET_OFFSET);


	newObj->InitCoord = newObj->Coord;							// remember where started

				/* MAKE SHADOW */
				
	shadowObj = AttachShadowToObject(newObj, 8, 8, false);

				/* MAKE STINGER */
				
	GiveWorkerBeeStinger(newObj);
				


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	AddToSplineObjectList(newObj);


			/* DETACH FROM LINKED LIST */
			
	DetachObject(newObj);							// detach enemy
	DetachObject(newObj->ChainNode);				// detach stinger
	DetachObject(shadowObj);						// detach shadow

	return(true);
}


/******************** MOVE WORKERBEE ON SPLINE ***************************/

static void MoveWorkerBeeOnSpline(ObjNode *theNode)
{
Boolean isVisible; 

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, WORKERBEE_SPLINE_SPEED);
	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/
			
	if (isVisible)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);		

		theNode->Coord.y = GetTerrainHeightAtCoord(theNode->Coord.x, theNode->Coord.z, FLOOR) - WORKERBEE_FOOT_OFFSET;	// calc y coord

		UpdateObjectTransforms(theNode);																// update transforms
		CalcObjectBoxFromNode(theNode);																	// update collision box
		UpdateShadow(theNode);																			// update shadow		
		AlignStingerOnBee(theNode);	
		
			/* SEE IF CLOSE ENOUGH TO ATTACK */
			
		if (CalcQuickDistance(gMyCoord.x, gMyCoord.z, theNode->Coord.x, theNode->Coord.z) < WORKERBEE_DETACH_DIST)
		{					
			MorphToSkeletonAnim(theNode->Skeleton, WORKERBEE_ANIM_WALK, 8);
			DetachEnemyFromSpline(theNode, MoveWorkerBee);
			return;
		}
			
	}
	
			/* HIDE SOME THINGS SINCE INVISIBLE */
	else
	{
//		if (theNode->ShadowNode)						// hide shadow
//			theNode->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;	
//		if (theNode->ChainNode)							// hide stinger
//			theNode->ChainNode->StatusBits |= STATUS_BIT_HIDDEN;	
	}
	
}




#pragma mark -

/******************* BALL HIT WORKERBEE *********************/
//
// OUTPUT: true if enemy gets killed
//
// Worker bees cannot be knocked down.  Instead, it just pisses them off.
//

Boolean BallHitWorkerBee(ObjNode *me, ObjNode *enemy)
{
	enemy->TimeUntilPound = 0;			// let enemy pound now
	if (me->Speed > 500.0f)				// play effect if hit fast enough
		PlayEffect_Parms3D(EFFECT_POUND, &gCoord, kMiddleC+2, 2.0);
	return(false);		
}




/****************** KILL FIRE WORKERBEE *********************/
//
// OUTPUT: true if ObjNode was deleted
//

Boolean KillWorkerBee(ObjNode *theNode)
{
	
		/* IF ON SPLINE, DETACH */
		
	DetachEnemyFromSpline(theNode, MoveWorkerBee);


			/* DEACTIVATE */
			
	if (gRealLevel != LEVEL_NUM_QUEENBEE)			// always come back on queen level
		theNode->TerrainItemPtr = nil;				// dont ever come back 
	theNode->CType = CTYPE_MISC;
	
	
	return(false);
}





/***************** UPDATE WORKERBEE ************************/

static void UpdateWorkerBee(ObjNode *theNode)
{
	theNode->TimeUntilPound -= gFramesPerSecondFrac;

	UpdateEnemy(theNode);
	AlignStingerOnBee(theNode);	
}


#pragma mark -

/***************** GIVE WORKER BEE STINGER ***************/

static void GiveWorkerBeeStinger(ObjNode *theNode)
{
ObjNode	*stinger;

			/* MAKE SPEAR OBJECT */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= HIVE_MObjType_Stinger;	
	gNewObjectDefinition.coord		= theNode->Coord;
	gNewObjectDefinition.flags 		= theNode->StatusBits;
	gNewObjectDefinition.slot 		= theNode->Slot+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= WORKERBEE_SCALE-(WORKERBEE_SCALE*STINGER_SCALE);
	stinger = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	theNode->ChainNode = stinger;
}


/***************** ALIGN STINGER ON BEE ******************/

static void AlignStingerOnBee(ObjNode *bee)
{
static const TQ3Point3D zero = {0,0,0};
ObjNode 		*stinger;
TQ3Matrix4x4	m,m2,m3;

	stinger = bee->ChainNode;
	if (!stinger)									// see if this bee still has a stinger
		return;

//	stinger->StatusBits &= ~STATUS_BIT_HIDDEN;							// make sure not hidden

			/* CALC MATRIX TO ALIGN THE STINGER */

	Q3Matrix4x4_SetScale(&m3, STINGER_SCALE, STINGER_SCALE, STINGER_SCALE);	// scale
	Q3Matrix4x4_SetRotate_X(&m2, .4);										// calc rotate matrix
	m2.value[3][1] = -14.0 * WORKERBEE_SCALE;								// insert translation
	m2.value[3][2] = 38.0 * WORKERBEE_SCALE;
	MatrixMultiplyFast(&m3, &m2, &m);
			
	FindJointFullMatrix(bee, WORKERBEE_JOINT_BUTT, &m2);

	MatrixMultiplyFast(&m, &m2, &stinger->BaseTransformMatrix);

			/* CALC COORD OF STINGER */
			
	Q3Point3D_Transform(&zero, &stinger->BaseTransformMatrix, &stinger->Coord);
}


/******************* SHOOT STINGER ***********************/

static void ShootStinger(ObjNode *bee)
{
ObjNode			*stinger;
TQ3Vector3D		delta;

	stinger = bee->ChainNode;
	if (!stinger)									// see if this bee still has a stinger
		return;

	AttachObject(stinger);							// make sure its in the linked list

	bee->ChainNode = nil;							// detach from chain
	stinger->MoveCall = MoveStinger;

	stinger->Delta.x = -sin(bee->Rot.y) * STINGER_SPEED;
	stinger->Delta.z = -cos(bee->Rot.y) * STINGER_SPEED;
	
	stinger->Rot.y = bee->Rot.y + PI;
	

		/* SET STINGER COLLISION INFO */
			
	stinger->CType = CTYPE_HURTME;
	stinger->CBits = CBITS_TOUCHABLE;
	stinger->Damage = .3;
	SetObjectCollisionBounds(stinger, 40,-40,-50,50,50,-50);


		/***************/
		/* MAKE SPARKS */
		/***************/

	int32_t pg = NewParticleGroup(
							PARTICLE_TYPE_FALLINGSPARKS,	// type
							PARTICLE_FLAGS_BOUNCE,		// flags
							500,						// gravity
							0,							// magnetism
							15,							// base scale
							1.3,						// decay rate
							0,							// fade rate
							PARTICLE_TEXTURE_YELLOWBALL);// texture
	
	if (pg != -1)
	{
		for (int i = 0; i < 15; i++)
		{
			delta.x = (RandomFloat()-.5f) * 400.0f;
			delta.y = (RandomFloat()-.5f) * 400.0f;
			delta.z = (RandomFloat()-.5f) * 400.0f;
			AddParticleToGroup(pg, &stinger->Coord, &delta, RandomFloat() + 1.0f, FULL_ALPHA);
		}
	}
	
			/* PLAY EFFECT */
			
	PlayEffect3D(EFFECT_STINGERSHOOT, &stinger->Coord);
}


/****************** MOVE STINGER ******************/

static void MoveStinger(ObjNode *theNode)
{
float fps = gFramesPerSecondFrac;

			/* SEE IF GONE */
			
	if (TrackTerrainItem(theNode))
	{
adios:
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);

	gDelta.y -= 300.0f * fps;					// gravity

	gCoord.x += gDelta.x * fps;					// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/* SEE IF HIT FLOOR OR CEILING */
		
	if ((gCoord.y < GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR)) ||
		(gCoord.y > GetTerrainHeightAtCoord(gCoord.x, gCoord.z, CEILING)))
	{
		goto adios;	
	}
	

	UpdateObject(theNode);
}












