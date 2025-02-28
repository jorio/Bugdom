/****************************/
/*    	TRIGGERS2	        */
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

static void MoveCheckpoint(ObjNode *straw);
static void MoveKingWaterPipe(ObjNode *theNode);
static void MoveLadyBugBonus(ObjNode *box);
static void MoveLadyBug(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	CHECKPOINT_SCALE	1.5f

#define	LOG_SCALE			6.0f

#define	LADYBUG_SCALE		.9f
#define	LADYBUG_CAGE_SCALE	.30f
#define LADYBUG_MAX_ALTITUDE_BEFORE_CULLING	3000.0f
#define LADYBUG_MAX_ALTITUDE_FOR_SHADOW		1250.0f



/**********************/
/*     VARIABLES      */
/**********************/

float	gCheckPointRot;

#define	DropletScaleXI	SpecialF[0]
#define	DropletScaleYI	SpecialF[1]
#define	DropletScaleZI	SpecialF[2]
#define	CheckPointNum	SpecialL[0]
#define	PlayerRot		SpecialF[3]


#define	PipeID			SpecialL[0]
#define	SpewWaterRegulator	SpecialF[0]
#define	WaterTimer		SpecialF[1]
#define	RefillTimer		SpecialF[2]
#define	SpewWater		Flag[0]


/************************* ADD CHECKPOINT *********************************/
//
// parm[0] = checkpoint num
// parm[1] = player rot 0..3
//

Boolean AddCheckpoint(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*straw, *droplet;
int		checkpointNum = itemPtr->parm[0];

			/****************/
			/* CREATE STRAW */
			/****************/

	gNewObjectDefinition.group 		= GLOBAL1_MGroupNum_Straw;	
	gNewObjectDefinition.type 		= GLOBAL1_MObjType_Straw;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT-1;
	gNewObjectDefinition.moveCall 	= MoveCheckpoint;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= CHECKPOINT_SCALE;
	straw = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (straw == nil)
		return(false);

	straw->TerrainItemPtr = itemPtr;			// keep ptr to item list
	straw->CType = CTYPE_MISC|CTYPE_BLOCKCAMERA;
	straw->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(straw,300,0,-20,20,20,-20);
		
		
			/******************/
			/* CREATE DROPLET */
			/******************/
			//
			// only add this if the checkpoint is newer than or equal to most recent checkpoint.
			//

	if (checkpointNum > gBestCheckPoint)	
	{		
		gNewObjectDefinition.type 		= GLOBAL1_MObjType_Droplet;	
		gNewObjectDefinition.coord.x 	+= 192.0f * CHECKPOINT_SCALE;
		gNewObjectDefinition.coord.y 	+= 224.0f * CHECKPOINT_SCALE;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOZWRITE;
		gNewObjectDefinition.slot++;
		gNewObjectDefinition.moveCall 	= nil;
		droplet = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		if (droplet == nil)
			return(false);
			
		MakeObjectTransparent(droplet, .6);						// make xparent
			
		droplet->DropletScaleXI = RandomFloat();
		droplet->DropletScaleYI = RandomFloat();
		droplet->DropletScaleZI = RandomFloat();
		droplet->CheckPointNum	= itemPtr->parm[0];				// save checkpoint #
	
	
				/* SET TRIGGER STUFF */
	
		droplet->CType 			= CTYPE_TRIGGER|CTYPE_PLAYERTRIGGERONLY|CTYPE_AUTOTARGET|CTYPE_AUTOTARGETJUMP;
		droplet->CBits 			= CBITS_ALLSOLID;
		droplet->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
		droplet->Kind 			= TRIGTYPE_CHECKPOINT;
	
		droplet->PlayerRot 		= (float)itemPtr->parm[1] * (PI2/4);
	
		SetObjectCollisionBounds(droplet,0,-73*CHECKPOINT_SCALE,
								-21*CHECKPOINT_SCALE,21*CHECKPOINT_SCALE,
								21*CHECKPOINT_SCALE,-21*CHECKPOINT_SCALE);
	
		straw->ChainNode = droplet;								// link drop to straw
		droplet->ChainHead = straw;
	}
	return(true);											// item was added
}


/************************** MOVE CHECKPOINT ******************************/

static void MoveCheckpoint(ObjNode *straw)
{
ObjNode	*droplet;

	if (TrackTerrainItem(straw))		// just check to see if it's gone
	{
		DeleteObject(straw);
		return;
	}
	
			/* UPDATE DROPLET */
			
	droplet = straw->ChainNode;										// this might be gone, so check
	if (droplet)
	{		
		droplet->DropletScaleXI += gFramesPerSecondFrac * 8.0f;
		droplet->DropletScaleYI += gFramesPerSecondFrac * 9.0f;
		droplet->DropletScaleZI += gFramesPerSecondFrac * 7.0f;
	
		droplet->Scale.x = CHECKPOINT_SCALE + (sin(droplet->DropletScaleXI) * .4f);
		droplet->Scale.y = CHECKPOINT_SCALE + (sin(droplet->DropletScaleYI) * .4f);
		droplet->Scale.z = CHECKPOINT_SCALE + (sin(droplet->DropletScaleZI) * .4f);
		
		UpdateObjectTransforms(droplet);
	}
	
}


/************** DO TRIGGER - CHECKPOINT ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_Checkpoint(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
short			num;
TQ3Vector3D		delta;

	(void) whoNode;
	(void) sideBits;

				/* TAG THE CHECKPOINT */

	num = theNode->CheckPointNum;						// get checkpt #
	if (num > gBestCheckPoint)							// see if this is the best one so far
	{
		gBestCheckPoint = num;
		gMostRecentCheckPointCoord = theNode->Coord;	// remember where this checkpoint is
		gCheckPointRot = theNode->PlayerRot;			// see what rot to restore player to				
	}		
	
			/******************/
			/* POP THE BUBBLE */
			/******************/

	int32_t pg = NewParticleGroup(
							PARTICLE_TYPE_FALLINGSPARKS,	// type
							PARTICLE_FLAGS_BOUNCE,		// flags
							800,						// gravity
							0,							// magnetism
							25,							// base scale
							-1.7,						// decay rate
							.9,							// fade rate
							PARTICLE_TEXTURE_BLUEFIRE);	// texture
	
	if (pg != -1)
	{
		for (int i = 0; i < 15; i++)
		{
			delta.x = (RandomFloat()-.5f) * 500.0f;
			delta.y = (RandomFloat()-.5f) * 500.0f;
			delta.z = (RandomFloat()-.5f) * 500.0f;
			AddParticleToGroup(pg, &theNode->Coord, &delta, RandomFloat() + 1.0f, FULL_ALPHA);
		}
	}

			/* PLAY SOUND */
			
	PlayEffect3D(EFFECT_CHECKPOINT, &theNode->Coord);
	
	
			/* REMOVE DROPLET FROM CHECKPOINT */
			
	theNode->ChainHead->ChainNode = nil;					// remove from parent chain
	DeleteObject(theNode);
	return(false);
}


