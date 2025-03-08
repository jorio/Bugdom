/****************************/
/*     TERRAIN.C           */
/* By Brian Greenstone      */
/* (C)1999 Pangea Software  */
/* (C)2023 Iliyas Jorio     */
/****************************/

/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*  PROTOTYPES             */
/****************************/

static void ScrollTerrainUp(long superRow, long superCol);
static void ScrollTerrainDown(long superRow, long superCol);
static void ScrollTerrainLeft(void);
static void ScrollTerrainRight(long superCol, long superRow, long tileCol, long tileRow);
static short GetFreeSuperTileMemory(void);
static inline void ReleaseSuperTileObject(int32_t superTileNum);
static void CalcNewItemDeleteWindow(void);
static short	BuildTerrainSuperTile(long	startCol, long startRow);
static Boolean IsSuperTileVisible(int32_t superTileNum, Byte layer);
static void DrawTileIntoMipmap(uint16_t tile, int row, int col, uint16_t* buffer);
static void	ShrinkSuperTileTextureMap(const u_short *srcPtr,u_short *destPtr);
//static void	ShrinkSuperTileTextureMapTo64(u_short *srcPtr,u_short *destPtr);
static void ShrinkHalf(const uint16_t* input, uint16_t* output, int outputSize);
static inline void ReleaseAllSuperTiles(void);
static void BuildSuperTileLOD(SuperTileMemoryType *superTilePtr, short lod);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	ITEM_WINDOW		1			// # supertiles for item add window (must be integer)
#define	OUTER_SIZE		0.6f		// size of border out of add window for delete window (can be float)

#define TILE_TEXTURE_INTERNAL_FORMAT	GL_RGB
#define TILE_TEXTURE_FORMAT				GL_BGRA_EXT
#define TILE_TEXTURE_TYPE				GL_UNSIGNED_SHORT_1_5_5_5_REV


/**********************/
/*     VARIABLES      */
/**********************/

static int		gTerrainTextureDetail = SUPERTILE_DETAIL_BEST;
int				gSuperTileActiveRange;

Boolean			gDoCeiling;
Boolean			gDisableHiccupTimer = false;

static int		gNumLODs = 0;

static u_char	gHiccupEliminator = 0;

u_short	**gTileDataHandle;

u_short	**gFloorMap = nil;								// 2 dimensional array of u_shorts (allocated below)
u_short	**gCeilingMap = nil;
u_short	**gVertexColors[MAX_LAYERS];

long	gTerrainTileWidth,gTerrainTileDepth;			// width & depth of terrain in tiles
long	gTerrainUnitWidth,gTerrainUnitDepth;			// width & depth of terrain in world units (see TERRAIN_POLYGON_SIZE)

long	gNumTerrainTextureTiles = 0;

long	gNumSuperTilesDeep,gNumSuperTilesWide;	  		// dimensions of terrain in terms of supertiles
long	gCurrentSuperTileRow,gCurrentSuperTileCol;

int32_t	gTerrainScrollBuffer[MAX_SUPERTILES_DEEP][MAX_SUPERTILES_WIDE];		// 2D array which has index to supertiles for each possible supertile

long	gNumFreeSupertiles = 0;
long	gSupertileBudget = 0;
static	SuperTileMemoryType		gSuperTileMemoryList[MAX_SUPERTILES];
Boolean gSuperTileMemoryListExists = false;

float	gTerrainItemDeleteWindow_Near,gTerrainItemDeleteWindow_Far,
		gTerrainItemDeleteWindow_Left,gTerrainItemDeleteWindow_Right;

static int		gTextureSizePerLOD[MAX_LODS] = { 0, 0, 0 };

static RenderModifiers gTerrainRenderMods;

			/* TILE SPLITTING TABLES */
			
					
					/* /  */
static const	Byte			gTileTriangles1_A[SUPERTILE_SIZE][SUPERTILE_SIZE][3] =
{
	{ { 6, 0, 1}, { 7, 1, 2}, { 8, 2, 3}, { 9, 3, 4}, {10, 4, 5} },
	{ {12, 6, 7}, {13, 7, 8}, {14, 8, 9}, {15, 9,10}, {16,10,11} },
	{ {18,12,13}, {19,13,14}, {20,14,15}, {21,15,16}, {22,16,17} },
	{ {24,18,19}, {25,19,20}, {26,20,21}, {27,21,22}, {28,22,23} },
	{ {30,24,25}, {31,25,26}, {32,26,27}, {33,27,28}, {34,28,29} },
};

static const	Byte			gTileTriangles2_A[SUPERTILE_SIZE][SUPERTILE_SIZE][3] =
{
	{ { 6, 1, 7}, { 7, 2, 8}, { 8, 3, 9}, {	9, 4,10}, {10, 5,11} },
	{ {12, 7,13}, {13, 8,14}, {14, 9,15}, {15,10,16}, {16,11,17} },
	{ {18,13,19}, {19,14,20}, {20,15,21}, {21,16,22}, {22,17,23} },
	{ {24,19,25}, {25,20,26}, {26,21,27}, {27,22,28}, {28,23,29} },
	{ {30,25,31}, {31,26,32}, {32,27,33}, {33,28,34}, {34,29,35} },
};

					/* \  */
static const	Byte			gTileTriangles1_B[SUPERTILE_SIZE][SUPERTILE_SIZE][3] =
{
	{ { 0, 7, 6}, { 1, 8, 7}, { 2, 9, 8}, { 3,10, 9}, { 4,11,10} },
	{ { 6,13,12}, { 7,14,13}, { 8,15,14}, { 9,16,15}, {10,17,16} },
	{ {12,19,18}, {13,20,19}, {14,21,20}, {15,22,21}, {16,23,22} },
	{ {18,25,24}, {19,26,25}, {20,27,26}, {21,28,27}, {22,29,28} },
	{ {24,31,30}, {25,32,31}, {26,33,32}, {27,34,33}, {28,35,34} },
};

static const Byte   gTileTriangles2_B[SUPERTILE_SIZE][SUPERTILE_SIZE][3] =
{
	{ { 0, 1, 7}, { 1, 2, 8}, { 2, 3, 9}, { 3, 4,10}, { 4, 5,11} },
	{ { 6, 7,13}, { 7, 8,14}, { 8, 9,15}, { 9,10,16}, {10,11,17} },
	{ {12,13,19}, {13,14,20}, {14,15,21}, {15,16,22}, {16,17,23} },
	{ {18,19,25}, {19,20,26}, {20,21,27}, {21,22,28}, {22,23,29} },
	{ {24,25,31}, {25,26,32}, {26,27,33}, {27,28,34}, {28,29,35} },
};

static const	Byte			gTileTriangleWinding[2][3] =
{
	{ 2, 1, 0 },  // floor
	{ 0, 1, 2 },  // ceiling
};



TQ3Point3D		gWorkGrid[SUPERTILE_SIZE+1][SUPERTILE_SIZE+1];
uint16_t		*gTempTextureBuffer = nil;

TQ3Vector3D		gRecentTerrainNormal[2];							// from _Planar



/****************** INIT TERRAIN MANAGER ************************/
//
// Only called at boot!
//

void InitTerrainManager(void)
{
	ClearScrollBuffer();
	
	
			/* ALLOC TEMP TEXTURE BUFF */
			//
			// This is the full 160x160 buffer that tiles are drawn into.
			//
			
	if (gTempTextureBuffer == nil)
	{
		gTempTextureBuffer = (uint16_t*) AllocPtr(SUPERTILE_TEXSIZE_MAX * SUPERTILE_TEXSIZE_MAX * sizeof(uint16_t));
		GAME_ASSERT(gTempTextureBuffer);
	}


			/* INIT RENDER MODIFIERS */

	Render_SetDefaultModifiers(&gTerrainRenderMods);
	gTerrainRenderMods.statusBits |= STATUS_BIT_NULLSHADER;
	gTerrainRenderMods.drawOrder = kDrawOrder_Terrain;
}


/****************** INIT CURRENT SCROLL SETTINGS *****************/

void InitCurrentScrollSettings(void)
{
long	x,y;
long	dummy1,dummy2;

	x = gMostRecentCheckPointCoord.x-(SUPERTILE_ACTIVE_RANGE*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE);
	y = gMostRecentCheckPointCoord.z-(SUPERTILE_ACTIVE_RANGE*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE);
	GetSuperTileInfo(x, y, &gCurrentSuperTileCol, &gCurrentSuperTileRow, &dummy1, &dummy2);

			/* INIT THE SCROLL BUFFER */

	ClearScrollBuffer();
	ReleaseAllSuperTiles();			
}


/***************** CLEAR SCROLL BUFFER ************************/

void ClearScrollBuffer(void)
{
long	row,col;

	for (row = 0; row < MAX_SUPERTILES_DEEP; row++)
		for (col = 0; col < MAX_SUPERTILES_WIDE; col++)
			gTerrainScrollBuffer[row][col] = EMPTY_SUPERTILE;
			
	gHiccupEliminator = 0;
}


/***************** DISPOSE TERRAIN **********************/
//
// Deletes any existing terrain data
//

