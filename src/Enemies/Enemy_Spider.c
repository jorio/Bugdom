/****************************/
/*   ENEMY: SPIDER.C		*/
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

static void MoveSpider(ObjNode *theNode);
static void MoveSpider_Waiting(ObjNode *theNode);
static void  MoveSpider_Drop(ObjNode *theNode);
static void  MoveSpider_Walking(ObjNode *theNode);
static void  MoveSpider_Spit(ObjNode *theNode);
static void  MoveSpider_Death(ObjNode *theNode);
static void  MoveSpider_GetOffButt(ObjNode *theNode);
static void  MoveSpider_FallOnButt(ObjNode *theNode);
static void SpiderShootWeb(ObjNode *theEnemy);
static void MoveThread(ObjNode *theNode);
static void MoveWebBullet(ObjNode *theNode);
static void MoveWebSphere(ObjNode *theNode);
static void  MoveSpider_Jump(ObjNode *theNode);
static void StartSpiderJump(ObjNode *theNode);
static void MoveSpiderOnSpline(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_SPIDER				5

#define	SPIDER_DROP_DIST		700.0f

#define SPIDER_ATTACK_DIST		500.0f
#define	SPIDER_JUMPDY			1100.0f

#define SPIDER_TURN_SPEED		.9f
#define	SPIDER_WALK_SPEED		300.0f
#define	SPIDER_KNOCKDOWN_SPEED	1400.0f					// speed ball needs to go to knock this down

#define	SPIDER_HEALTH			.3f		
#define	SPIDER_DAMAGE			0.2f

#define	SPIDER_SCALE			.9f
#define	SPIDER_START_YOFF		1000.0f

#define SPIDER_SPLINE_SPEED		45.0f

enum
{
	SPIDER_ANIM_WAIT,
	SPIDER_ANIM_DROP,
	SPIDER_ANIM_WALK,
	SPIDER_ANIM_ATTACK,
	SPIDER_ANIM_DIE,
	SPIDER_ANIM_FALLONBUTT,
	SPIDER_ANIM_GETOFFBUTT,
	SPIDER_ANIM_JUMP
};

#define	SPIDER_FOOT_OFFSET		55.0f



#define	THREAD_YOFF			100.0f
#define	WEB_SPEED			600.0f
#define	WEB_SPHERE_SCALE	1.2f
#define	SPHERE_DURATION		2.5f				// # seconds that sphere last


/*********************/
/*    VARIABLES      */
/*********************/

#define	ShootWeb		Flag[0]
#define	ButtTimer		SpecialF[0]				// timer for on butt


/************************ ADD SPIDER ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Spider(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj,*threadObj;
static const Byte type[] =
{
	0,						// lawn
	0, 						// pond
	FOREST_MObjType_Thread,	// forest
	0,						// hive
	NIGHT_MObjType_Thread,	// night
	0						// anthill
};

	if (gNumEnemies >= MAX_ENEMIES)					// keep from getting absurd
		return(false);

	if (gNumEnemyOfKind[ENEMY_KIND_SPIDER] >= MAX_SPIDER)
		return(false);

				/*********************/
				/* CREATE THE SPIDER */
				/*********************/
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_SPIDER,x,z,SPIDER_SCALE);
	if (newObj == nil)
		return(false);
	newObj->TerrainItemPtr = itemPtr;

	
				/* SET BETTER INFO */

	newObj->Coord.y 	+= SPIDER_START_YOFF;					// start spider up high
	newObj->StatusBits	|= STATUS_BIT_HIDDEN;					// its hidden to start
	newObj->MoveCall 	= MoveSpider;							// set move call
	newObj->Health 		= SPIDER_HEALTH;
	newObj->Damage 		= SPIDER_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_SPIDER;
	
				/* SET COLLISION INFO */
				
	newObj->CBits = ALL_SOLID_SIDES;
	newObj->CBits		= CBITS_NOTTOP;
	newObj->CType = 0;											// no collision yet
	SetObjectCollisionBounds(newObj, 40,-SPIDER_FOOT_OFFSET,-90,90,90,-90);


				/*********************/
				/* CREATE THE THREAD */
				/*********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= type[gLevelType];
	gNewObjectDefinition.coord.y 	+= THREAD_YOFF;
	gNewObjectDefinition.flags 		= STATUS_BIT_HIDDEN;			// initially its hidden
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= SPIDER_SCALE;
	threadObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (threadObj == nil)
		return(false);

	newObj->ChainNode = threadObj;								// chain thread to spider


				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, 8, 8, false);

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_SPIDER]++;
	return(true);
}



/********************* MOVE SPIDER **************************/

