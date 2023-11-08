/****************************/
/*    	WATERBUG.C	        */
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

static void MoveWaterBug(ObjNode *theNode);
static void DoWaterBugCollisionDetect(ObjNode *theNode);
static void SprayWater(ObjNode *bug);
static void PutCaptionOnBug(ObjNode *bug);
static void MoveCaption(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	WATERBUG_SCALE	1.4

#define	MAX_THRUST			2000.0f
#define	WATERBUG_MAX_SPEED	1200.0f

#define	WATER_BUG_FOOT_OFFSET	90.0f

#define WATERBUG_MAX_TURN_SPEED	(275.0f * PI / 180.0f)	// in radians per second


/**********************/
/*     VARIABLES      */
/**********************/

ObjNode *gCurrentWaterBug = nil;				
static int32_t	gWaterBugParticleGroup = -1;
static float	gWaterSprayRegulator = 0;

#define OriginalRot	SpecialF[0]
#define	IsPaidFor	Flag[0]


/********************** ADD WATERBUG *************************/
//
// parm[0] = initial aim (0..15 ccw)
//

Boolean AddWaterBug(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode		*newObj;
TQ3Point3D	where;
Boolean		isPaidFor;

	if (gLevelType != LEVEL_TYPE_POND)					// verify level
		DoFatalAlert("AddWaterBug: wrong level");
		
	isPaidFor = itemPtr->flags & ITEM_FLAGS_USER1;		// see if this guy has been paid for
		
	where.x = x;
	where.y = WATER_Y+WATER_BUG_FOOT_OFFSET;
	where.z = z;
	
			/**************************/
			/* CREATE SKELETON OBJECT */	
			/**************************/
	
	gNewObjectDefinition.type 		= SKELETON_TYPE_WATERBUG;
	gNewObjectDefinition.animNum 	= WATERBUG_ANIM_WAIT;
	gNewObjectDefinition.coord		= where;
	gNewObjectDefinition.slot 		= PLAYER_SLOT;
	gNewObjectDefinition.flags 		= STATUS_BIT_ROTXZY;
	gNewObjectDefinition.moveCall 	= MoveWaterBug;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/16.0);
	gNewObjectDefinition.scale 		= WATERBUG_SCALE;
	newObj 							= MakeNewSkeletonObject(&gNewObjectDefinition);	

	newObj->TerrainItemPtr = itemPtr;					// keep ptr to item list
	newObj->InitCoord = gNewObjectDefinition.coord;		// remember where started
	newObj->OriginalRot = gNewObjectDefinition.rot;		// remember initial rotation
	newObj->IsPaidFor 	= isPaidFor;					// remember if paid for
	
			/* SET TRIGGER STUFF */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_PLAYERTRIGGERONLY|CTYPE_AUTOTARGET|CTYPE_AUTOTARGETJUMP;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= SIDE_BITS_TOP;			// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_WATERBUG;

	newObj->Mode			= WATERBUG_MODE_WAITING;		// set waterbug mode

			/* SET COLLISION INFO */

	SetObjectCollisionBounds(newObj,45,-200,-50,50,50,-50);

	return(true);							// item was added
}


/******************* MOVE WATERBUG **********************/