#pragma mark -

/****************** ADD EXIT LOG ************************/
//
// itemPtr->parm[0] = rotation 0..3 (ccw)
//

Boolean AddExitLog(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*logObj,*end;
CollisionBoxType *boxPtr;
Byte		rot = itemPtr->parm[0];
float		y;

			/********************/
			/* ADD MODEL OF LOG */
			/********************/
			
	gNewObjectDefinition.group 		= GLOBAL2_MGroupNum_ExitLog;	
	gNewObjectDefinition.type 		= GLOBAL2_MObjType_ExitLog;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= y = GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= rot * (PI/2);
	gNewObjectDefinition.scale 		= LOG_SCALE;
	logObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (logObj == nil)
		return(false);


	logObj->TerrainItemPtr = itemPtr;			// keep ptr to item list
	logObj->CType = CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_IMPENETRABLE;
	logObj->CBits = CBITS_ALLSOLID;

			/* BUILD COLLISION BOXES */			
	
	AllocateCollisionBoxMemory(logObj, 3);							// alloc 3 collision boxes

	boxPtr = logObj->CollisionBoxes;			

	boxPtr[0].top 		= y + (90*LOG_SCALE);
	boxPtr[0].bottom	= y + (0*LOG_SCALE);
	boxPtr[1].top 		= y + (90*LOG_SCALE);
	boxPtr[1].bottom	= y + (0*LOG_SCALE);
	boxPtr[2].top 		= y + (90*LOG_SCALE);
	boxPtr[2].bottom	= y + (56*LOG_SCALE);

	switch(rot)
	{
		case	0:
				boxPtr[0].left 		= x - (63*LOG_SCALE);
				boxPtr[0].right 	= x - (30*LOG_SCALE);
				boxPtr[0].front 	= z + (104*LOG_SCALE);
				boxPtr[0].back 		= z - (68*LOG_SCALE);

				boxPtr[1].left 		= x + (30*LOG_SCALE);
				boxPtr[1].right 	= x + (63*LOG_SCALE);
				boxPtr[1].front 	= z + (104*LOG_SCALE);
				boxPtr[1].back 		= z - (68*LOG_SCALE);

				boxPtr[2].left 		= x - (30*LOG_SCALE);
				boxPtr[2].right 	= x + (30*LOG_SCALE);
				boxPtr[2].front 	= z + (104*LOG_SCALE);
				boxPtr[2].back 		= z - (68*LOG_SCALE);
				break;

		case	1:
				boxPtr[0].left 		= x - (68*LOG_SCALE);
				boxPtr[0].right 	= x + (104*LOG_SCALE);
				boxPtr[0].front 	= z - (30*LOG_SCALE);
				boxPtr[0].back 		= z - (63*LOG_SCALE);

				boxPtr[1].left 		= x - (68*LOG_SCALE);
				boxPtr[1].right 	= x + (104*LOG_SCALE);
				boxPtr[1].front 	= z + (63*LOG_SCALE);
				boxPtr[1].back 		= z + (30*LOG_SCALE);

				boxPtr[2].left 		= x - (68*LOG_SCALE);
				boxPtr[2].right 	= x + (104*LOG_SCALE);
				boxPtr[2].front 	= z + (30*LOG_SCALE);
				boxPtr[2].back 		= z - (30*LOG_SCALE);
				break;					

		case	2:
				boxPtr[0].left 		= x - (63*LOG_SCALE);
				boxPtr[0].right 	= x - (30*LOG_SCALE);
				boxPtr[0].front 	= z - (68*LOG_SCALE);
				boxPtr[0].back 		= z + (104*LOG_SCALE);

				boxPtr[1].left 		= x + (30*LOG_SCALE);
				boxPtr[1].right 	= x + (63*LOG_SCALE);
				boxPtr[1].front 	= z - (68*LOG_SCALE);
				boxPtr[1].back 		= z + (104*LOG_SCALE);

				boxPtr[2].left 		= x - (30*LOG_SCALE);
				boxPtr[2].right 	= x + (30*LOG_SCALE);
				boxPtr[2].front 	= z - (68*LOG_SCALE);
				boxPtr[2].back 		= z + (104*LOG_SCALE);
				break;

		case	3:
				boxPtr[0].left 		= x - (104*LOG_SCALE);
				boxPtr[0].right 	= x + (68*LOG_SCALE);
				boxPtr[0].front 	= z - (30*LOG_SCALE);
				boxPtr[0].back 		= z - (63*LOG_SCALE);

				boxPtr[1].left 		= x - (104*LOG_SCALE);
				boxPtr[1].right 	= x + (68*LOG_SCALE);
				boxPtr[1].front 	= z + (63*LOG_SCALE);
				boxPtr[1].back 		= z + (30*LOG_SCALE);

				boxPtr[2].left 		= x - (104*LOG_SCALE);
				boxPtr[2].right 	= x + (68*LOG_SCALE);
				boxPtr[2].front 	= z + (30*LOG_SCALE);
				boxPtr[2].back 		= z - (30*LOG_SCALE);
				break;


	}

	KeepOldCollisionBoxes(logObj);							// set old stuff
		
		
			/*************************/
			/* CREATE TRIGGER AT END */
			/*************************/

				/* FRONT END */
			
	gNewObjectDefinition.genre 		= EVENT_GENRE;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	end = MakeNewObject(&gNewObjectDefinition);
	if (end == nil)
		return(false);

	end->CType 			= CTYPE_TRIGGER|CTYPE_PLAYERTRIGGERONLY;
	end->CBits 			= CBITS_ALLSOLID;
	end->TriggerSides 	= ALL_SOLID_SIDES;					// side(s) to activate it
	end->Kind 			= TRIGTYPE_EXITLOG;
	
	
	switch(rot)
	{
		case	0:
				SetObjectCollisionBounds(end,56*LOG_SCALE,0*LOG_SCALE,
										-30*LOG_SCALE,30*LOG_SCALE,
										80*LOG_SCALE,24*LOG_SCALE);
				break;

		case	3:
				SetObjectCollisionBounds(end,56*LOG_SCALE,0*LOG_SCALE,
										-80*LOG_SCALE,-24*LOG_SCALE,
										30*LOG_SCALE,-30*LOG_SCALE);
				break;

		case	2:
				SetObjectCollisionBounds(end,56*LOG_SCALE,0*LOG_SCALE,
										-30*LOG_SCALE,30*LOG_SCALE,
										-24*LOG_SCALE,-80*LOG_SCALE);
				break;

		case	1:
				SetObjectCollisionBounds(end,56*LOG_SCALE,0*LOG_SCALE,
										24*LOG_SCALE,80*LOG_SCALE,
										30*LOG_SCALE,-30*LOG_SCALE);
				break;
				
	}
	
	logObj->ChainNode = end;
	
	return(true);											// item was added
}