static void MoveSpider(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
		{
			MoveSpider_Waiting,
			MoveSpider_Drop,
			MoveSpider_Walking,
			MoveSpider_Spit,
			MoveSpider_Death,
			MoveSpider_FallOnButt,
			MoveSpider_GetOffButt,
			MoveSpider_Jump
		};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}


/********************** MOVE SPIDER: WAITING ******************************/

static void  MoveSpider_Waiting(ObjNode *theNode)
{
			/* SEE IF DROP DOWN */
			
	if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < SPIDER_DROP_DIST)
	{
		theNode->StatusBits &= ~STATUS_BIT_HIDDEN;					// not hidden anymore
		SetSkeletonAnim(theNode->Skeleton, SPIDER_ANIM_DROP);
		if (theNode->ChainNode)
			theNode->ChainNode->StatusBits &= ~STATUS_BIT_HIDDEN;	// unhide thread also
		theNode->CType = CTYPE_ENEMY|CTYPE_KICKABLE|CTYPE_AUTOTARGET|CTYPE_SPIKED;
		MoveSpider_Drop(theNode);									// call this to make sure everything is primed okay
		return;
	}
	UpdateEnemy(theNode);
}



/********************** MOVE SPIDER: DROP ******************************/

static void  MoveSpider_Drop(ObjNode *theNode)
{
float	y;
ObjNode	*threadObj;

	threadObj = theNode->ChainNode;

	y = GetTerrainHeightAtCoord(gCoord.x,gCoord.z,FLOOR);		// get ground y

	gDelta.y = -((gCoord.y - y)*1.7f + 100.0f);					// drop speed slows as approaches ground
	gCoord.y += gDelta.y * gFramesPerSecondFrac;
	

			/* SEE IF HIT GROUND */
			
	if ((gCoord.y - SPIDER_FOOT_OFFSET) <= y)
	{
		gCoord.y = y + SPIDER_FOOT_OFFSET;	
		MorphToSkeletonAnim(theNode->Skeleton, SPIDER_ANIM_WALK, 5);
		if (threadObj)
		{
			threadObj->MoveCall = MoveThread;				// give it a move call
			theNode->ChainNode = nil;						// detach from chain
		}
	}
	
			/* SLOWLY ROTATE TOWARD PLAYER */
			
	TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, SPIDER_TURN_SPEED, false);			
	
	
	UpdateEnemy(theNode);


			/* UPDATE THREAD */
			
	if (threadObj)
		threadObj->Coord.y = gCoord.y + THREAD_YOFF;	
	UpdateObjectTransforms(threadObj);
}

/********************** MOVE SPIDER: WALKING ******************************/

static void  MoveSpider_Walking(ObjNode *theNode)
{
float		r,fps,aim,speed;
Boolean		canWeb;

	fps = gFramesPerSecondFrac;
		
	speed = SPIDER_WALK_SPEED;
		
	theNode->Skeleton->AnimSpeed = speed * .004f;				// keep anim synced with max walk speed
	theNode->Speed = speed;

			/* MOVE TOWARD PLAYER */
			
	aim = TurnObjectTowardTarget(theNode, &gCoord, gMyCoord.x, gMyCoord.z, SPIDER_TURN_SPEED, false);			

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;
	gDelta.y -= ENEMY_GRAVITY*fps;											// add gravity
	MoveEnemy(theNode);

				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

			/*********************************/
			/* SEE IF CLOSE ENOUGH TO ATTACK */
			/*********************************/
			
	canWeb = true;
	if (gPlayerMode == PLAYER_MODE_BUG)										// if player already webbed then dont do it again
		if (gPlayerObj->Skeleton->AnimNum == PLAYER_ANIM_WEBBED)
			canWeb = false;
			
			/* SEE IF SHOOT WEB */
			
	if (canWeb)
	{
		if (aim < 1.0f)
		{
			if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < SPIDER_ATTACK_DIST)
			{
				MorphToSkeletonAnim(theNode->Skeleton, SPIDER_ANIM_ATTACK, 2.0);				
				theNode->ShootWeb = false;
			}
		}
	}
			/* SEE IF POUNCE */
			
	else
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z) < SPIDER_ATTACK_DIST)
		{
			StartSpiderJump(theNode);
		}
	}	

	
	UpdateEnemy(theNode);		
}


/********************** MOVE SPIDER: SPIT ******************************/

