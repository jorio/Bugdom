/****************************/
/*   		ITEMS.C		    */
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

static void MoveCyc(ObjNode *theNode);
static void MoveAntPipe(ObjNode *theNode);
static void MoveStump(ObjNode *stump);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	TREE_SCALE		20.0f

#define	HORIZ_PIPE_SCALE	.5f
#define	BENT_PIPE_SCALE		1.0f

#define	STUMP_SCALE			25.0f
#define	HIVE_SCALE			17.0f

#define	GRASS_SCALE			.15f
#define	COSMO_SCALE			.4f
#define	POPPY_SCALE			.4f
#define	CLOVER_SCALE		.15f
#define	WEED_SCALE			.2f

/*********************/
/*    VARIABLES      */
/*********************/

ObjNode	*gCyclorama;
ObjNode	*gHiveObj;

float	gCycScale;

#define	ValveID		SpecialL[0]
#define	ValvePipe	Flag[0]
#define	SpewWater	Flag[1]
#define	SpewWaterTimer SpecialF[0]

#define HiveWobbleIndex		SpecialF[0]
#define	HiveWobbleStrength	SpecialF[1]
#define	FireTimer			SpecialF[2]
#define HiveOnFire			SpecialF[3]
#define	HiveBurning			Flag[0]

/********************* INIT ITEMS MANAGER *************************/

void InitItemsManager(void)
{
	InitLiquids();
	InitDetonators();
	InitWaterValves();
	InitRootSwings();
	gBatExists = false;
	gCurrentCarryingFireFly = gCurrentChasingFireFly = nil;
	gCurrentDragonFly = gCurrentWaterBug = nil;

	gHiveObj = nil;
}





#pragma mark -


/************************* CREATE CYCLORAMA *********************************/

void CreateCyclorama(void)
{
	GAME_ASSERT_MESSAGE(!gCyclorama, "cyclorama already created");

	gNewObjectDefinition.group	= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 	= 0;						// cyc is always 1st in level-specific list
	gNewObjectDefinition.coord 	= gMyCoord;
	gNewObjectDefinition.slot 	= 0;
	gNewObjectDefinition.moveCall = MoveCyc;
	gNewObjectDefinition.rot 	= 0;
	gNewObjectDefinition.scale 	= gCycScale;

	// Notes on cyclorama status bits:
	// - HIDDEN because we'll draw it manually in DrawTerrain.
	// - Don't set NOZWRITE, contrary to the original source code. The cyc does appear to clip
	// through the terrain and fences on the OS 9 version, effectively reducing draw distance
	// somewhat. See: faraway fences seen from the starting position in level 4.
	gNewObjectDefinition.flags 	= STATUS_BIT_DONTCULL | STATUS_BIT_NULLSHADER | STATUS_BIT_NOFOG | STATUS_BIT_HIDDEN;

	gCyclorama = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	gCyclorama->RenderModifiers.drawOrder = kDrawOrder_Cyclorama;
}

/************************* DRAW CYCLORAMA *********************************/
//
// The cyclorama is drawn manually in DrawTerrain, before the normal object draw loop
//

void DrawCyclorama(void)
{
	if (!gCyclorama)
		return;

	gCyclorama->RenderModifiers.statusBits = gCyclorama->StatusBits & ~STATUS_BIT_HIDDEN;
	Render_SubmitMeshList(
			gCyclorama->NumMeshes,
			gCyclorama->MeshList,
			&gCyclorama->BaseTransformMatrix,
			&gCyclorama->RenderModifiers,
			&gCyclorama->Coord);
}

/******************** MOVE CYC ***********************/

static void MoveCyc(ObjNode *theNode)
{
static const float cycM[] = 				// bigger numbers mean background tracks closer with camera
{
	.6,				// lawn
	.5,				// pond
	.9,				// forest
	.5,				// hive
	.8,				// night
	.5				// ant hill
};

	theNode->Coord = gGameViewInfoPtr->currentCameraCoords;
	theNode->Coord.y *= cycM[gLevelType];			// only moves fractionally with y

	UpdateObjectTransforms(theNode);
}