static void MoveWaterBug(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	y;

	switch(theNode->Mode)
	{
		case	WATERBUG_MODE_WAITING:
				StopObjectStreamEffect(theNode);
				
						/* KEEP DOWN */
						
				gDelta.y -= DEFAULT_GRAVITY * fps;
				gCoord.y += gDelta.y * fps;
				y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR);
				if ((gCoord.y-WATER_BUG_FOOT_OFFSET) < y)
				{
					gCoord.y += y - (gCoord.y-WATER_BUG_FOOT_OFFSET);
					gDelta.y = 0;
				}
				if ((gCoord.y-WATER_BUG_FOOT_OFFSET) < WATER_Y)
				{
					gCoord.y = WATER_Y + WATER_BUG_FOOT_OFFSET;
					gDelta.y = 0;
				}
				break;
				
		case	WATERBUG_MODE_COAST:
			
				StopObjectStreamEffect(theNode);

					/* SEE IF GONE */
					
				if (TrackTerrainItem(theNode))
				{
					DeleteObject(theNode);
					return;
				}				

					
				GetObjectInfo(theNode);


					/* IF NOT VISIBLE, THEN RESET TO ORIGINAL COORDS */
					//
					// This is my cheezy way of getting the bug back to where he should be.
					//
					
				if (theNode->StatusBits & STATUS_BIT_ISCULLED)
				{
					gCoord = theNode->InitCoord;
					theNode->Rot.y = theNode->OriginalRot;
					gDelta.x = gDelta.y = gDelta.z = 0;
					theNode->Mode = WATERBUG_MODE_WAITING;
					UpdateObject(theNode);
					return;
				}
				
						/*************/
						/* DO MOTION */
						/*************/
						
				ApplyFrictionToDeltas(200.0f*fps,&gDelta);			// do friction				
				gCoord.x += gDelta.x * fps;							// move it
				gCoord.z += gDelta.z * fps;								
				
						/* SEE IF GROUNDED */
						
				y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR);				// get ground y here
				if ((gCoord.y-WATER_BUG_FOOT_OFFSET) < y)
				{
					gCoord.x = theNode->OldCoord.x;
					gCoord.z = theNode->OldCoord.z;				
//					gCoord.y = y+WATER_BUG_FOOT_OFFSET;
//					theNode->Mode = WATERBUG_MODE_GROUNDED;
//					ApplyFrictionToDeltas(2000.0f*fps,&gDelta);						// apply extreme friction			
				}
				
				
						/******************/
						/* ROTATE TO FLAT */
						/******************/
						
				if (theNode->Rot.x < 0.0f)
				{
					theNode->Rot.x += fps*3.0f;
					if (theNode->Rot.x > 0.0f)
						theNode->Rot.x = 0;				
				}
				else
				if (theNode->Rot.x > 0.0f)
				{
					theNode->Rot.x -= fps*3.0f;
					if (theNode->Rot.x < 0.0f)
						theNode->Rot.x = 0;				
				}

				if (theNode->Rot.z < 0.0f)
				{
					theNode->Rot.z += fps*3.0f;
					if (theNode->Rot.z > 0.0f)
						theNode->Rot.z = 0;				
				}
				else
				if (theNode->Rot.z > 0.0f)
				{
					theNode->Rot.z -= fps*3.0f;
					if (theNode->Rot.z < 0.0f)
						theNode->Rot.z = 0;				
				}
				
				DoWaterBugCollisionDetect(theNode);
				UpdateObject(theNode);				
				break;
				
		case	WATERBUG_MODE_RIDE:
				break;
	}
}


/************** DO TRIGGER - WATERBUG ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_WaterBug(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
	(void) sideBits;
	
			/* IF PLAYER IS BALL, THEN DO NOTHING */
			
	if (gPlayerMode == PLAYER_MODE_BALL)
		return(true);

				/* SEE IF CAN AFFORD TO RIDE */

	if (!theNode->IsPaidFor)
	{
		if (!DoWeHaveEnoughMoney())
		{
			PutCaptionOnBug(theNode);
			return(true);		
		}
		UseMoney();
		theNode->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;		// set item list flag so we know its paid for permanently
		theNode->IsPaidFor = true;
	}

		
			/* SET PLAYER ANIMATION TO RIDE THIS GUY */
			
	MorphToSkeletonAnim(whoNode->Skeleton, PLAYER_ANIM_RIDEWATERBUG, 7);

	gCurrentWaterBug = theNode;								// remember who we're riding
	gWaterSprayRegulator = 0;
	gWaterBugParticleGroup = -1;							// make a new particle group

	theNode->Mode = WATERBUG_MODE_RIDE;						// set mode to riding		
		
	theNode->CType &= ~(CTYPE_AUTOTARGET|CTYPE_AUTOTARGETJUMP);			// dont auto target anymore				
		
		
	return(true);
}


/******************* DO WATERBUG COLLISION DETECT *******************/

static void DoWaterBugCollisionDetect(ObjNode *theNode)
{
	HandleCollisions(theNode, CTYPE_MISC);
	DoFenceCollision(theNode,1);
}

#pragma mark -


/****************** DRIVE WATER BUG **********************/
//
// This function is called by a Player Bug function.
//