static void  MoveSpider_Spit(ObjNode *theNode)
{

			/********************/
			/* SEE IF SHOOT WEB */
			/********************/
			
	if (theNode->ShootWeb)
	{
		theNode->ShootWeb = false;
	
		SpiderShootWeb(theNode);
	}

			/* SEE IF DONE WITH ANIM */
			
	if (theNode->Skeleton->AnimHasStopped)
	{
		SetSkeletonAnim(theNode->Skeleton, SPIDER_ANIM_WALK);	
	}


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

	UpdateEnemy(theNode);		
}


/********************** MOVE SPIDER: FALLONBUTT ******************************/

static void  MoveSpider_FallOnButt(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(140.0,&gDelta);
		
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity

	MoveEnemy(theNode);


				/* SEE IF DONE */
			
	theNode->ButtTimer -= gFramesPerSecondFrac;
	if (theNode->ButtTimer <= 0.0)
	{
		SetSkeletonAnim(theNode->Skeleton, SPIDER_ANIM_GETOFFBUTT);
	}

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


	UpdateEnemy(theNode);		
}


/********************** MOVE SPIDER: DEATH ******************************/

static void  MoveSpider_Death(ObjNode *theNode)
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
			
	UpdateEnemy(theNode);		

}


/********************** MOVE SPIDER: GET OFF BUTT ******************************/

static void  MoveSpider_GetOffButt(ObjNode *theNode)
{
	ApplyFrictionToDeltas(60.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity

	MoveEnemy(theNode);


				/* SEE IF DONE */
			
	if (theNode->Skeleton->AnimHasStopped)
	{
		SetSkeletonAnim(theNode->Skeleton, SPIDER_ANIM_WALK);
	}


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;



	UpdateEnemy(theNode);		
}


/********************** MOVE SPIDER: JUMP ******************************/

static void MoveSpider_Jump(ObjNode *theNode)
{
float	fps;

	fps = gFramesPerSecondFrac;
		
	gDelta.y -= ENEMY_GRAVITY * fps;					// gravity
	MoveEnemy(theNode);
	
	
				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


			/* SEE IF LANDED */
				
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		gDelta.y = 0;
		gDelta.x = 0;
		gDelta.z = 0;		
		MorphToSkeletonAnim(theNode->Skeleton, SPIDER_ANIM_WALK, 5);	
	}
	
	UpdateEnemy(theNode);		
}






#pragma mark -

/********************* MOVE THREAD **************************/

static void MoveThread(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode) || (theNode->StatusBits & STATUS_BIT_ISCULLED))
	{
		DeleteObject(theNode);
		return;
	}
}



/********************* SPIDER SHOOT WEB ************************/
//
// The web bullet is actually a non-harmful trigger object.
// When the player triggers it, they get paralyzed and the spider will pounce.
//

static void SpiderShootWeb(ObjNode *theEnemy)
{
ObjNode	*newObj;
static const TQ3Point3D inPt = {0,50,-40};
float	r;
static const Byte type[] =
{
	0,						// lawn
	0, 						// pond
	FOREST_MObjType_WebBullet,	// forest
	0,						// hive
	NIGHT_MObjType_WebBullet,	// night
	0						// anthill
};


			/*********************/
			/* CREATE WEB BULLET */
			/*********************/

	FindCoordOnJoint(theEnemy, 0, &inPt, &gNewObjectDefinition.coord);	// get coord of mouth

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= type[gLevelType];	
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER | STATUS_BIT_GLOW | STATUS_BIT_ROTXZY | STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveWebBullet;
	gNewObjectDefinition.rot 		= theEnemy->Rot.y + PI;
	gNewObjectDefinition.scale 		= SPIDER_SCALE * .1;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;

	newObj->Health 			= 1.0;							// timer for duration & fading
	newObj->SpecialF[3]		= gNewObjectDefinition.scale;	// f3 is initial scale

			/* SET TRIGGER STUFF */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_PLAYERTRIGGERONLY|CTYPE_AUTOTARGET;
	newObj->CBits 			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind 			= TRIGTYPE_WEBBULLET;
	SetObjectCollisionBounds(newObj,50,-100,-50,50,50,-50);


			/* SET VELOCITY INFO */

	r = theEnemy->Rot.y;
	newObj->Delta.x = -sin(r) * WEB_SPEED;
	newObj->Delta.z = -cos(r) * WEB_SPEED;

}


/******************** MOVE WEB BULLET *************************/