#pragma mark -

/************************* ADD CLOVER *********************************/

Boolean AddClover(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		n;
float	s;
	
	if (gLevelType != LEVEL_TYPE_LAWN)
		DoFatalAlert("AddClover: not on this level!");
				
	n = itemPtr->parm[0];
	if (n > 1)
		DoFatalAlert("AddClover: illegal clover type");
				
	gNewObjectDefinition.group 		= LAWN2_MGroupNum_Clover;	
	gNewObjectDefinition.type 		= LAWN2_MObjType_Clover + n;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	if (gRecentTerrainNormal[FLOOR].y < .85f)			// if on slope then sink it a little
		gNewObjectDefinition.coord.y -= 40.0f;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= s = CLOVER_SCALE + RandomFloat()*.1f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC; //|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,2100.0f*s,0,
							-200.0f*s,200.0f*s,
							200.0f*s,-200.0f*s);

	return(true);													// item was added
}


/************************* ADD GRASS *********************************/
//
// Different grass on different levels.
//

Boolean AddGrass(TerrainItemEntryType *itemPtr, long  x, long z)
{
int		n;
ObjNode	*newObj;

	n = itemPtr->parm[0];
	if (n > 1)
		DoFatalAlert("AddGrass: illegal grass type");
			
	switch(gLevelType)
	{
		case	LEVEL_TYPE_LAWN:
				gNewObjectDefinition.type 	= LAWN2_MObjType_Grass + n;	
				gNewObjectDefinition.group 	= MODEL_GROUP_LEVELSPECIFIC2;	
				break;

		case	LEVEL_TYPE_FOREST:
				gNewObjectDefinition.type 	= FOREST_MObjType_Grass + n;	
				gNewObjectDefinition.group 	= MODEL_GROUP_LEVELSPECIFIC;	
				break;

		case	LEVEL_TYPE_NIGHT:
				gNewObjectDefinition.type 	= NIGHT_MObjType_Grass + n;	
				gNewObjectDefinition.group 	= MODEL_GROUP_LEVELSPECIFIC;	
				break;
	
		default:
				DoFatalAlert("AddGrass: not on this level, buddy!");
	}
			
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	if (gRecentTerrainNormal[FLOOR].y < .85)			// if on slope then sink it a little
		gNewObjectDefinition.coord.y -= 40.0f;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 40+itemPtr->parm[0];
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= GRASS_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,6500*GRASS_SCALE,0,
								-500*GRASS_SCALE,500*GRASS_SCALE,
								500*GRASS_SCALE,-500*GRASS_SCALE);

	return(true);													// item was added
}


/************************* ADD WEED *********************************/

Boolean AddWeed(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
	
			
	gNewObjectDefinition.group 		= LAWN2_MGroupNum_Weed;	
	gNewObjectDefinition.type 		= LAWN2_MObjType_Weed + itemPtr->parm[0];	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	if (gRecentTerrainNormal[FLOOR].y < .85)			// if on slope then sink it a little
		gNewObjectDefinition.coord.y -= 40.0f;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 655;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= WEED_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC; //|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,5000*WEED_SCALE,0,
								-570*WEED_SCALE,570*WEED_SCALE,
								570*WEED_SCALE,-570*WEED_SCALE);

	return(true);													// item was added
}



/************************* ADD SUNFLOWER *********************************/

Boolean AddSunFlower(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
	
			
	gNewObjectDefinition.group 		= LAWN2_MGroupNum_Sunflower;	
	gNewObjectDefinition.type 		= LAWN2_MObjType_Sunflower;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 101;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= .15;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC; //|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,600,0,-40,40,40,-40);

	return(true);													// item was added
}




/************************* ADD COSMO *********************************/

Boolean AddCosmo(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
	
			
	gNewObjectDefinition.group 		= LAWN2_MGroupNum_Cosmo;	
	gNewObjectDefinition.type 		= LAWN2_MObjType_Cosmo;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 102;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= COSMO_SCALE + RandomFloat()*.05f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Damage = .05;											// these do minimal damage
	newObj->CType = CTYPE_MISC; //|CTYPE_BLOCKCAMERA; //|CTYPE_HURTME;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,700*COSMO_SCALE,0,
							-160*COSMO_SCALE,160*COSMO_SCALE,
							160*COSMO_SCALE,-160*COSMO_SCALE);
	return(true);													// item was added
}