void DisposeTerrain(void)
{
int	i;

	if (gTileDataHandle)
	{
		DisposeHandle((Handle)gTileDataHandle);
		gTileDataHandle = nil;
	}

	if (gMasterItemList)
	{
		DisposeHandle((Handle)gMasterItemList);
		gMasterItemList = nil;
	}
	
	if (gTerrainItemLookupTableX != nil)
	{
	  	DisposePtr((Ptr)gTerrainItemLookupTableX);
	  	gTerrainItemLookupTableX = nil;
	}

	if (gFloorMap != nil)
	{
		Free2DArray((void**) gFloorMap);
		gFloorMap = nil;
	}

	if (gCeilingMap != nil)
	{
		Free2DArray((void**) gCeilingMap);
		gCeilingMap = nil;
	}

	if (gMapYCoords)
	{
		Free2DArray((void**) gMapYCoords);
		gMapYCoords = nil;
	}
	
	if (gMapInfoMatrix)
	{
		Free2DArray((void**) gMapInfoMatrix);
		gMapInfoMatrix = nil;
	}

	if (gVertexColors[0])
	{
		Free2DArray((void**) gVertexColors[0]);
		gVertexColors[0] = nil;
	}
	if (gVertexColors[1])
	{
		Free2DArray((void**) gVertexColors[1]);
		gVertexColors[1] = nil;
	}
			/* NUKE SPLINE DATA */

	if (gSplineList)
	{
		for (i = 0; i < gNumSplines; i++)
		{
			DisposeHandle((Handle)(*gSplineList)[i].nubList);	// nuke nub list
			DisposeHandle((Handle)(*gSplineList)[i].pointList);	// nuke point list
			DisposeHandle((Handle)(*gSplineList)[i].itemList);	// nuke item list
		}
		DisposeHandle((Handle) gSplineList);
		gSplineList = nil;										// make sure to clear handle to prevent double-free next time
	}

			/* NUKE FENCE DATA */

	if (gFenceList)
	{
		for (i = 0; i < gNumFences; i++)
		{
			DisposePtr((Ptr)gFenceList[i].sectionVectors);		// nuke section vectors
			DisposeHandle((Handle)(gFenceList[i].nubList));		// nuke nub list
		}
		DisposePtr((Ptr) gFenceList);
		gFenceList = nil;										// make sure to clear pointer to prevent double-free next time
	}

	
	ReleaseAllSuperTiles();
}


#pragma mark -

/************** CREATE SUPERTILE MEMORY LIST ********************/

void CreateSuperTileMemoryList(void)
{
long							u,v,i,numLayers;
static 	TQ3TriMeshTriangleData	newTriangle[NUM_TRIS_IN_SUPERTILE];
static	TQ3Param2D				uvs[NUM_VERTICES_IN_SUPERTILE];



	gSupertileBudget = gSuperTileActiveRange * gSuperTileActiveRange * 4;		// calc # supertiles we will need

	long upperBound = gNumSuperTilesDeep * gNumSuperTilesWide;					// if we have the budget to show the entire map at once,
	if (gSupertileBudget > upperBound)											// cap supertile budget to # of supertiles in map
		gSupertileBudget = upperBound;

	GAME_ASSERT(gSupertileBudget <= MAX_SUPERTILES);

#if _DEBUG
	SDL_Log("Supertile budget: %ld\n", gSupertileBudget);
#endif

	if (gDoCeiling)
		numLayers = 2;
	else
		numLayers = 1;


			/* PREPARE TEXTURE DETAIL CONSTANTS ACCORDING TO USER PREFS */

	// Fill out gNumLODs and textureSize[] according to user pref for texture detail (source port addition)
	gTerrainTextureDetail = gGamePrefs.lowDetail
		? SUPERTILE_DETAIL_WORST
		: SUPERTILE_DETAIL_BEST;

retryParseLODPref:
	switch (gTerrainTextureDetail)
	{
	case SUPERTILE_DETAIL_PROGRESSIVE:
		gNumLODs = 3;
		gTextureSizePerLOD[0] = SUPERTILE_TEXSIZE_SHRUNK;
		gTextureSizePerLOD[1] = SUPERTILE_TEXSIZE_SHRUNK / 2;
		gTextureSizePerLOD[2] = SUPERTILE_TEXSIZE_SHRUNK / 4;
		break;

	case SUPERTILE_DETAIL_SHRUNK:
		gNumLODs = 1;
		gTextureSizePerLOD[0] = SUPERTILE_TEXSIZE_SHRUNK;
		break;

	case SUPERTILE_DETAIL_LOSSLESS:
		gNumLODs = 1;
		gTextureSizePerLOD[0] = SUPERTILE_TEXSIZE_LOSSLESS;
		break;

	case SUPERTILE_DETAIL_SEAMLESS:
		gNumLODs = 1;
		gTextureSizePerLOD[0] = SUPERTILE_TEXSIZE_SEAMLESS;
		break;

	default:
		DoAlert("Unknown terrain texture detail pref! (%d)", gTerrainTextureDetail);

		// Set a sane fallback value and try again
		gTerrainTextureDetail = SUPERTILE_DETAIL_BEST;
		goto retryParseLODPref;
	}


	
			/* INIT UV LIST */
	
	i = 0;	
	if (gTerrainTextureDetail == SUPERTILE_DETAIL_SEAMLESS)
	{
		for (v = 0; v <= SUPERTILE_SIZE; v++)						// sets uv's 0.0 -> 1.0 for single texture map
		{
			for (u = 0; u <= SUPERTILE_SIZE; u++)
			{
				uvs[i].u = (1.0f + u) / (2.0f + SUPERTILE_SIZE);
				uvs[i].v = (1.0f + v) / (2.0f + SUPERTILE_SIZE);
				i++;
			}	
		}
	}
	else
	{
		for (v = 0; v <= SUPERTILE_SIZE; v++)						// sets uv's 0.0 -> 1.0 for single texture map
		{
			for (u = 0; u <= SUPERTILE_SIZE; u++)
			{
				uvs[i].u = (float)u / (float)SUPERTILE_SIZE;
				uvs[i].v = (float)v / (float)SUPERTILE_SIZE;
				i++;
			}	
		}
	}


#if 0
		/* INIT COLOR LIST */
		
	i = 0;	
	for (v = 0; v <= SUPERTILE_SIZE; v++)						// sets uv's 0.0 -> 1.0 for single texture map
	{
		for (u = 0; u <= SUPERTILE_SIZE; u++)
		{
			vertexColors[i].r = 1;
			vertexColors[i].g = 1;
			vertexColors[i].b = 1;
			i++;
		}	
	}
#endif

			/********************************************/
			/* FOR EACH POSSIBLE SUPERTILE ALLOC MEMORY */
			/********************************************/

	gNumFreeSupertiles = 0;

	for (i = 0; i < gSupertileBudget; i++)
	{
		SuperTileMemoryType* superTile = &gSuperTileMemoryList[i];

		superTile->mode = SUPERTILE_MODE_FREE;									// it's free for use
		gNumFreeSupertiles++;

				/************************************************/
				/* CREATE TEXTURE & TRIMESH FOR FLOOR & CEILING */
				/************************************************/

		for (int layer = 0; layer < numLayers; layer++)							// do it for floor & ceiling trimeshes
		{
				/*****************************/
				/* DO OUR OWN FAUX-LOD THING */
				/*****************************/
				//
				// Normally there are 3 LOD's, but if we are low on memory, then we'll skip LOD #0 which
				// is the big one, and only do the other 2 smaller ones.
				//

			for (int lod = 0; lod < gNumLODs; lod++)
			{
				int size = gTextureSizePerLOD[lod];								// get size of texture @ this lod
				GAME_ASSERT_MESSAGE(size >= 0, "gTextureSizePerLOD not initialized!");

						/* MAKE BLANK TEXTURE */

				superTile->textureData[layer][lod] = (uint16_t*) NewPtrClear(size * size * sizeof(uint16_t));	// alloc memory for texture
				GAME_ASSERT(superTile->textureData[layer][lod]);

				superTile->glTextureName[layer][lod] = Render_LoadTexture(		// create texture from buffer
						TILE_TEXTURE_INTERNAL_FORMAT,
						size,
						size,
						TILE_TEXTURE_FORMAT,
						TILE_TEXTURE_TYPE,
						superTile->textureData[layer][lod],
						kRendererTextureFlags_ClampBoth
				);
				CHECK_GL_ERROR();
				GAME_ASSERT(superTile->glTextureName[layer][lod]);
			}

				/* CREATE AN EMPTY TRIMESH STRUCTURE */

			TQ3TriMeshData* tmd = Q3TriMeshData_New(
					NUM_TRIS_IN_SUPERTILE,
					NUM_VERTICES_IN_SUPERTILE,
					kQ3TriMeshDataFeatureVertexUVs | kQ3TriMeshDataFeatureVertexNormals | kQ3TriMeshDataFeatureVertexColors
			);
			GAME_ASSERT(tmd);

			_Static_assert(sizeof(uvs) == sizeof(tmd->vertexUVs[0]) * NUM_VERTICES_IN_SUPERTILE, "supertile UV array size mismatch");

			SDL_memcpy(tmd->triangles,		newTriangle,	sizeof(tmd->triangles[0]) * NUM_TRIS_IN_SUPERTILE);
			SDL_memcpy(tmd->vertexUVs,		uvs,			sizeof(tmd->vertexUVs[0]) * NUM_VERTICES_IN_SUPERTILE);

			tmd->bBox.isEmpty = kQ3False;										// calc bounding box
			tmd->bBox.min.x = tmd->bBox.min.y = tmd->bBox.min.z = 0;
			tmd->bBox.max.x = tmd->bBox.max.y = tmd->bBox.max.z = TERRAIN_SUPERTILE_UNIT_SIZE;

			tmd->glTextureName = gSuperTileMemoryList[i].glTextureName[layer][0];	// set LOD 0 texture by default
			tmd->texturingMode = kQ3TexturingModeOpaque;

			gSuperTileMemoryList[i].triMeshDataPtrs[layer] = tmd;
		}
	}

	gSuperTileMemoryListExists = true;
}


/********************* DISPOSE SUPERTILE MEMORY LIST **********************/