void DriveWaterBug(ObjNode *bug, ObjNode *player)
{
float	fps = gFramesPerSecondFrac;
float	ax,az,rot,y;
float	mouseDX,oldSpeed,mx,my;
short	anim;

	GetObjectInfo(bug);									// this will override object info for player

	bug->Skeleton->AnimSpeed = 2.0;

		/* DO ROTATION WITH MOUSE DX */
			
	GetMouseDelta(&mx, &my);								// remember, this is premultiplied by fractional fps
	if (gPlayerUsingKeyControl)
	{
		mouseDX = mx * .003f;
	}
	else
	{
		mouseDX = mx * .004f;
	}

	float maxTurnSpeed = WATERBUG_MAX_TURN_SPEED * fps;			// clamp turn speed to avoid mouse boom
	mouseDX = ClampFloat(mouseDX, -maxTurnSpeed, maxTurnSpeed);

	bug->Rot.y -= mouseDX;
	
			/*********************/
			/* CALC ACCEL VECTOR */
			/*********************/
			
	rot = bug->Rot.y;
	ax = -sin(rot) * MAX_THRUST;
	az = -cos(rot) * MAX_THRUST;

	gDelta.x += ax * fps;
	gDelta.z += az * fps;
	gDelta.y = 0;
	
	
				/* CALC SPEED */
				
	oldSpeed = bug->Speed;
	bug->Speed = FastVectorLength2D(gDelta.x, gDelta.z);			// calc 2D speed value

	if (bug->Speed > WATERBUG_MAX_SPEED)							// check max/min speeds
	{
		float	scale;
		
		scale = WATERBUG_MAX_SPEED/bug->Speed;						// get 100-% over
		gDelta.x *= scale;											// adjust back to max (note, we dont mess with y since gravity and jump need different max's)
		gDelta.z *= scale;
		bug->Speed = WATERBUG_MAX_SPEED;
	}
	
			/*****************/
			/* MOVE WATERBUG */
			/*****************/
			
	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;


			/*************/
			/* CALC TILT */
			/*************/
			
	if (bug->Speed > 0.0f)
	{			
		TQ3Vector2D	v1,v2;
		
		FastNormalizeVector2D(gDelta.x, gDelta.z, &v1);		// normalize my motion vector
		FastNormalizeVector2D(ax, az, &v2);					// normalize my thrust vector
		
		rot = Q3Vector2D_Cross(&v1,&v2);					// calc cross product
		bug->Rot.z = rot * -.8f;	
	}
	else
		bug->Rot.z = 0;												// no tilt since not moving

		/* SET ANIM BASED ON TILT */
		
	if (bug->Rot.z > .15f)
		anim = WATERBUG_ANIM_LEFT;
	else
	if (bug->Rot.z < -.15f)
		anim = WATERBUG_ANIM_RIGHT;
	else
		anim = WATERBUG_ANIM_CENTER;

	if (anim != bug->Skeleton->AnimNum)
		MorphToSkeletonAnim(bug->Skeleton, anim, 5);
		

			/* DO NOSE TILT */
			
	rot = (bug->Speed - oldSpeed) * .02f;
	if (rot < 0.0f)
		rot = 0;
	if (bug->Rot.x < rot)
	{
		bug->Rot.x += 1.5f * fps;
		if (bug->Rot.x > rot)
			bug->Rot.x = rot;
	}
	else
	{
		bug->Rot.x -= 1.5f * fps;
		if (bug->Rot.x < rot)
			bug->Rot.x = rot;
	}
	
	
		/*******************/
		/* SEE IF GROUNDED */
		/*******************/

	y = GetTerrainHeightAtCoord(gCoord.x, gCoord.z, FLOOR);				// get ground y here
	if ((gCoord.y-WATER_BUG_FOOT_OFFSET) < y)
	{
		gCoord.x = bug->OldCoord.x;
		gCoord.z = bug->OldCoord.z;				

//		gCoord.y = y+WATER_BUG_FOOT_OFFSET;
//		bug->Mode = WATERBUG_MODE_GROUNDED;		
//		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_JUMP, 9);		// throw player off
//		player->Delta.y = 1200.0f;
		
//		MorphToSkeletonAnim(bug->Skeleton, WATERBUG_ANIM_OUTOFSERVICE, 5);
//		UpdateObject(bug);
//		return;
	}

		/***********************/
		/* DO COLLISION DETECT */
		/***********************/
		
	DoWaterBugCollisionDetect(bug);
	
		

		/********************/
		/* MAKE WATER SPRAY */
		/********************/

	SprayWater(bug);


			/****************************/
			/* UPDATE BUG & PLAYER INFO */ 
			/****************************/
			//
			// note: player's coord is calculated upon return to MovePlayerBug_RideWaterBug.
			//
			
	UpdateObject(bug);
	player->Delta = gDelta;
	player->Rot.y = bug->Rot.y;					// keep player rot updated even tho dont need it right now
	
	
		/* START/UPDATE ENGINE SOUND */

	if (bug->EffectChannel == -1)
		bug->EffectChannel = PlayEffect3D(EFFECT_BOATENGINE, &gCoord);
	else
		Update3DSoundChannel(EFFECT_BOATENGINE, &bug->EffectChannel, &gCoord);
}



