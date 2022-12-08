/****************************/
/*    	DRAGONFLY.C	        */
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/*******************/
/*   PROTOTYPES    */
/*******************/

static void MoveDragonFly(ObjNode *theNode);
static void DoDragonFlyCollisionDetect(ObjNode *theNode);
static void DragonFlyShootFireball(ObjNode *bug);
static void MoveFireball(ObjNode *theNode);
static void UpdateDragonFly(ObjNode *theNode, Boolean useSound);
static void ExplodeFireball(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	DRAGONFLY_SCALE		2.0f

#define	MAX_THRUST			3000.0f
#define	DRAGONFLY_MAX_SPEED	900.0f
#define DRAGONFLY_MAX_TURN_SPEED	(165.0f * PI / 180.0f)	// in radians per second

#define	MAX_TILT				.6f

#define	DRAGONFLY_BOTTOM_OFF	80.0f
#define	DRAGONFLY_TOP_OFF		70.0f

enum
{
	DRAGONFLY_ANIM_FLY,
	DRAGONFLY_ANIM_WAIT
};

enum
{
	DRAGONFLY_MODE_NONE,
	DRAGONFLY_MODE_LAND
};




/**********************/
/*     VARIABLES      */
/**********************/

ObjNode *gCurrentDragonFly = nil;				


#define SparkTimer	SpecialF[0]
#define	PGroupA		SpecialL[0]
#define	PGroupB		SpecialL[1]
#define	PGroupAMagic	SpecialL[2]
#define	PGroupBMagic	SpecialL[3]

/********************** ADD DRAGONFLY *************************/
//
// parm[0] = initial aim (0..15 clockwise)
//

Boolean AddDragonFly(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode		*newObj;
TQ3Point3D	where;

	if (gLevelType != LEVEL_TYPE_FOREST)					// verify level
		DoFatalAlert("AddDragonFly: wrong level");
		
	where.x = x;
	where.y = GetTerrainHeightAtCoord(x, z, FLOOR)+DRAGONFLY_BOTTOM_OFF;
	where.z = z;
	
			/**************************/
			/* CREATE SKELETON OBJECT */	
			/**************************/
	
	gNewObjectDefinition.type 		= SKELETON_TYPE_DRAGONFLY;
	gNewObjectDefinition.animNum 	= DRAGONFLY_ANIM_WAIT;
	gNewObjectDefinition.coord		= where;
	gNewObjectDefinition.slot 		= PLAYER_SLOT;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= MoveDragonFly;
	gNewObjectDefinition.rot 		= itemPtr->parm[0] * (PI2/16);
	gNewObjectDefinition.scale 		= DRAGONFLY_SCALE;
	newObj 							= MakeNewSkeletonObject(&gNewObjectDefinition);	

	newObj->TerrainItemPtr = itemPtr;					// keep ptr to item list
	newObj->InitCoord = gNewObjectDefinition.coord;		// remember where started
	newObj->Mode = DRAGONFLY_MODE_NONE;
	
			/* SET TRIGGER STUFF */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_PLAYERTRIGGERONLY|CTYPE_AUTOTARGET|CTYPE_AUTOTARGETJUMP;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= SIDE_BITS_TOP;			// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_DRAGONFLY;


			/* SET COLLISION INFO */
			
//	SetObjectCollisionBounds(newObj,DRAGONFLY_TOP_OFF,-40,-80,80,80,-80);
	SetObjectCollisionBounds(newObj,DRAGONFLY_TOP_OFF,-40,-140,140,140,-140);


				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, 8, 15, false);

	return(true);							// item was added
}


/******************* MOVE DRAGONFLY **********************/

static void MoveDragonFly(ObjNode *theNode)
{
float	y;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}
	
		/* IF THIS IS BEING CALLED THEN THERE SHOULDNT BE ANY SOUND */
	
	StopObjectStreamEffect(theNode);

	GetObjectInfo(theNode);
	
				/***********************/
				/* SEE IF LANDING MODE */
				/***********************/
				
	if (theNode->Mode == DRAGONFLY_MODE_LAND)
	{
		gDelta.y -= 1400.0f * gFramesPerSecondFrac;
		ApplyFrictionToDeltas(20, &gDelta);
		gCoord.x += gDelta.x * gFramesPerSecondFrac;
		gCoord.y += gDelta.y * gFramesPerSecondFrac;
		gCoord.z += gDelta.z * gFramesPerSecondFrac;
	
		y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR);			// see if hit ground
		if ((gCoord.y - DRAGONFLY_BOTTOM_OFF) <= y)
		{
			gCoord.y = y + DRAGONFLY_BOTTOM_OFF;
			gDelta.y = 0;
			theNode->Mode = DRAGONFLY_MODE_NONE;
			theNode->CType |= CTYPE_AUTOTARGET|CTYPE_AUTOTARGETJUMP;	// now it auto bops again
			MorphToSkeletonAnim(theNode->Skeleton, DRAGONFLY_ANIM_WAIT, 3);
		}	
		UpdateDragonFly(theNode, false);
		return;
	}

	UpdateObject(theNode);
}


