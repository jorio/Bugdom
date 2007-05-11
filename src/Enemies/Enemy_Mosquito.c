/****************************/
/*   ENEMY: MOSQUITO.C		*/
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode					*gCurrentNode;
extern	SplineDefType			**gSplineList;
extern	TQ3Point3D				gCoord,gMyCoord;
extern	short					gNumEnemies;
extern	float					gFramesPerSecondFrac;
extern	TQ3Vector3D			gDelta;
extern	signed char			gNumEnemyOfKind[];
extern	ObjNode				*gPlayerObj;
extern	Byte				gPlayerMode;
extern	Boolean				gPlayerGotKilledFlag;
extern	u_short				gLevelType;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveMosquito(ObjNode *theNode);
static void MoveMosquito_Flying(ObjNode *theNode);
static void  MoveMosquito_Biting(ObjNode *theNode);
static void MoveMosquitoOnSpline(ObjNode *theNode);
static void  MoveMosquito_Death(ObjNode *theNode);
static void  MoveMosquito_Sucking(ObjNode *theNode);
static void UpdateMosquito(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_MOSQUITO				4

#define	MOSQUITO_CHASE_RANGE		400.0f
#define	MOSQUITO_BITE_RANGE			160.0f

#define MOSQUITO_TURN_SPEED			4.5f
#define MOSQUITO_CHASE_SPEED		460.0f
#define MOSQUITO_SPLINE_SPEED		200.0f

#define	MAX_MOSQUITO_RANGE			(MOSQUITO_CHASE_RANGE+800.0f)	// max distance this enemy can go from init coord

#define	MOSQUITO_HEALTH				1.0f		
#define	MOSQUITO_DAMAGE				0.04f
#define	MOSQUITO_SCALE				.8f
#define	MOSQUITO_FLIGHT_HEIGHT		350.0f


enum
{
	MOSQUITO_ANIM_FLY,
	MOSQUITO_ANIM_BITE,
	MOSQUITO_ANIM_DEATH,
	MOSQUITO_ANIM_SUCK,
	MOSQUITO_ANIM_FLYFULL
};


enum
{
	MOSQUITO_MODE_WAITING,
	MOSQUITO_MODE_GOHOME,
	MOSQUITO_MODE_CHASE,
	MOSQUITO_MODE_REALIGN
};

#define	MosquitoWobbleOff		sin(theNode->Wobble += fps*11.0f) * 10.0f


#define MOSQUITO_HEAD_JOINT			4

#define	MOSQUITO_KNOCKDOWN_SPEED	1400.0f		// speed ball needs to go to knock this down



/*********************/
/*    VARIABLES      */
/*********************/

static const TQ3Point3D gTipOffset = {0,-28,92};

#define	Wobble			SpecialF[2]
#define StuckTimer		SpecialF[0]
#define	StuckInGround	Flag[0]				// true if stinger stuck in ground
#define HonorRange		Flag[1]				// true = check max range


/************************ ADD MOSQUITO ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Mosquito(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj,*shadowObj;

	if (gNumEnemies >= MAX_ENEMIES)								// keep from getting absurd
		return(false);

				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_MOSQUITO,x,z,MOSQUITO_SCALE);
	if (newObj == nil)
		return(false);
	newObj->TerrainItemPtr = itemPtr;

	SetSkeletonAnim(newObj->Skeleton, MOSQUITO_ANIM_FLY);
	

				/* SET BETTER INFO */
			
	newObj->Coord.y 	+= MOSQUITO_FLIGHT_HEIGHT;			
	newObj->MoveCall 	= MoveMosquito;							// set move call
	newObj->Health 		= MOSQUITO_HEALTH;
	newObj->Damage 		= MOSQUITO_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_MOSQUITO;
	newObj->CType		|= CTYPE_KICKABLE|CTYPE_AUTOTARGET;		// these can be kicked
	newObj->CBits		= CBITS_NOTTOP;
	
	newObj->Mode		= MOSQUITO_MODE_WAITING;
	newObj->HonorRange	= true;
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 70,-40,-40,40,40,-40);


				/* MAKE SHADOW */
				
	shadowObj = AttachShadowToObject(newObj, 7, 7, false);

	newObj->InitCoord = newObj->Coord;							// remember where started

	newObj->Wobble = 0;


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_MOSQUITO]++;
	return(true);
}





/********************* MOVE MOSQUITO **************************/

static void MoveMosquito(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveMosquito_Flying,
					MoveMosquito_Biting,
					MoveMosquito_Death,
					MoveMosquito_Sucking,
					MoveMosquito_Flying		// flyfull
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}


/********************** MOVE MOSQUITO: FLYING ******************************/