static void MoveWebBullet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	t;

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
del:	
		DeleteObject(theNode);
		return;
	}

			/* SEE IF BURNED OUT */
			
	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
		goto del;


	GetObjectInfo(theNode);
	
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	t = theNode->SpecialF[3] += fps * .5f;	
	theNode->Scale.x = t;
	theNode->Scale.y = t;
	theNode->Scale.z = t;
	
	theNode->Rot.z += fps*2.0f;
	
	
			/* FADE OUT */
			
	t = theNode->Health * 3.0f;
	if (t < 1.0f)
		MakeObjectTransparent(theNode, t);
	
	UpdateObject(theNode);
}


/************** DO TRIGGER - WEB BULLET ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_WebBullet(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
ObjNode *newObj;
static const Byte type[] =
{
	0,						// lawn
	0, 						// pond
	FOREST_MObjType_WebSphere,	// forest
	0,						// hive
	NIGHT_MObjType_WebSphere,	// night
	0						// anthill
};


	(void) whoNode;
	(void) sideBits;
	
	DeleteObject(theNode);												// delete the web bullet

			/****************************/
			/* PUT PLAYER INTO WEB MODE */
			/****************************/
						
	if (gPlayerMode == PLAYER_MODE_BALL)								// see if turn into bug
		InitPlayer_Bug(gPlayerObj, &gPlayerObj->Coord, gPlayerObj->Rot.y, PLAYER_ANIM_WEBBED);
	else
	{
		if (gPlayerObj->Skeleton->AnimNum == PLAYER_ANIM_WEBBED)		// see if already webbed
			return(false);
		else
			MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_WEBBED, 5);
	}

			/* CREATE THE SPHERE */
		
	gNewObjectDefinition.coord 		= gPlayerObj->Coord;	
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= type[gLevelType];	
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER | STATUS_BIT_GLOW | STATUS_BIT_KEEPBACKFACES_2PASS;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+20;
	gNewObjectDefinition.moveCall 	= MoveWebSphere;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= WEB_SPHERE_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->Health = SPHERE_DURATION;

	return(false);
}


/******************* MOVE WEB SPHERE *********************/

static void MoveWebSphere(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
Boolean	gotout = false;

	theNode->Health -= fps;

			/*************************/
			/* SEE IF PLAYER GOT OUT */
			/*************************/

	
		/* SEE IF PLAYER IS BALL OR NOT IN CORRECT ANIM */
			
	if (gPlayerMode == PLAYER_MODE_BALL)
		gotout = true;
	else
	if (gPlayerObj->Skeleton->AnimNum != PLAYER_ANIM_WEBBED)		
		gotout = true;


			/* SEE IF SPHERE TIMES OUT */
	
	else
	if (theNode->Health <= 0.0f)
	{
		if (gPlayerMode == PLAYER_MODE_BUG)								
			MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_STAND, 3);
		gotout = true;
	}


			/* IF OUT, THEN BLOW UP SPHERE */
			
	if (gotout)
	{
		QD3D_ExplodeGeometry(theNode, 700.0f, SHARD_MODE_BOUNCE | SHARD_MODE_NULLSHADER, 1, .6);
		DeleteObject(theNode);
	}

			/*******************/
			/* UPDATE & WOBBLE */
			/*******************/
			
	theNode->Coord = gMyCoord;
	
	theNode->Scale.x = WEB_SPHERE_SCALE + sin(theNode->SpecialF[0] += fps*6.0f) * .1f;
	theNode->Scale.y = WEB_SPHERE_SCALE + cos(theNode->SpecialF[1] += fps*8.0f) * .1f;
	theNode->Scale.z = WEB_SPHERE_SCALE + sin(theNode->SpecialF[2] += fps*5.0f) * .1f;
	
	UpdateObjectTransforms(theNode);
}


#pragma mark -

/******************* BALL HIT SPIDER *********************/
//
// OUTPUT: true if enemy gets killed
//

Boolean BallHitSpider(ObjNode *me, ObjNode *enemy)
{
Boolean	killed = false;

				/************************/
				/* HURT & KNOCK ON BUTT */
				/************************/
				
	if (me->Speed > SPIDER_KNOCKDOWN_SPEED)
	{	
				/*****************/
				/* KNOCK ON BUTT */
				/*****************/
					
		killed = KnockSpiderOnButt(enemy, me->Delta.x * .8f, me->Delta.y * .8f + 250.0f, me->Delta.z * .8f, .6);

		PlayEffect_Parms3D(EFFECT_POUND, &gCoord, kMiddleC+2, 2.0);
	}
	
	return(killed);
}