/***************** SPRAY WATER *************************/

static void SprayWater(ObjNode *bug)
{			
float	fps = gFramesPerSecondFrac;

	if (!VerifyParticleGroup(gWaterBugParticleGroup))
	{
				/* START WATER PARTICLE GROUP */
new_pgroup:		
		gWaterBugParticleGroup = NewParticleGroup(
												PARTICLE_TYPE_FALLINGSPARKS,// type
												0,							// flags
												1400,						// gravity
												0,							// magnetism
												45,							// base scale
												-1.7,						// decay rate
												1.0,						// fade rate
												PARTICLE_TEXTURE_PATCHY);	// texture
	}
	
				/* ADD PARTICLE TO GROUP */
				
	if (gWaterBugParticleGroup != -1)
	{
		gWaterSprayRegulator += fps;				// see if time to spew water
		if (gWaterSprayRegulator > 0.02f)
		{		
			TQ3Point3D	pt,buttCoord;
			TQ3Vector3D delta;
			static const TQ3Point3D buttOff = {0,0,70};
			
			gWaterSprayRegulator = 0;
	
			FindCoordOnJoint(bug, 0, &buttOff, &buttCoord);				// get coord of butt
				
			FastNormalizeVector(gDelta.x, 0, gDelta.z, &delta);			// calc spray delta shooting out of butt
			delta.x *= -300.0f;
			delta.z *= -300.0f;

			delta.x += (RandomFloat()-.5f) * 50.0f;						// spray delta
			delta.z += (RandomFloat()-.5f) * 50.0f;
			delta.y = 500.0f + RandomFloat() * 100.0f;
	
			pt.x = buttCoord.x + (RandomFloat()-.5f) * 80.0f;			// random noise to coord
			pt.y = WATER_Y+50.0f;
			pt.z = buttCoord.z + (RandomFloat()-.5f) * 80.0f;
			
	
			if (AddParticleToGroup(gWaterBugParticleGroup, &pt, &delta, RandomFloat() + 2.0f, FULL_ALPHA))
				goto new_pgroup;
		}
	}		
}


/******************** PUT CAPTION ON BUG ***********************/

static void PutCaptionOnBug(ObjNode *bug)
{
ObjNode	*newObj;
static const TQ3Point3D headOff = {0,60,-40};

	if (bug->ChainNode)
	{
		bug->ChainNode->SpecialF[0] = 2;			// reset the timer
		return;
	}
	
		/* CALC COORD */
		
	FindCoordOnJoint(bug, 5, &headOff, &gNewObjectDefinition.coord);
	

			/* MAKE OBJ */
			
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= POND_MObjType_Caption;	
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= bug->Slot+1;
	gNewObjectDefinition.moveCall 	= MoveCaption;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;

	bug->ChainNode = newObj;
	newObj->ChainHead = bug;

	newObj->SpecialF[0] = 2;
}


/******************* MOVE CAPTION ******************/

static void MoveCaption(ObjNode *theNode)
{
static const TQ3Vector3D up = {0,1,0};

	theNode->SpecialF[0] -= gFramesPerSecondFrac;
	if (theNode->SpecialF[0] <= 0.0f)
	{
		theNode->ChainHead->ChainNode = nil;
		theNode->ChainHead = nil;
		DeleteObject(theNode);
		return;
	}

	SetLookAtMatrixAndTranslate(&theNode->BaseTransformMatrix, &up, &theNode->Coord, &gGameViewInfoPtr->currentCameraCoords);
}