void DisposeSuperTileMemoryList(void)
{
int		numLayers;

	if (gSuperTileMemoryListExists == false)
		return;

	if (gDoCeiling)
		numLayers = 2;
	else
		numLayers = 1;
		

			/*************************************/
			/* FOR EACH SUPERTILE DEALLOC MEMORY */
			/*************************************/

	const int numSuperTiles = gSupertileBudget;

	for (int i = 0; i < numSuperTiles; i++)
	{
		SuperTileMemoryType* superTile = &gSuperTileMemoryList[i];

		for (int layer = 0; layer < numLayers; layer++)
		{
				/* NUKE TEXTURES */

			for (int lod = 0; lod < gNumLODs; lod++)
			{
				DisposePtr((Ptr) superTile->textureData[layer][lod]);
				superTile->textureData[layer][lod] = nil;
				superTile->hasLOD[lod] = false;

				if (superTile->glTextureName[layer][lod])
				{
					glDeleteTextures(1, &superTile->glTextureName[layer][lod]);
					superTile->glTextureName[layer][lod] = 0;
				}
			}

				/* NUKE TRIMESH DATA */

			Q3TriMeshData_Dispose(gSuperTileMemoryList[i].triMeshDataPtrs[layer]);
			gSuperTileMemoryList[i].triMeshDataPtrs[layer] = nil;
		}
	}
	
	gSuperTileMemoryListExists = false;
}


/***************** GET FREE SUPERTILE MEMORY *******************/
//
// Finds one of the preallocated supertile memory blocks and returns its index
// IT ALSO MARKS THE BLOCK AS USED
//
// OUTPUT: index into gSuperTileMemoryList
//

static short GetFreeSuperTileMemory(void)
{
				/* SCAN FOR A FREE BLOCK */

	for (int32_t i = 0; i < gSupertileBudget; i++)
	{
		if (gSuperTileMemoryList[i].mode == SUPERTILE_MODE_FREE)
		{
			gSuperTileMemoryList[i].mode = SUPERTILE_MODE_USED;
			gNumFreeSupertiles--;
			GAME_ASSERT(gNumFreeSupertiles >= 0);
			return(i);
		}
	}

	DoFatalAlert("No Free Supertiles!");
	return(-1);											// ERROR, NO FREE BLOCKS!!!! SHOULD NEVER GET HERE!
}

#pragma mark -


/******************* BUILD TERRAIN SUPERTILE *******************/
//
// Builds a new supertile which has scrolled on
//
// INPUT: startCol = starting column in map
//		  startRow = starting row in map
//
// OUTPUT: index to supertile
//