/************************* ADD POPPY *********************************/

Boolean AddPoppy(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
			
	gNewObjectDefinition.group 		= LAWN2_MGroupNum_Poppy;	
	gNewObjectDefinition.type 		= LAWN2_MObjType_Poppy;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 103;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= POPPY_SCALE + RandomFloat()*.05f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC; //|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,1900*POPPY_SCALE,0,
							-300*POPPY_SCALE,300*POPPY_SCALE,
							300*POPPY_SCALE,-300*POPPY_SCALE);
	return(true);													// item was added
}


#pragma mark -

/************************* ADD WALL END *********************************/

Boolean AddWallEnd(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
	
			
	gNewObjectDefinition.group 		= GLOBAL1_MGroupNum_WallEnd;	
	gNewObjectDefinition.type 		= GLOBAL1_MObjType_WallEnd;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR) - 30.0f;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 104;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= 0;
	
	if (gLevelType == LEVEL_TYPE_POND)
		gNewObjectDefinition.scale 		= 1.5;	
	else
		gNewObjectDefinition.scale 		= .7;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_IMPENETRABLE;
	newObj->CBits = CBITS_ALLSOLID;

	if (gLevelType == LEVEL_TYPE_POND)
		SetObjectCollisionBounds(newObj,1100,0,-350,350,350,-350);
	else
		SetObjectCollisionBounds(newObj,500,0,-170,170,170,-170);

	return(true);													// item was added
}

#pragma mark -


/************************* ADD TREE *********************************/

Boolean AddTree(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	y;
CollisionBoxType *boxPtr;
	
	if (gLevelType != LEVEL_TYPE_FOREST)
		DoFatalAlert("AddTree: not on this level, Bud");

			/***************/
			/* CREATE TREE */
			/***************/
			
	gNewObjectDefinition.group 		= FOREST_MGroupNum_Tree;	
	gNewObjectDefinition.type 	= FOREST_MObjType_Tree;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= y = GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 200;
	gNewObjectDefinition.moveCall 	= nil;							// once added, this thing doesnt ever go away
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= TREE_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	
			/******************************/
			/* BUILD TREE COLLISION BOXES */			
			/******************************/
	
	AllocateCollisionBoxMemory(newObj, 7);							// alloc 7 collision boxes

	boxPtr = newObj->CollisionBoxes;			

				/* TRUNK */
				
	boxPtr[0].left 		= x - (25*TREE_SCALE);
	boxPtr[0].right 	= x + (25*TREE_SCALE);
	boxPtr[0].top 		= y + (616*TREE_SCALE);
	boxPtr[0].bottom	= y + (0*TREE_SCALE);
	boxPtr[0].back 		= z - (25*TREE_SCALE);
	boxPtr[0].front 	= z + (25*TREE_SCALE);

				/* TOP X-SPAN */
				
	boxPtr[1].left 		= x - (218*TREE_SCALE);
	boxPtr[1].right 	= x + (218*TREE_SCALE);
	boxPtr[1].top 		= y + (568*TREE_SCALE);
	boxPtr[1].bottom	= y + (424*TREE_SCALE);
	boxPtr[1].back 		= z - (5*TREE_SCALE);
	boxPtr[1].front 	= z + (5*TREE_SCALE);

				/* TOP Z-SPAN */
				
	boxPtr[2].left 		= x - (5*TREE_SCALE);
	boxPtr[2].right 	= x + (5*TREE_SCALE);
	boxPtr[2].top 		= y + (568*TREE_SCALE);
	boxPtr[2].bottom	= y + (424*TREE_SCALE);
	boxPtr[2].back 		= z - (218*TREE_SCALE);
	boxPtr[2].front 	= z + (218*TREE_SCALE);

				/* MID X-SPAN */
				
	boxPtr[3].left 		= x - (256*TREE_SCALE);
	boxPtr[3].right 	= x + (256*TREE_SCALE);
	boxPtr[3].top 		= y + (409*TREE_SCALE);
	boxPtr[3].bottom	= y + (245*TREE_SCALE);
	boxPtr[3].back 		= z - (5*TREE_SCALE);
	boxPtr[3].front 	= z + (5*TREE_SCALE);

				/* MID Z-SPAN */

	boxPtr[4].left 		= x - (5*TREE_SCALE);
	boxPtr[4].right 	= x + (5*TREE_SCALE);
	boxPtr[4].top 		= y + (409*TREE_SCALE);
	boxPtr[4].bottom	= y + (245*TREE_SCALE);
	boxPtr[4].back 		= z - (256*TREE_SCALE);
	boxPtr[4].front 	= z + (256*TREE_SCALE);

				/* BOTTOM X-SPAN */
				
	boxPtr[5].left 		= x - (283*TREE_SCALE);
	boxPtr[5].right 	= x + (283*TREE_SCALE);
	boxPtr[5].top 		= y + (232*TREE_SCALE);
	boxPtr[5].bottom	= y + (48*TREE_SCALE);
	boxPtr[5].back 		= z - (5*TREE_SCALE);
	boxPtr[5].front 	= z + (5*TREE_SCALE);

				/* BOTTOM Z-SPAN */
				
	boxPtr[6].left 		= x - (5*TREE_SCALE);
	boxPtr[6].right 	= x + (5*TREE_SCALE);
	boxPtr[6].top 		= y + (232*TREE_SCALE);
	boxPtr[6].bottom	= y + (48*TREE_SCALE);
	boxPtr[6].back 		= z - (283*TREE_SCALE);
	boxPtr[6].front 	= z + (283*TREE_SCALE);

	KeepOldCollisionBoxes(newObj);							// set old stuff
	
	return(true);											// item was added
}

