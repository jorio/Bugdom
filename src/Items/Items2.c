/****************************/
/*   	ITEMS2.C		    */
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

static void MoveFirecracker(ObjNode *theNode);
static void MoveHiveDoor(ObjNode *theNode);
static void MoveRootSwing(ObjNode *root);
static void MoveFirecrackerAtNight(ObjNode *theNode);
static void MoveHoneycombPlatformOnSpline(ObjNode *theNode);
static ObjNode *MakeOpenHiveDoor(TQ3Point3D *coord, Byte rot, Byte color);
static void SetRootAnimTimeIndex(ObjNode *theNode);
static void MoveHoneyTube(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/



#define	HIVE_DOOR_SCALE			7.0f

#define	NUM_JOINTS_IN_ROOT		7
#define	MAX_ROOT_SYNCS			4

/*********************/
/*    VARIABLES      */
/*********************/



ObjNode	*gCurrentRope,*gPrevRope;
short	gCurrentRopeJoint;

static float	gRootAnimTimeIndex[MAX_ROOT_SYNCS];

#define	RootSync		SpecialL[0]

#define	DetonatorID		SpecialL[0]
#define	DoorAim			SpecialL[1]
#define	DoorColor		SpecialL[2]

#define	ZigZag			Flag[0]

#pragma mark -


/************************* ADD FIRECRACKER *********************************/
//
// parm0 = detonator ID # or type (firecracker/cherry bomb)
//

Boolean AddFirecracker(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
long	id				= -1;

	if (!(gLevelTypeMask & ((1<<LEVEL_TYPE_HIVE) | (1<<LEVEL_TYPE_NIGHT))))
		DoFatalAlert("AddFirecracker: not on this level!");


		/* SEE IF DETONATOR ALREADY EXPLODED */
			
	if (gLevelType == LEVEL_TYPE_HIVE)
	{
		id = itemPtr->parm[0];							// get ID #
		if (gDetonatorBlown[id])						// see if already exploded
			return(true);
	}

			/* MAKE OBJECT */
			
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 250;
	
	if (gLevelType == LEVEL_TYPE_NIGHT)
	{
		gNewObjectDefinition.moveCall 	= MoveFirecrackerAtNight;	
		gNewObjectDefinition.type 		= NIGHT_MObjType_CherryBomb + itemPtr->parm[0];	
		gNewObjectDefinition.scale 		= .6;
	}
	else
	{
		gNewObjectDefinition.moveCall 	= MoveFirecracker;
		gNewObjectDefinition.type 		= HIVE_MObjType_Firecracker;	
		gNewObjectDefinition.scale 		= .3;
	}
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);


	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION INFO */
			
	newObj->CType = CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_ALLSOLID;
	
	if (gLevelType == LEVEL_TYPE_NIGHT)
	{
		if (itemPtr->parm[0] == 0)									// cherry bomb
			SetObjectCollisionBounds(newObj,200,0,-160,160,160,-160);		
		else														// firecracker
			SetObjectCollisionBounds(newObj,60,0,-70,70,30,-30);
	}
	else	
	{
		SetObjectCollisionBounds(newObj,60,0,-70,70,30,-30);
		newObj->DetonatorID = id;									// remember ID #
	}
	return(true);													// item was added
}


/******************** MOVE FIRECRACKER *************************/

static void MoveFirecracker(ObjNode *theNode)
{
int	id;

	if (TrackTerrainItem(theNode))				// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

			/* SEE IF NEED TO DETONATE */

	id = theNode->DetonatorID;					// get id #	
	if (gDetonatorBlown[id])					// see if detonator has been triggered
	{
		ExplodeFirecracker(theNode, false);	
	}
}


/******************** MOVE FIRECRACKER AT NIGHT *************************/

static void MoveFirecrackerAtNight(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))								// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}
	
		/* SEE IF A SPARK HIT IT */
		
	if (ParticleHitObject(theNode,PARTICLE_FLAGS_HOT))
	{	
		ExplodeFirecracker(theNode, true);
	}
}


/************* EXPLODE FIRECRACKER ********************/