static short	BuildTerrainSuperTile(long	startCol, long startRow)
{
long	 			row,col,row2,col2;
int32_t				superTileNum;
float				height,miny,maxy;
TQ3TriMeshData		*triMeshData;
TQ3Vector3D			*vertexNormalList;
u_short				tile;
TQ3Point3D			*pointList;
TQ3TriMeshTriangleData	*triangleList;
SuperTileMemoryType	*superTilePtr;
TQ3ColorRGBA		*vertexColorList;
float				brightness;
float				ambientR,ambientG,ambientB;
float				fillR0,fillG0,fillB0;
float				fillR1,fillG1,fillB1;
TQ3Vector3D			*fillDir0,*fillDir1;
Byte				numFillLights, numLayers;

static TQ3Vector3D	faceNormal[NUM_TRIS_IN_SUPERTILE];

	if (gDoCeiling)
		numLayers = 2;
	else
		numLayers = 1;


	superTileNum = GetFreeSuperTileMemory();					// get memory block for the data
	superTilePtr = &gSuperTileMemoryList[superTileNum];			// get ptr to it

	if (gDisableHiccupTimer)
		superTilePtr->hiccupTimer = 0;
	else
		superTilePtr->hiccupTimer = (gHiccupEliminator++ & 0x3) + 1;	// set hiccup timer to aleiviate hiccup caused by massive texture uploading

	for (int lod = 0; lod < MAX_LODS; lod++)
		superTilePtr->hasLOD[lod] = false;						// LOD isnt built yet

	for (int layer = 0; layer < MAX_LAYERS; layer++)
	{
		superTilePtr->coord[layer] = (TQ3Point3D)				// also remember world coords
		{
			startCol*TERRAIN_POLYGON_SIZE + TERRAIN_SUPERTILE_UNIT_SIZE/2,
			0,																		// y is set later
			startRow*TERRAIN_POLYGON_SIZE + TERRAIN_SUPERTILE_UNIT_SIZE/2,
		};
	}

	superTilePtr->left = (startCol * TERRAIN_POLYGON_SIZE);		// also save left/back coord
	superTilePtr->back = (startRow * TERRAIN_POLYGON_SIZE);


		/* GET LIGHT DATA */

	brightness = gGameViewInfoPtr->lightList.ambientBrightness;				// get ambient brightness
	ambientR = gGameViewInfoPtr->lightList.ambientColor.r * brightness;		// calc ambient color
	ambientG = gGameViewInfoPtr->lightList.ambientColor.g * brightness;		
	ambientB = gGameViewInfoPtr->lightList.ambientColor.b * brightness;

	brightness = gGameViewInfoPtr->lightList.fillBrightness[0];				// get fill brightness 0
	fillR0 = gGameViewInfoPtr->lightList.fillColor[0].r * brightness;		// calc ambient color
	fillG0 = gGameViewInfoPtr->lightList.fillColor[0].g * brightness;		
	fillB0 = gGameViewInfoPtr->lightList.fillColor[0].b * brightness;		
	fillDir0 = &gGameViewInfoPtr->lightList.fillDirection[0];		// get fill direction

	numFillLights = gGameViewInfoPtr->lightList.numFillLights;
	if (numFillLights > 1)
	{
		brightness = gGameViewInfoPtr->lightList.fillBrightness[1];			// get fill brightness 1
		fillR1 = gGameViewInfoPtr->lightList.fillColor[1].r * brightness;	// calc ambient color
		fillG1 = gGameViewInfoPtr->lightList.fillColor[1].g * brightness;		
		fillB1 = gGameViewInfoPtr->lightList.fillColor[1].b * brightness;		
		fillDir1 = &gGameViewInfoPtr->lightList.fillDirection[1];
	}
	else
	{
		fillR1 = 0;
		fillG1 = 0;
		fillB1 = 0;
		fillDir1 = nil;
	}

		/***********************************************************/
		/*                DO FLOOR & CEILING LAYERS                */
		/***********************************************************/

	for (int layer = 0; layer < numLayers; layer++)							// do floor & ceiling
	{
					/*******************/
					/* GET THE TRIMESH */
					/*******************/
					
		triMeshData = gSuperTileMemoryList[superTileNum].triMeshDataPtrs[layer];	// get ptr to triMesh data
		pointList = triMeshData->points;									// get ptr to point/vertex list
		triangleList = triMeshData->triangles;								// get ptr to triangle index list
		vertexColorList = triMeshData->vertexColors;						// get ptr to vertex color
		vertexNormalList = triMeshData->vertexNormals;						// get ptr to vertex normals

		miny = 1000000;														// init bbox counters
		maxy = -miny;
				
	
				/**********************************/
				/* CREATE VERTICES FOR THIS LAYER */
				/**********************************/
	
		for (row2 = 0; row2 <= SUPERTILE_SIZE; row2++)
		{
			row = row2 + startRow;
			
			for (col2 = 0; col2 <= SUPERTILE_SIZE; col2++)
			{
				col = col2 + startCol;
				
				if ((row >= gTerrainTileDepth) || (col >= gTerrainTileWidth)) // check for edge vertices (off map array)
					height = 0;
				else
					height = gMapYCoords[row][col].layerY[layer];			// get pixel height here
	
				gWorkGrid[row2][col2].x = (col*TERRAIN_POLYGON_SIZE);
				gWorkGrid[row2][col2].z = (row*TERRAIN_POLYGON_SIZE);
				gWorkGrid[row2][col2].y = height;							// save height @ this tile's upper left corner
					
				
				if (height > maxy)											// keep track of min/max
					maxy = height;
				if (height < miny)
					miny = height;			
			}
		}	
	
				/*********************************/
				/* CREATE TERRAIN MESH POLYGONS  */
				/*********************************/

		int i;
						/* SET VERTEX COORDS */

		i = 0;			
		for (row = 0; row < (SUPERTILE_SIZE+1); row++)
		{
			for (col = 0; col < (SUPERTILE_SIZE+1); col++)
				pointList[i++] = gWorkGrid[row][col];						// copy from other list
		}
	
					/* UPDATE TRIMESH DATA WITH NEW INFO */
#if _DEBUG
		SDL_memset(gTempTextureBuffer, 0xFF, SUPERTILE_TEXSIZE_MAX * SUPERTILE_TEXSIZE_MAX * sizeof(uint16_t));
#endif

		i = 0;
		for (row2 = 0; row2 < SUPERTILE_SIZE; row2++)
		{
			row = row2 + startRow;
	
			for (col2 = 0; col2 < SUPERTILE_SIZE; col2++)
			{
	
				col = col2 + startCol;
	
						/* SET SPLITTING INFO */

				const Byte* tri1;
				const Byte* tri2;

				if (gMapInfoMatrix[row][col].splitMode[layer] == SPLIT_BACKWARD)	// set coords & uv's based on splitting
				{
						/* \ */
					tri1 = gTileTriangles1_B[row2][col2];
					tri2 = gTileTriangles2_B[row2][col2];
				}
				else
				{
						/* / */
					tri1 = gTileTriangles1_A[row2][col2];
					tri2 = gTileTriangles2_A[row2][col2];
				}

				triangleList[i].pointIndices[0] 	= tri1[gTileTriangleWinding[layer][0]];
				triangleList[i].pointIndices[1] 	= tri1[gTileTriangleWinding[layer][1]];
				triangleList[i++].pointIndices[2] 	= tri1[gTileTriangleWinding[layer][2]];
				triangleList[i].pointIndices[0] 	= tri2[gTileTriangleWinding[layer][0]];
				triangleList[i].pointIndices[1] 	= tri2[gTileTriangleWinding[layer][1]];
				triangleList[i++].pointIndices[2] 	= tri2[gTileTriangleWinding[layer][2]];
			}
		}

							/* CALC FACE NORMALS */
						
		for (i = 0; i < NUM_TRIS_IN_SUPERTILE; i++)
		{
			CalcFaceNormal( &pointList[triangleList[i].pointIndices[0]],
							&pointList[triangleList[i].pointIndices[1]],
							&pointList[triangleList[i].pointIndices[2]],
							&faceNormal[i]);
		}

				/******************************/
				/* CALCULATE VERTEX NORMALS   */
				/******************************/
	
		i = 0;
		for (row = 0; row <= SUPERTILE_SIZE; row++)
		{
			for (col = 0; col <= (SUPERTILE_SIZE*2); col += 2)
			{
				TQ3Vector3D	*n1,*n2;
				float		avX,avY,avZ;
				TQ3Vector3D	nA,nB;
				long		ro,co;
				
				/* SCAN 4 TILES AROUND THIS TILE TO CALC AVERAGE NORMAL FOR THIS VERTEX */
				//
				// We use the face normal already calculated for triangles inside the supertile,
				// but for tiles/tris outside the supertile (on the borders), we need to calculate
				// the face normals there.
				//
				
				avX = avY = avZ = 0;									// init the normal	
				
				for (ro = -1; ro <= 0; ro++)
				{
					for (co = -2; co <= 0; co+=2)
					{
						long	cc = col + co;
						long	rr = row + ro;
						
						if ((cc >= 0) && (cc < (SUPERTILE_SIZE*2)) && (rr >= 0) && (rr < SUPERTILE_SIZE)) // see if this vertex is in supertile bounds							 
						{					
							n1 = &faceNormal[rr * (SUPERTILE_SIZE*2) + cc];					// average 2 triangles...
							n2 = n1+1;
							avX += n1->x + n2->x;											// ...and average with current average
							avY += n1->y + n2->y;
							avZ += n1->z + n2->z;
						}
						else																// tile is out of supertile, so calc face normal & average
						{
							CalcTileNormals(layer, rr+startRow, (cc>>1)+startCol, &nA,&nB);	// calculate the 2 face normals for this tile
							avX += nA.x + nB.x;												// average with current average
							avY += nA.y + nB.y;
							avZ += nA.z + nB.z;
						}
					}
				}
				FastNormalizeVector(avX, avY, avZ, &vertexNormalList[i++]);					// normalize the vertex normal	
			}
		}
		
		if (vertexColorList)
		{
				/*****************************/
				/* CALCULATE VERTEX COLORS   */
				/*****************************/
					
			i = 0;
			for (row = 0; row <= SUPERTILE_SIZE; row++)
			{
				for (col = 0; col <= SUPERTILE_SIZE; col++)
				{
					u_short	color = gVertexColors[layer][row+startRow][col+startCol];
					float	r,g,b,dot;
					float	lr,lg,lb;
					
							/* GET VERTEX DIFFUSE COLOR */
							
					r = (float)(color>>11) * (1.0f/32.0f);
					g = (float)((color>>5) & 0x3f) * (1.0f/64.0f);
					b = (float)(color&0x1f) * (1.0f/32.0f);
	
							/* APPLY LIGHTING TO THE VERTEX */
					
					lr = ambientR;												// factor in the ambient
					lg = ambientG;
					lb = ambientB;
					
					dot = vertexNormalList[i].x * fillDir0->x;					// calc dot product of fill #0
					dot += vertexNormalList[i].y * fillDir0->y;
					dot += vertexNormalList[i].z * fillDir0->z;
					dot = -dot;
	
					if (dot > 0.0f)
					{					
						lr += fillR0 * dot;
						lg += fillG0 * dot;
						lb += fillB0 * dot;					
					}
	
					if (numFillLights > 1)
					{
						dot = vertexNormalList[i].x * fillDir1->x;				// calc dot product of fill #1
						dot += vertexNormalList[i].y * fillDir1->y;
						dot += vertexNormalList[i].z * fillDir1->z;
						dot = -dot;
						
						if (dot > 0.0f)
						{					
							lr += fillR1 * dot;
							lg += fillG1 * dot;
							lb += fillB1 * dot;					
						}
					}
					
					r *= lr;													// apply final lighting to diffuse color
					if (r > 1.0f)
						r = 1.0f;
					g *= lg;
					if (g > 1.0f)
						g = 1.0f;
					b *= lb;
					if (b > 1.0f)
						b = 1.0f;
										
	
							/* SAVE COLOR INTO LIST */
							
					vertexColorList[i].r = r;
					vertexColorList[i].g = g;
					vertexColorList[i].b = b;
					i++;
				}
			}
		}

					/********************/
					/* ASSEMBLE TEXTURE */
					/********************/

		int textureMinRow = 0;
		int textureMinCol = 0;
		int textureMaxRow = textureMinRow + SUPERTILE_SIZE;
		int textureMaxCol = textureMinCol + SUPERTILE_SIZE;

		if (gTerrainTextureDetail == SUPERTILE_DETAIL_SEAMLESS)
		{
			textureMinRow--;
			textureMinCol--;
			textureMaxRow++;
			textureMaxCol++;
		}

		for (row2 = textureMinRow; row2 < textureMaxRow; row2++)
		{
			row = row2 + startRow;
	
			for (col2 = textureMinCol; col2 < textureMaxRow; col2++)
			{
				col = col2 + startCol;
	
						/* ADD TILE TO PIXMAP */

				if (row < 0 || row >= gTerrainTileDepth ||
					col < 0 || col >= gTerrainTileWidth)
				{
					tile = 0;
				}
				else if (layer == 0)
				{
					tile = gFloorMap[row][col];				// get tile from floor map...
				}
				else
				{
					tile = gCeilingMap[row][col];			// ...or get tile from ceiling map
				}

				if (gTerrainTextureDetail == SUPERTILE_DETAIL_SEAMLESS)
				{
					DrawTileIntoMipmap(tile, row2+1, col2+1, gTempTextureBuffer);		// draw into mipmap
				}
				else
				{
					DrawTileIntoMipmap(tile, row2, col2, gTempTextureBuffer);		// draw into mipmap
				}
			}
		}

				/************************/
				/* UPDATE TEXTURE LOD 0 */
				/************************/
				//
				// If we are in low-memory mode, then we shrink the texture to LOD #1 instead of LOD #0 and we shrink it to 64x64
				//

		{
			if (gTerrainTextureDetail == SUPERTILE_DETAIL_LOSSLESS
					|| gTerrainTextureDetail == SUPERTILE_DETAIL_SEAMLESS)
			{
				SDL_memcpy(superTilePtr->textureData[layer][0], gTempTextureBuffer, sizeof(gTempTextureBuffer[0]) * gTextureSizePerLOD[0] * gTextureSizePerLOD[0]);
			}
			else
			{
				ShrinkSuperTileTextureMap(gTempTextureBuffer, superTilePtr->textureData[layer][0]);				// shrink to 128x128
			}

			superTilePtr->hasLOD[0] = true;

			Render_UpdateTexture(
					superTilePtr->glTextureName[layer][0],
					0,
					0,
					gTextureSizePerLOD[0],
					gTextureSizePerLOD[0],
					TILE_TEXTURE_FORMAT,
					TILE_TEXTURE_TYPE,
					superTilePtr->textureData[layer][0],
					0);
		}




				/**********************/
				/* UPDATE THE TRIMESH */
				/**********************/
	
				/* SET BOUNDING BOX */
				
		triMeshData->bBox.min.x = gWorkGrid[0][0].x;
		triMeshData->bBox.max.x = triMeshData->bBox.min.x+TERRAIN_SUPERTILE_UNIT_SIZE;
		triMeshData->bBox.min.y = miny;
		triMeshData->bBox.max.y = maxy;
		triMeshData->bBox.min.z = gWorkGrid[0][0].z;
		triMeshData->bBox.max.z = triMeshData->bBox.min.z + TERRAIN_SUPERTILE_UNIT_SIZE;


				/******************************************/
				/* CALC COORD & RADIUS FOR CULLING SPHERE */
				/******************************************/

		// Calc center Y coord as average of top & bottom.
		// This Y coord is not used to translate since the terrain has no translation matrix.
		// Instead, this is used by the frustum culling routine.
		superTilePtr->coord[layer].y = (miny + maxy) * .5f;
		
		// Calc radius of supertile bounding sphere
		superTilePtr->radius[layer] = 0.5f * Q3Point3D_Distance(&triMeshData->bBox.min, &triMeshData->bBox.max);

	}	// j (layer)
									
	return(superTileNum);
}



/********************** BUILD SUPERTILE LEVEL OF DETAIL ********************/
//
// Called from DrawTerrain to generate the LOD's from the source LOD=0 geometry
//

static void BuildSuperTileLOD(SuperTileMemoryType *superTilePtr, short lod)
{
	if (superTilePtr->hasLOD[lod])									// see if already has LOD
		return;

	GAME_ASSERT_MESSAGE(lod != 0, "Cannot use BuildSuperTileLOD for LOD 0!");

	if (lod >= gNumLODs)
		DoFatalAlert("BuildSuperTileLOD: lod overflow");

	superTilePtr->hasLOD[lod] = true;								// has LOD now

	int numLayers = gDoCeiling ? 2 : 1;

	for (int j = 0; j < numLayers; j++)
	{
			/*************************/
			/* SHRINK & COPY TEXTURE */
			/*************************/

			/* GET POINTERS TO TEXTURE BUFFERS */

		uint16_t*	baseBuffer	= superTilePtr->textureData[j][lod-1];	// build new LOD from inferior LOD
		uint16_t*	newBuffer	= superTilePtr->textureData[j][lod];

			/* SHRINK IMAGE */

		ShrinkHalf(baseBuffer, newBuffer, gTextureSizePerLOD[lod]);

			/* UPDATE THE TEXTURE */

		Render_UpdateTexture(
				superTilePtr->glTextureName[j][lod],
				0,
				0,
				gTextureSizePerLOD[lod],
				gTextureSizePerLOD[lod],
				TILE_TEXTURE_FORMAT,
				TILE_TEXTURE_TYPE,
				newBuffer,
				0);
	}
}