/************************* ADD STUMP *********************************/

Boolean AddStump(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode				*newObj,*hive;
CollisionBoxType 	*boxPtr;
float				y;
	
	if (gLevelType != LEVEL_TYPE_FOREST)
		DoFatalAlert("AddStump: not on this level, Bud");

			/***************/
			/* CREATE STUMP */
			/***************/
			
	gNewObjectDefinition.group 		= FOREST_MGroupNum_Tree;	
	gNewObjectDefinition.type 		= FOREST_MObjType_Stump;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= y = GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 200;
	gNewObjectDefinition.moveCall 	= MoveStump;					// once added, this thing doesnt ever go away
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= STUMP_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION */
			
	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;

	AllocateCollisionBoxMemory(newObj, 2);							// alloc 2 collision boxes
	boxPtr = newObj->CollisionBoxes;			

	boxPtr[0].top 		= y + (386*STUMP_SCALE);
	boxPtr[0].bottom	= y + (0*STUMP_SCALE);
	boxPtr[0].left 		= x - (18*STUMP_SCALE);
	boxPtr[0].right 	= x + (18*STUMP_SCALE);
	boxPtr[0].front 	= z + (18*STUMP_SCALE);
	boxPtr[0].back 		= z - (18*STUMP_SCALE);

	boxPtr[1].top 		= y + (162*STUMP_SCALE);
	boxPtr[1].bottom	= y + (134*STUMP_SCALE);
	boxPtr[1].left 		= x + (18*STUMP_SCALE);
	boxPtr[1].right 	= x + (194*STUMP_SCALE);
	boxPtr[1].front 	= z + (13*STUMP_SCALE);
	boxPtr[1].back 		= z - (13*STUMP_SCALE);

	KeepOldCollisionBoxes(newObj);							// set old stuff


			/*************/
			/* MAKE HIVE */
			/*************/
			
	gNewObjectDefinition.type 		= FOREST_MObjType_Hive;	
	gNewObjectDefinition.coord.x 	+= 130*STUMP_SCALE;
	gNewObjectDefinition.coord.y 	+= 150*STUMP_SCALE;
	gNewObjectDefinition.scale 		= HIVE_SCALE;
	gHiveObj = hive = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (hive)
	{
		x = gNewObjectDefinition.coord.x;
		y = gNewObjectDefinition.coord.y;
		
		newObj->ChainNode = hive;							// connect to chain
		
		hive->Health = 1.0;									// health of hive
		
		hive->CType = CTYPE_MISC;
		hive->CBits = CBITS_ALLSOLID;

		AllocateCollisionBoxMemory(hive, 3);							// alloc 3 collision boxes
		boxPtr = hive->CollisionBoxes;			

		boxPtr[0].top 		= y + (0*HIVE_SCALE);
		boxPtr[0].bottom	= y - (113*HIVE_SCALE);
		boxPtr[0].left 		= x - (6*HIVE_SCALE);
		boxPtr[0].right 	= x + (6*HIVE_SCALE);
		boxPtr[0].front 	= z + (6*HIVE_SCALE);
		boxPtr[0].back 		= z - (6*HIVE_SCALE);

		boxPtr[1].top 		= y - (113*HIVE_SCALE);
		boxPtr[1].bottom	= y - (198*HIVE_SCALE);
		boxPtr[1].left 		= x - (31*HIVE_SCALE);
		boxPtr[1].right 	= x + (31*HIVE_SCALE);
		boxPtr[1].front 	= z + (31*HIVE_SCALE);
		boxPtr[1].back 		= z - (31*HIVE_SCALE);

		boxPtr[2].top 		= y - (147*HIVE_SCALE);
		boxPtr[2].bottom	= y - (163*HIVE_SCALE);
		boxPtr[2].left 		= x - (10*HIVE_SCALE);
		boxPtr[2].right 	= x + (10*HIVE_SCALE);
		boxPtr[2].front 	= z + (41*HIVE_SCALE);
		boxPtr[2].back 		= z + (31*HIVE_SCALE);

		KeepOldCollisionBoxes(hive);							// set old stuff

	}
	return(true);													// item was added
}