/************** DO TRIGGER - EXIT LOG ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_ExitLog(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
	(void) theNode;
	(void) whoNode;
	(void) sideBits;

	gAreaCompleted = true;
	return(false);
}


#pragma mark -


/************************* ADD KING WATER PIPE *********************************/
//
// parm[0] = ID#
//

Boolean AddKingWaterPipe(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
	
	if (gRealLevel != LEVEL_NUM_ANTKING)
		DoFatalAlert("AddKingWaterPipe: not on this level!");
	
			
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= ANTHILL_MObjType_KingPipe;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveKingWaterPipe;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 4.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


				/* SET TRIGGER STUFF */
	
	newObj->CType 			= CTYPE_MISC|CTYPE_TRIGGER|CTYPE_PLAYERTRIGGERONLY|CTYPE_AUTOTARGET|
							CTYPE_KICKABLE|CTYPE_IMPENETRABLE;
	newObj->CBits 			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind 			= TRIGTYPE_KINGPIPE;
	
	SetObjectCollisionBounds(newObj,900,0,-120,120,120,-120);


				/* WATER PARAMS */

	newObj->PipeID = itemPtr->parm[0];						// get pipe ID#
	newObj->SpewWater = false;
	
	newObj->InitCoord.y += 300.0f;
	return(true);											// item was added
}