/********************* DRAW TILE INTO MIPMAP *************************/

static void DrawTileIntoMipmap(uint16_t tile, int row, int col, uint16_t *buffer)
{
uint16_t texMapNum;
uint16_t flipRotBits;
const uint16_t* tileData;

const int tileSize = OREOMAP_TILE_SIZE;

const int bufWidth
	= (gTerrainTextureDetail == SUPERTILE_DETAIL_SEAMLESS)
	? SUPERTILE_TEXSIZE_SEAMLESS
	: SUPERTILE_TEXSIZE_LOSSLESS;


			/* EXTRACT BITS INFO FROM TILE */
				
	flipRotBits = tile&(TILE_FLIPXY_MASK|TILE_ROTATE_MASK);		// get flip & rotate bits
	texMapNum = tile&TILENUM_MASK; 								// filter out texture #

	if (texMapNum >= gNumTerrainTextureTiles)					// make sure not illegal tile #
	{
//		DoFatalAlert("DrawTileIntoMipmap: illegal tile #");
		texMapNum = 0;
	}

				/* CALC PTRS */

	const int startX = col * tileSize;
	const int startY = row * tileSize;

	buffer += (startY * bufWidth) + startX;						// get dest
	tileData = (*gTileDataHandle) + (texMapNum * tileSize*tileSize);	// get src


	switch(flipRotBits)         								// set uv's based on flip & rot bits
	{
				/* NO FLIP & NO ROT */
					/* XYFLIP ROT 2 */

		case	0:
		case	TILE_FLIPXY_MASK | TILE_ROT2:
				for (int y = 0; y < tileSize; y++)
				{
					SDL_memcpy(buffer, tileData, tileSize * sizeof(uint16_t));

					buffer += bufWidth;						// next line in dest
					tileData += tileSize;					// next line in src
				}
				break;

					/* FLIP X */
				/* FLIPY ROT 2 */

		case	TILE_FLIPX_MASK:
		case	TILE_FLIPY_MASK | TILE_ROT2:
				for (int y = 0; y < tileSize; y++)
				{
					for (int x = 0; x < tileSize; x++)
						buffer[x] = tileData[tileSize-1-x];

					buffer += bufWidth;						// next line in dest
					tileData += tileSize;					// next line in src
				}
				break;

					/* FLIP Y */
				/* FLIPX ROT 2 */

		case	TILE_FLIPY_MASK:
		case	TILE_FLIPX_MASK | TILE_ROT2:
				tileData += tileSize*(tileSize-1);
				for (int y = 0; y < tileSize; y++)
				{
					SDL_memcpy(buffer, tileData, tileSize * sizeof(uint16_t));

					buffer += bufWidth;						// next line in dest
					tileData -= tileSize;					// next line in src
				}
				break;


				/* FLIP XY */
				/* NO FLIP ROT 2 */

		case	TILE_FLIPXY_MASK:
		case	TILE_ROT2:
				tileData += tileSize*(tileSize-1);
				for (int y = 0; y < tileSize; y++)
				{
					for (int x = 0; x < tileSize; x++)
						buffer[x] = tileData[tileSize-1-x];

					buffer += bufWidth;						// next line in dest
					tileData -= tileSize;					// next line in src
				}
				break;

				/* NO FLIP ROT 1 */
				/* FLIP XY ROT 3 */

		case	TILE_ROT1:
		case	TILE_FLIPXY_MASK | TILE_ROT3:
				buffer += (tileSize-1);						// draw to right col from top row of src
				for (int y = 0; y < tileSize; y++)
				{
					for (int x = 0; x < tileSize; x++)
						buffer[bufWidth*x] = tileData[x];
						
					buffer--;								// prev col in dest
					tileData += tileSize;					// next line in src
				}
				break;

				/* NO FLIP ROT 3 */
				/* FLIP XY ROT 1 */

		case	TILE_ROT3:
		case	TILE_FLIPXY_MASK | TILE_ROT1:
				for (int y = 0; y < tileSize; y++)
				{
					for (int x = 0; x < tileSize; x++)
						buffer[bufWidth*(tileSize-1-x)] = tileData[x];		// backwards

					buffer++;								// next col in dest
					tileData += tileSize;					// next line in src
				}
				break;

				/* FLIP X ROT 1 */
				/* FLIP Y ROT 3 */

		case	TILE_FLIPX_MASK | TILE_ROT1:
		case	TILE_FLIPY_MASK | TILE_ROT3:
				buffer += (tileSize-1);
				for (int y = 0; y < tileSize; y++)
				{
					for (int x = 0; x < tileSize; x++)
						buffer[bufWidth*(tileSize-1-x)] = tileData[x];		// backwards

					buffer--;								// prev col in dest
					tileData += tileSize;					// next line in src
				}
				break;

				/* FLIP X ROT 3 */
				/* FLIP Y ROT 1 */

		case	TILE_FLIPX_MASK | TILE_ROT3:
		case	TILE_FLIPY_MASK | TILE_ROT1:
				for (int y = 0; y < tileSize; y++)			// draw to right col from top row of src
				{
					for (int x = 0; x < tileSize; x++)
						buffer[bufWidth*x] = tileData[x];

					buffer++;								// next col in dest
					tileData += tileSize;					// next line in src
				}
				break;
	}
}


/************ SHRINK SUPERTILE TEXTURE MAP ********************/
//
// Shrinks a 160x160 src texture to a 128x128 dest texture
//

static void	ShrinkSuperTileTextureMap(const u_short *srcPtr, u_short *dstPtr)
{
	_Static_assert(SUPERTILE_TEXSIZE_SHRUNK == 128, "rewrite this for new supertile texmap size!");
	_Static_assert(SUPERTILE_SIZE * OREOMAP_TILE_SIZE == 160, "rewrite this for new oreomap tile size!");

	for (int y = 0; y < 128; y++)
	{
		for (int x = 0; x < 128; x += 4)					// shrink a block of 5 pixels from src into 4 pixels in dst
		{
			u_short c1 = srcPtr[3];							// get colors to average
			u_short c2 = srcPtr[4];
			u_short r = ((c1>>10) + (c2>>10))>>1;			// compute average
			u_short g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
			u_short b = ((c1&0x1f) + (c2&0x1f)) >> 1;

			dstPtr[0] = srcPtr[0];							// save #0
			dstPtr[1] = srcPtr[1];							// save #1
			dstPtr[2] = srcPtr[2];							// save #2
			dstPtr[3] = (r<<10)|(g<<5)|b;					// save #3 as average of #3 and #4

			dstPtr += 4;
			srcPtr += 5;
		}

		if (y % 4 == 3)							// skip every 4th line
			srcPtr +=  SUPERTILE_SIZE * OREOMAP_TILE_SIZE;
	}
}


#if 0	// Unused in this version of Bugdom
/************ SHRINK SUPERTILE TEXTURE MAP TO 64 ********************/
//
// Shrinks a 160x160 src texture to a 64x64 dest texture
//
// Source port note: unused in this version of Bugdom.
// Simplified the function, but didn't test it.

static void	ShrinkSuperTileTextureMapTo64(u_short *srcPtr, u_short *dstPtr)
{
	_Static_assert(SUPERTILE_SIZE * OREOMAP_TILE_SIZE == 160, "rewrite this for new oreomap tile size!");

	for (int y = 0; y < 64; y++)
	{
		for (int x = 0; x < 64; x += 2)					// shrink a block of 5 pixels from src into 2 pixels in dst
		{
			dstPtr[0] = srcPtr[0];
			dstPtr[1] = srcPtr[2];
			dstPtr += 2;
			srcPtr += 5;
		}

		srcPtr += SUPERTILE_SIZE * OREOMAP_TILE_SIZE;	// skip every other line in src

		if (y % 2 == 0)									// skip another line
			srcPtr += SUPERTILE_SIZE * OREOMAP_TILE_SIZE;
	}
}
#endif


/************ SHRINK SQUARE TEXTURE TO HALF SIZE ******************/

static void ShrinkHalf(const uint16_t* input, uint16_t* output, int outputSize)
{
	int inputWidth = outputSize * 2;
	const uint16_t* nextLine = input + inputWidth;

	for (int y = 0; y < outputSize; y++)
	{
		for (int x = 0; x < outputSize; x++)
		{
			uint16_t	r,g,b;
			uint16_t	pixel;

			pixel = *input++;							// get a pixel
			r = (pixel >> 10) & 0x1f;
			g = (pixel >> 5) & 0x1f;
			b = pixel & 0x1f;

			pixel = *input++;							// get next pixel
			r += (pixel >> 10) & 0x1f;
			g += (pixel >> 5) & 0x1f;
			b += pixel & 0x1f;


			pixel = *nextLine++;						// get a pixel from next line
			r += (pixel >> 10) & 0x1f;
			g += (pixel >> 5) & 0x1f;
			b += pixel & 0x1f;

			pixel = *nextLine++;						// get next pixel from next line
			r += (pixel >> 10) & 0x1f;
			g += (pixel >> 5) & 0x1f;
			b += pixel & 0x1f;

			r >>= 2;									// calc average
			g >>= 2;
			b >>= 2;

			*output++ = (r<<10) | (g<<5) | b;			// save new pixel
		}
		input += inputWidth;							// skip a line
		nextLine += inputWidth;
	}
}



/******************* RELEASE SUPERTILE OBJECT *******************/
//
// Deactivates the terrain object and releases its memory block
//