/****************** MOVE STUMP **************************/

static void MoveStump(ObjNode *stump)
{
ObjNode *hive;
float	fps = gFramesPerSecondFrac;
float	r;
int		i,n;

	hive = stump->ChainNode;
	if (hive == nil)
		return;

		/* MAKE HIVE WOBBLE */
		
	hive->HiveWobbleIndex += fps * 4.0f;
	if (hive->HiveWobbleStrength > 0.0f)
	{
		hive->HiveWobbleStrength -= fps * .01f;
		if (hive->HiveWobbleStrength < 0.0f)
			hive->HiveWobbleStrength = 0.0f;		
	}


	r = sin(hive->HiveWobbleIndex);
	r *= hive->HiveWobbleStrength;
	
	hive->Rot.x = r;
	
	UpdateObjectTransforms(hive);
	
	
		/********************/
		/* SEE IF ON FLAMES */
		/********************/
		
	if (hive->HiveBurning)
	{	
		TQ3Point3D	hiveBody;
		
		hiveBody.x = hive->Coord.x;
		hiveBody.y = hive->Coord.y - (163 * HIVE_SCALE);
		hiveBody.z = hive->Coord.z;
		
		
			/* UPDATE CRACKLE */

		if (hive->EffectChannel == -1)
		{			
			hive->EffectChannel = PlayEffect3D(EFFECT_FIRECRACKLE, &hiveBody);
		}
		else
		{
			Update3DSoundChannel(EFFECT_FIRECRACKLE, &hive->EffectChannel, &hiveBody);
		}
	
	
			/* UPDATE FIRE */
			
		hive->HiveOnFire -= fps;
		if (hive->HiveOnFire < 0.0f)
			gAreaCompleted = true;
	
		n = 0;

		if (!VerifyParticleGroup(hive->ParticleGroup))
		{
new_group:
			hive->ParticleGroup = NewParticleGroup(
													PARTICLE_TYPE_FALLINGSPARKS,	// type
													0,							// flags
													-1000,							// gravity
													0,							// magnetism
													40,							// base scale
													-1.0,						// decay rate
													.3,							// fade rate
													PARTICLE_TEXTURE_FIRE);		// texture

			n++;
		}

		if (hive->ParticleGroup != -1)
		{
			hive->FireTimer += gFramesPerSecondFrac;
			if (hive->FireTimer > .01f)
			{
				TQ3Vector3D	delta;
				TQ3Point3D  pt;

				hive->FireTimer = 0;

				for (i = 0; i < 8; i++)
				{
					pt.x = hiveBody.x + (RandomFloat()-.5f) * (60 * HIVE_SCALE);
					pt.y = hiveBody.y + (RandomFloat()-.5f) * (70 * HIVE_SCALE);
					pt.z = hiveBody.z + (RandomFloat()-.5f) * (60 * HIVE_SCALE);
					
					delta.x = (RandomFloat()-.5f) * 20.0f;
					delta.y = (RandomFloat()-.5f) * 30.0f + 40.0f;
					delta.z = (RandomFloat()-.5f) * 20.0f;
					
					if (AddParticleToGroup(hive->ParticleGroup, &pt, &delta, RandomFloat() + 3.0f, FULL_ALPHA) && (n==0))
						goto new_group;
				}
			}
		}
	
	}
}