/************** DO TRIGGER - DRAGONFLY ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_DragonFly(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
	(void) sideBits;

			/* IF PLAYER IS BALL, THEN DO NOTHING */
			
	if (gPlayerMode == PLAYER_MODE_BALL)
		return(true);

	theNode->MoveCall = nil;								// disable normal move function
	theNode->CType = 0;										// no collision while riding

			/* SET PLAYER ANIMATION TO RIDE THIS GUY */
			
	MorphToSkeletonAnim(whoNode->Skeleton, PLAYER_ANIM_RIDEDRAGONFLY, 8);

	MorphToSkeletonAnim(theNode->Skeleton, DRAGONFLY_ANIM_FLY, 5);		// dragonfly is flying
		
	gCurrentDragonFly = theNode;								// remember who we're riding
			
		
	return(true);
}


/******************* DO DRAGONFLY COLLISION DETECT *******************/

static void DoDragonFlyCollisionDetect(ObjNode *theNode)
{
float	y;

	HandleCollisions(theNode, CTYPE_MISC);
	DoFenceCollision(theNode, .3);

			/* SEE IF HIT GROUND */
			
	y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR);	
	if ((gCoord.y - DRAGONFLY_BOTTOM_OFF) <= y)
	{
		gCoord.y = y + DRAGONFLY_BOTTOM_OFF;
		gDelta.y = 0;
	}
}

#pragma mark-

/************************ PLAYER OFF DRAGONFLY ****************************/

void PlayerOffDragonfly(void)
{
	gCurrentDragonFly->MoveCall = MoveDragonFly;		// reset the move call
	gCurrentDragonFly->Mode = DRAGONFLY_MODE_LAND;
	gCurrentDragonFly->Rot.x =
	gCurrentDragonFly->Rot.z = 0;
	gCurrentDragonFly->CType = CTYPE_TRIGGER|CTYPE_PLAYERTRIGGERONLY;
	gCurrentDragonFly = nil;							// not on this anymore
}


#pragma mark -


/****************** DRIVE DRAGONFLY **********************/
//
// This function is called by a Player Bug function.
//

