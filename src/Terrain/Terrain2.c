/****************************/
/*   	TERRAIN2.C 	        */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static Boolean NilAdd(TerrainItemEntryType *itemPtr,long x, long z);


/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/

int						gMaxItemsAllocatedInAPass = 0;			// used for debug
short	  				gNumTerrainItems;
TerrainItemEntryType	**gTerrainItemLookupTableX = nil;
TerrainItemEntryType 	**gMasterItemList = nil;

Ptr						gMaxItemAddress;			// addr of last item in current item list

TerrainYCoordType		**gMapYCoords = nil;		// 2D array of map vertex y coords

TerrainInfoMatrixType	**gMapInfoMatrix = nil;


/**********************/
/*     TABLES         */
/**********************/

#define	MAX_ITEM_NUM	63					// for error checking!

static Boolean (*gTerrainItemAddRoutines[])(TerrainItemEntryType *, long, long) =
{
		NilAdd,								// My Start Coords
		AddLadyBugBonus,					// 1: LadyBug Bonus
		AddNut,								// 2: Nut
		AddEnemy_BoxerFly,					// 3: ENEMY: BOXERFLY
		AddRock,							// 4: Rock
		AddClover,							// 5: Clover
		AddGrass,							// 6: Grass
		AddWeed,							// 7: Weed
		NilAdd,								// 8: Slug enemy
		AddEnemy_Ant,						// 9: Ant
		AddSunFlower,						// 10: Sunflower
		AddCosmo,							// 11: Cosmo
		AddPoppy,							// 12: Poppy
		AddWallEnd,							// 13: Wall End
		AddWaterPatch,						// 14: Water Patch
		AddEnemy_FireAnt,					// 15: FireAnt
		AddWaterBug,						// 16: WaterBug
		AddTree,							// 17: Tree (flight level)
		AddDragonFly,						// 18: Dragonfly
		AddCatTail,							// 19: Cat Tail
		AddDuckWeed,						// 20: Duck Weed
		AddLilyFlower,						// 21: Lily Flower
		AddLilyPad,							// 22: Lily Pad
		AddPondGrass,						// 23: Pond Grass
		AddReed,							// 24: Reed
		AddEnemy_PondFish,					// 25: Pond Fish Enemy
		AddHoneycombPlatform,				// 26: Honeycomb platform
		AddHoneyPatch,						// 27: Honey Patch
		AddFirecracker,						// 28: Firecracker
		AddDetonator,						// 29: Detonator
		AddHiveDoor,						// 30: Hive Door
		AddEnemy_Mosquito,					// 31: Mosquito Enemy
		AddCheckpoint,						// 32: Checkpoint
		AddLawnDoor,						// 33: Lawn Door
		AddDock,							// 34: Dock
		NilAdd,								// 35: Foot
		AddEnemy_Spider,					// 36: ENEMY: SPIDER
		NilAdd,								// 37: ENEMY: CATERPILLER
		AddFireFly,							// 38: Firefly
		AddExitLog,							// 39: Exit Log
		AddRootSwing,						// 40: Root swing
		AddThorn,							// 41: Thorn Bush
		NilAdd,								// 42: FireFly Target Location
		AddFireWall,						// 43: Fire Wall
		AddWaterValve,						// 44: Water Valve
		AddHoneyTube,						// 45: Honey Tube
		AddEnemy_Larva,						// 46: ENEMY: LARVA
		AddEnemy_FlyingBee,					// 47: ENEMY: FLYING BEE
		AddEnemy_WorkerBee,					// 48: ENEMY: WORKER BEE
		AddEnemy_QueenBee,					// 49: ENEMY: QUEEN BEE
		AddRockLedge,						// 50: Rock Ledge
		AddStump,							// 51: Stump
		AddRollingBoulder,					// 52: Rolling Boulder
		AddEnemy_Roach,						// 53: ENEMY: ROACH
		AddEnemy_Skippy,					// 54: ENEMY: SKIPPY
		AddSlimePatch,						// 55: Slime Patch
		AddLavaPatch,						// 56: Lava Patch
		AddBentAntPipe,						// 57: Bent Ant Pipe
		AddHorizAntPipe,					// 58: Horiz Ant Pipe
		AddEnemy_KingAnt,					// 59: ENEMY: KING ANT
		AddFaucet,							// 60: Water Faucet
		AddWoodPost,						// 61: Wooden Post
		AddFloorSpike,						// 62: Floor Spike
		AddKingWaterPipe,					// 63: King Water Pipe
};