/****************** MOVE KING WATER PIPE *********************/

static void MoveKingWaterPipe(ObjNode *theNode)
{
long	i;

		/*********************/
		/* SEE IF SPEW WATER */
		/*********************/

	if (theNode->SpewWater)
	{
				/* UPDATE WATER EFFECT */
				
		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_WATERLEAK, &theNode->InitCoord, kMiddleC-3, 1.5);
		else
			Update3DSoundChannel(EFFECT_WATERLEAK, &theNode->EffectChannel, &theNode->InitCoord);
	
				/* UPDATE PARTICLES */
				
		theNode->SpewWaterRegulator += gFramesPerSecondFrac;			// check timer
		if (theNode->SpewWaterRegulator > .03f)
		{
			theNode->SpewWaterRegulator = 0;
			
			
				/* MAKE PARTICLE GROUP */
				
			if (!VerifyParticleGroup(theNode->ParticleGroup))
			{
new_group:
				theNode->ParticleGroup = NewParticleGroup(
														PARTICLE_TYPE_FALLINGSPARKS,// type
														PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_EXTINGUISH,		// flags
														300,						// gravity
														0,							// magnetism
														30,							// base scale
														-1.3,						// decay rate
														.8,							// fade rate
														PARTICLE_TEXTURE_PATCHY);	// texture
			}
			
			
					/* ADD PARTICLES */
					
			if (theNode->ParticleGroup != -1)
			{
				TQ3Vector3D	delta;
				
				for (i = 0; i < 8; i++)
				{
					delta.x = (RandomFloat()-.5f) * 950.0f;
					delta.y = 30;
					delta.z = (RandomFloat()-.5f) * 950.0f;
					if (AddParticleToGroup(theNode->ParticleGroup, &theNode->InitCoord, &delta, 1.5f, FULL_ALPHA))
						goto new_group;
				}
			}
		}
		
			/* SEE IF TIME TO STOP */
			
		theNode->WaterTimer	-= gFramesPerSecondFrac;
		if (theNode->WaterTimer <= 0.0f)
		{
			theNode->SpewWater = false;
			theNode->RefillTimer = 5;
		}
	}
	
		/*******************/
		/* DONT SPEW WATER */
		/*******************/
		
	else
	{
		theNode->RefillTimer -= gFramesPerSecondFrac;	
	
		if (theNode->EffectChannel != -1)
			StopAChannel(&theNode->EffectChannel);
	}
		
}