static inline void ReleaseSuperTileObject(int32_t superTileNum)
{
	GAME_ASSERT(superTileNum >= 0);
	GAME_ASSERT(superTileNum < gSupertileBudget);

	if (gSuperTileMemoryList[superTileNum].mode != SUPERTILE_MODE_FREE)
	{
		gSuperTileMemoryList[superTileNum].mode = SUPERTILE_MODE_FREE;		// it's free!
		gNumFreeSupertiles++;
	}

	GAME_ASSERT(gNumFreeSupertiles <= gSupertileBudget);
}

/******************** RELEASE ALL SUPERTILES ************************/

static inline void ReleaseAllSuperTiles(void)
{
	for (int32_t i = 0; i < gSupertileBudget; i++)
		ReleaseSuperTileObject(i);

	GAME_ASSERT(gNumFreeSupertiles == gSupertileBudget);
}

#pragma mark -

/********************* DRAW TERRAIN **************************/
//
// This is the main call to update the screen.  It draws all ObjNode's and the terrain itself
//

void DrawTerrain(const QD3DSetupOutputType *setupInfo)
{
	int numLayers = gDoCeiling? 2: 1;

		/* GET CURRENT CAMERA COORD */
		
	TQ3Point3D cameraCoord = setupInfo->currentCameraCoords;
	

				/* DRAW STUFF */

	for (int i = 0; i < gSupertileBudget; i++)
	{
		if (gSuperTileMemoryList[i].mode != SUPERTILE_MODE_USED)		// if supertile is being used, then draw it
			continue;
		
				/* SEE IF DO HICCUP PREVENTION */
				
		if (!gDisableHiccupTimer)
		{
			if (gSuperTileMemoryList[i].hiccupTimer != 0)				// see if this supertile is still in hiccup prevention mode
			{
				gSuperTileMemoryList[i].hiccupTimer--;
				continue;
			}
		}

		for (int j = 0; j < numLayers; j++)								// DRAW FLOOR & CEILING
		{
			if (!IsSuperTileVisible(i,j))								// make sure it's visible
				continue;


				/**************************************/
				/* DRAW THE TRIMESH IN THIS SUPERTILE */
				/**************************************/

			int lod = 0;

			if (gTerrainTextureDetail == SUPERTILE_DETAIL_PROGRESSIVE)	// the only detail level with 3 LODs
			{
						/* SEE WHICH LOD TO USE */

				TQ3Point3D tileCoord = gSuperTileMemoryList[i].coord[j];	// get x & z coords of tile
				float dist = CalcQuickDistance(cameraCoord.x, cameraCoord.z, tileCoord.x, tileCoord.z);

				if (dist < 1300.0f)
					lod = 0;
				else if (dist < 1700.0f)
					lod = 1;
				else
					lod = 2;

						/* MAKE SURE THE LOD IS BUILT */

				if (!gSuperTileMemoryList[i].hasLOD[lod])
				{
					GAME_ASSERT(lod != 0);	// can't build LOD 0 like that

					for (int lodToBuild = 0; lodToBuild <= lod; lodToBuild++)	// any LOD requires all of the inferior LODs to be built
					{
						if (!gSuperTileMemoryList[i].hasLOD[lodToBuild])
							BuildSuperTileLOD(&gSuperTileMemoryList[i], lodToBuild);
					}
				}
			}

						/* USE LOD TEXTURE */

			gSuperTileMemoryList[i].triMeshDataPtrs[j]->glTextureName = gSuperTileMemoryList[i].glTextureName[j][lod];

						/* SUBMIT FOR DRAWING */

			Render_SubmitMesh(gSuperTileMemoryList[i].triMeshDataPtrs[j], nil, &gTerrainRenderMods, &gSuperTileMemoryList[i].coord[j]);
		}
	}

		/* DRAW OBJECTS */

	DrawCyclorama();
	DrawFences(setupInfo);												// draw these first

	if (gDoAutoFade)													// avoid clover-shaped holes in fences
		Render_FlushQueue();

	DrawObjects(setupInfo);												// draw objNodes
	QD3D_DrawShards(setupInfo);											// draw "shard" particles
	DrawParticleGroup(setupInfo);										// draw alpha-blended particle groups

	Render_FlushQueue();												// flush before drawing 2D stuff

	switch (gDebugMode)
	{
		case DEBUG_MODE_BOXES:
			Render_ResetColor();
			for (ObjNode* node = gFirstNodePtr; node; node = node->NextNode)
				DrawCollisionBoxes(node);
			DrawNormal();
			break;

		case DEBUG_MODE_SPLINES:
			Render_ResetColor();
			DrawSplines();
			break;
	}

		/* DRAW IN-GAME 2D ELEMENTS */

	Render_Enter2D_NormalizedCoordinates(setupInfo->aspectRatio);
	DrawLensFlare(setupInfo);
	if (gPauseQuad)
		Render_SubmitMesh(gPauseQuad, NULL, &kDefaultRenderMods_UI, &kQ3Point3D_Zero);
	Render_FlushQueue();
	Render_Exit2D();
}


/***************** GET TERRAIN HEIGHT AT COORD ******************/
//
// Given a world x/z coord, return the y coord based on height map
//
// INPUT: x/z = world coords 
//
// OUTPUT: y = world y coord
//

float	GetTerrainHeightAtCoord(float x, float z, long layer)
{
TQ3PlaneEquation	planeEq;
int					row,col;
TQ3Point3D			p[4];
float				xi,zi;

	if (!gFloorMap)														// make sure there's a terrain
		return(0);
		
	if (layer == CEILING)												// make sure there is a ceiling
		if (!gDoCeiling)
			return(10000000);											// no ceiling, so just return a really high number	


	col = x * TERRAIN_POLYGON_SIZE_Frac;								// see which row/col we're on
	row = z * TERRAIN_POLYGON_SIZE_Frac;			
				
	if ((col < 0) || (col >= gTerrainTileWidth))						// check bounds
		return(0);
	if ((row < 0) || (row >= gTerrainTileDepth))
		return(0);
				
	xi = x - (col * TERRAIN_POLYGON_SIZE);								// calc x/z offset into the tile
	zi = z - (row * TERRAIN_POLYGON_SIZE);
				
				
				
					/* BUILD VERTICES FOR THE 4 CORNERS OF THE TILE */
				
	p[0].x = col * TERRAIN_POLYGON_SIZE;								// far left
	p[0].y = gMapYCoords[row][col].layerY[layer];
	p[0].z = row * TERRAIN_POLYGON_SIZE;

	p[1].x = p[0].x + TERRAIN_POLYGON_SIZE;								// far right
	p[1].y = gMapYCoords[row][col+1].layerY[layer];
	p[1].z = p[0].z;

	p[2].x = p[1].x;													// near right
	p[2].y = gMapYCoords[row+1][col+1].layerY[layer];
	p[2].z = p[1].z + TERRAIN_POLYGON_SIZE;

	p[3].x = col * TERRAIN_POLYGON_SIZE;								// near left
	p[3].y = gMapYCoords[row+1][col].layerY[layer];
	p[3].z = p[2].z;

				

		/************************************/
		/* CALC PLANE EQUATION FOR TRIANGLE */
		/************************************/
		
	if (gMapInfoMatrix[row][col].splitMode[layer] == SPLIT_BACKWARD)			// if \ split
	{
		if (layer == 0)
		{
			if (xi < zi)														// which triangle are we on?
				CalcPlaneEquationOfTriangle(&planeEq, &p[0], &p[2],&p[3]);		// calc plane equation for left triangle
			else
				CalcPlaneEquationOfTriangle(&planeEq, &p[0], &p[1], &p[2]);		// calc plane equation for right triangle
		}
		else																	// clockwise for ceiling
		{
			if (xi < zi)														// which triangle are we on?
				CalcPlaneEquationOfTriangle(&planeEq, &p[3], &p[2], &p[0]);		// calc plane equation for left triangle
			else
				CalcPlaneEquationOfTriangle(&planeEq, &p[2], &p[1], &p[0]);		// calc plane equation for right triangle
		}
	}
	else																		// otherwise, / split
	{
		xi = TERRAIN_POLYGON_SIZE-xi;											// flip x
		if (layer == 0)
		{	
			if (xi > zi)
				CalcPlaneEquationOfTriangle(&planeEq, &p[0], &p[1], &p[3]);		// calc plane equation for left triangle
			else
				CalcPlaneEquationOfTriangle(&planeEq, &p[1], &p[2], &p[3]);		// calc plane equation for right triangle
		}
		else																	// clockwise for ceiling
		{			
			if (xi > zi)
				CalcPlaneEquationOfTriangle(&planeEq, &p[3], &p[1], &p[0]);		// calc plane equation for left triangle
			else
				CalcPlaneEquationOfTriangle(&planeEq, &p[3], &p[2], &p[1]);		// calc plane equation for right triangle
		}
			
	}			

	gRecentTerrainNormal[layer] = planeEq.normal;								// remember the normal here

	return (IntersectionOfYAndPlane(x,z,&planeEq));								// calc intersection
}


/***************** GET SUPERTILE INFO ******************/
//
// Given a world x/z coord, return some supertile info
//
// INPUT: x/y = world x/y coords
// OUTPUT: row/col in tile coords and supertile coords
//

void GetSuperTileInfo(long x, long z, long *superCol, long *superRow, long *tileCol, long *tileRow)
{
long	row,col;


//	if ((x < 0) || (y < 0))									// see if out of bounds
//		return;
//	if ((x >= gTerrainUnitWidth) || (y >= gTerrainUnitDepth))
//		return;

	col = x * (1.0f/TERRAIN_SUPERTILE_UNIT_SIZE);					// calc supertile relative row/col that the coord lies on
	row = z * (1.0f/TERRAIN_SUPERTILE_UNIT_SIZE);

	*superRow = row;										// return which supertile relative row/col it is
	*superCol = col;
	*tileRow = row*SUPERTILE_SIZE;							// return which tile row/col the super tile starts on
	*tileCol = col*SUPERTILE_SIZE;
}