/********************* BUILD TERRAIN ITEM LIST ***********************/
//
// Build sorted lists of terrain items
//

void BuildTerrainItemList(void)
{
long			col,itemCol,itemNum,nextCol,prevCol;
TerrainItemEntryType *lastPtr,*itemPtr;

			/* ALLOC MEMORY FOR LOOKUP TABLE */

	if (gTerrainItemLookupTableX)
	{
		DisposePtr((Ptr)gTerrainItemLookupTableX);
		gTerrainItemLookupTableX = nil;
	}
		
	gTerrainItemLookupTableX = (TerrainItemEntryType **)AllocPtr(sizeof(TerrainItemEntryType *)*gNumSuperTilesWide);
	GAME_ASSERT(gTerrainItemLookupTableX);

	if (gNumTerrainItems == 0)
		return;

	itemPtr = *gMasterItemList; 									// get pointer to data inside the LOCKED handle


				/* BUILD HORIZ LOOKUP TABLE */

	gMaxItemAddress = (Ptr)&itemPtr[gNumTerrainItems-1]; 			// remember addr of last item

	lastPtr = &itemPtr[0];
	nextCol = 0;													// start @ col 0
	prevCol = -1;

	for (itemNum = 0; itemNum < gNumTerrainItems; itemNum++)
	{
		itemCol = itemPtr[itemNum].x / (SUPERTILE_SIZE*OREOMAP_TILE_SIZE);	// get column of item (supertile relative)
		if (itemCol >= gNumSuperTilesWide)
		{
			DoFatalAlert("Warning! Item off right side of universe!");
		}
		else
		if (itemCol < 0)
		{
			DoAlert("Warning! Item off left side of universe!");
			goto trail;
		}

		if (itemCol < prevCol)										// see if ERROR - list not sorted correctly!!!
			DoAlert("Error! ObjectList not sorted right!");

		if (itemCol > prevCol)										// see if changed
		{
			for (col = nextCol; col <= itemCol; col++)				// filler pointers
				gTerrainItemLookupTableX[col] = &itemPtr[itemNum];

			prevCol = itemCol;
			nextCol = itemCol+1;
			lastPtr = &itemPtr[itemNum];
		}
	}
trail:
	for (col = nextCol; col < gNumSuperTilesWide; col++)				// set trailing column pointers
	{
		gTerrainItemLookupTableX[col] = lastPtr;
	}
	

			/* FIGURE OUT WHERE THE STARTING POINT IS */
			
	FindMyStartCoordItem();										// look thru items for my start coords
	
}



/******************** FIND MY START COORD ITEM *******************/
//
// Scans thru item list for item type #14 which is a teleport reciever / start coord,
// or scans for receiving teleporter (#11) if gTeleportInfo.activateFlag is set.
//

void FindMyStartCoordItem(void)
{
long					i;
TerrainItemEntryType	*itemPtr;

	itemPtr = *gMasterItemList; 												// get pointer to data inside the LOCKED handle

				/* SCAN FOR "START COORD" ITEM */

	for (i= 0; i < gNumTerrainItems; i++)
		if (itemPtr[i].type == MAP_ITEM_MYSTARTCOORD)						// see if it's a MyStartCoord item
		{
			gMyCoord.x = gMyStartX = (itemPtr[i].x * MAP2UNIT_VALUE);		// convert to world coords
			gMyCoord.z = gMyStartZ = itemPtr[i].y * MAP2UNIT_VALUE;
			gMyStartAim = itemPtr[i].parm[0];								// get aim 0..7
			goto gotit;
		}

	DoAlert("No Start Coord or Teleporter Item Found!");

	gMyStartX = 0;
	gMyStartZ = 0;

gotit:	
	gMostRecentCheckPointCoord.x = gMyStartX;
	gMostRecentCheckPointCoord.z = gMyStartZ;
	gMostRecentCheckPointCoord.y = 0;
}



/****************** SCAN FOR PLAYFIELD ITEMS *******************/
//
// Given this range, scan for items.  Coords are in supertile relative row/col values.
//