void ExplodeFirecracker(ObjNode *fc, Boolean makeShockwave)
{
long			pg,i;
TQ3Vector3D		delta;

	fc->TerrainItemPtr = nil;							// dont ever come back	
	
	QD3D_ExplodeGeometry(fc, 2000.0f, SHARD_MODE_BOUNCE, 1, .6);
	DeleteObject(fc);
	
	
		/* CREATE SPARK EXPLOSION */
		
			/* white sparks */
				
	pg = NewParticleGroup(	0,							// magic num
							PARTICLE_TYPE_FALLINGSPARKS,	// type
							PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_HOT,		// flags
							400,						// gravity
							0,							// magnetism
							35,							// base scale
							1.5,						// decay rate
							0,							// fade rate
							PARTICLE_TEXTURE_WHITE);	// texture
	
	if (pg != -1)
	{
		for (i = 0; i < 40; i++)
		{
			delta.x = (RandomFloat()-.5f) * 1200.0f;
			delta.y = RandomFloat() * 1100.0f;
			delta.z = (RandomFloat()-.5f) * 1200.0f;
			AddParticleToGroup(pg, &fc->Coord, &delta, RandomFloat() + 1.0f, FULL_ALPHA);
		}
	}
	
				/* fire sparks */
				
	pg = NewParticleGroup(	0,							// magic num
							PARTICLE_TYPE_FALLINGSPARKS,	// type
							PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_HOT,		// flags
							400,						// gravity
							0,							// magnetism
							30,							// base scale
							-2.1,						// decay rate
							2.5,						// fade rate
							PARTICLE_TEXTURE_FIRE);		// texture
	
	if (pg != -1)
	{
		for (i = 0; i < 50; i++)
		{
			delta.x = (RandomFloat()-.5f) * 1000.0f;
			delta.y = RandomFloat() * 800.0f;
			delta.z = (RandomFloat()-.5f) * 1000.0f;
			AddParticleToGroup(pg, &fc->Coord, &delta, RandomFloat() + 1.0f, FULL_ALPHA);
		}
	}		

			/* PLAY FIRECRACKER SOUND */

	PlayEffect_Parms3D(EFFECT_KABLAM, &fc->Coord, kMiddleC, 15);
	
//	if (gLevelType == LEVEL_TYPE_HIVE)
//		PlayEffect_Parms3D(EFFECT_FIRECRACKER, &fc->Coord, kMiddleC-6, 15);
//	else	
//		PlayEffect_Parms3D(EFFECT_FIRECRACKER, &fc->Coord, kMiddleC-3, 10);


			/* MAKE DEADLY SHOCKWAVE */
			
	if (makeShockwave)
		if (gLevelType == LEVEL_TYPE_NIGHT)
			MakeShockwave(&fc->Coord, 300);

}


/************************* ADD HIVE DOOR *********************************/
//
// parm0 = detonator ID #
// parm1 = rot 0..3
// parm2 = color
//

Boolean AddHiveDoor(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		id;
Byte	rot,color;
	
	if (gLevelType != LEVEL_TYPE_HIVE)
		DoFatalAlert("AddHiveDoor: not on this level!");

	rot = itemPtr->parm[1];
	color = itemPtr->parm[2];

	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;

			/* SEE IF MAKE BLOWN OPEN DOOR */
			
	id = itemPtr->parm[0];
	if (gDetonatorBlown[id])										// see if detonator has been triggered
	{
		newObj = MakeOpenHiveDoor(&gNewObjectDefinition.coord, rot, color);
		newObj->TerrainItemPtr = itemPtr;							// keep ptr to item list
		return(true);
	}

			/***************/
			/* MAKE OBJECT */
			/***************/
			
	gNewObjectDefinition.group 		= HIVE_MGroupNum_HiveDoor;	
	gNewObjectDefinition.type 		= HIVE_MObjType_HiveDoor_Green + color;	
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 500;
	gNewObjectDefinition.moveCall 	= MoveHiveDoor;
	gNewObjectDefinition.rot 		= (float)rot * (PI2/4);
	gNewObjectDefinition.scale 		= HIVE_DOOR_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;							// keep ptr to item list

	newObj->DoorAim = rot;										// remember rot/aim
	newObj->DoorColor = color;

			/* SET COLLISION */
			
	newObj->CType = CTYPE_MISC|CTYPE_IMPENETRABLE;
	newObj->CBits = CBITS_ALLSOLID;
	
	if (rot & 1)									// vert	
	{
		SetObjectCollisionBounds(newObj,136*HIVE_DOOR_SCALE,0,
								-40*HIVE_DOOR_SCALE,40*HIVE_DOOR_SCALE,
								146*HIVE_DOOR_SCALE,-146*HIVE_DOOR_SCALE);
	}
	else														// horiz
	{
		SetObjectCollisionBounds(newObj,136*HIVE_DOOR_SCALE,0,
								-146*HIVE_DOOR_SCALE,146*HIVE_DOOR_SCALE,
								40*HIVE_DOOR_SCALE,-40*HIVE_DOOR_SCALE);
	}

	newObj->DetonatorID = id;									// remember ID #
	return(true);												// item was added
}