/************** DO TRIGGER - KING WATER PIPE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_KingWaterPipe(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
	(void) sideBits;
	
	if (whoNode->Speed < 500.0f)					// must hit it with reasonable speed
		return(true);

	if (gPlayerMode == PLAYER_MODE_BUG)				// only the ball can trigger this
		return(true);

	if ((theNode->RefillTimer <= 0.0f) && (!theNode->SpewWater))
	{
		theNode->ParticleGroup = -1;
		theNode->SpewWater = true;
		theNode->WaterTimer = 4;
	}
	
	
	PlayEffect3D(EFFECT_PIPECLANG, &theNode->InitCoord);
	
	return(true);
}


/****************** KICK KING WATER PIPE *************************/

void KickKingWaterPipe(ObjNode *theNode)
{
	if ((theNode->RefillTimer <= 0.0f) && (!theNode->SpewWater))
	{
		theNode->SpewWater = true;
		theNode->WaterTimer = 4;
	}
	PlayEffect3D(EFFECT_PIPECLANG, &theNode->InitCoord);
}

#pragma mark -


/************************* ADD LADYBUG BONUS *********************************/

Boolean AddLadyBugBonus(TerrainItemEntryType *itemPtr, long  x, long z)
{
static const TQ3Point2D po[4] =
{
	{-430*LADYBUG_CAGE_SCALE,-430*LADYBUG_CAGE_SCALE},
	{-430*LADYBUG_CAGE_SCALE, 430*LADYBUG_CAGE_SCALE},
	{ 430*LADYBUG_CAGE_SCALE, 430*LADYBUG_CAGE_SCALE},
	{ 430*LADYBUG_CAGE_SCALE,-430*LADYBUG_CAGE_SCALE},
};
ObjNode	*post[4],*cage, *bug;
int		i;
float	y;

	y = GetTerrainHeightAtCoord(x,z,FLOOR);

			/******************/
			/* CREATE 4 POSTS */
			/******************/

	gNewObjectDefinition.slot 			= TRIGGER_SLOT;
	
	for (i = 0; i < 4; i++)
	{
		gNewObjectDefinition.group 		= GLOBAL1_MGroupNum_LadyBug;	
		gNewObjectDefinition.type 		= GLOBAL1_MObjType_LadyBugPost;	
		gNewObjectDefinition.coord.x 	= x + po[i].x;
		gNewObjectDefinition.coord.y 	= y + 10.0f;
		gNewObjectDefinition.coord.z 	= z + po[i].y;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
		gNewObjectDefinition.slot++;
		if (i == 0)
			gNewObjectDefinition.moveCall = MoveLadyBugBonus;
		else
			gNewObjectDefinition.moveCall = nil;
		gNewObjectDefinition.rot 		= -PI/2 + (PI/2 * i);
		gNewObjectDefinition.scale 		= LADYBUG_CAGE_SCALE;
		post[i] = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		GAME_ASSERT(post[i]);

		if (i == 0)
			post[0]->TerrainItemPtr = itemPtr;			// keep ptr to item list
		else
			post[i-1]->ChainNode = post[i];				// keep in chain						
	}		
		
			/***************/
			/* CREATE CAGE */
			/***************/

	gNewObjectDefinition.type 		= GLOBAL1_MObjType_LadyBugCage;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES | gAutoFadeStatusBits;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	cage = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	GAME_ASSERT(cage);

	cage->CType 			= CTYPE_MISC|CTYPE_KICKABLE|CTYPE_BLOCKCAMERA|CTYPE_TRIGGER;
	cage->CBits 			= CBITS_ALLSOLID;
	cage->TriggerSides 		= ALL_SOLID_SIDES;				// side(s) to activate it
	cage->Kind 				= TRIGTYPE_CAGE;

	SetObjectCollisionBounds(cage,200,0,-130,130,130,-130);

	post[3]->ChainNode = cage;	
	cage->ChainHead = post[0];



			/****************/
			/* MAKE LADYBUG */
			/****************/
				
	gNewObjectDefinition.type 		= SKELETON_TYPE_LADYBUG;
	gNewObjectDefinition.animNum 	= LADYBUG_ANIM_WAIT;							
	gNewObjectDefinition.coord.y 	= y + 100.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;	
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.scale 		= LADYBUG_SCALE;
	bug = MakeNewSkeletonObject(&gNewObjectDefinition);
	GAME_ASSERT(bug);

	cage->ChainNode = bug;

	return(true);											// item was added
}