void ScanForPlayfieldItems(long top, long bottom, long left, long right)
{
TerrainItemEntryType *itemPtr;
long			type,n;
Boolean			flag;
long			maxX,minX,maxY,minY;
long			realX,realZ;

	if (gNumTerrainItems == 0)
		return;

	itemPtr = gTerrainItemLookupTableX[left];					// get pointer to 1st item at this X
	if ((Ptr)itemPtr > gMaxItemAddress)					// see if out of bounds			
		return;

	minX = left*(SUPERTILE_SIZE*OREOMAP_TILE_SIZE);
	maxX = right*(SUPERTILE_SIZE*OREOMAP_TILE_SIZE);			// calc min/max coords to be in range (map relative)
	maxX += (SUPERTILE_SIZE*OREOMAP_TILE_SIZE)-1;

	minY = top*(SUPERTILE_SIZE*OREOMAP_TILE_SIZE);
	maxY = bottom*(SUPERTILE_SIZE*OREOMAP_TILE_SIZE);
	maxY += (SUPERTILE_SIZE*OREOMAP_TILE_SIZE)-1;

	n = 0;														// init counter

	while ((itemPtr->x >= minX) && (itemPtr->x <= maxX)) 		// check all items in this column range
	{
		if ((itemPtr->y >= minY) && (itemPtr->y <= maxY))		// & this row range
		{
					/* ADD AN ITEM */

			if (itemPtr->flags&ITEM_FLAGS_INUSE)				// see if item available
				goto skip;
				
			type = itemPtr->type;								// get item #
			if (type > MAX_ITEM_NUM)							// error check!
			{
				DoAlert("Illegal Map Item Type! (%d)", type);
			}

			realX = itemPtr->x * MAP2UNIT_VALUE;				// calc & pass 3-space coords
			realZ = itemPtr->y * MAP2UNIT_VALUE;
	
			flag = gTerrainItemAddRoutines[type](itemPtr,realX, realZ); // call item's ADD routine
			if (flag)
				itemPtr->flags |= ITEM_FLAGS_INUSE;				// set in-use flag
				
			n++;												// inc counter
		}
skip:		
		itemPtr++;												// point to next item in list
		if ((Ptr)itemPtr > gMaxItemAddress)
			break;
	}
	
	if (n > gMaxItemsAllocatedInAPass)							// update this for debug purposes
		gMaxItemsAllocatedInAPass = n;
	
}


/******************** NIL ADD ***********************/
//
// nothing add
//

static Boolean NilAdd(TerrainItemEntryType *itemPtr,long x, long z)
{
	(void) itemPtr;
	(void) x;
	(void) z;

	return(false);
}

/***************** IS POSITION OUT OF RANGE ******************/
//
// Returns true if position is out of range
//

Boolean IsPositionOutOfRange(float x, float z)
{
	return x < gTerrainItemDeleteWindow_Left
		|| x > gTerrainItemDeleteWindow_Right
		|| z < gTerrainItemDeleteWindow_Far
		|| z > gTerrainItemDeleteWindow_Near;
}

/***************** IS POSITION OUT OF RANGE ******************/
//
// Returns true if position is out of range
//
// INPUT: range = INTEGER range to add to delete window
//

Boolean IsPositionOutOfRange_Far(float x, float z, float range)
{
	return x < gTerrainItemDeleteWindow_Left - range
		|| x > gTerrainItemDeleteWindow_Right + range
		|| z < gTerrainItemDeleteWindow_Far + range
		|| z > gTerrainItemDeleteWindow_Near - range;
}

/***************** TRACK TERRAIN ITEM ******************/
//
// Returns true if theNode is out of range
//

Boolean TrackTerrainItem(ObjNode *theNode)
{
	return IsPositionOutOfRange(theNode->Coord.x, theNode->Coord.z);
}

/***************** TRACK TERRAIN ITEM FAR ******************/
//
// Returns true if theNode is out of range
//
// INPUT: range = INTEGER range to add to delete window
//

Boolean TrackTerrainItem_Far(ObjNode *theNode, float range)
{
	return IsPositionOutOfRange_Far(theNode->Coord.x, theNode->Coord.z, range);
}

/*************************** ROTATE ON TERRAIN ***************************/
//
// Rotates an object's x & z such that it's lying on the terrain.
//