/****************** RATTLE HIVE **********************/
//
// Called when dragonfly bullet hits hive.
//

void RattleHive(ObjNode *hive)
{
int	i;
		/* HURT IT */
		
	hive->Health -= .02f;
	if (hive->Health <= 0.0f)			// see if killed it
	{
		if (!hive->HiveBurning)
		{
			hive->HiveOnFire = 6;
			hive->HiveBurning = true;
		}
	}
	else
	{

			/* SET IT SWINGING */
			
		hive->HiveWobbleStrength = .12f;
		
			/* SPAWN A FEW BEES */

		for (i = 0; i < 3; i++)
		{
			TQ3Point3D	p;
			
			p.x = hive->Coord.x + (RandomFloat()-.5f) * 400.0f;
			p.y = hive->Coord.y - (163 * HIVE_SCALE) + (RandomFloat()-.5f) * 300.0f;
			p.z = hive->Coord.z + (20 * HIVE_SCALE) + (RandomFloat()-.5f) * 200.0f;
			
			MakeFlyingBee(&p);	
		}
	}	

	gInfobarUpdateBits |= UPDATE_BOSS;
}


#pragma mark -




/************************* ADD CATTAIL *********************************/

Boolean AddCatTail(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
	
	if (gLevelType != LEVEL_TYPE_POND)
		DoFatalAlert("AddCatTail: not on this level!");

			
	gNewObjectDefinition.group 		= POND_MGroupNum_CatTail;	
	gNewObjectDefinition.type 		= POND_MObjType_CatTail;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER;
	gNewObjectDefinition.slot 		= 215;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= .15f + RandomFloat()*.1f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,300,0,-20,20,20,-20);

	return(true);													// item was added
}



/************************* ADD DUCKWEED *********************************/

Boolean AddDuckWeed(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
	
	if (gLevelType != LEVEL_TYPE_POND)
		DoFatalAlert("AddDuckWeed: not on this level!");

			
	gNewObjectDefinition.group 		= POND_MGroupNum_DuckWeed;	
	gNewObjectDefinition.type 		= POND_MObjType_DuckWeed;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER;
	gNewObjectDefinition.slot 		= 213;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= .15f + RandomFloat()*.1f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,1000,0,-20,20,20,-20);

	return(true);													// item was added
}


/************************* ADD LILY FLOWER *********************************/

Boolean AddLilyFlower(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
	
	if (gLevelType != LEVEL_TYPE_POND)
		DoFatalAlert("AddLilyFlower: not on this level!");

			
	gNewObjectDefinition.group 		= POND_MGroupNum_LilyFlower;	
	gNewObjectDefinition.type 		= POND_MObjType_LilyFlower;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= WATER_Y;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER;
	gNewObjectDefinition.slot 		= 300;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= 3.0f + RandomFloat()*.5f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,100,-100,-150,150,150,-150);

	return(true);													// item was added
}