/********************** MAKE OPEN HIVE DOOR **********************/

static ObjNode *MakeOpenHiveDoor(TQ3Point3D *coord, Byte rot, Byte color)
{
ObjNode	*newObj;
CollisionBoxType *boxPtr;
float	x = coord->x;
float	y = coord->y;
float	z = coord->z;

			/* MAKE OBJECT */
			
	gNewObjectDefinition.group 		= HIVE_MGroupNum_HiveDoor;	
	gNewObjectDefinition.type 		= HIVE_MObjType_HiveDoor_GreenOpen + color;	
	gNewObjectDefinition.coord 		= *coord;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 200;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)rot * (PI2/4);
	gNewObjectDefinition.scale 		= HIVE_DOOR_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(nil);

			/* SET COLLISION */
			
	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	

	AllocateCollisionBoxMemory(newObj, 4);							// alloc 4 collision boxes
	boxPtr = newObj->CollisionBoxes;								// get ptr to 1st box 

	boxPtr[0].top 		= y + (136*HIVE_DOOR_SCALE);
	boxPtr[0].bottom 	= y + (0*HIVE_DOOR_SCALE);
	boxPtr[1].top 		= y + (136*HIVE_DOOR_SCALE);
	boxPtr[1].bottom 	= y + (61*HIVE_DOOR_SCALE);
	boxPtr[2].top 		= y + (136*HIVE_DOOR_SCALE);
	boxPtr[2].bottom 	= y + (0*HIVE_DOOR_SCALE);
	boxPtr[3].top 		= y + (6*HIVE_DOOR_SCALE);
	boxPtr[3].bottom 	= y + (0*HIVE_DOOR_SCALE);
	
	if (rot & 1)			
	{
		boxPtr[0].left 		= x + (-30*HIVE_DOOR_SCALE);
		boxPtr[0].right 	= x + (30*HIVE_DOOR_SCALE);
		boxPtr[0].front 	= z + (146*HIVE_DOOR_SCALE);
		boxPtr[0].back 		= z + (32*HIVE_DOOR_SCALE);

		boxPtr[1].left 		= x + (-30*HIVE_DOOR_SCALE);
		boxPtr[1].right 	= x + (30*HIVE_DOOR_SCALE);
		boxPtr[1].front 	= z + (32*HIVE_DOOR_SCALE);
		boxPtr[1].back 		= z + (-32*HIVE_DOOR_SCALE);

		boxPtr[2].left 		= x + (-30*HIVE_DOOR_SCALE);
		boxPtr[2].right 	= x + (30*HIVE_DOOR_SCALE);
		boxPtr[2].front 	= z + (-32*HIVE_DOOR_SCALE);
		boxPtr[2].back 		= z + (-146*HIVE_DOOR_SCALE);

		boxPtr[3].left 		= x + (-30*HIVE_DOOR_SCALE);
		boxPtr[3].right 	= x + (30*HIVE_DOOR_SCALE);
		boxPtr[3].front 	= z + (32*HIVE_DOOR_SCALE);
		boxPtr[3].back 		= z + (-32*HIVE_DOOR_SCALE);
	}
	else
	{
		boxPtr[0].left 		= x + (-146*HIVE_DOOR_SCALE);
		boxPtr[0].right 	= x + (-32*HIVE_DOOR_SCALE);
		boxPtr[0].front 	= z + (30*HIVE_DOOR_SCALE);
		boxPtr[0].back 		= z + (-30*HIVE_DOOR_SCALE);

		boxPtr[1].left 		= x + (-32*HIVE_DOOR_SCALE);
		boxPtr[1].right 	= x + (32*HIVE_DOOR_SCALE);
		boxPtr[1].front 	= z + (30*HIVE_DOOR_SCALE);
		boxPtr[1].back 		= z + (-30*HIVE_DOOR_SCALE);

		boxPtr[2].left 		= x + (32*HIVE_DOOR_SCALE);
		boxPtr[2].right 	= x + (146*HIVE_DOOR_SCALE);
		boxPtr[2].front 	= z + (30*HIVE_DOOR_SCALE);
		boxPtr[2].back 		= z + (-30*HIVE_DOOR_SCALE);

		boxPtr[3].left 		= x + (-32*HIVE_DOOR_SCALE);
		boxPtr[3].right 	= x + (32*HIVE_DOOR_SCALE);
		boxPtr[3].front 	= z + (30*HIVE_DOOR_SCALE);
		boxPtr[3].back 		= z + (-30*HIVE_DOOR_SCALE);
	}
	
	return(newObj);
}