void DriveDragonFly(ObjNode *bug, ObjNode *player)
{
float	fps = gFramesPerSecondFrac;
float	rot;
float	mouseDX,mouseDY;
float	mx,my;

static const TQ3Vector3D unit = {0,0,-MAX_THRUST};
TQ3Vector3D		forward;
TQ3Matrix4x4	m;

	(void) player;

	GetObjectInfo(bug);									// this will override object info for player


		/* DO ROTATION WITH MOUSE DX/DY */
			
	GetMouseDelta(&mx, &my);							// remember, this is premultiplied by fractional fps
	if (gPlayerUsingKeyControl)
	{
		mouseDX = mx * .0018f;
		mouseDY = my * -.0018f;
	}
	else
	{
		mouseDX = mx * .003f;
		mouseDY = my * -.003f;
	}

	float maxTurnSpeed = DRAGONFLY_MAX_TURN_SPEED * fps;		// clamp turn speed to prevent mouse boom
	mouseDX = ClampFloat(mouseDX, -maxTurnSpeed, maxTurnSpeed);
	mouseDY = ClampFloat(mouseDY, -maxTurnSpeed, maxTurnSpeed);

	switch (gGamePrefs.dragonflyControl)
	{
		case kDragonflySteering_Normal:
			break;
		case kDragonflySteering_InvertX:
			mouseDX *= -1;
			break;
		case kDragonflySteering_InvertY:
			mouseDY *= -1;
			break;
		case kDragonflySteering_InvertXY:
			mouseDX *= -1;
			mouseDY *= -1;
			break;
	}

	bug->Rot.y -= mouseDX;
	bug->Rot.x -= mouseDY;
	
	if (bug->Rot.x > MAX_TILT)				// limit tilt
		bug->Rot.x = MAX_TILT;
	else
	if (bug->Rot.x < -MAX_TILT)				
		bug->Rot.x = -MAX_TILT;
		
				
			/*********************/
			/* CALC ACCEL VECTOR */
			/*********************/
			
	Q3Matrix4x4_SetRotate_XYZ(&m, bug->Rot.x, bug->Rot.y, bug->Rot.z);
	Q3Vector3D_Transform(&unit, &m, &forward);

	gDelta.x += forward.x * fps;
	gDelta.y += forward.y * fps;
	gDelta.z += forward.z * fps;
	
	
				/* CALC SPEED */
				
//	oldSpeed = bug->Speed;
	bug->Speed = FastVectorLength3D(gDelta.x, gDelta.y, gDelta.z);	// calc 3D speed value

	if (bug->Speed > DRAGONFLY_MAX_SPEED)							// check max/min speeds
	{
		float	scale;
		
		scale = DRAGONFLY_MAX_SPEED/bug->Speed;						// get 100-% over
		gDelta.x *= scale;											// adjust back to max (note, we dont mess with y since gravity and jump need different max's)
		gDelta.y *= scale;
		gDelta.z *= scale;
		bug->Speed = DRAGONFLY_MAX_SPEED;
	}
	
			/*****************/
			/* MOVE DRAGONFLY */
			/*****************/
			
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* SEE IF MAKE BAT ATTACK */
			
	if (gRealLevel == LEVEL_NUM_BEACH)	
	{
		if (gCoord.y > MAX_DRAGONFLY_FLIGHT_HEIGHT)				// see if reached max height
			MakeBat(gCoord.x, gCoord.y + 100.0f, gCoord.z);		// create a bat to nab me
	}
	else
	if (gRealLevel == LEVEL_NUM_FLIGHT)	
	{
		if ((gCoord.y-GetTerrainHeightAtCoord(gCoord.x,gCoord.z,FLOOR)) > MAX_DRAGONFLY_FLIGHT_HEIGHT2)						// see if reached max height
			MakeBat(gCoord.x, gCoord.y + 100.0f, gCoord.z);		// create a bat to nab me
	}
	

			/*************/
			/* CALC TILT */
			/*************/
			
	if (bug->Speed > 0.0f)
	{			
		TQ3Vector2D	v1,v2;
		
		FastNormalizeVector2D(gDelta.x, gDelta.z, &v1);		// normalize my motion vector
		FastNormalizeVector2D(forward.x, forward.z, &v2);	// normalize my thrust vector
		
		rot = Q3Vector2D_Cross(&v1,&v2);					// calc cross product
		bug->Rot.z = rot * -.6f;	
	}
	else
		bug->Rot.z = 0;										// no tilt since not moving


		/***********************/
		/* DO COLLISION DETECT */
		/***********************/
		
	DoDragonFlyCollisionDetect(bug);



		/**********************/
		/* SEE IF BREATH FIRE */
		/**********************/

	if (GetNewKeyState(kKey_Kick))
		DragonFlyShootFireball(bug);
	

			/**********/
			/* UPDATE */ 
			/**********/
			
	UpdateDragonFly(bug, true);	
}


/******************* UPDATE DRAGONFLY *********************/

static void UpdateDragonFly(ObjNode *theNode, Boolean useSound)
{	
	UpdateObject(theNode);
	
		/* UPDATE SOUND */
		
	if (theNode->EffectChannel == -1)
	{
		if (useSound)
			theNode->EffectChannel = PlayEffect3D(EFFECT_HELICOPTER, &gCoord);
	}
	else
		Update3DSoundChannel(EFFECT_HELICOPTER, &theNode->EffectChannel, &gCoord);
}


/**************** DRAGONFLY SHOOT FIREBALL ********************/

static void DragonFlyShootFireball(ObjNode *bug)
{
ObjNode			*newObj;
static const 	TQ3Point3D off = {0,0, -80};							// offset to top of head
TQ3Vector3D		delta;


			/* CALC COORD OF MOUTH */
			
	FindCoordOnJoint(bug, 0, &off, &gNewObjectDefinition.coord);		// get coord of head

			/******************/
			/* MAKE NEW EVENT */
			/******************/
			
	gNewObjectDefinition.genre 		= EVENT_GENRE;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 500;
	gNewObjectDefinition.moveCall 	= MoveFireball;
	newObj = MakeNewObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;
		
			/* SET COLLISION INFO */
			
	newObj->CType 			= CTYPE_HURTENEMY;
	newObj->CBits			= CBITS_TOUCHABLE;
	SetObjectCollisionBounds(newObj,70,-70,-70,70,70,-70);
		
	newObj->Health = 1.8;				
	newObj->Damage = 2.0;					// causes massive damage
	
	newObj->SparkTimer = 0;					// spark generator timer
	newObj->PGroupA = 						// no particle groups yet
	newObj->PGroupB = -1;
	
			/* CALC DELTA VECTOR */
			
	FastNormalizeVector(gDelta.x, gDelta.y, gDelta.z, &delta);			// get normalized aim vector
	delta.x *= DRAGONFLY_MAX_SPEED * 4.3f;
	delta.y *= DRAGONFLY_MAX_SPEED * 4.3f;
	delta.z *= DRAGONFLY_MAX_SPEED * 4.3f;
	newObj->Delta = delta;
	
	
			/* PLAY SOUND */
			
	PlayEffect(EFFECT_PLASMABURST);
}