/************************* ADD LILY PAD *********************************/

Boolean AddLilyPad(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
	
	if (gLevelType != LEVEL_TYPE_POND)
		DoFatalAlert("AddLilyPad: not on this level!");

			
	gNewObjectDefinition.group 		= POND_MGroupNum_LilyPad;	
	gNewObjectDefinition.type 		= POND_MObjType_LilyPad;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= WATER_Y;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER;
	gNewObjectDefinition.slot 		= 211;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= 2.5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC|CTYPE_BLOCKSHADOW|CTYPE_BLOCKCAMERA|CTYPE_IMPENETRABLE|CTYPE_IMPENETRABLE2;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,15,-900,-400,400,400,-400);

	return(true);													// item was added
}

/************************* ADD POND GRASS *********************************/

Boolean AddPondGrass(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
	
	if (gLevelType != LEVEL_TYPE_POND)
		DoFatalAlert("AddPondGrass: not on this level!");

	if (itemPtr->parm[0] > 2)
		DoFatalAlert("AddPondGrass:parm[0] out of range!");


			
	gNewObjectDefinition.group 		= POND_MGroupNum_PondGrass;	
	gNewObjectDefinition.type 		= POND_MObjType_PondGrass + itemPtr->parm[0];	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER;
	gNewObjectDefinition.slot 		= 210;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= .25f + RandomFloat()*.1f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,1000,0,-20,20,20,-20);

	return(true);													// item was added
}


/************************* ADD REED *********************************/

Boolean AddReed(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
	
	if (gLevelType != LEVEL_TYPE_POND)
		DoFatalAlert("AddReed: not on this level!");

	if (itemPtr->parm[0] > 1)
		DoFatalAlert("AddReed:parm[0] out of range!");

			
	gNewObjectDefinition.group 		= POND_MGroupNum_Reed;	
	gNewObjectDefinition.type 		= POND_MObjType_Reed + itemPtr->parm[0];	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NULLSHADER;
	gNewObjectDefinition.slot 		= 200;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= .4f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	
	if (itemPtr->parm[0] == 0)
		SetObjectCollisionBounds(newObj,1000,0,-35,35,35,-35);
	else
		SetObjectCollisionBounds(newObj,1000,0,-90,90,90,-90);

	return(true);													// item was added
}


#pragma mark -


/************************* ADD BENT ANT PIPE *********************************/
//
// parm[0] = rot ccw
// parm[1] = valve ID#
// parm[3]:bit0 = spew water if valve open
// parm[3]:bit1 = spew water always
//

Boolean AddBentAntPipe(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
TQ3Matrix4x4	m;
static const TQ3Point3D waterPt = {45*BENT_PIPE_SCALE, 178*BENT_PIPE_SCALE, 45*BENT_PIPE_SCALE};
	
			
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= ANTHILL_MObjType_BentPipe;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveAntPipe;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI/2);
	gNewObjectDefinition.scale 		= BENT_PIPE_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


				/* SET COLLISION */
					
	newObj->CType = CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,300,0,-50,50,50,-50);

				/* WATER PARAMS */

	newObj->ValveID = itemPtr->parm[1];
	newObj->ValvePipe = itemPtr->parm[3] & 1;
	newObj->SpewWater = itemPtr->parm[3] & (1<<1);


				/* CALC COORD OF WATER LEAK */
				
	Q3Matrix4x4_SetRotate_Y(&m, newObj->Rot.y);
	Q3Point3D_Transform(&waterPt, &m, &newObj->InitCoord);
	newObj->InitCoord.x += newObj->Coord.x;
	newObj->InitCoord.y += newObj->Coord.y;
	newObj->InitCoord.z += newObj->Coord.z;

	return(true);													// item was added
}


/************************* ADD HORIZ ANT PIPE *********************************/
//
// parm[0] = rot ccw
// parm[1] = height to place
// parm[2] = valve ID#
// parm[3]:bit0 = spew water if valve open
// parm[3]:bit1 = spew water always
//