/******************** MOVE HIVE DOOR *************************/

static void MoveHiveDoor(ObjNode *theNode)
{
int	id;

	if (TrackTerrainItem(theNode))				// just check to see if it's gone
	{
		DeleteObject(theNode);									
		return;
	}
	
			/* SEE IF NEED TO DETONATE */

	id = theNode->DetonatorID;						// get id #	
	if (gDetonatorBlown[id])						// see if detonator has been triggered
	{
		MakeOpenHiveDoor(&theNode->Coord, theNode->DoorAim, theNode->DoorColor);	// make open door model here
		DeleteObject(theNode);						// delete the old door
	}	
}


#pragma mark -


/************************* ADD DOCK *********************************/
//
// parm0 = rot 0..3
//

Boolean AddDock(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
	
	if (gLevelType != LEVEL_TYPE_POND)
		DoFatalAlert("AddDock: not on this level!");

			/* GET Y COORD */
			
	gNewObjectDefinition.group 		= POND_MGroupNum_Dock;	
	gNewObjectDefinition.type 		= POND_MObjType_Dock;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= WATER_Y;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 300;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= itemPtr->parm[0]*(PI/2);
	gNewObjectDefinition.scale 		= 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC|CTYPE_BLOCKSHADOW|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_ALLSOLID;
	
	if (itemPtr->parm[0] & 1)									
		SetObjectCollisionBounds(newObj,35,-200,-215,215,127,-127);
	else
		SetObjectCollisionBounds(newObj,35,-200,-127,127,215,-215);

	return(true);													// item was added
}


#pragma mark -

/************************* INIT ROOT SWINGS ****************************/
//
// The roots are skeleton objects, but we need to keep the animation synchronized so we are going
// to setup a few global time index variables which each root will use to do its animation.
//

void InitRootSwings(void)
{
int	i;

	for (i = 0; i < MAX_ROOT_SYNCS; i++)
	{
		gRootAnimTimeIndex[i] = (float)i / (float)MAX_ROOT_SYNCS;
	}
}

/********************* UPDATE ROOT SWINGS ********************/

void UpdateRootSwings(void)
{
int	i;

	if (gLevelType != LEVEL_TYPE_ANTHILL)
		return;

	for (i = 0; i < MAX_ROOT_SYNCS; i++)
	{
		gRootAnimTimeIndex[i] += gFramesPerSecondFrac * .45f;
		if (gRootAnimTimeIndex[i] >= 1.0f)
			gRootAnimTimeIndex[i] -= 1.0f;
	}

}


/************************* ADD ROOT SWING *********************************/
//
// parm[0] = rotation 0..8
// parm[1] = sync # 0..3
// parm[2] = scale factor
//

Boolean AddRootSwing(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj,*h;
float	scaleFactor = itemPtr->parm[2];	
	
	if (gLevelType != LEVEL_TYPE_ANTHILL)
		DoFatalAlert("AddRootSwing: not on this level!");

			/* CREATE ROOT OBJ */
			
	gNewObjectDefinition.type 		= SKELETON_TYPE_ROOTSWING;
	gNewObjectDefinition.animNum 	= 0;							
	gNewObjectDefinition.coord.x	= x;
	gNewObjectDefinition.coord.y	= GetTerrainHeightAtCoord(x,z,CEILING);
	gNewObjectDefinition.coord.z	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;	
	gNewObjectDefinition.slot		= PLAYER_SLOT-2;		// move before player update
	gNewObjectDefinition.moveCall 	= MoveRootSwing;
	gNewObjectDefinition.scale 		= 1.4f + (scaleFactor * .3f);
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/8.0);
	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	if (newObj == nil)
		DoFatalAlert("AddRootSwing: MakeNewSkeletonObject failed!");

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list
	
			/* CREATE INVISIBLE HOPPABLE TARGET */
	
	gNewObjectDefinition.genre 		= EVENT_GENRE;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	h = MakeNewObject(&gNewObjectDefinition);
	if (h)
	{		
		newObj->CType 			= 0;
		newObj->CBits			= 0;
	}
	
	newObj->ChainNode = h;
	
	
			/* SET ANIM SYNC STUFF */
			
	newObj->Skeleton->AnimSpeed = 0;				// these dont animate on their own
	newObj->RootSync = itemPtr->parm[1];
	if (newObj->RootSync >= MAX_ROOT_SYNCS)
		DoFatalAlert("AddRootSwing: illegal Root Sync value");
	
	SetRootAnimTimeIndex(newObj);
	
	return(true);													// item was added
}