/********************* MOVE FIREBALL **********************/

static void MoveFireball(ObjNode *theNode)
{
int	i;
TQ3Vector3D	delta;
float	fps = gFramesPerSecondFrac;

			/* SEE IF BURNED OUT */
			
	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}


			/****************************/
			/* MOVE THE FIREBALL OBJECT */
			/****************************/

	GetObjectInfo(theNode);
	
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;
	
			/* SEE IF HIT ANYTHING */
		
	if (gCoord.y <= GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR))
	{
		ExplodeFireball(theNode);
		return;
	}
	
	if (DoSimpleBoxCollision(gCoord.y+20,gCoord.y-20,gCoord.x-20,gCoord.x+20,gCoord.z+20,gCoord.z-20, CTYPE_MISC))
	{
		ExplodeFireball(theNode);

			/* SEE IF HIT HIVE */
	
		if ((gCollisionList[0].objectPtr->Group == MODEL_GROUP_LEVELSPECIFIC) &&
			(gCollisionList[0].objectPtr->Type == FOREST_MObjType_Hive))
		{
			RattleHive(gCollisionList[0].objectPtr);
		}
		
		return;
	}

	
	UpdateObject(theNode);


			/*********************/
			/* LEAVE SPARK TRAIL */
			/*********************/

	theNode->SparkTimer -= fps;
	if (theNode->SparkTimer <= 0.0f)
	{
		theNode->SparkTimer = .02;


				/*********************/
				/* DO FIRE PARTICLES */
				/*********************/
	
				/* SEE IF MAKE NEW GROUP */
				
		if ((theNode->PGroupA == -1) || (!VerifyParticleGroupMagicNum(theNode->PGroupA, theNode->PGroupAMagic)))
		{
			theNode->PGroupAMagic = MyRandomLong();							// generate a random magic num
			
			theNode->PGroupA = NewParticleGroup(theNode->PGroupAMagic,		// magic num
												PARTICLE_TYPE_FALLINGSPARKS,// type
												PARTICLE_FLAGS_HOT,			// flags
												0,							// gravity
												0,							// magnetism
												20,							// base scale
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
				
		if ((theNode->PGroupB == -1) || (!VerifyParticleGroupMagicNum(theNode->PGroupB, theNode->PGroupBMagic)))
		{
			theNode->PGroupBMagic = MyRandomLong();									// generate a random magic num
			
			theNode->PGroupB = NewParticleGroup(theNode->PGroupBMagic,		// magic num
												PARTICLE_TYPE_FALLINGSPARKS,// type
												PARTICLE_FLAGS_HOT,			// flags
												900,						// gravity
												0,							// magnetism
												15,							// base scale
												1.6,						// decay rate
												0,							// fade rate
												PARTICLE_TEXTURE_ORANGESPOT);	// texture
		}
	
	
				/* ADD PARTICLES TO SPARK */
				
		if (theNode->PGroupB != -1)
		{
			for (i = 0; i < 3; i++)
			{
				delta.x = (RandomFloat()-.5f) * 900.0f;
				delta.y = (RandomFloat()-.5f) * 900.0f;
				delta.z = (RandomFloat()-.5f) * 900.0f;
				
				AddParticleToGroup(theNode->PGroupB, &gCoord, &delta, RandomFloat()*2.0f + 2.0f, FULL_ALPHA);
			}
		}			
	}
}


/****************** EXPLODE FIREBALL ***********************/

static void ExplodeFireball(ObjNode *theNode)
{
long			pg,i;
TQ3Vector3D		delta;

	
	PlayEffect_Parms3D(EFFECT_PLASMAEXPLODE, &theNode->Coord, kMiddleC-4, 6.0);



			/*******************/
			/* SPARK EXPLOSION */
			/*******************/

			/* white sparks */
				
	pg = NewParticleGroup(	0,							// magic num
							PARTICLE_TYPE_FALLINGSPARKS,	// type
							PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_HOT,		// flags
							400,						// gravity
							0,							// magnetism
							40,							// base scale
							0,							// decay rate
							.7,							// fade rate
							PARTICLE_TEXTURE_BLUEFIRE);		// texture
	
	for (i = 0; i < 60; i++)
	{
		delta.x = (RandomFloat()-.5f) * 1400.0f;
		delta.y = (RandomFloat()-.5f) * 1400.0f;
		delta.z = (RandomFloat()-.5f) * 1400.0f;
		AddParticleToGroup(pg, &theNode->Coord, &delta, RandomFloat() + 1.0f, FULL_ALPHA);
	}

	DeleteObject(theNode);
}