/******************** DO MY TERRAIN UPDATE ********************/

void DoMyTerrainUpdate(void)
{
long	x,y;
long	superCol,superRow,tileCol,tileRow;
TQ3Vector2D	look;


			/* CALC PIXEL COORDS OF FAR LEFT SUPER TILE */
			//
			// Use point projected n units in front of camera as the "location"
			//
			

//	x = gMyCoord.x-(SUPERTILE_ACTIVE_RANGE*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE);
//	y = gMyCoord.z-(SUPERTILE_ACTIVE_RANGE*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE);

	look.x = gGameViewInfoPtr->currentCameraLookAt.x - gGameViewInfoPtr->currentCameraCoords.x;
	look.y = gGameViewInfoPtr->currentCameraLookAt.z - gGameViewInfoPtr->currentCameraCoords.z;
	FastNormalizeVector2D(look.x, look.y, &look);

	x = gGameViewInfoPtr->currentCameraCoords.x + (look.x * 500.0f);
	y = gGameViewInfoPtr->currentCameraCoords.z + (look.y * 500.0f);

	x -= (SUPERTILE_ACTIVE_RANGE*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE);
	y -= (SUPERTILE_ACTIVE_RANGE*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE);


			/* SEE IF WE'VE SCROLLED A WHOLE SUPERTILE YET */

	GetSuperTileInfo(x,y,&superCol,&superRow,&tileCol,&tileRow); 		// get supertile coord info


		// NOTE: DO VERTICAL FIRST!!!!

				/* SEE IF SCROLLED UP */

	if (superRow > gCurrentSuperTileRow)
	{
		if (superRow > (gCurrentSuperTileRow+1))						// check for overload scroll
			DoFatalAlert("DoMyTerrainUpdate: scrolled up > 1 tile!");
		ScrollTerrainUp(superRow,superCol);
		gCurrentSuperTileRow = superRow;
	}
	else
				/* SEE IF SCROLLED DOWN */

	if (superRow < gCurrentSuperTileRow)
	{
		if (superRow < (gCurrentSuperTileRow-1))						// check for overload scroll
			DoFatalAlert("DoMyTerrainUpdate: scrolled down < -1 tile!");
		ScrollTerrainDown(superRow,superCol);
		gCurrentSuperTileRow = superRow;
	}

			/* SEE IF SCROLLED LEFT */

	if (superCol > gCurrentSuperTileCol)
	{
		if (superCol > (gCurrentSuperTileCol+1))						// check for overload scroll
			DoFatalAlert("DoMyTerrainUpdate: scrolled left > 1 tile!");
		ScrollTerrainLeft();
	}
	else
				/* SEE IF SCROLLED RIGHT */

	if (superCol < gCurrentSuperTileCol)
	{
		if (superCol < (gCurrentSuperTileCol-1))						// check for overload scroll
			DoFatalAlert("DoMyTerrainUpdate: scrolled right < -1 tile!");
		ScrollTerrainRight(superCol,superRow,tileCol,tileRow);
		gCurrentSuperTileCol = superCol;
	}

	CalcNewItemDeleteWindow();							// recalc item delete window

}


/****************** CALC NEW ITEM DELETE WINDOW *****************/

static void CalcNewItemDeleteWindow(void)
{
float	temp,temp2;

				/* CALC LEFT SIDE OF WINDOW */

	temp = gCurrentSuperTileCol*TERRAIN_SUPERTILE_UNIT_SIZE;			// convert to unit coords
	temp2 = temp - (ITEM_WINDOW+OUTER_SIZE)*TERRAIN_SUPERTILE_UNIT_SIZE;// factor window left			
	gTerrainItemDeleteWindow_Left = temp2;


				/* CALC RIGHT SIDE OF WINDOW */

	temp += SUPERTILE_DIST_WIDE*TERRAIN_SUPERTILE_UNIT_SIZE;			// calc offset to right side
	temp += (ITEM_WINDOW+OUTER_SIZE)*TERRAIN_SUPERTILE_UNIT_SIZE;		// factor window right
	gTerrainItemDeleteWindow_Right = temp;


				/* CALC FAR SIDE OF WINDOW */

	temp = gCurrentSuperTileRow*TERRAIN_SUPERTILE_UNIT_SIZE;			// convert to unit coords
	temp2 = temp - (ITEM_WINDOW+OUTER_SIZE)*TERRAIN_SUPERTILE_UNIT_SIZE;// factor window top/back
	gTerrainItemDeleteWindow_Far = temp2;


				/* CALC NEAR SIDE OF WINDOW */

	temp += SUPERTILE_DIST_DEEP*TERRAIN_SUPERTILE_UNIT_SIZE;			// calc offset to bottom side
	temp += (ITEM_WINDOW+OUTER_SIZE)*TERRAIN_SUPERTILE_UNIT_SIZE;		// factor window bottom/front
			
	gTerrainItemDeleteWindow_Near = temp;
}



/********************** SCROLL TERRAIN UP *************************/
//
// INPUT: superRow = new supertile row #
//

static void ScrollTerrainUp(long superRow, long superCol)
{
long	col,i,bottom,left,right;
int32_t	superTileNum;
long	tileRow,tileCol;

	gHiccupEliminator = 0;															// reset this when about to make a new row/col of supertiles

			/* PURGE OLD TOP ROW */

	if ((gCurrentSuperTileRow < gNumSuperTilesDeep) && (gCurrentSuperTileRow >= 0))	// check if off map
	{
		for (i = 0; i < SUPERTILE_DIST_WIDE; i++)
		{
			col = gCurrentSuperTileCol+i;
			if (col >= gNumSuperTilesWide)											// check if off map
				break;
			if (col < 0)
				continue;

			superTileNum = gTerrainScrollBuffer[gCurrentSuperTileRow][col];			// get supertile for that spot
			if (superTileNum != EMPTY_SUPERTILE)
			{
				ReleaseSuperTileObject(superTileNum);						 		// free the supertile
				gTerrainScrollBuffer[gCurrentSuperTileRow][col] = EMPTY_SUPERTILE;
			}
		}
	}


		/* CREATE NEW BOTTOM ROW */

	superRow += SUPERTILE_DIST_DEEP-1;		 				   				// calc row # of bottom supertile row
	tileRow = superRow * SUPERTILE_SIZE;  									// calc row # of bottom tile row

	if (superRow >= gNumSuperTilesDeep)										// see if off map
		return;
	if (superRow < 0)
		return;

	tileCol = gCurrentSuperTileCol * SUPERTILE_SIZE;						// calc col # bot left tile col

	for (col = gCurrentSuperTileCol; col < (gCurrentSuperTileCol + SUPERTILE_DIST_WIDE); col++)
	{
		if (col >= gNumSuperTilesWide)
			goto check_items;
		if (col < 0)
			goto next;

		if (gTerrainScrollBuffer[superRow][col] == EMPTY_SUPERTILE)					// see if something is already there
		{
			if ((tileCol >= 0) && (tileCol < gTerrainTileWidth))
			{
				superTileNum = BuildTerrainSuperTile(tileCol,tileRow); 					// make new terrain object
				gTerrainScrollBuffer[superRow][col] = superTileNum;						// save into scroll buffer array
			}
		}
next:
		tileCol += SUPERTILE_SIZE;
	}

check_items:

			/* ADD ITEMS ON BOTTOM */

	bottom = superRow+ITEM_WINDOW;
	left = superCol-ITEM_WINDOW;
	right = superCol+SUPERTILE_DIST_WIDE-1+ITEM_WINDOW;

	if (left < 0)
		left = 0;
	else
	if (left >= gNumSuperTilesWide)
		return;

	if (right < 0)
		return;
	if (right >= gNumSuperTilesWide)
		right = gNumSuperTilesWide-1;

	if (bottom >= gNumSuperTilesDeep)
		return;
	if (bottom < 0)
		return;

	ScanForPlayfieldItems(bottom,bottom,left,right);
}


/********************** SCROLL TERRAIN DOWN *************************/
//
// INPUT: superRow = new supertile row #
//

static void ScrollTerrainDown(long superRow, long superCol)
{
long	col,i,row,top,left,right;
int32_t	superTileNum;
long	tileRow,tileCol;

	gHiccupEliminator = 0;															// reset this when about to make a new row/col of supertiles

			/* PURGE OLD BOTTOM ROW */

	row = gCurrentSuperTileRow+SUPERTILE_DIST_DEEP-1;						// calc supertile row # for bottom row

	if ((row < gNumSuperTilesDeep) && (row >= 0))							// check if off map
	{
		for (i = 0; i < SUPERTILE_DIST_WIDE; i++)
		{
			col = gCurrentSuperTileCol+i;
			if (col >= gNumSuperTilesWide)									// check if off map
				break;
			if (col < 0)
				continue;

			superTileNum = gTerrainScrollBuffer[row][col];					// get supertile for that spot
			if (superTileNum != EMPTY_SUPERTILE)
			{
				ReleaseSuperTileObject(superTileNum);	  						// free the terrain object
				gTerrainScrollBuffer[row][col] = EMPTY_SUPERTILE;
			}
		}
	}


				/* CREATE NEW TOP ROW */

	if (superRow < 0) 														// see if off map
		return;
	if (superRow >= gNumSuperTilesDeep)
		return;

	tileCol = gCurrentSuperTileCol * SUPERTILE_SIZE;						// calc col # bot left tile col
	tileRow = superRow * SUPERTILE_SIZE;  									// calc col # bot left tile col

	for (col = gCurrentSuperTileCol; col < (gCurrentSuperTileCol + SUPERTILE_DIST_WIDE); col++)
	{
		if (col >= gNumSuperTilesWide)										// see if off map
			goto check_items;
		if (col < 0)
			goto next;

		if (gTerrainScrollBuffer[superRow][col] == EMPTY_SUPERTILE)					// see if something is already there
		{
			if ((tileCol >= 0) && (tileCol < gTerrainTileWidth))
			{
				superTileNum = BuildTerrainSuperTile(tileCol,tileRow); 				// make new terrain object
				gTerrainScrollBuffer[superRow][col] = superTileNum;					// save into scroll buffer array
			}
		}
next:
		tileCol += SUPERTILE_SIZE;
	}

check_items:

			/* ADD ITEMS ON TOP */

	top = superRow-ITEM_WINDOW;
	left = superCol-ITEM_WINDOW;
	right = superCol+SUPERTILE_DIST_WIDE-1+ITEM_WINDOW;

	if (left < 0)
		left = 0;
	else
	if (left >= gNumSuperTilesWide)
		return;

	if (right < 0)
		return;
	else
	if (right >= gNumSuperTilesWide)
		right = gNumSuperTilesWide-1;

	if (top >= gNumSuperTilesDeep)
		return;
	if (top < 0)
		return;

	ScanForPlayfieldItems(top,top,left,right);

}