/****************** MOVE ROOT SWING *******************/

static void MoveRootSwing(ObjNode *root)
{
int			a,j;
TQ3Point3D	p;

	SetRootAnimTimeIndex(root);

			/* SEE IF GONE */
			
	if (TrackTerrainItem(root))
	{
		DeleteObject(root);
		return;
	}
		
		/***********************************/
		/* CHECK IF PLAYER SHOULD LATCH ON */
		/***********************************/
	
	if (root == gPrevRope)								// see if ignore this one
		return;
	
	if (gPlayerMode == PLAYER_MODE_BALL)				// not while ball
		return;
	
	a = gPlayerObj->Skeleton->AnimNum;
	if ((a != PLAYER_ANIM_JUMP) && (a != PLAYER_ANIM_FALL))	// only while jumping or falling
		return;
	
	
			/* CHECK LAST FEW JOINTS FOR COLLISION */
			
	for (j = 2; j < NUM_JOINTS_IN_ROOT; j++)			// note we skip the 1st few joint since we dont want to grab that stump
	{
		FindCoordOfJoint(root, j, &p);
	
		if (j == (NUM_JOINTS_IN_ROOT - 2))				// update boppable on this joint
		{
			if (root->ChainNode)
				root->ChainNode->Coord = p;		
		}
	
		if (DoSimpleBoxCollisionAgainstPlayer(p.y+15.0f, p.y-15.0f,		// see if this joint hit player
											p.x-10.0f, p.x+10.0f,
											p.z+10.0f,p.z-10.0f))
		{
			if (j == (NUM_JOINTS_IN_ROOT-1))				// the last joint is a dummy joint, so bumb back one
				j--;
			PlayerGrabRootSwing(root, j);
			gPlayerObj->Rot.y += root->Rot.y;				// adjust for root's rot
			break;		
		}
	}
}


/************ SET ROOT ANIM TIME INDEX *****************/

static void SetRootAnimTimeIndex(ObjNode *theNode)
{
int		syncNum;
float	t;

	syncNum = theNode->RootSync;

	t = gRootAnimTimeIndex[syncNum];			// get 0..1 time index
	
	theNode->Skeleton->CurrentAnimTime = theNode->Skeleton->MaxAnimTime * t;	// convert to actual time value
}


#pragma mark -


/************************* ADD ROCK *********************************/
//
// parm[0] = type:
//
//		LAWN:
//			0 = big rock A
//			1 = little rock
//
//		NIGHT & FOREST:
//			0 = flat
//			1 = normal
//