/****************** KNOCK SPIDER ON BUTT ********************/
//
// INPUT: dx,y,z = delta to apply to fireant 
//

Boolean KnockSpiderOnButt(ObjNode *enemy, float dx, float dy, float dz, float damage)
{
	if (enemy->Skeleton->AnimNum == SPIDER_ANIM_FALLONBUTT)		// see if already in butt mode
		return(false);
		
		/* IF ON SPLINE, DETACH */
		
	DetachEnemyFromSpline(enemy, MoveSpider);


			/* GET IT MOVING */
		
	enemy->Delta.x = dx;
	enemy->Delta.y = dy;
	enemy->Delta.z = dz;

	MorphToSkeletonAnim(enemy->Skeleton, SPIDER_ANIM_FALLONBUTT, 9.0);
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


/****************** KILL FIRE SPIDER *********************/
//
// OUTPUT: true if ObjNode was deleted
//

Boolean KillSpider(ObjNode *theNode)
{
	
		/* IF ON SPLINE, DETACH */
		
	DetachEnemyFromSpline(theNode, MoveSpider);


			/* DEACTIVATE */
			
	theNode->TerrainItemPtr = nil;				// dont ever come back
	theNode->CType = CTYPE_MISC;
	
	if (theNode->Skeleton->AnimNum != SPIDER_ANIM_DIE)			
		SetSkeletonAnim(theNode->Skeleton, SPIDER_ANIM_DIE);	
	
	return(false);
}


/**************** START SPIDER JUMP *******************/

static void StartSpiderJump(ObjNode *theNode)
{
	MorphToSkeletonAnim(theNode->Skeleton, SPIDER_ANIM_JUMP, 5.0);
	gDelta.y = SPIDER_JUMPDY;
	gDelta.x = gMyCoord.x - gCoord.x;
	gDelta.z = gMyCoord.z - gCoord.z;
}




#pragma mark -

/************************ PRIME MOSQUITO SPIDER *************************/

Boolean PrimeEnemy_Spider(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj,*shadowObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_SPIDER,x,z, SPIDER_SCALE);
	if (newObj == nil)
		return(false);
		
	DetachObject(newObj);										// detach this object from the linked list
		
	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;
	
	SetSkeletonAnim(newObj->Skeleton, SPIDER_ANIM_WALK);
	

				/* SET BETTER INFO */
			
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		+= SPIDER_FOOT_OFFSET;			
	newObj->SplineMoveCall 	= MoveSpiderOnSpline;				// set move call
	newObj->Health 			= SPIDER_HEALTH;
	newObj->Damage 			= SPIDER_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_SPIDER;
	newObj->CType			|= CTYPE_KICKABLE|CTYPE_AUTOTARGET;	// these can be kicked
	newObj->CBits		= CBITS_NOTTOP;

	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 40,-SPIDER_FOOT_OFFSET,-90,90,90,-90);


				/* MAKE SHADOW */
				
	shadowObj = AttachShadowToObject(newObj, 8, 8, false);
	DetachObject(shadowObj);									// detach this object from the linked list


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	AddToSplineObjectList(newObj);

	return(true);
}

/******************** MOVE SPIDER ON SPLINE ***************************/
//
// NOTE: This is called for every Spider on the spline in the entire level.
//		The isVisible flag determines if this particular one is in visible range.
//		It actually means the enemy is on an active supertile.
//

static void MoveSpiderOnSpline(ObjNode *theNode)
{
Boolean isVisible; 

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, SPIDER_SPLINE_SPEED);
	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/
			
	if (isVisible)
	{
		theNode->Skeleton->AnimSpeed = .9;
		
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);		

		theNode->Coord.y = GetTerrainHeightAtCoord(theNode->Coord.x, theNode->Coord.z, FLOOR) + SPIDER_FOOT_OFFSET;	// calc y coord

		UpdateObjectTransforms(theNode);																// update transforms
		CalcObjectBoxFromNode(theNode);																	// update collision box
		UpdateShadow(theNode);																			// update shadow		
		
				/*********************************/
				/* SEE IF CLOSE ENOUGH TO ATTACK */
				/*********************************/
				
		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gMyCoord.x, gMyCoord.z) < SPIDER_ATTACK_DIST)
		{
					/* REMOVE FROM SPLINE */
					
			DetachEnemyFromSpline(theNode, MoveSpider);
		}		
	}
	
			/* NOT VISIBLE */
	else
	{
//		if (theNode->ShadowNode)									// hide shadow
//			theNode->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;	
	}
}