/********************** SCROLL TERRAIN LEFT *************************/
//
// Assumes gCurrentSuperTileCol & gCurrentSuperTileRow are in current positions, will do gCurrentSuperTileCol++ at end of routine.
//

static void ScrollTerrainLeft(void)
{
long	row,top,bottom,right;
int32_t	superTileNum;
long 	tileCol,tileRow,newSuperCol;
long	bottomRow;

	gHiccupEliminator = 0;															// reset this when about to make a new row/col of supertiles

	bottomRow = gCurrentSuperTileRow + SUPERTILE_DIST_DEEP;								// calc bottom row (+1)


			/* PURGE OLD LEFT COL */

	if ((gCurrentSuperTileCol < gNumSuperTilesWide) && (gCurrentSuperTileCol >= 0))		// check if on map
	{
		for (row = gCurrentSuperTileRow; row < bottomRow; row++)
		{
			if (row >= gNumSuperTilesDeep)												// check if off map
				break;
			if (row < 0)
				continue;

			superTileNum = gTerrainScrollBuffer[row][gCurrentSuperTileCol]; 			// get supertile for that spot
			if (superTileNum != EMPTY_SUPERTILE)
			{
				ReleaseSuperTileObject(superTileNum);									// free the terrain object
				gTerrainScrollBuffer[row][gCurrentSuperTileCol] = EMPTY_SUPERTILE;
			}
		}
	}


		/* CREATE NEW RIGHT COL */

	newSuperCol = gCurrentSuperTileCol+SUPERTILE_DIST_WIDE;	   					// calc col # of right supertile col
	tileCol = newSuperCol * SUPERTILE_SIZE; 		 							// calc col # of right tile col
	tileRow = gCurrentSuperTileRow * SUPERTILE_SIZE;

	if (newSuperCol >= gNumSuperTilesWide)										// see if off map
		goto exit;
	if (newSuperCol < 0)
		goto exit;

	for (row = gCurrentSuperTileRow; row < bottomRow; row++)
	{
		if (row >= gNumSuperTilesDeep)
			break;
		if (row < 0)
			goto next;

		if (gTerrainScrollBuffer[row][newSuperCol] == EMPTY_SUPERTILE)			// make sure nothing already here
		{
			superTileNum = BuildTerrainSuperTile(tileCol,tileRow); 				// make new terrain object
			gTerrainScrollBuffer[row][newSuperCol] = superTileNum;				// save into scroll buffer array
		}
next:
		tileRow += SUPERTILE_SIZE;
	}

			/* ADD ITEMS ON RIGHT */

	top = gCurrentSuperTileRow-ITEM_WINDOW;
	bottom = gCurrentSuperTileRow + (SUPERTILE_DIST_DEEP-1) + ITEM_WINDOW;
	right = newSuperCol+ITEM_WINDOW;

	if (right < 0)
		goto exit;
	if (right >= gNumSuperTilesWide)
		goto exit;

	if (top < 0)
		top = 0;
//		goto exit;
	else
	if (top >= gNumSuperTilesDeep)
		return;

//	if (bottom >= gNumSuperTilesDeep)
//		goto exit;
	if (bottom < 0)
		goto exit;

	ScanForPlayfieldItems(top,bottom,right,right);
	
exit:
    gCurrentSuperTileCol++;	
}


/********************** SCROLL TERRAIN RIGHT *************************/

static void ScrollTerrainRight(long superCol, long superRow, long tileCol, long tileRow)
{
long	col,i,row;
int32_t	superTileNum;
long	top,bottom,left;

	gHiccupEliminator = 0;															// reset this when about to make a new row/col of supertiles

			/* PURGE OLD RIGHT ROW */

	col = gCurrentSuperTileCol+SUPERTILE_DIST_WIDE-1;						// calc supertile col # for right col

	if ((col < gNumSuperTilesWide) && (col >= 0))							// check if off map
	{
		for (i = 0; i < SUPERTILE_DIST_DEEP; i++)
		{
			row = gCurrentSuperTileRow+i;
			if (row >= gNumSuperTilesDeep)									// check if off map
				break;
			if (row < 0)
				continue;

			superTileNum = gTerrainScrollBuffer[row][col];						// get terrain object for that spot
			if (superTileNum != EMPTY_SUPERTILE)
			{
				ReleaseSuperTileObject(superTileNum);		 					// free the terrain object
				gTerrainScrollBuffer[row][col] = EMPTY_SUPERTILE;
			}
		}
	}

		/* CREATE NEW LEFT ROW */

	if (superCol < 0) 														// see if off map
		return;
	if (superCol >= gNumSuperTilesWide)
		return;

	for (row = gCurrentSuperTileRow; row < (gCurrentSuperTileRow + SUPERTILE_DIST_DEEP); row++)
	{
		if (row >= gNumSuperTilesDeep)										// see if off map
			goto check_items;
		if (row < 0)
			goto next;

		if (gTerrainScrollBuffer[row][superCol] == EMPTY_SUPERTILE)						// see if something is already there
		{
			if ((tileRow >= 0) && (tileRow < gTerrainTileDepth))
			{
				superTileNum = BuildTerrainSuperTile(tileCol,tileRow); 				// make new terrain object
				gTerrainScrollBuffer[row][superCol] = superTileNum;					// save into scroll buffer array
			}
		}
next:
		tileRow += SUPERTILE_SIZE;
	}

			/* ADD ITEMS ON LEFT */

check_items:
	top = superRow-ITEM_WINDOW;
	bottom = superRow + (SUPERTILE_DIST_DEEP-1)+ITEM_WINDOW;
	left = superCol-ITEM_WINDOW;

	if (left < 0)
		return;
	if (left >= gNumSuperTilesWide)
		return;

	if (top >= gNumSuperTilesDeep)
		return;
	if (top < 0)
		top = 0;

//	if (bottom >= gNumSuperTilesDeep)
//		bottom = gNumSuperTilesDeep-1;
	if (bottom < 0)
		return;

	ScanForPlayfieldItems(top,bottom,left,left);
}


/**************** PRIME INITIAL TERRAIN ***********************/
//
// INPUT:	justReset = true if only resetting to last checkpoint
//

void PrimeInitialTerrain(Boolean justReset)
{
long	i,w;

	gDisableHiccupTimer = true;
	
			/* PRIME OTHER STUFF */
			
	if (!justReset)
	{
		PrimeSplines();
		PrimeFences();
	}		

			/* PRIME THE SUPERTILES */
			
	w = SUPERTILE_DIST_WIDE+ITEM_WINDOW+1;

	gCurrentSuperTileCol -= w;								// start left and scroll into position

	for (i=0; i < w; i++)
	{
		ScrollTerrainLeft();
		CalcNewItemDeleteWindow();							// recalc item delete window
	}	
	
}


/**************** IS SUPERTILE VISIBLE *******************/
//
// Returns false if is not in current camera's viewing frustum
//

static Boolean IsSuperTileVisible(int32_t superTileNum, Byte layer)
{
	const SuperTileMemoryType* superTile = &gSuperTileMemoryList[superTileNum];
	float radius = superTile->radius[layer];
	return IsSphereInFrustum_XZ(&superTile->coord[layer], radius);
}


/*************** CALCULATE SPLIT MODE MATRIX ***********************/

void CalculateSplitModeMatrix(void)
{
int		row,col;
Byte	numLayers,j;
float	y0,y1,y2,y3;

	if (gDoCeiling)
		numLayers = 2;
	else
		numLayers = 1;


	for (j = 0; j < numLayers; j++)							// floor & ceiling
	{
		for (row = 0; row < gTerrainTileDepth; row++)
		{	
			for (col = 0; col < gTerrainTileWidth; col++)
			{
					/* GET Y COORDS OF 4 VERTICES */
					
				y0 = gMapYCoords[row][col].layerY[j];
				y1 = gMapYCoords[row][col+1].layerY[j];
				y2 = gMapYCoords[row+1][col+1].layerY[j];
				y3 = gMapYCoords[row+1][col].layerY[j];

						/* QUICK CHECK FOR FLAT POLYS */
	
				if ((y0 == y1) && (y0 == y2) && (y0 == y3))							// see if all same level
					gMapInfoMatrix[row][col].splitMode[j] = SPLIT_BACKWARD;
					
					/* CALC FOLD-SPLIT */
				else
				{
					if (fabs(y0-y2) < fabs(y1-y3))
						gMapInfoMatrix[row][col].splitMode[j] = SPLIT_BACKWARD; 	// use \ splits
					else
						gMapInfoMatrix[row][col].splitMode[j] = SPLIT_FORWARD;		// use / splits
				}				
				

	
			}
		}
	}



}