void RotateOnTerrain(ObjNode *theNode, float yOffset)
{
TQ3Vector3D	up;
float		r,x,z,y;
TQ3Point3D	to;
TQ3Matrix4x4	*m,m2;

			/* GET CENTER Y COORD & TERRAIN NORMAL */
			
	x = theNode->Coord.x;
	z = theNode->Coord.z;
	y = theNode->Coord.y = GetTerrainHeightAtCoord(x, z, FLOOR) + yOffset;	
	up = gRecentTerrainNormal[FLOOR];
	
	
			/* CALC "TO" COORD */
			
	r = theNode->Rot.y;
	to.x = x + sin(r) * 30.0f;
	to.z = z + cos(r) * 30.0f;
	to.y = GetTerrainHeightAtCoord(to.x, to.z, FLOOR) + yOffset;	
	
	
			/* CREATE THE MATRIX */
	
	m = &theNode->BaseTransformMatrix;
	SetLookAtMatrix(m, &up, &theNode->Coord, &to);
	
	
		/* POP IN THE TRANSLATE INTO THE MATRIX */
			
	m->value[3][0] = x;
	m->value[3][1] = y;
	m->value[3][2] = z;


			/* SET SCALE */
					
	Q3Matrix4x4_SetScale(&m2, theNode->Scale.x,				// make scale matrix
							 	theNode->Scale.y,			
							 	theNode->Scale.z);
	MatrixMultiply(&m2, m, m);
}



#pragma mark ======= TERRAIN PRE-CONSTRUCTION =========




/********************** CALC TILE NORMALS *****************************/
//
// Given a row, col coord, calculate the face normals for the 2 triangles.
//

void CalcTileNormals(long layer, long row, long col, TQ3Vector3D *n1, TQ3Vector3D *n2)
{
static TQ3Point3D	p1 = {0,0,0};
static TQ3Point3D	p2 = {TERRAIN_POLYGON_SIZE,0,0};
static TQ3Point3D	p3 = {TERRAIN_POLYGON_SIZE,0,TERRAIN_POLYGON_SIZE};
static TQ3Point3D	p4 = {0, 0, TERRAIN_POLYGON_SIZE};


		/* MAKE SURE ROW/COL IS IN RANGE */
			
	if ((row >= gTerrainTileDepth) || (row < 0) ||
		(col >= gTerrainTileWidth) || (col < 0))
	{
		n1->x = n2->x = 0;						// pass back up vector by default since our of range
		n1->y = n2->y = 1;
		n1->z = n2->z = 0;
		return;
	}

	p1.y = gMapYCoords[row][col].layerY[layer];		// far left	
	p2.y = gMapYCoords[row][col+1].layerY[layer];	// far right	
	p3.y = gMapYCoords[row+1][col+1].layerY[layer];	// near right	
	p4.y = gMapYCoords[row+1][col].layerY[layer];	// near left
	
	
		/* CALC NORMALS BASED ON SPLIT */

	if (layer == 0)									// normal direction varies if floor or ceiling
	{
	
				/* COUNTER-CLOCKWISE FOR FLOOR */
				
		if (gMapInfoMatrix[row][col].splitMode[0] == SPLIT_BACKWARD)
		{
			CalcFaceNormal(&p1, &p4, &p3, n1);		// fl, nl, nr
			CalcFaceNormal(&p1, &p3, &p2, n2);		// f1, nr, fr
		}
		else
		{
			CalcFaceNormal(&p1, &p4, &p2, n1);		// fl, nl, fr
			CalcFaceNormal(&p2, &p4, &p3, n2);		// fr, nl, nr
		}
	}
	else
	{
			/* CLOCKWISE FOR CEILING */
	
		if (gMapInfoMatrix[row][col].splitMode[1] == SPLIT_BACKWARD)
		{
			CalcFaceNormal(&p3, &p4, &p1, n1);		// nr, nl, fl
			CalcFaceNormal(&p2, &p3, &p1, n2);		// fr, nr, fl
		}
		else
		{
			CalcFaceNormal(&p2, &p4, &p1, n1);		// fr, nl, fl
			CalcFaceNormal(&p3, &p4, &p2, n2);		// nr, nl, fr
		}
	}
}


/****************** DO ITEM SHADOW CASTING **********************/
//
// Scans thru item list and casts a shadown onto the terrain
// by darkening the vertex colors of the terrain.
//