/************************** MOVE LADYBUG BONUS ******************************/

static void MoveLadyBugBonus(ObjNode *box)
{

	if (TrackTerrainItem(box))		// just check to see if it's gone
	{
		DeleteObject(box);
		return;
	}
	
}


/************** KICK LADYBUG BOX ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean KickLadyBugBox(ObjNode *cage)
{
ObjNode	*bug,*p0,*p1,*p2,*p3;

		/* RELEASE THE LADY BUG */
		
	bug = cage->ChainNode;
	if (bug)
	{
		bug->MoveCall = MoveLadyBug;
		MorphToSkeletonAnim(bug->Skeleton, LADYBUG_ANIM_UNFOLD, 7);
		bug->StatusBits |= STATUS_BIT_DONTCULL;	// don't cull her as she takes off so the sfx isn't cut off abruptly
												// -- MoveLadyBug will cull her when she's risen high enough.
		cage->ChainNode = nil;					// detach bug from chain
		AttachShadowToObject(bug, 5, 5, false);	// give her a shadow
	}
	
	GetLadyBug();								// give me credit for this


		/* GET HEAD OF CHAIN */
		
	p0 = cage->ChainHead;
	p0->TerrainItemPtr = nil;					// dont ever come back!

	p1 = p0->ChainNode;
	p2 = p1->ChainNode;
	p3 = p2->ChainNode;
	p3->ChainNode = nil;			// detach cage from chain


		/* EXPLODE THE CAGE */
				
	QD3D_ExplodeGeometry(cage, 700.0f, SHARD_MODE_BOUNCE | SHARD_MODE_NULLSHADER, 1, .6);
	DeleteObject(cage);

	return(true);
}


/***************** MOVE LADY BUG *******************/

static void MoveLadyBug(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode) || (theNode->StatusBits & STATUS_BIT_ISCULLED))		// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}
	
	GetObjectInfo(theNode);

	if (theNode->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(theNode->Skeleton, LADYBUG_ANIM_FLY, 8);

	if (theNode->Skeleton->AnimNum == LADYBUG_ANIM_FLY)
	{
		gDelta.y += 100.0 * gFramesPerSecondFrac;
		gCoord.y += gDelta.y * gFramesPerSecondFrac;	
		theNode->Rot.y += gFramesPerSecondFrac;

		float altitude = gCoord.y - theNode->InitCoord.y;

		// Fade out shadow based on altitude
		float shadowTransparency = 1.0f - altitude / LADYBUG_MAX_ALTITUDE_FOR_SHADOW;
		if (shadowTransparency < 0)
			shadowTransparency = 0;
		MakeObjectTransparent(theNode->ShadowNode, shadowTransparency);

		// Culling is disabled on a rescued ladybug until she reaches an altitude threshold
		// so the sound effect isn't cut off abruptly.
		if (altitude > LADYBUG_MAX_ALTITUDE_BEFORE_CULLING)			// if risen above altitude threshold, allow culling
			theNode->StatusBits &= ~STATUS_BIT_DONTCULL;
	}

	UpdateObject(theNode);
	
		/* UPDATE SOUND */
		
	if (theNode->Skeleton->AnimNum == LADYBUG_ANIM_FLY)
	{
		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect3D(EFFECT_RESCUE, &gCoord);
		else
			Update3DSoundChannel(EFFECT_RESCUE, &theNode->EffectChannel, &gCoord);
	}
}

/************** DO TRIGGER - CAGE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_Cage(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
	(void) sideBits;
	
			/********************************/
			/* IF BALL, THEN SMASH OPEN NOW */
			/********************************/
			
	if (gPlayerMode == PLAYER_MODE_BALL)	
	{
		if (whoNode->Speed > 900.0f)					// gotta be going fast enough
		{
			KickLadyBugBox(theNode);
			PlayEffect_Parms3D(EFFECT_POUND, &gCoord, kMiddleC+2, 2.0);
		}
	}
	
	return(true);
}