Boolean AddHorizAntPipe(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode			*newObj;
TQ3Matrix4x4	m;
static const TQ3Point3D waterPt = {-58*HORIZ_PIPE_SCALE, 400*HORIZ_PIPE_SCALE, 214*HORIZ_PIPE_SCALE};
			
					/* MAKE OBJ */
					
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= ANTHILL_MObjType_HorizPipe;	
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR) + ((float)itemPtr->parm[1] * 10.0f);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveAntPipe;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI/2);
	gNewObjectDefinition.scale 		= HORIZ_PIPE_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


				/* SET COLLISION */
					
	newObj->CType = CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_ALLSOLID;
	
	if (itemPtr->parm[0] & 1)
		SetObjectCollisionBounds(newObj,300,0,-100,100,500,-500);
	else
		SetObjectCollisionBounds(newObj,300,0,-500,500,100,-100);


				/* WATER PARAMS */

	newObj->ValveID = itemPtr->parm[2];
	newObj->ValvePipe = itemPtr->parm[3] & 1;
	newObj->SpewWater = itemPtr->parm[3] & (1<<1);


				/* CALC COORD OF WATER LEAK */
				
	Q3Matrix4x4_SetRotate_Y(&m, newObj->Rot.y);
	Q3Point3D_Transform(&waterPt, &m, &newObj->InitCoord);
	newObj->InitCoord.x += newObj->Coord.x;
	newObj->InitCoord.y += newObj->Coord.y;
	newObj->InitCoord.z += newObj->Coord.z;

	return(true);													// item was added
}


/****************** MOVE ANT PIPE *********************/

static void MoveAntPipe(ObjNode *theNode)
{
long	i;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}


		/* SEE IF ACTIVATE VALVE PIPE */
		
	if (theNode->ValvePipe)
		if (gValveIsOpen[theNode->ValveID])
			theNode->SpewWater = true;


		/*********************/
		/* SEE IF SPEW WATER */
		/*********************/

	if (theNode->SpewWater)
	{
				/* UPDATE WATER EFFECT */
				
		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect3D(EFFECT_WATERLEAK, &theNode->InitCoord);
		else
			Update3DSoundChannel(EFFECT_WATERLEAK, &theNode->EffectChannel, &theNode->InitCoord);
	
		theNode->SpewWaterTimer += gFramesPerSecondFrac;			// check timer
		if (theNode->SpewWaterTimer > .05f)
		{
			theNode->SpewWaterTimer = 0;
			
			
			
				/* MAKE PARTICLE GROUP */

			if (!VerifyParticleGroup(theNode->ParticleGroup))
			{
				theNode->ParticleGroup = NewParticleGroup(
														PARTICLE_TYPE_FALLINGSPARKS,// type
														PARTICLE_FLAGS_BOUNCE,		// flags
														700,						// gravity
														0,							// magnetism
														25,							// base scale
														-1.2,						// decay rate
														.6,							// fade rate
														PARTICLE_TEXTURE_PATCHY);	// texture
			}
			
			
					/* ADD PARTICLES */
					
			if (theNode->ParticleGroup != -1)
			{
				TQ3Vector3D	delta;
				
				for (i = 0; i < 3; i++)
				{
					delta.x = (RandomFloat()-.5f) * 150.0f;
					delta.y = 0;
					delta.z = (RandomFloat()-.5f) * 150.0f;
					AddParticleToGroup(theNode->ParticleGroup, &theNode->InitCoord, &delta, 1.0f, FULL_ALPHA);
				}
			}
		}
	}
	
		/* NOT SPEWING, SO STOP SOUND IF ANY */
		
	else
		StopAChannel(&theNode->EffectChannel);
	
}


#pragma mark -

/************************* ADD WOOD POST *********************************/

Boolean AddWoodPost(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
				
	if (gLevelType != LEVEL_TYPE_FOREST)
		DoFatalAlert("AddWoodPost: not on this level!");
				
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= FOREST_MObjType_WoodPost;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainHeightAtCoord(x,z,FLOOR) - 30.0f;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 70;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 10;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC|CTYPE_IMPENETRABLE;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,5000,-300,-550,550,550,-550);

	return(true);													// item was added
}