Boolean AddRock(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
	
	if (!(gLevelTypeMask & ((1<<LEVEL_TYPE_LAWN) | (1<<LEVEL_TYPE_NIGHT) | (1<<LEVEL_TYPE_FOREST))))
		DoFatalAlert("AddRock: not on this level!");

			/* MAKE OBJ */
			
	
	switch(gLevelType)
	{
		case	LEVEL_TYPE_NIGHT:
				gNewObjectDefinition.group 	= MODEL_GROUP_LEVELSPECIFIC;	
				gNewObjectDefinition.type 	= NIGHT_MObjType_FlatRock + itemPtr->parm[0];	
				gNewObjectDefinition.scale  = .6;
				break;
				
		case	LEVEL_TYPE_LAWN:
				gNewObjectDefinition.group 	= MODEL_GROUP_LEVELSPECIFIC2;	
				gNewObjectDefinition.type 	= LAWN2_MObjType_Rock1 + itemPtr->parm[0];	
				gNewObjectDefinition.scale 	= 4.0;
				break;
				
		case	LEVEL_TYPE_FOREST:
				gNewObjectDefinition.group 	= MODEL_GROUP_LEVELSPECIFIC;	
				gNewObjectDefinition.type 	= FOREST_MObjType_FlatRock + itemPtr->parm[0];	
				gNewObjectDefinition.scale  = .9;
				break;
	}
	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 480;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= sin(x)*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;									// keep ptr to item list


			/* SET COLLISION */
			
	newObj->CType = CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_ALLSOLID;
	
	switch(gLevelType)
	{
		case	LEVEL_TYPE_NIGHT:
				if (itemPtr->parm[0] & 1)									
					SetObjectCollisionBounds(newObj,140,-100,-100,100,100,-100);	// normal
				else
					SetObjectCollisionBounds(newObj,50,-100,-100,100,100,-100);		// flat
				break;

		case	LEVEL_TYPE_FOREST:
				if (itemPtr->parm[0] & 1)									
					SetObjectCollisionBounds(newObj,210,-150,-150,150,150,-150);	// normal
				else
					SetObjectCollisionBounds(newObj,75,-150,-150,150,150,-150);		// flat
				break;
				
		case	LEVEL_TYPE_LAWN:
				if (itemPtr->parm[0] & 1)									
					SetObjectCollisionBounds(newObj,190,-200,-150,150,150,-150);		// small
				else
				{
					SetObjectCollisionBounds(newObj,600,-200,-360,360,360,-360);		// big
					newObj->CType |= CTYPE_IMPENETRABLE;
				}
				break;
	}

	return(true);														// item was added
}




#pragma mark -

/************************* ADD HONEY TUBE *********************************/
//
// parm[0] = type 0..3
// parm[1] = aim 0..3
// parm[2] = scale

Boolean AddHoneyTube(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	s;
int		n;

	n = itemPtr->parm[0];
	if ( n == 1)		// no more squiggles
		 n = 2;
	
	if (gLevelType != LEVEL_TYPE_HIVE)
		DoFatalAlert("AddHoneyTube: not on this level!");

	s = ((float)itemPtr->parm[2] * .5f) + 1.0f;
	

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= HIVE_MObjType_BentTube + n;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES | gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 550;
	gNewObjectDefinition.moveCall 	= MoveHoneyTube;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2/4);
	gNewObjectDefinition.scale 		= 3.0 * s;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	
	SetObjectCollisionBounds(newObj,400 * s,0,-70*s,70*s,70*s,-70*s);	// normal

	return(true);														// item was added
}


/************************ MOVE HONEY TUBE *************************/

static void MoveHoneyTube(ObjNode *theNode)
{

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

//	if (theNode->EffectChannel == -1)
//		theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_PUMP, &theNode->Coord, kMiddleC+(MyRandomLong()&3), .5);
//	else
//		Update3DSoundChannel(&theNode->EffectChannel, &theNode->Coord);
}


/******************** UPDATE HONEY TUBE TEXTURE ANIMATION ************************/

void UpdateHoneyTubeTextureAnimation(void)
{
	if (gLevelType != LEVEL_TYPE_HIVE)
		return;

				/* MOVE UVS */

	float du = 0.0f;
	float dv = 0.6f * gFramesPerSecondFrac;


	for (int type = HIVE_MObjType_BentTube; type <= HIVE_MObjType_TaperTube; type++)
	{
		GAME_ASSERT_MESSAGE(
				type == HIVE_MObjType_BentTube || type == HIVE_MObjType_SquiggleTube || type == HIVE_MObjType_StraightTube || type == HIVE_MObjType_TaperTube,
				"Did Beehive tube object types get shuffled around in the enum?");

		GAME_ASSERT(gObjectGroupList[MODEL_GROUP_LEVELSPECIFIC][type].numMeshes == 2);

		// Mesh #0 is the lattice; Mesh #1 is the inner tube
		QD3D_ScrollUVs(gObjectGroupList[MODEL_GROUP_LEVELSPECIFIC][type].meshes[1], du, dv);
	}
}

#pragma mark -




/************************ PRIME HONEYCOMB PLATFORM *************************/
//
// parm[3]: bit 1 = small
//			bit 2 = zigzag

Boolean PrimeHoneycombPlatform(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement,h;
Boolean			isSmall = itemPtr->parm[3] & (1<<1);
Boolean			zigzag = itemPtr->parm[3] & (1<<2);

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);

	if (itemPtr->parm[1] == 0)
		h = 11;
	else
		h = itemPtr->parm[1];
	

		/* MAKE OBJECT */

	gNewObjectDefinition.group 		= HIVE_MGroupNum_Platform;	
	gNewObjectDefinition.type 		= HIVE_MObjType_WoodPlatform;	
	gNewObjectDefinition.coord.x 	= x;