static void  MoveMosquito_Flying(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	r,aim;

	theNode->Skeleton->AnimSpeed = 2.0f;

				/* SEE IF SWITCH TO NON-FULL FLYING ANIM */
				
	if (theNode->StatusBits & STATUS_BIT_ISCULLED)
		if (theNode->Skeleton->AnimNum == MOSQUITO_ANIM_FLYFULL)
			SetSkeletonAnim(theNode->Skeleton, MOSQUITO_ANIM_FLY);



	switch(theNode->Mode)
	{
			/*********************/
			/* GO UP TO TARGET Y */
			/*********************/
	
		case	MOSQUITO_MODE_REALIGN:
				if (gCoord.y < GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR) + MOSQUITO_FLIGHT_HEIGHT + MosquitoWobbleOff)
					gCoord.y += 150.0f *fps;
				else
					theNode->Mode = MOSQUITO_MODE_GOHOME;
				break;
	
			/*****************************/
			/* JUST HOVERING AND WAITING */
			/*****************************/
			
		case	MOSQUITO_MODE_WAITING:
				TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, MOSQUITO_TURN_SPEED, false);			
				
					/* SEE IF CLOSE ENOUGH TO ATTACK */
					
				if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < MOSQUITO_CHASE_RANGE)
				{
					theNode->Mode = MOSQUITO_MODE_CHASE;
					if (theNode->Skeleton->AnimNum != MOSQUITO_ANIM_FLY)
						MorphToSkeletonAnim(theNode->Skeleton, MOSQUITO_ANIM_FLYFULL, 6);
				}
				gCoord.y += MosquitoWobbleOff;
				break;
	
				
				/*****************/
				/* FLY BACK HOME */
				/*****************/
				
		case	MOSQUITO_MODE_GOHOME:
		
						/* MOVE TOWARD HOME */
						
				TurnObjectTowardTarget(theNode, &gCoord, theNode->InitCoord.x, theNode->InitCoord.z, MOSQUITO_TURN_SPEED, false);

				r = theNode->Rot.y;
				gDelta.x = sin(r) * -MOSQUITO_CHASE_SPEED;
				gDelta.z = cos(r) * -MOSQUITO_CHASE_SPEED;
				
				gCoord.x += gDelta.x * fps;
				gCoord.z += gDelta.z * fps;
				gCoord.y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR) + MOSQUITO_FLIGHT_HEIGHT;	// calc y coord
				gCoord.y += MosquitoWobbleOff;

						/* SEE IF THERE */
						
				if (CalcQuickDistance(gCoord.x, gCoord.z, theNode->InitCoord.x, theNode->InitCoord.z) < 150.0f)
					theNode->Mode = MOSQUITO_MODE_WAITING;
				break;
				
		
		
				/**************/
				/* CHASE MODE */
				/**************/
				
		default:
		
						/* MOVE TOWARD PLAYER */
						
				aim = TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, MOSQUITO_TURN_SPEED, false);			

				r = theNode->Rot.y;
				gDelta.x = sin(r) * -MOSQUITO_CHASE_SPEED;
				gDelta.z = cos(r) * -MOSQUITO_CHASE_SPEED;
				
				gCoord.x += gDelta.x * fps;
				gCoord.z += gDelta.z * fps;
				gCoord.y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR) + MOSQUITO_FLIGHT_HEIGHT;	// calc y coord
				gCoord.y += MosquitoWobbleOff;
		
				
				/* SEE IF CLOSE ENOUGH TO BITE */
					
				if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < MOSQUITO_BITE_RANGE)
				{
					MorphToSkeletonAnim(theNode->Skeleton, MOSQUITO_ANIM_BITE, 3.0);
					theNode->StuckInGround = false;
				}
				
					/* SEE IF TOO FAR AWAY AND NEED TO FLY BACK HOME */
					
				if (theNode->HonorRange)								// see if we care about range
				{
					if (CalcQuickDistance(theNode->InitCoord.x, theNode->InitCoord.z, gCoord.x, gCoord.z) > MAX_MOSQUITO_RANGE)
						theNode->Mode = MOSQUITO_MODE_GOHOME;				
				}
	}

				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/

	if (gLevelType == LEVEL_TYPE_POND)									// make sure not underwater
		if (gCoord.y < (WATER_Y+150))
			gCoord.y = WATER_Y+150;
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

	UpdateMosquito(theNode);		
	
}


/********************** MOVE MOSQUITO: BITING ******************************/