void DoItemShadowCasting(void)
{
long				i;
static TQ3Vector3D up = {0,1,0};
float				height,dot,length;
TQ3Vector2D			lightVector;
TQ3Point2D			from,to;
float				x,z,t;
long				row,col;
Byte				**shadowFlags;

			/* INIT SHADOW FLAGS TEMP BUFFER */
			
	Alloc_2d_array(Byte, shadowFlags, gTerrainTileDepth+1, gTerrainTileWidth+1);

	for (row = 0; row <= gTerrainTileDepth; row++)
		for (col = 0; col <= gTerrainTileWidth; col++)
			shadowFlags[row][col] = 0;


			/* GET MAIN LIGHT VECTOR INFO */

	lightVector.x = gGameViewInfoPtr->lightList.fillDirection[0].x;
	lightVector.y = gGameViewInfoPtr->lightList.fillDirection[0].z;
	Q3Vector2D_Normalize(&lightVector, &lightVector);

	dot = -Q3Vector3D_Dot(&up,&gGameViewInfoPtr->lightList.fillDirection[0]);
	dot = 1.0 - dot;
	
			/***********************/
			/* SCAN THRU ITEM LIST */
			/***********************/
			
	for (i = 0; i < gNumTerrainItems; i++)
	{
			/* SEE WHICH THINGS WE SUPPORT & GET PARMS */
				
		switch((*gMasterItemList)[i].type)
		{
			case	5:						// clover
					height = 1000;
					break;
					
			case	6:						// grass
					height = 1200;
					break;
					
			case	7:						// weed
					height = 2500;
					break;
					
			case	10:						// sunflower
					height = 3000;
					break;

			case	11:						// cosmo
					height = 2000;
					break;

			case	12:						// poppy
					height = 2000;
					break;

			case	19:						// cattail
					height = 3000;
					break;

			case	20:						// duckweed
					height = 2500;
					break;

			case	21:						// lily flower
					height = 2000;
					break;

			case	22:						// lily pad
					height = 2000;
					break;
					
			case	23:						// pond grass
					height = 2500;
					break;

			case	24:						// reed
					height = 2500;
					break;

			case	45:						// honey tube
					height = 1200;
					break;

			case	51:						// tree stump
					height = 3000;
					break;

			case	57:						// ant pipe
			case	58:
					height = 1000;
					break;

			case	60:						// faucet
					height = 1200;
					break;

			case	63:						// king pipe
					height = 1200;
					break;
					
			default:
					continue;		
		}
		
			/* CALCULATE LINE TO DRAW SHADOW ALONG */
			
		from.x = (int)(*gMasterItemList)[i].x * MAP2UNIT_VALUE;
		from.y = (int)(*gMasterItemList)[i].y * MAP2UNIT_VALUE;
				
		to.x = from.x + lightVector.x * (height * dot);
		to.y = from.y + lightVector.y * (height * dot);
		
		length = Q3Point2D_Distance(&from, &to);
		
		
			/***************************************/
			/* SCAN ALONG LIGHT AND SHADE VERTICES */
			/***************************************/
					
		for (t = 1.0; t > 0.0f; t -= 1.0f / (length/TERRAIN_POLYGON_SIZE))
		{
			float	oneMinusT = 1.0f - t;
			float	r,g,b;
			float	ro,co;
			u_short	*color;
			
			x = (from.x * oneMinusT) + (to.x * t);			// calc center x
			z = (from.y * oneMinusT) + (to.y * t);
		
			for (ro = -.5; ro <= .5; ro += .5)
			{
				for (co = -.5; co <= .5; co += .5)
				{
					row = z / TERRAIN_POLYGON_SIZE + ro;			// calc row/col
					col = x / TERRAIN_POLYGON_SIZE + co;
		
					if ((row < 0) || (col < 0))						// check for out of bounds
						continue;
					if ((row >= gTerrainTileDepth) || (col >= gTerrainTileWidth))	
						continue;
					
					if (shadowFlags[row][col])						// see if this already shadowed
						continue;
		
					shadowFlags[row][col] = 1;						// set flag
					
				
						/* EXTRACT RGB */
						
					color = &gVertexColors[FLOOR][row][col];
					
					r = (*color >> 11);
					r /= 0x1f;
					g = (*color >> 5) & 0x3f;
					g /= 0x3f;
					b = (*color & 0x1f);
					b /= 0x1f;
					
						/* FADE IT */
						
					r *= .7f;
					g *= .7f;
					b *= .7f;
					
					
						/* SAVE RGB */
						
					*color = (int)(r*(float)0x1f)<<11;
					*color |= (int)(g*(float)0x3f)<<5;
					*color |= (int)(b*(float)0x1f);	
				}// co
			} // ro
		}		
	}
	
	
			/* CLEANUP */
			
	Free2DArray((void**) shadowFlags);
	shadowFlags = nil;
}


