//	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR) + 50.0f + ((float)itemPtr->parm[1] * 4.0f);
	gNewObjectDefinition.coord.y 	= (h * -50.0f);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_ONSPLINE|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= PLAYER_SLOT-1;			// must do platforms before player motion
	gNewObjectDefinition.moveCall 	= nil;	
	gNewObjectDefinition.rot 		= 0;
	
	if (isSmall)
		gNewObjectDefinition.scale 		= HONEYCOMB_PLATFORM_SCALE2;
	else
		gNewObjectDefinition.scale 		= HONEYCOMB_PLATFORM_SCALE;

	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);
		
	DetachObject(newObj);									// detach this object from the linked list
			

				/* SET MORE INFO */
			
	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveHoneycombPlatformOnSpline;	// set move call
	newObj->CType			= CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_BLOCKCAMERA|CTYPE_IMPENETRABLE|
								CTYPE_BLOCKSHADOW|CTYPE_IMPENETRABLE2;
	newObj->CBits			= CBITS_ALLSOLID;
	
	newObj->ZigZag 			= zigzag;
	
	if (isSmall)
	{
		SetObjectCollisionBounds(newObj,65*HONEYCOMB_PLATFORM_SCALE2,-300*HONEYCOMB_PLATFORM_SCALE2,
							-200*HONEYCOMB_PLATFORM_SCALE2,200*HONEYCOMB_PLATFORM_SCALE2,
							200*HONEYCOMB_PLATFORM_SCALE2,-200*HONEYCOMB_PLATFORM_SCALE2);
	}
	else
	{
		SetObjectCollisionBounds(newObj,65*HONEYCOMB_PLATFORM_SCALE,-300*HONEYCOMB_PLATFORM_SCALE,
							-200*HONEYCOMB_PLATFORM_SCALE,200*HONEYCOMB_PLATFORM_SCALE,
							200*HONEYCOMB_PLATFORM_SCALE,-200*HONEYCOMB_PLATFORM_SCALE);
	}

			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	AddToSplineObjectList(newObj);

	return(true);
}

/******************** MOVE HONEYCOMB PLATFORM ON SPLINE ***************************/

static void MoveHoneycombPlatformOnSpline(ObjNode *theNode)
{
Boolean isVisible; 

	isVisible = IsSplineItemVisible(theNode);					// update its visibility


		/* MOVE ALONG THE SPLINE */

	if (theNode->ZigZag)
		IncreaseSplineIndexZigZag(theNode, 90);
	else
		IncreaseSplineIndex(theNode, 90);
		
	GetObjectCoordOnSpline(theNode, &theNode->Coord.x, &theNode->Coord.z);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/
			
	if (isVisible)
	{
		UpdateObjectTransforms(theNode);												// update transforms
		CalcObjectBoxFromNode(theNode);													// update collision box
		theNode->Delta.x = (theNode->Coord.x - theNode->OldCoord.x) * gFramesPerSecond;	// calc deltas
		theNode->Delta.y = (theNode->Coord.y - theNode->OldCoord.y) * gFramesPerSecond;	
		theNode->Delta.z = (theNode->Coord.z - theNode->OldCoord.z) * gFramesPerSecond;	
	}
	
			/* NOT VISIBLE */
	else
	{
	}
}

#pragma mark -

/************************* ADD ROCK LEDGE *********************************/

Boolean AddRockLedge(TerrainItemEntryType *itemPtr, long  x, long z)
{
	(void) itemPtr;
	(void) x;
	(void) z;

	return(true);													// item was added
}


/************************* ADD FAUCET *********************************/
//
// parm0 = rot 0..3
//

Boolean AddFaucet(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
	
	if (gLevelType != LEVEL_TYPE_LAWN)
		DoFatalAlert("AddFaucet: not on this level!");

			/* GET Y COORD */
			
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= LAWN1_MObjType_WaterFaucet;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z, FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 333;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= itemPtr->parm[0]*(PI/2);
	gNewObjectDefinition.scale 		= 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_ALLSOLID;
	
	SetObjectCollisionBounds(newObj,600,0,-110,110,110,-110);

	return(true);													// item was added
}