static void  MoveMosquito_Biting(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	r,y;
TQ3Point3D	tipPt;


	if (theNode->StuckInGround)								// see if stuck in ground
	{
		theNode->StuckTimer -= fps;
		if (theNode->StuckTimer <= 0.0f)					// see if get unstuck
		{
			MorphToSkeletonAnim(theNode->Skeleton, MOSQUITO_ANIM_FLY, 3);
			theNode->StuckInGround = false;
			theNode->Mode = MOSQUITO_MODE_REALIGN;
		}
	}
	else	
	{
					/**********************/
					/* MOVE TOWARD PLAYER */
					/**********************/
							
		TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, MOSQUITO_TURN_SPEED, false);			

		r = theNode->Rot.y;
		gDelta.x = sin(r) * -MOSQUITO_CHASE_SPEED;
		gDelta.z = cos(r) * -MOSQUITO_CHASE_SPEED;
		gDelta.y -= 2000.0f * fps;							// move down during attack

		gCoord.x += gDelta.x * fps;
		gCoord.y += gDelta.y * fps;
		gCoord.z += gDelta.z * fps;

				/* SEE IF HIT WATER */
				
		if (gLevelType == LEVEL_TYPE_POND)						// make sure not underwater
			if (gCoord.y < (WATER_Y+150))
			{
				gCoord.y = WATER_Y+150;
				MorphToSkeletonAnim(theNode->Skeleton, MOSQUITO_ANIM_FLY, 5);
				theNode->Mode = MOSQUITO_MODE_REALIGN;
			}

				/* UPDATE BASE MATRIX */
				
		theNode->Coord = gCoord;		
		UpdateObjectTransforms(theNode);					// make sure base matrix is up-to-date!

		
				/*********************/
				/* SEE IF HIT GROUND */
				/*********************/
				
		FindCoordOnJoint(theNode, MOSQUITO_HEAD_JOINT, &gTipOffset, &tipPt);		// get coord of tip of stinger
		y = GetTerrainHeightAtCoord(tipPt.x, tipPt.z, FLOOR);					// get ground y at tip pt
		if (tipPt.y <= y)														// see if hit ground
		{
			gCoord.y += y-tipPt.y;
			gDelta.x = 
			gDelta.y = 
			gDelta.z = 0;
			theNode->StuckInGround = true;
			theNode->StuckTimer = 1.1f;
		}
		
				/*********************/
				/* SEE IF HIT PLAYER */
				/*********************/
				
		if (!theNode->StuckInGround)
		{
			if (DoSimplePointCollision(&tipPt, CTYPE_PLAYER))
			{
				PlayEffect3D(EFFECT_SLURP, &gCoord);
			
				MorphToSkeletonAnim(theNode->Skeleton, MOSQUITO_ANIM_SUCK, 6);	
				
				if (gPlayerMode == PLAYER_MODE_BALL)				
					InitPlayer_Bug(gPlayerObj, &gPlayerObj->Coord, gPlayerObj->Rot.y, PLAYER_ANIM_BLOODSUCK);
				else												
					MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_BLOODSUCK, 7);	
					
				gPlayerObj->Delta.x = 
				gPlayerObj->Delta.y = 
				gPlayerObj->Delta.z = 0;
					
						/* HURT PLAYER & VERIFY */
						
				PlayerGotHurt(nil, .2, true, false, false, 2.0);					// make invincible for 2 seconds
				if (gPlayerGotKilledFlag)
				{
					MorphToSkeletonAnim(theNode->Skeleton, MOSQUITO_ANIM_FLY, 6);	
					theNode->Mode = MOSQUITO_MODE_REALIGN;
					goto update;
				}
					
					/* CALL THIS TO ALIGN STUFF RIGHT NOW */
					
				MoveMosquito_Sucking(theNode);
				return;
			}
		}
	}		

				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

update:
	UpdateMosquito(theNode);		
	
}


/********************** MOVE MOSQUITO: DEATH ******************************/

static void  MoveMosquito_Death(ObjNode *theNode)
{
				/* GET INFO */
				
	GetObjectInfo(theNode);
	
	
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
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;			// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEATH_ENEMY_COLLISION_CTYPES))
		return;


				/* UPDATE */
			
	UpdateMosquito(theNode);		
}

/********************** MOVE MOSQUITO: SUCKING ******************************/

static void  MoveMosquito_Sucking(ObjNode *theNode)
{
TQ3Point3D	tipPt,headPt;
static const TQ3Point3D headOffset = {0,18,-17};


				/* CALC COORDS OF TIP & HEAD */
				
	FindCoordOnJoint(theNode, MOSQUITO_HEAD_JOINT, &gTipOffset, &tipPt);		
	FindCoordOnJoint(gPlayerObj, BUG_LIMB_NUM_HEAD, &headOffset, &headPt);		


			/* CALC OFFSET TO PUT STINGER INTO HEAD */
			
	gCoord.x = headPt.x - (tipPt.x - gCoord.x);
	gCoord.y = headPt.y - (tipPt.y - gCoord.y);
	gCoord.z = headPt.z - (tipPt.z - gCoord.z);


			/* SEE IF DONE */
			
	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, MOSQUITO_ANIM_FLYFULL, 5);
		theNode->Mode = MOSQUITO_MODE_REALIGN;
		MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_STAND, 7);
		gPlayerObj->CType = CTYPE_PLAYER;
	}
	
	UpdateMosquito(theNode);			
}


/***************** UPDATE MOSQUITO ********************/

static void UpdateMosquito(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimNum != MOSQUITO_ANIM_DEATH)
	{
		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect3D(EFFECT_BUZZ, &theNode->Coord);
		else
			Update3DSoundChannel(EFFECT_BUZZ, &theNode->EffectChannel, &theNode->Coord);
	}

	UpdateEnemy(theNode);			

}


#pragma mark -

/************************ PRIME MOSQUITO ENEMY *************************/

Boolean PrimeEnemy_Mosquito(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj,*shadowObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_MOSQUITO,x,z, MOSQUITO_SCALE);
	if (newObj == nil)
		return(false);
		
	DetachObject(newObj);									// detach this object from the linked list
		
	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;
	
	SetSkeletonAnim(newObj->Skeleton, MOSQUITO_ANIM_FLY);
	

				/* SET BETTER INFO */
			
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		+= MOSQUITO_FLIGHT_HEIGHT;			
	newObj->SplineMoveCall 	= MoveMosquitoOnSpline;				// set move call
	newObj->Health 			= MOSQUITO_HEALTH;
	newObj->Damage 			= MOSQUITO_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_MOSQUITO;
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

/******************** MOVE MOSQUITO ON SPLINE ***************************/
//
// NOTE: This is called for every Mosquito on the spline in the entire level.
//		The isVisible flag determines if this particular one is in visible range.
//		It actually means the enemy is on an active supertile.
//

static void MoveMosquitoOnSpline(ObjNode *theNode)
{
Boolean isVisible; 
float	fps = gFramesPerSecondFrac;

	theNode->Skeleton->AnimSpeed = 2.0f;


	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, MOSQUITO_SPLINE_SPEED);
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

		theNode->Coord.y = GetTerrainHeightAtCoord(theNode->Coord.x, theNode->Coord.z, FLOOR) + MOSQUITO_FLIGHT_HEIGHT;	// calc y coord
		theNode->Coord.y += MosquitoWobbleOff;															// do wobble

		if (gLevelType == LEVEL_TYPE_POND)																	// make sure not underwater
			if (gCoord.y < (WATER_Y+150))
				gCoord.y = WATER_Y+150;

		UpdateObjectTransforms(theNode);																// update transforms
		CalcObjectBoxFromNode(theNode);																	// update collision box
		UpdateShadow(theNode);																			// update shadow		
		
				/*********************************/
				/* SEE IF CLOSE ENOUGH TO ATTACK */
				/*********************************/
				
		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gMyCoord.x, gMyCoord.z) < MOSQUITO_CHASE_RANGE)
		{
					/* REMOVE FROM SPLINE */
					
			DetachEnemyFromSpline(theNode, MoveMosquito);

			theNode->Mode		= MOSQUITO_MODE_CHASE;
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


/******************* BALL HIT MOSQUITO *********************/
//
// OUTPUT: true if enemy gets killed
//

Boolean BallHitMosquito(ObjNode *me, ObjNode *enemy)
{
Boolean	killed = false;

				/************************/
				/* HURT & KNOCK ON BUTT */
				/************************/
				
	if (me->Speed > MOSQUITO_KNOCKDOWN_SPEED)
	{	
		KillMosquito(enemy, me->Delta.x*.5f, 400, me->Delta.z*.5f);
   		killed = true;
		PlayEffect_Parms3D(EFFECT_POUND, &gCoord, kMiddleC+2, 2.0);
	}
	
	return(killed);
}


/****************** KILL MOSQUITO *********************/
//
// OUTPUT: true if ObjNode was deleted
//

Boolean KillMosquito(ObjNode *theNode, float dx, float dy, float dz)
{
		/* SEE IF THIS WAS SUCKING PLAYER */

	if (theNode->Skeleton->AnimNum == MOSQUITO_ANIM_SUCK)
	{
		if (gPlayerMode == PLAYER_MODE_BUG)
			MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_STAND, 7);	// player back to standing
	}

		/* STOP BUZZ */
		
	if (theNode->EffectChannel != -1)
		StopAChannel(&theNode->EffectChannel);
	
		/* IF ON SPLINE, DETACH */
		
	DetachEnemyFromSpline(theNode, MoveMosquito);


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
			
	if (theNode->Skeleton->AnimNum != MOSQUITO_ANIM_DEATH)
		SetSkeletonAnim(theNode->Skeleton, MOSQUITO_ANIM_DEATH);
		
	theNode->Delta.x = dx;	
	theNode->Delta.y = dy;	
	theNode->Delta.z = dz;	
	
	return(false);
}










