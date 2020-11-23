/****************************/
/*     TERRAIN.C           */
/* By Brian Greenstone      */
/****************************/

/***************/
/* EXTERNALS   */
/***************/



extern	ObjNode		*gFirstNodePtr;
extern	TerrainItemEntryType	**gTerrainItemLookupTableX;
extern	TQ3Matrix4x4		gCameraWorldToViewMatrix, gCameraViewToFrustumMatrix;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	TQ3Point3D		gMyCoord;
extern	TerrainYCoordType		**gMapYCoords;
extern	TerrainInfoMatrixType	**gMapInfoMatrix;
extern	TerrainItemEntryType 	**gMasterItemList;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	float					gYon;
extern	long					gNumSplines;
extern	SplineDefType			**gSplineList;
extern	FenceDefType	*gFenceList;
extern	long			gNumFences;
extern	long		gMyStartX,gMyStartZ;
extern	TQ3Point3D	gMostRecentCheckPointCoord;

/****************************/
/*  PROTOTYPES             */
/****************************/

static void ScrollTerrainUp(long superRow, long superCol);
static void ScrollTerrainDown(long superRow, long superCol);
static void ScrollTerrainLeft(void);
static void ScrollTerrainRight(long superCol, long superRow, long tileCol, long tileRow);
static short GetFreeSuperTileMemory(void);
static inline void ReleaseSuperTileObject(short superTileNum);
static void CalcNewItemDeleteWindow(void);
static short	BuildTerrainSuperTile(long	startCol, long startRow);
static Boolean IsSuperTileVisible(short superTileNum, Byte layer);
static void DrawTileIntoMipmap(uint16_t tile, int row, int col, uint16_t* buffer);
static inline void	ShrinkSuperTileTextureMap(const u_short *srcPtr,u_short *destPtr);
static inline void	ShrinkSuperTileTextureMapTo64(u_short *srcPtr,u_short *destPtr);
static inline void ReleaseAllSuperTiles(void);
static void BuildSuperTileLOD(SuperTileMemoryType *superTilePtr, short lod);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	ITEM_WINDOW		1			// # supertiles for item add window (must be integer)
#define	OUTER_SIZE		0.6f		// size of border out of add window for delete window (can be float)


/**********************/
/*     VARIABLES      */
/**********************/


int				gSuperTileActiveRange;

Boolean			gDoCeiling;
Boolean			gDisableHiccupTimer = false;

static u_char	gHiccupEliminator = 0;

u_short	**gTileDataHandle;

u_short	**gFloorMap = nil;								// 2 dimensional array of u_shorts (allocated below)
u_short	**gCeilingMap = nil;
#if USE_PATH_LAYER
u_char	**gPathMap = nil;
#endif
u_short	**gVertexColors[2];

long	gTerrainTileWidth,gTerrainTileDepth;			// width & depth of terrain in tiles
long	gTerrainUnitWidth,gTerrainUnitDepth;			// width & depth of terrain in world units (see TERRAIN_POLYGON_SIZE)

long	gNumTerrainTextureTiles = 0;
Ptr		gTerrainHeightMapPtrs[MAX_HEIGHTMAP_TILES];

long	gNumSuperTilesDeep,gNumSuperTilesWide;	  		// dimensions of terrain in terms of supertiles
long	gCurrentSuperTileRow,gCurrentSuperTileCol;

u_char	gTerrainScrollBuffer[MAX_SUPERTILES_DEEP][MAX_SUPERTILES_WIDE];		// 2D array which has index to supertiles for each possible supertile

short	gNumFreeSupertiles;
static	SuperTileMemoryType		gSuperTileMemoryList[MAX_SUPERTILES];
static Boolean gSuperTileMemoryListExists = false;

float	gTerrainItemDeleteWindow_Near,gTerrainItemDeleteWindow_Far,
		gTerrainItemDeleteWindow_Left,gTerrainItemDeleteWindow_Right;

//TileAttribType	**gTileAttributes = nil;

u_short		gTileFlipRotBits;
short		gTileAttribParm0;
Byte		gTileAttribParm1,gTileAttribParm2;

float		gSuperTileRadius;			// normal x/z radius

float		gUnitToPixel = 1.0f/(TERRAIN_POLYGON_SIZE/TERRAIN_HMTILE_SIZE);

const float gOneOver_TERRAIN_POLYGON_SIZE = (1.0f / TERRAIN_POLYGON_SIZE);

static Handle	gTerrainTextureBuffers[MAX_SUPERTILES][2][NUM_LOD];

			/* TILE SPLITTING TABLES */
			
					
					/* /  */
const static	Byte			gTileTriangles1_A[SUPERTILE_SIZE][SUPERTILE_SIZE][3] =
{
	6,0,1,		7,1,2,		8,2,3,		9,3,4,		10,4,5,
	12,6,7,		13,7,8,		14,8,9,		15,9,10,	16,10,11,
	18,12,13,	19,13,14,	20,14,15,	21,15,16,	22,16,17,
	24,18,19,	25,19,20,	26,20,21,	27,21,22,	28,22,23,
	30,24,25,	31,25,26,	32,26,27,	33,27,28,	34,28,29
};

const static	Byte			gTileTriangles2_A[SUPERTILE_SIZE][SUPERTILE_SIZE][3] =
{
	6,1,7,		7,2,8,		8,3,9,		9,4,10,		10,5,11,
	12,7,13,	13,8,14,	14,9,15,	15,10,16,	16,11,17,
	18,13,19,	19,14,20,	20,15,21,	21,16,22,	22,17,23,
	24,19,25,	25,20,26,	26,21,27,	27,22,28,	28,23,29,
	30,25,31,	31,26,32,	32,27,33,	33,28,34,	34,29,35
};

					/* \  */
const static	Byte			gTileTriangles1_B[SUPERTILE_SIZE][SUPERTILE_SIZE][3] =
{
	0,7,6,		1,8,7,		2,9,8,		3,10,9,		4,11,10,
	6,13,12,	7,14,13,	8,15,14,	9,16,15,	10,17,16,
	12,19,18,	13,20,19,	14,21,20,	15,22,21,	16,23,22,
	18,25,24,	19,26,25,	20,27,26,	21,28,27,	22,29,28,
	24,31,30,	25,32,31,	26,33,32,	27,34,33,	28,35,34
};

const static	Byte			gTileTriangles2_B[SUPERTILE_SIZE][SUPERTILE_SIZE][3] =
{
	0,1,7,		1,2,8,		2,3,9,		3,4,10,		4,5,11,
	6,7,13,		7,8,14,		8,9,15,		9,10,16,	10,11,17,
	12,13,19,	13,14,20,	14,15,21,	15,16,22,	16,17,23,
	18,19,25,	19,20,26,	20,21,27,	21,22,28,	22,23,29,
	24,25,31,	25,26,32,	26,27,33,	27,28,34,	28,29,35
};

const static	Byte			gTileTriangleWinding[2][3] =
{
	{ 2, 1, 0 },  // floor
	{ 0, 1, 2 },  // ceiling
};


TQ3Point3D		gWorkGrid[SUPERTILE_SIZE+1][SUPERTILE_SIZE+1];
u_short			*gTempTextureBuffer = nil;

TQ3Vector3D		gRecentTerrainNormal[2];							// from _Planar



/****************** INIT TERRAIN MANAGER ************************/
//
// Only called at boot!
//

void InitTerrainManager(void)
{
 	gSuperTileRadius = sqrt(2) * (TERRAIN_SUPERTILE_UNIT_SIZE/2);

	ClearScrollBuffer();
	
	
			/* ALLOC TEMP TEXTURE BUFF */
			//
			// This is the full 160x160 buffer that tiles are drawn into.
			//
			
	if (gTempTextureBuffer == nil)
	{
		gTempTextureBuffer = (u_short *)AllocPtr(TEMP_TEXTURE_BUFF_SIZE * TEMP_TEXTURE_BUFF_SIZE * sizeof(u_short *));
		GAME_ASSERT(gTempTextureBuffer);
	}
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
		Free_2d_array(gFloorMap);
		gFloorMap = nil;
	}

	if (gCeilingMap != nil)
	{
		Free_2d_array(gCeilingMap);
		gCeilingMap = nil;
	}

#if USE_PATH_LAYER
	if (gPathMap != nil)
	{
		Free_2d_array(gPathMap);
		gPathMap = nil;
	}
#endif	

	if (gMapYCoords)
	{
		Free_2d_array(gMapYCoords);
		gMapYCoords = nil;
	}
	
	if (gMapInfoMatrix)
	{
		Free_2d_array(gMapInfoMatrix);
		gMapInfoMatrix = nil;
	}

	if (gVertexColors[0])
	{
		Free_2d_array(gVertexColors[0]);
		gVertexColors[0] = nil;
	}
	if (gVertexColors[1])
	{
		Free_2d_array(gVertexColors[1]);
		gVertexColors[1] = nil;
	}
			/* NUKE SPLINE DATA */

	if (gSplineList)
	{
		for (i = 0; i < gNumSplines; i++)
		{
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
long							u,v,i,j,maxSuperTilesNeeded,numLayers;
TQ3Status						status;
TQ3TriMeshData					triMeshData;
static 	TQ3Point3D				p[NUM_VERTICES_IN_SUPERTILE];
static 	TQ3TriMeshTriangleData	newTriangle[NUM_TRIS_IN_SUPERTILE];
static	TQ3TriMeshAttributeData	triangleAttribs,vertAttribs[2];
static	TQ3Vector3D				faceNormals[NUM_TRIS_IN_SUPERTILE];
static  TQ3ColorRGB				vertexColors[NUM_VERTICES_IN_SUPERTILE];
static	TQ3Param2D				uvs[NUM_VERTICES_IN_SUPERTILE];
TQ3Object						tempTriMesh;
static const int				textureSize[NUM_LOD] = {SUPERTILE_TEXMAP_SIZE, SUPERTILE_TEXMAP_SIZE/2, SUPERTILE_TEXMAP_SIZE/4};


	maxSuperTilesNeeded = gSuperTileActiveRange*gSuperTileActiveRange*4;		// calc # supertiles we will need
	if (gDoCeiling)
		numLayers = 2;
	else
		numLayers = 1;

			/* INIT UV LIST */
			
	i = 0;	
	for (v = 0; v <= SUPERTILE_SIZE; v++)						// sets uv's 0.0 -> 1.0 for single texture map
	{
		for (u = 0; u <= SUPERTILE_SIZE; u++)
		{
			uvs[i].u = (float)u / (float)SUPERTILE_SIZE;
			uvs[i].v = 1.0f - ((float)v / (float)SUPERTILE_SIZE);	
			i++;
		}	
	}
	
	
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
					
				/* SET DATA */

	triangleAttribs.attributeType 		= kQ3AttributeTypeNormal;			// set attribute Type
	triangleAttribs.data 				= &faceNormals[0];					// point to attribute data
	triangleAttribs.attributeUseArray 	= nil;								// (not used)

	vertAttribs[0].attributeUseArray 	= nil;								// (not used)
	vertAttribs[1].attributeUseArray 	= nil;										

	vertAttribs[0].attributeType 		= kQ3AttributeTypeShadingUV;		// set attrib type == shading UV
	vertAttribs[0].data 				= &uvs[0];							// point to vertex UV's
	
					/* SET 2ND ATTRIBUTE TO DIFFUSE COLOR */
					//
					// NOTE: Depending on whether the terrain is pre-lit or not will
					//		make attrib #1 either a diffuse color or a vertex normal.
					//		Since TQ3ColorRGB and TQ3Vector3D are both 3-floats in
					//		size, they can share the same data array.  All that 
					//		needs to be done later is to change the attributeType
					//		field to normal or diffuse depending on the need.
					//
					
					
	vertAttribs[1].attributeType 		= kQ3AttributeTypeDiffuseColor;		// set attrib type == diffuse color
	vertAttribs[1].data 				= &vertexColors[0];					// point to vertex colors



			/********************************************/
			/* FOR EACH POSSIBLE SUPERTILE ALLOC MEMORY */
			/********************************************/

	gNumFreeSupertiles = maxSuperTilesNeeded;
				
	for (i = 0; i < maxSuperTilesNeeded; i++)
	{
		gSuperTileMemoryList[i].mode = SUPERTILE_MODE_FREE;					// it's free for use

				/**************************************/
				/* CREATE TEXTURE FOR FLOOR & CEILING */
				/**************************************/
				
		for (j = 0; j < numLayers; j++)
		{
			int						size;				
			Handle					blankTexHand;
			TQ3SurfaceShaderObject	blankTexObject;
			TQ3AttributeSet			geometryAttribSet;
			Byte					lod;
			
			
				/*****************************/
				/* DO OUR OWN FAUX-LOD THING */
				/*****************************/
				//
				// Normally there are 3 LOD's, but if we are low on memory, then we'll skip LOD #0 which
				// is the big one, and only do the other 2 smaller ones.
				//
				
			lod = 0;

				
			for (; lod < NUM_LOD; lod++)
			{
				size = textureSize[lod];										// get size of texture @ this lod
				
						/* MAKE BLANK TEXTURE */
	
				blankTexHand = AllocHandle(size * size * sizeof(short));	// alloc memory for texture
				GAME_ASSERT(blankTexHand);
				HLockHi(blankTexHand);
		
				blankTexObject = QD3D_Data16ToTexture_NoMip(*blankTexHand, size, size);	// create texture from buffer
				GAME_ASSERT(blankTexObject);
				
				gTerrainTextureBuffers[i][j][lod] = blankTexHand;				// keep this pointer since we need to manually dispose of this later
				
		
						/* ADD TO ATTRIBUTE SET */
							
				geometryAttribSet = Q3AttributeSet_New();						// make attrib set
				GAME_ASSERT(geometryAttribSet);

				status = Q3AttributeSet_Add(geometryAttribSet, kQ3AttributeTypeSurfaceShader, &blankTexObject);	
				GAME_ASSERT(status);
				Q3Object_Dispose(blankTexObject);								// free extra ref to shader
	
				gSuperTileMemoryList[i].texture[lod][j] = geometryAttribSet;	// save this reference to the attrib set
				
			}
		}	

				/*************************************/
				/* CREATE AN EMPTY TRIMESH STRUCTURE */
				/*************************************/
				 			
		triMeshData.triMeshAttributeSet			= nil;
		triMeshData.numTriangles 				= NUM_TRIS_IN_SUPERTILE;	// n triangles in each trimesh
		triMeshData.triangles 					= &newTriangle[0];			// point to triangle list
		triMeshData.numTriangleAttributeTypes 	= 1;						// all triangles share the same attribs
		triMeshData.triangleAttributeTypes 		= &triangleAttribs;
		triMeshData.numEdges 					= 0;						// our trimesh doesnt have any edge info
		triMeshData.edges 						= nil;
		triMeshData.numEdgeAttributeTypes 		= 0;						
		triMeshData.edgeAttributeTypes 			= nil;
		triMeshData.numPoints 					= NUM_VERTICES_IN_SUPERTILE;// set n # vertices		
		triMeshData.points 						= &p[0];					// point to bogus temp point list
		triMeshData.numVertexAttributeTypes 	= 2;						// 2 attrib types: uv's, normals or colors
		triMeshData.vertexAttributeTypes 		= &vertAttribs[0];
		triMeshData.bBox.isEmpty 				= kQ3False;					// calc bounding box			
		triMeshData.bBox.min.x = triMeshData.bBox.min.y = triMeshData.bBox.min.z = 0;
		triMeshData.bBox.max.x = triMeshData.bBox.max.y = triMeshData.bBox.max.z = TERRAIN_SUPERTILE_UNIT_SIZE;
		

				/***************************/
				/* CREATE THE TRIMESH DATA */
				/***************************/
				//
				// I'm doing a little trick here.  I create a temporary Trimesh
				// object from our local data and then do a GetData on the new
				// Trimesh.  This will effectively give me a new copy of
				// all of the TriMesh data so that I don't have to 
				// allocate the memory myself.
				//
				// Once I'm done, I can throw the TriMesh object out since I dont
				// need it anymore. I simply need to remember to do a ClearData when I want
				// to dispose of this data.
				//
					
		for (j = 0; j < numLayers; j++)											// do it for floor & ceiling trimeshes
		{
			tempTriMesh = Q3TriMesh_New(&triMeshData);					// make the temp trimesh
			GAME_ASSERT(tempTriMesh);

			status = Q3TriMesh_GetData(tempTriMesh, &gSuperTileMemoryList[i].triMeshData[j]);	// get the data
			GAME_ASSERT(status);

			Q3Object_Dispose(tempTriMesh);								// dispose of the temp trimesh object
		}
	}	// i
	
	gSuperTileMemoryListExists = true;
}


/********************* DISPOSE SUPERTILE MEMORY LIST **********************/

void DisposeSuperTileMemoryList(void)
{
int		numSuperTiles,i,j,lod,numLayers;

    if (gSuperTileMemoryListExists == false)
        return;

	if (gDoCeiling)
		numLayers = 2;
	else
		numLayers = 1;
		

			/*************************************/
			/* FOR EACH SUPERTILE DEALLOC MEMORY */
			/*************************************/
	
	numSuperTiles = gSuperTileActiveRange*gSuperTileActiveRange*4;
				
	for (i = 0; i < numSuperTiles; i++)
	{

				
		for (j = 0; j < numLayers; j++)
		{
				/* NUKE TEXTURES */
				
			for (lod = 0; lod < NUM_LOD; lod++)
			{
				Q3Object_Dispose(gSuperTileMemoryList[i].texture[lod][j]);
				DisposeHandle(gTerrainTextureBuffers[i][j][lod]);	
			}
										
						
						
				/* NUKE TRIMESH DATA */
				//
				// Remember to set triMesh attribute set to nil since it was nil when we created the triMesh earlier.
				//
					
			gSuperTileMemoryList[i].triMeshData[j].triMeshAttributeSet = nil;
			
			Q3TriMesh_EmptyData(&gSuperTileMemoryList[i].triMeshData[j]);

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
short	i;

				/* SCAN FOR A FREE BLOCK */

	for (i = 0; i < MAX_SUPERTILES; i++)
	{
		if (gSuperTileMemoryList[i].mode == SUPERTILE_MODE_FREE)
		{
			gSuperTileMemoryList[i].mode = SUPERTILE_MODE_USED;
			gNumFreeSupertiles--;			
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
TQ3Status			status;
long	 			row,col,row2,col2,i;
Byte				j;
short				superTileNum;
float				height,miny,maxy;
TQ3TriMeshData		*triMeshData;
TQ3Vector3D			*faceNormal,*vertexNormalList;
u_short				tile;
TQ3Point3D			*pointList;
TQ3TriMeshTriangleData	*triangleList;
TQ3StorageObject	mipmapStorage;
unsigned char		*buffer;
u_long				validSize,bufferSize;
SuperTileMemoryType	*superTilePtr;
TQ3ColorRGB			*vertexColorList;
float				b;
float				ambientR,ambientG,ambientB;
float				fillR0,fillG0,fillB0;
float				fillR1,fillG1,fillB1;
TQ3Vector3D			*fillDir0,*fillDir1;
Byte				numFillLights, numLayers;
static  TQ3Vector3D	tempVertexNormalList[NUM_VERTICES_IN_SUPERTILE];


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
	
	for (j=0; j < NUM_LOD; j++)
		superTilePtr->hasLOD[j] = false;						// LOD isnt built yet

	superTilePtr->x = (startCol * TERRAIN_POLYGON_SIZE) + (TERRAIN_SUPERTILE_UNIT_SIZE/2);		// also remember world coords
	superTilePtr->z = (startRow * TERRAIN_POLYGON_SIZE) + (TERRAIN_SUPERTILE_UNIT_SIZE/2);

	superTilePtr->left = (startCol * TERRAIN_POLYGON_SIZE);		// also save left/back coord
	superTilePtr->back = (startRow * TERRAIN_POLYGON_SIZE);

			
		/* GET LIGHT DATA */
		
	b = gGameViewInfoPtr->lightList.ambientBrightness;				// get ambient brightness
	ambientR = gGameViewInfoPtr->lightList.ambientColor.r * b;		// calc ambient color
	ambientG = gGameViewInfoPtr->lightList.ambientColor.g * b;		
	ambientB = gGameViewInfoPtr->lightList.ambientColor.b * b;		
		
	b = gGameViewInfoPtr->lightList.fillBrightness[0];				// get fill brightness 0
	fillR0 = gGameViewInfoPtr->lightList.fillColor[0].r * b;		// calc ambient color
	fillG0 = gGameViewInfoPtr->lightList.fillColor[0].g * b;		
	fillB0 = gGameViewInfoPtr->lightList.fillColor[0].b * b;		
	fillDir0 = &gGameViewInfoPtr->lightList.fillDirection[0];		// get fill direction

	numFillLights = gGameViewInfoPtr->lightList.numFillLights;
	if (numFillLights > 1)
	{
		b = gGameViewInfoPtr->lightList.fillBrightness[1];			// get fill brightness 1
		fillR1 = gGameViewInfoPtr->lightList.fillColor[1].r * b;	// calc ambient color
		fillG1 = gGameViewInfoPtr->lightList.fillColor[1].g * b;		
		fillB1 = gGameViewInfoPtr->lightList.fillColor[1].b * b;		
		fillDir1 = &gGameViewInfoPtr->lightList.fillDirection[1];
	}

		/***********************************************************/
		/*                DO FLOOR & CEILING LAYERS                */
		/***********************************************************/
					
	for (j = 0; j < numLayers; j++)													// do floor & ceiling
	{
					/*******************/
					/* GET THE TRIMESH */
					/*******************/
					
		triMeshData = &gSuperTileMemoryList[superTileNum].triMeshData[j];	// get ptr to triMesh data
		pointList = triMeshData->points;									// get ptr to point/vertex list
		triangleList = triMeshData->triangles;								// get ptr to triangle index list
		faceNormal = triMeshData->triangleAttributeTypes->data;				// get ptr to face normals		
					
				/* DETERMING IF VERTEX ATTRIBS HAVE NORMALS OR COLOR */
					
		vertexColorList = triMeshData->vertexAttributeTypes[1].data;	// get ptr to vertex color
		triMeshData->vertexAttributeTypes[1].attributeType = kQ3AttributeTypeDiffuseColor;
		
		vertexNormalList = &tempVertexNormalList[0];					// need scratch area for temp normals
	
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
					height = gMapYCoords[row][col].layerY[j]; 				// get pixel height here
	
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
	
						/* SET VERTEX COORDS */
		
		i = 0;			
		for (row = 0; row < (SUPERTILE_SIZE+1); row++)
		{
			for (col = 0; col < (SUPERTILE_SIZE+1); col++)
				pointList[i++] = gWorkGrid[row][col];						// copy from other list
		}
	
					/* UPDATE TRIMESH DATA WITH NEW INFO */
					
		i = 0;			
		for (row2 = 0; row2 < SUPERTILE_SIZE; row2++)
		{
			row = row2 + startRow;
	
			for (col2 = 0; col2 < SUPERTILE_SIZE; col2++)
			{
	
				col = col2 + startCol;
	
						/* ADD TILE TO PIXMAP */
						
				if (j == 0)
					tile = gFloorMap[row][col];				// get tile from floor map...
				else
					tile = gCeilingMap[row][col];				// ...or get tile from ceiling map
					
				DrawTileIntoMipmap(tile,row2,col2,gTempTextureBuffer);		// draw into mipmap


						/* SET SPLITTING INFO */

				const Byte* tri1;
				const Byte* tri2;

				if (gMapInfoMatrix[row][col].splitMode[j] == SPLIT_BACKWARD)	// set coords & uv's based on splitting
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

				triangleList[i].pointIndices[0] 	= tri1[gTileTriangleWinding[j][0]];
				triangleList[i].pointIndices[1] 	= tri1[gTileTriangleWinding[j][1]];
				triangleList[i++].pointIndices[2] 	= tri1[gTileTriangleWinding[j][2]];
				triangleList[i].pointIndices[0] 	= tri2[gTileTriangleWinding[j][0]];
				triangleList[i].pointIndices[1] 	= tri2[gTileTriangleWinding[j][1]];
				triangleList[i++].pointIndices[2] 	= tri2[gTileTriangleWinding[j][2]];
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
							CalcTileNormals(j, rr+startRow, (cc>>1)+startCol, &nA,&nB);		// calculate the 2 face normals for this tile
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
					u_short	color = gVertexColors[j][row+startRow][col+startCol];
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
				
				/******************/
				/* UPDATE TEXTURE */
				/******************/
				//
				// If we are in low-memory mode, then we shrink the texture to LOD #1 instead of LOD #0 and we shrink it to 64x64
				//
		{
			mipmapStorage = QD3D_GetMipmapStorageObjectFromAttrib(gSuperTileMemoryList[superTileNum].texture[0][j]);// get storage object of LOD #0
			GAME_ASSERT(mipmapStorage);

			status = Q3MemoryStorage_GetBuffer(mipmapStorage, &buffer, &validSize, &bufferSize);			// get ptr to the buffer
			GAME_ASSERT(status);
			
			ShrinkSuperTileTextureMap(gTempTextureBuffer,(u_short *)buffer);								// shrink to 128x128
			superTilePtr->hasLOD[0] = true;
		}
			
		status = Q3MemoryStorage_SetBuffer(mipmapStorage, buffer, validSize, bufferSize);
		GAME_ASSERT(status);
		Q3Object_Dispose(mipmapStorage);												// nuke the mipmap storage object

				/***********************/
				/* CALC COORD & RADIUS */
				/***********************/
	
		superTilePtr->y[j] = (miny+maxy)*.5f;	// This y coord is not used to translate since the terrain has no translation matrix
												// Instead, this is used by the cone-of-vision routine for culling tests
				
		height = (maxy-miny) * .5f;
		if (height > gSuperTileRadius)
			superTilePtr->radius[j] = height;
		else						
			superTilePtr->radius[j] = gSuperTileRadius;
	
	
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


	}	// j (layer)
									
	return(superTileNum);
}


/********************** BUILD SUPERTILE LEVEL OF DETAIL ********************/
//
// Called from DrawTerrain to generate the LOD's from the source LOD=0 geometry
//

static void BuildSuperTileLOD(SuperTileMemoryType *superTilePtr, short lod)
{
short				j,x,y;
TQ3AttributeSet		baseData,newData;
u_short				*baseBuffer, *newBuffer, *nextLine, *newBufferPtr;
u_long				validSize,bufferSize;
TQ3StorageObject 	baseTexture,newTexture;
int					size,numLayers;


	if (superTilePtr->hasLOD[lod])									// see if already has LOD
		return;
		
	if (lod >= NUM_LOD)
		DoFatalAlert("BuildSuperTileLOD: lod overflow");
		
	superTilePtr->hasLOD[lod] = true;								// has LOD now

	if (gDoCeiling)
		numLayers = 2;
	else
		numLayers = 1;

	for (j = 0; j < numLayers; j++)
	{
			/*************************/
			/* SHRINK & COPY TEXTURE */
			/*************************/
			
		switch(lod)
		{
							/***********/
							/* 1ST LOD */
							/***********/
							
			case	1:
						
						/* GET POINTERS TO TEXTURE BUFFERS */
							
					baseData = superTilePtr->texture[0][j];					// get level 0 attrib set
					newData = superTilePtr->texture[1][j];					// get level 1 attrib set
					
					baseTexture = QD3D_GetMipmapStorageObjectFromAttrib(baseData);
					Q3MemoryStorage_GetBuffer(baseTexture, (u_char **)&baseBuffer,&validSize, &bufferSize);
					Q3Object_Dispose(baseTexture);
					
					newTexture = QD3D_GetMipmapStorageObjectFromAttrib(newData);
					Q3MemoryStorage_GetBuffer(newTexture, (u_char **)&newBufferPtr,&validSize, &bufferSize);
							
					newBuffer = newBufferPtr;
					nextLine = baseBuffer + SUPERTILE_TEXMAP_SIZE;
					
					size = SUPERTILE_TEXMAP_SIZE/2;
					
					for (y = 0; y < size; y++)
					{
						for (x = 0; x < size; x++)
						{
							u_short	r,g,b;
							u_short	pixel;
							
							pixel = *baseBuffer++;							// get a pixel
							r = (pixel >> 10) & 0x1f;	
							g = (pixel >> 5) & 0x1f;	
							b = pixel & 0x1f;	
							
							pixel = *baseBuffer++;							// get next pixel
							r += (pixel >> 10) & 0x1f;	
							g += (pixel >> 5) & 0x1f;	
							b += pixel & 0x1f;	
			
			
							pixel = *nextLine++;							// get a pixel from next line
							r += (pixel >> 10) & 0x1f;	
							g += (pixel >> 5) & 0x1f;	
							b += pixel & 0x1f;	
							
							pixel = *nextLine++;							// get next pixel from next line
							r += (pixel >> 10) & 0x1f;	
							g += (pixel >> 5) & 0x1f;	
							b += pixel & 0x1f;	
							
							r >>= 2;										// calc average
							g >>= 2;
							b >>= 2;
							
							*newBuffer++ = (r<<10) | (g<<5) | b;			// save new pixel
						}
						baseBuffer += SUPERTILE_TEXMAP_SIZE;				// skip a line
						nextLine += SUPERTILE_TEXMAP_SIZE;				
					}	
					break;
					
							/***********/
							/* 2nd LOD */
							/***********/
							//
							// LOD #2 is built from LOD #1
							//
					
			case	2:
						/* GET POINTERS TO TEXTURE BUFFERS */
							
					baseData = superTilePtr->texture[1][j];					// get level 1 attrib set
					newData = superTilePtr->texture[2][j];					// get level 2 attrib set
					baseTexture = QD3D_GetMipmapStorageObjectFromAttrib(baseData);
					Q3MemoryStorage_GetBuffer(baseTexture, (u_char **)&baseBuffer,
											&validSize, &bufferSize);
					Q3Object_Dispose(baseTexture);
					newTexture = QD3D_GetMipmapStorageObjectFromAttrib(newData);
					Q3MemoryStorage_GetBuffer(newTexture, (u_char **)&newBufferPtr,
											&validSize, &bufferSize);
							
					newBuffer = newBufferPtr;
					nextLine = baseBuffer + SUPERTILE_TEXMAP_SIZE/2;
					
					size = SUPERTILE_TEXMAP_SIZE/4;
					
					for (y = 0; y < size; y++)
					{
						for (x = 0; x < size; x++)
						{
							u_short	r,g,b;
							u_short	pixel;
							
							pixel = *baseBuffer++;							// get a pixel
							r = (pixel >> 10) & 0x1f;	
							g = (pixel >> 5) & 0x1f;	
							b = pixel & 0x1f;	
							
							pixel = *baseBuffer++;							// get next pixel
							r += (pixel >> 10) & 0x1f;	
							g += (pixel >> 5) & 0x1f;	
							b += pixel & 0x1f;	
			
			
							pixel = *nextLine++;							// get a pixel from next line
							r += (pixel >> 10) & 0x1f;	
							g += (pixel >> 5) & 0x1f;	
							b += pixel & 0x1f;	
							
							pixel = *nextLine++;							// get next pixel from next line
							r += (pixel >> 10) & 0x1f;	
							g += (pixel >> 5) & 0x1f;	
							b += pixel & 0x1f;	
							
							r >>= 2;										// calc average
							g >>= 2;
							b >>= 2;
							
							*newBuffer++ = (r<<10) | (g<<5) | b;			// save new pixel
						}
						baseBuffer += SUPERTILE_TEXMAP_SIZE/2;				// skip a line
						nextLine += SUPERTILE_TEXMAP_SIZE/2;				
					}	
					break;
		}
		
				/* UPDATE THE BUFFER */

		Q3MemoryStorage_SetBuffer(newTexture, (u_char *)newBufferPtr, validSize, bufferSize);
		Q3Object_Dispose(newTexture);			
	}
}




/********************* DRAW TILE INTO MIPMAP *************************/

static void DrawTileIntoMipmap(uint16_t tile, int row, int col, uint16_t* buffer)
{
uint16_t		texMapNum,flipRotBits;
const uint64_t*	tileData64;
const uint16_t* tileData16;
int				y;	
uint64_t*		ptr64;
uint16_t*		ptr16;

			/* EXTRACT BITS INFO FROM TILE */
				
	flipRotBits = tile&(TILE_FLIPXY_MASK|TILE_ROTATE_MASK);		// get flip & rotate bits
	texMapNum = tile&TILENUM_MASK; 								// filter out texture #

	if (texMapNum >= gNumTerrainTextureTiles)					// make sure not illegal tile #
	{
//		DoFatalAlert("DrawTileIntoMipmap: illegal tile #");
		texMapNum = 0;
	}
				/* CALC PTRS */
				
	buffer += ((row * OREOMAP_TILE_SIZE) * (SUPERTILE_SIZE * OREOMAP_TILE_SIZE)) + (col * OREOMAP_TILE_SIZE);		// get dest
	tileData64 = (const uint64_t *)((*gTileDataHandle) + (texMapNum * OREOMAP_TILE_SIZE * OREOMAP_TILE_SIZE));		// get src


	switch(flipRotBits)         											// set uv's based on flip & rot bits
	{
				/* NO FLIP & NO ROT */
					/* XYFLIP ROT 2 */

		case	0:
		case	TILE_FLIPXY_MASK | TILE_ROT2:
				ptr64 = (uint64_t *)buffer;
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					ptr64[0] = tileData64[0];
					ptr64[1] = tileData64[1];
					ptr64[2] = tileData64[2];
					ptr64[3] = tileData64[3];
					ptr64[4] = tileData64[4];
					ptr64[5] = tileData64[5];
					ptr64[6] = tileData64[6];
					ptr64[7] = tileData64[7];
						
					ptr64 += (OREOMAP_TILE_SIZE/4)*SUPERTILE_SIZE;			// next line in dest
					tileData64 += (OREOMAP_TILE_SIZE/4);						// next line in src
				}
				break;

					/* FLIP X */
				/* FLIPY ROT 2 */

		case	TILE_FLIPX_MASK:
		case	TILE_FLIPY_MASK | TILE_ROT2:
				ptr16 = (uint16_t *)buffer;
				tileData16 = (const uint16_t *)tileData64;
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					ptr16[ 0] = tileData16[31];
					ptr16[ 1] = tileData16[30];
					ptr16[ 2] = tileData16[29];
					ptr16[ 3] = tileData16[28];
					ptr16[ 4] = tileData16[27];
					ptr16[ 5] = tileData16[26];
					ptr16[ 6] = tileData16[25];
					ptr16[ 7] = tileData16[24];
					ptr16[ 8] = tileData16[23];
					ptr16[ 9] = tileData16[22];
					ptr16[10] = tileData16[21];
					ptr16[11] = tileData16[20];
					ptr16[12] = tileData16[19];
					ptr16[13] = tileData16[18];
					ptr16[14] = tileData16[17];
					ptr16[15] = tileData16[16];
					ptr16[16] = tileData16[15];
					ptr16[17] = tileData16[14];
					ptr16[18] = tileData16[13];
					ptr16[19] = tileData16[12];
					ptr16[20] = tileData16[11];
					ptr16[21] = tileData16[10];
					ptr16[22] = tileData16[ 9];
					ptr16[23] = tileData16[ 8];
					ptr16[24] = tileData16[ 7];
					ptr16[25] = tileData16[ 6];
					ptr16[26] = tileData16[ 5];
					ptr16[27] = tileData16[ 4];
					ptr16[28] = tileData16[ 3];
					ptr16[29] = tileData16[ 2];
					ptr16[30] = tileData16[ 1];
					ptr16[31] = tileData16[ 0];
						
					ptr16 += OREOMAP_TILE_SIZE * SUPERTILE_SIZE;		// next line in dest
					tileData16 += OREOMAP_TILE_SIZE;					// next line in src
				}
				break;

					/* FLIP Y */
				/* FLIPX ROT 2 */

		case	TILE_FLIPY_MASK:
		case	TILE_FLIPX_MASK | TILE_ROT2:
				ptr64 = (uint64_t *)buffer;
				tileData64 += (OREOMAP_TILE_SIZE*(OREOMAP_TILE_SIZE-1)*2)/8;
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					ptr64[0] = tileData64[0];
					ptr64[1] = tileData64[1];
					ptr64[2] = tileData64[2];
					ptr64[3] = tileData64[3];
					ptr64[4] = tileData64[4];
					ptr64[5] = tileData64[5];
					ptr64[6] = tileData64[6];
					ptr64[7] = tileData64[7];
						
					ptr64 += (OREOMAP_TILE_SIZE/4)*SUPERTILE_SIZE;			// next line in dest
					tileData64 -= (OREOMAP_TILE_SIZE*2/8);					// next line in src
				}
				break;


				/* FLIP XY */
				/* NO FLIP ROT 2 */

		case	TILE_FLIPXY_MASK:
		case	TILE_ROT2:
				ptr16 = (uint16_t *)buffer;
				tileData16 = (uint16_t *)(tileData64 + (OREOMAP_TILE_SIZE*(OREOMAP_TILE_SIZE-1)*2)/8);
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					ptr16[ 0] = tileData16[31];
					ptr16[ 1] = tileData16[30];
					ptr16[ 2] = tileData16[29];
					ptr16[ 3] = tileData16[28];
					ptr16[ 4] = tileData16[27];
					ptr16[ 5] = tileData16[26];
					ptr16[ 6] = tileData16[25];
					ptr16[ 7] = tileData16[24];
					ptr16[ 8] = tileData16[23];
					ptr16[ 9] = tileData16[22];
					ptr16[10] = tileData16[21];
					ptr16[11] = tileData16[20];
					ptr16[12] = tileData16[19];
					ptr16[13] = tileData16[18];
					ptr16[14] = tileData16[17];
					ptr16[15] = tileData16[16];
					ptr16[16] = tileData16[15];
					ptr16[17] = tileData16[14];
					ptr16[18] = tileData16[13];
					ptr16[19] = tileData16[12];
					ptr16[20] = tileData16[11];
					ptr16[21] = tileData16[10];
					ptr16[22] = tileData16[ 9];
					ptr16[23] = tileData16[ 8];
					ptr16[24] = tileData16[ 7];
					ptr16[25] = tileData16[ 6];
					ptr16[26] = tileData16[ 5];
					ptr16[27] = tileData16[ 4];
					ptr16[28] = tileData16[ 3];
					ptr16[29] = tileData16[ 2];
					ptr16[30] = tileData16[ 1];
					ptr16[31] = tileData16[ 0];
						
					ptr16 += (OREOMAP_TILE_SIZE/4)*SUPERTILE_SIZE*4;			// next line in dest
					tileData16 -= (OREOMAP_TILE_SIZE*2/2);					// next line in src
				}
				break;

				/* NO FLIP ROT 1 */
				/* FLIP XY ROT 3 */

		case	TILE_ROT1:
		case	TILE_FLIPXY_MASK | TILE_ROT3:
				ptr16 = (uint16_t *)buffer + (OREOMAP_TILE_SIZE-1);			// draw to right col from top row of src
				tileData16 = (uint16_t *)tileData64;
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 0] = tileData16[ 0];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 1] = tileData16[ 1];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 2] = tileData16[ 2];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 3] = tileData16[ 3];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 4] = tileData16[ 4];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 5] = tileData16[ 5];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 6] = tileData16[ 6];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 7] = tileData16[ 7];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 8] = tileData16[ 8];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 9] = tileData16[ 9];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*10] = tileData16[10];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*11] = tileData16[11];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*12] = tileData16[12];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*13] = tileData16[13];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*14] = tileData16[14];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*15] = tileData16[15];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*16] = tileData16[16];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*17] = tileData16[17];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*18] = tileData16[18];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*19] = tileData16[19];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*20] = tileData16[20];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*21] = tileData16[21];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*22] = tileData16[22];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*23] = tileData16[23];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*24] = tileData16[24];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*25] = tileData16[25];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*26] = tileData16[26];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*27] = tileData16[27];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*28] = tileData16[28];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*29] = tileData16[29];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*30] = tileData16[30];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*31] = tileData16[31];
						
					ptr16--;											// prev col in dest
					tileData16 += OREOMAP_TILE_SIZE;					// next line in src
				}
				break;

				/* NO FLIP ROT 3 */
				/* FLIP XY ROT 1 */

		case	TILE_ROT3:
		case	TILE_FLIPXY_MASK | TILE_ROT1:
				ptr16 = (u_short *)buffer;
				tileData16 = (u_short *)tileData64;
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*31] = tileData16[ 0];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*30] = tileData16[ 1];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*29] = tileData16[ 2];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*28] = tileData16[ 3];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*27] = tileData16[ 4];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*26] = tileData16[ 5];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*25] = tileData16[ 6];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*24] = tileData16[ 7];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*23] = tileData16[ 8];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*22] = tileData16[ 9];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*21] = tileData16[10];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*20] = tileData16[11];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*19] = tileData16[12];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*18] = tileData16[13];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*17] = tileData16[14];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*16] = tileData16[15];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*15] = tileData16[16];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*14] = tileData16[17];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*13] = tileData16[18];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*12] = tileData16[19];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*11] = tileData16[20];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*10] = tileData16[21];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 9] = tileData16[22];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 8] = tileData16[23];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 7] = tileData16[24];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 6] = tileData16[25];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 5] = tileData16[26];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 4] = tileData16[27];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 3] = tileData16[28];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 2] = tileData16[29];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 1] = tileData16[30];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 0] = tileData16[31];
						
					ptr16++;											// next col in dest
					tileData16 += OREOMAP_TILE_SIZE;					// next line in src
				}
				break;

				/* FLIP X ROT 1 */
				/* FLIP Y ROT 3 */

		case	TILE_FLIPX_MASK | TILE_ROT1:
		case	TILE_FLIPY_MASK | TILE_ROT3:
				ptr16 = (uint16_t *)buffer + (OREOMAP_TILE_SIZE-1);
				tileData16 = (const uint16_t *)tileData64;
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*31] = tileData16[ 0];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*30] = tileData16[ 1];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*29] = tileData16[ 2];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*28] = tileData16[ 3];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*27] = tileData16[ 4];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*26] = tileData16[ 5];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*25] = tileData16[ 6];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*24] = tileData16[ 7];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*23] = tileData16[ 8];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*22] = tileData16[ 9];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*21] = tileData16[10];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*20] = tileData16[11];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*19] = tileData16[12];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*18] = tileData16[13];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*17] = tileData16[14];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*16] = tileData16[15];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*15] = tileData16[16];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*14] = tileData16[17];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*13] = tileData16[18];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*12] = tileData16[19];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*11] = tileData16[20];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*10] = tileData16[21];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 9] = tileData16[22];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 8] = tileData16[23];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 7] = tileData16[24];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 6] = tileData16[25];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 5] = tileData16[26];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 4] = tileData16[27];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 3] = tileData16[28];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 2] = tileData16[29];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 1] = tileData16[30];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 0] = tileData16[31];
						
					ptr16--;											// next col in dest
					tileData16 += OREOMAP_TILE_SIZE;					// next line in src
				}
				break;

				/* FLIP X ROT 3 */
				/* FLIP Y ROT 1 */

		case	TILE_FLIPX_MASK | TILE_ROT3:
		case	TILE_FLIPY_MASK | TILE_ROT1:
				ptr16 = (uint16_t *)buffer;							// draw to right col from top row of src
				tileData16 = (uint16_t *)tileData64;
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 0] = tileData16[ 0];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 1] = tileData16[ 1];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 2] = tileData16[ 2];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 3] = tileData16[ 3];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 4] = tileData16[ 4];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 5] = tileData16[ 5];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 6] = tileData16[ 6];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 7] = tileData16[ 7];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 8] = tileData16[ 8];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE* 9] = tileData16[ 9];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*10] = tileData16[10];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*11] = tileData16[11];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*12] = tileData16[12];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*13] = tileData16[13];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*14] = tileData16[14];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*15] = tileData16[15];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*16] = tileData16[16];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*17] = tileData16[17];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*18] = tileData16[18];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*19] = tileData16[19];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*20] = tileData16[20];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*21] = tileData16[21];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*22] = tileData16[22];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*23] = tileData16[23];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*24] = tileData16[24];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*25] = tileData16[25];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*26] = tileData16[26];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*27] = tileData16[27];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*28] = tileData16[28];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*29] = tileData16[29];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*30] = tileData16[30];
					ptr16[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*31] = tileData16[31];
						
					ptr16++;											// prev col in dest
					tileData16 += OREOMAP_TILE_SIZE;					// next line in src
				}
				break;
	}


}




/************ SHRINK SUPERTILE TEXTURE MAP ********************/
//
// Shrinks a 160x160 src texture to a 128x128 dest texture
//

static inline void	ShrinkSuperTileTextureMap(const u_short *srcPtr,u_short *destPtr)
{
#if (SUPERTILE_TEXMAP_SIZE != 128)
	ReWrite this!
#endif
#if ((SUPERTILE_SIZE * OREOMAP_TILE_SIZE) != 160)
	ReWrite this!
#endif

short	y,v;
u_short	c1,c2,r,g,b;

	for (v = y = 0; y < SUPERTILE_TEXMAP_SIZE; y++)
	{
		
		c1 = srcPtr[3];									// get colors to average
		c2 = srcPtr[4];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[0] = srcPtr[0];							// save 0
		destPtr[1] = srcPtr[1];							// save 1
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[2] = srcPtr[2];							// save 2
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[3] = (r<<10)|(g<<5)|b;					// save 3
		
		

		c1 = srcPtr[8];									
		c2 = srcPtr[9];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[4] = srcPtr[5];
		destPtr[5] = srcPtr[6];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[6] = srcPtr[7];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[7] = (r<<10)|(g<<5)|b;					


		c1 = srcPtr[13];									
		c2 = srcPtr[14];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[8] = srcPtr[10];
		destPtr[9] = srcPtr[11];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[10] = srcPtr[12];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[11] = (r<<10)|(g<<5)|b;					


		c1 = srcPtr[18];									
		c2 = srcPtr[19];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[12] = srcPtr[15];
		destPtr[13] = srcPtr[16];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[14] = srcPtr[17];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[15] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[23];									
		c2 = srcPtr[24];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[16] = srcPtr[20];
		destPtr[17] = srcPtr[21];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[18] = srcPtr[22];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[19] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[28];									
		c2 = srcPtr[29];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[20] = srcPtr[25];
		destPtr[21] = srcPtr[26];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[22] = srcPtr[27];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[23] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[33];									
		c2 = srcPtr[34];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[24] = srcPtr[30];
		destPtr[25] = srcPtr[31];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[26] = srcPtr[32];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[27] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[38];									
		c2 = srcPtr[39];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[28] = srcPtr[35];
		destPtr[29] = srcPtr[36];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[30] = srcPtr[37];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[31] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[43];									
		c2 = srcPtr[44];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[32] = srcPtr[40];
		destPtr[33] = srcPtr[41];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[34] = srcPtr[42];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[35] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[48];									
		c2 = srcPtr[49];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[36] = srcPtr[45];
		destPtr[37] = srcPtr[46];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[38] = srcPtr[47];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[39] = (r<<10)|(g<<5)|b;

		
		c1 = srcPtr[53];									
		c2 = srcPtr[54];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[40] = srcPtr[50];
		destPtr[41] = srcPtr[51];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[42] = srcPtr[52];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[43] = (r<<10)|(g<<5)|b;
		

		c1 = srcPtr[58];									
		c2 = srcPtr[59];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[44] = srcPtr[55];
		destPtr[45] = srcPtr[56];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[46] = srcPtr[57];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[47] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[63];
		c2 = srcPtr[64];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[48] = srcPtr[60];
		destPtr[49] = srcPtr[61];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[50] = srcPtr[62];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[51] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[68];									
		c2 = srcPtr[69];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[52] = srcPtr[65];
		destPtr[53] = srcPtr[66];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[54] = srcPtr[67];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[55] = (r<<10)|(g<<5)|b;

		
		c1 = srcPtr[73];									
		c2 = srcPtr[74];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[56] = srcPtr[70];
		destPtr[57] = srcPtr[71];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[58] = srcPtr[72];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[59] = (r<<10)|(g<<5)|b;
	
		
		c1 = srcPtr[78];									
		c2 = srcPtr[79];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[60] = srcPtr[75];
		destPtr[61] = srcPtr[76];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[62] = srcPtr[77];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[63] = (r<<10)|(g<<5)|b;
	
		
		c1 = srcPtr[83];									
		c2 = srcPtr[84];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[64] = srcPtr[80];
		destPtr[65] = srcPtr[81];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[66] = srcPtr[82];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[67] = (r<<10)|(g<<5)|b;
			

		c1 = srcPtr[88];									
		c2 = srcPtr[89];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[68] = srcPtr[85];
		destPtr[69] = srcPtr[86];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[70] = srcPtr[87];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[71] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[93];									
		c2 = srcPtr[94];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[72] = srcPtr[90];
		destPtr[73] = srcPtr[91];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[74] = srcPtr[92];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[75] = (r<<10)|(g<<5)|b;

		
		c1 = srcPtr[98];									
		c2 = srcPtr[99];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[76] = srcPtr[95];
		destPtr[77] = srcPtr[96];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[78] = srcPtr[97];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[79] = (r<<10)|(g<<5)|b;
		

		c1 = srcPtr[103];									
		c2 = srcPtr[104];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[80] = srcPtr[100];
		destPtr[81] = srcPtr[101];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[82] = srcPtr[102];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[83] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[108];									
		c2 = srcPtr[109];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[84] = srcPtr[105];
		destPtr[85] = srcPtr[106];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[86] = srcPtr[107];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[87] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[113];									
		c2 = srcPtr[114];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[88] = srcPtr[110];
		destPtr[89] = srcPtr[111];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[90] = srcPtr[112];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[91] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[118];									
		c2 = srcPtr[119];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[92] = srcPtr[115];
		destPtr[93] = srcPtr[116];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[94] = srcPtr[117];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[95] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[123];									
		c2 = srcPtr[124];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[96] = srcPtr[120];
		destPtr[97] = srcPtr[121];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[98] = srcPtr[122];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[99] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[128];									
		c2 = srcPtr[129];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[100] = srcPtr[125];
		destPtr[101] = srcPtr[126];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[102] = srcPtr[127];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[103] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[133];									
		c2 = srcPtr[134];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[104] = srcPtr[130];
		destPtr[105] = srcPtr[131];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[106] = srcPtr[132];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[107] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[138];									
		c2 = srcPtr[139];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[108] = srcPtr[135];
		destPtr[109] = srcPtr[136];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[110] = srcPtr[137];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[111] = (r<<10)|(g<<5)|b;


		c1 = srcPtr[143];									
		c2 = srcPtr[144];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[112] = srcPtr[140];
		destPtr[113] = srcPtr[141];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[114] = srcPtr[142];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[115] = (r<<10)|(g<<5)|b;

	
		c1 = srcPtr[148];									
		c2 = srcPtr[149];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[116] = srcPtr[145];
		destPtr[117] = srcPtr[146];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[118] = srcPtr[147];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[119] = (r<<10)|(g<<5)|b;
	
		
		c1 = srcPtr[154];									
		c2 = srcPtr[155];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[120] = srcPtr[150];
		destPtr[121] = srcPtr[151];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[122] = srcPtr[152];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[123] = (r<<10)|(g<<5)|b;
	
		
		c1 = srcPtr[158];									
		c2 = srcPtr[159];
		r = ((c1>>10) + (c2>>10))>>1;
		destPtr[124] = srcPtr[155];
		destPtr[125] = srcPtr[156];
		g = (((c1>>5)&0x1f) + ((c2>>5)&0x1f))>>1;
		destPtr[126] = srcPtr[157];
		b = ((c1&0x1f) + (c2&0x1f)) >> 1;
		destPtr[127] = (r<<10)|(g<<5)|b;
	
	
		destPtr += SUPERTILE_TEXMAP_SIZE;								// next line in dest
		
		if (++v == 4)													// skip every 4th line
		{
			srcPtr +=  SUPERTILE_SIZE * OREOMAP_TILE_SIZE * 2;	
			v = 0;
		}
		else
			srcPtr +=  SUPERTILE_SIZE * OREOMAP_TILE_SIZE;
	}
}


/************ SHRINK SUPERTILE TEXTURE MAP TO 64 ********************/
//
// Shrinks a 160x160 src texture to a 64x64 dest texture
//

static inline void	ShrinkSuperTileTextureMapTo64(u_short *srcPtr,u_short *destPtr)
{
#if ((SUPERTILE_SIZE * OREOMAP_TILE_SIZE) != 160)
	ReWrite this!
#endif

short	y,v;

	for (v = y = 0; y < 64; y++)
	{
		destPtr[0] = srcPtr[0];
		destPtr[1] = srcPtr[2];
		
		destPtr[2] = srcPtr[5];
		destPtr[3] = srcPtr[7];

		destPtr[4] = srcPtr[10];
		destPtr[5] = srcPtr[12];

		destPtr[6] = srcPtr[15];
		destPtr[7] = srcPtr[17];

		destPtr[8] = srcPtr[20];
		destPtr[9] = srcPtr[22];

		destPtr[10] = srcPtr[25];
		destPtr[11] = srcPtr[27];

		destPtr[12] = srcPtr[30];
		destPtr[13] = srcPtr[32];

		destPtr[14] = srcPtr[35];
		destPtr[15] = srcPtr[37];

		destPtr[16] = srcPtr[40];
		destPtr[17] = srcPtr[42];

		destPtr[18] = srcPtr[45];
		destPtr[19] = srcPtr[47];

		destPtr[20] = srcPtr[50];
		destPtr[21] = srcPtr[52];

		destPtr[22] = srcPtr[55];
		destPtr[23] = srcPtr[57];

		destPtr[24] = srcPtr[60];
		destPtr[25] = srcPtr[62];

		destPtr[26] = srcPtr[65];
		destPtr[27] = srcPtr[67];
	
		destPtr[28] = srcPtr[70];
		destPtr[29] = srcPtr[72];
	
		destPtr[30] = srcPtr[75];
		destPtr[31] = srcPtr[77];
	
		destPtr[32] = srcPtr[80];
		destPtr[33] = srcPtr[82];
		
		destPtr[34] = srcPtr[85];
		destPtr[35] = srcPtr[87];

		destPtr[36] = srcPtr[90];
		destPtr[37] = srcPtr[92];

		destPtr[38] = srcPtr[95];
		destPtr[39] = srcPtr[97];

		destPtr[40] = srcPtr[100];
		destPtr[41] = srcPtr[102];

		destPtr[42] = srcPtr[105];
		destPtr[43] = srcPtr[107];

		destPtr[44] = srcPtr[110];
		destPtr[45] = srcPtr[112];

		destPtr[46] = srcPtr[115];
		destPtr[47] = srcPtr[117];

		destPtr[48] = srcPtr[120];
		destPtr[49] = srcPtr[122];

		destPtr[50] = srcPtr[125];
		destPtr[51] = srcPtr[127];

		destPtr[52] = srcPtr[130];
		destPtr[53] = srcPtr[132];

		destPtr[54] = srcPtr[135];
		destPtr[55] = srcPtr[137];

		destPtr[56] = srcPtr[140];
		destPtr[57] = srcPtr[142];

		destPtr[58] = srcPtr[145];
		destPtr[59] = srcPtr[147];
	
		destPtr[60] = srcPtr[150];
		destPtr[61] = srcPtr[152];
	
		destPtr[62] = srcPtr[155];
		destPtr[63] = srcPtr[157];
	
		destPtr += 64;													// next line in dest

		srcPtr +=  SUPERTILE_SIZE * OREOMAP_TILE_SIZE * 2;				// skip 2 lines in src
		
		if (v++)														// skip every other line
		{
			v = 0;
			srcPtr +=  SUPERTILE_SIZE * OREOMAP_TILE_SIZE;
		}
	}
}


/******************* RELEASE SUPERTILE OBJECT *******************/
//
// Deactivates the terrain object and releases its memory block
//

static inline void ReleaseSuperTileObject(short superTileNum)
{
	gSuperTileMemoryList[superTileNum].mode = SUPERTILE_MODE_FREE;		// it's free!
	gNumFreeSupertiles++;
}

/******************** RELEASE ALL SUPERTILES ************************/

static inline void ReleaseAllSuperTiles(void)
{
long	i;

	for (i = 0; i < MAX_SUPERTILES; i++)
		ReleaseSuperTileObject(i);

	gNumFreeSupertiles = MAX_SUPERTILES;
}

#pragma mark -

/********************* DRAW TERRAIN **************************/
//
// This is the main call to update the screen.  It draws all ObjNode's and the terrain itself
//

void DrawTerrain(const QD3DSetupOutputType *setupInfo)
{
short	i, numLayers;
Byte	j;
TQ3Status	myStatus;
float	dist;
TQ3Point3D		cameraCoord, tileCoord;
TQ3ViewObject	view = setupInfo->viewObject;
			
	if (gDoCeiling)
		numLayers = 2;
	else
		numLayers = 1;
			
		/* GET CURRENT CAMERA COORD */
		
	cameraCoord = setupInfo->currentCameraCoords;
	
			
				/* DRAW STUFF */
				
	QD3D_SetTextureWrapMode(kQAGL_Clamp);								// clamp textures for nicer seams
	QD3D_SetTriangleCacheMode(false);
	Q3Shader_Submit(setupInfo->nullShaderObject, view);					// use NULL shader to draw terrain
	
	
	for (i = 0; i < MAX_SUPERTILES; i++)
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

		tileCoord.x = gSuperTileMemoryList[i].x;						// get x & z coords of tile
		tileCoord.z = gSuperTileMemoryList[i].z;
		
		for (j = 0; j < numLayers; j++)									// DRAW FLOOR & CEILING
		{
			if (!IsSuperTileVisible(i,j))								// make sure it's visible
				continue;

				/**************************************/
				/* DRAW THE TRIMESH IN THIS SUPERTILE */
				/**************************************/

				
					/* SEE WHICH LOD TO USE */

			dist = CalcQuickDistance(cameraCoord.x, cameraCoord.z, tileCoord.x, tileCoord.z);
			
			
						/* USE LOD 0 */
						
			if (dist < 1300.0f)
			{
				gSuperTileMemoryList[i].triMeshData[j].triMeshAttributeSet = gSuperTileMemoryList[i].texture[0][j];
			}
				
						/* USE LOD 1 */
			else
			if (dist < 1700.0f)
			{
use_1:			
				if (!gSuperTileMemoryList[i].hasLOD[1])					 // see if 2nd LOD has been built yet
					BuildSuperTileLOD(&gSuperTileMemoryList[i], 1);				

				gSuperTileMemoryList[i].triMeshData[j].triMeshAttributeSet = gSuperTileMemoryList[i].texture[1][j];
			}
			
						/* USE LOD 2 */
			else
			{
				if (!gSuperTileMemoryList[i].hasLOD[2])						// see if 3rd LOD has been built yet
				{
					if (gSuperTileMemoryList[i].hasLOD[1])					// in order to build 3rd, we need 2nd
						BuildSuperTileLOD(&gSuperTileMemoryList[i], 2);
					else
						goto use_1;
				}
				gSuperTileMemoryList[i].triMeshData[j].triMeshAttributeSet = gSuperTileMemoryList[i].texture[2][j];
			}
			
			myStatus = Q3TriMesh_Submit(&gSuperTileMemoryList[i].triMeshData[j],view);				
			if ( myStatus == kQ3Failure )
			{
				DoAlert("DrawTerrain: Q3TriMesh_Submit failed!");
				QD3D_ShowRecentError();	
			}
		}		
	}
	QD3D_SetTextureWrapMode(kQAGL_Repeat);								// let textures wrap/repeat
	Q3Shader_Submit(setupInfo->shaderObject, view);						// set the normal shader
	QD3D_SetTriangleCacheMode(true);


		/* DRAW OBJECTS */
		
		
	DrawFences(setupInfo);												// draw these first
	DrawObjects(setupInfo);												// draw objNodes
	QD3D_DrawParticles(setupInfo);
	DrawParticleGroup(setupInfo);
	DrawLensFlare(setupInfo);										
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
short	superTileNum;
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
short	superTileNum;
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
short	superTileNum;
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
short	superTileNum;
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


#if 0 // SOURCE PORT NOTE: GetTileAttribsAtRowCol is missing some stuff...
/***************** GET TILE ATTRIBS ******************/
//
// Given a world x/z coord, return the attribs there
// NOTE: does it by calculating the row/col and then calling other routine.
//
// INPUT: x/z = world coords in INTEGER format
//
// OUTPUT: attribs
//

u_short	GetTileAttribs(long x, long z)
{
long	row,col;

	if ((x < 0) || (z < 0))										// see if out of bounds
		return(0);
	if ((x >= gTerrainUnitWidth) || (z >= gTerrainUnitDepth))
		return(0);

	col = (float)x*gOneOver_TERRAIN_POLYGON_SIZE;	 							// calc map row/col that the coord lies on
	row = (float)z*gOneOver_TERRAIN_POLYGON_SIZE;

	return(GetTileAttribsAtRowCol(row,col));
 }


#if 0
/******************** GET TILE ATTRIBS AT ROW COL *************************/
//
// OUTPUT: 	attrib bits
//			gTileAttribParm0..2 = tile attribs
//  		gTileFlipRotBits = flip/rot bits of tile
//

u_short	GetTileAttribsAtRowCol(short row, short col)
{
u_short	tile,texMapNum,attribBits;

	tile = gFloorMap[row][col];						// get tile data from map
	texMapNum = tile&TILENUM_MASK; 										// filter out texture #
	gTileFlipRotBits = tile&(TILE_FLIPXY_MASK|TILE_ROTATE_MASK);		// get flip & rotate bits

	attribBits = (*gTileAttributes)[texMapNum].bits;				// get attribute bits
	gTileAttribParm0 = (*gTileAttributes)[texMapNum].parm0;		// and parameters
	gTileAttribParm1 = (*gTileAttributes)[texMapNum].parm1;
	gTileAttribParm2 = (*gTileAttributes)[texMapNum].parm2;


	return(attribBits);
}
#endif
#endif // #if 0 (Source port)


#if USE_PATH_LAYER
/******************** GET TILE COLLISION BITS AT ROW COL *************************/
//
// Reads the tile on the *** PATH *** layer and converts it into a top/bottom/left/right bit field.
//
// INPUT:	checkAlt = also check seconday collision tiles (for enemies usually)
//

u_short	GetTileCollisionBitsAtRowCol(short row, short col, Boolean checkAlt)
{
u_short	tile;

	tile = gPathMap[row][col];						// get path data from map
	tile = tile&TILENUM_MASK;							   		// filter out tile # 
	if (tile == 0)
		return(0);

			/* CHECK PRIMARY COLLISION TILES */
			
	switch(tile)
	{
	 	case	PATH_TILE_SOLID_ALL:
				return(SIDE_BITS_TOP|SIDE_BITS_BOTTOM|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOP:
				return(SIDE_BITS_TOP);

		case	PATH_TILE_SOLID_RIGHT:
				return(SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_BOTTOM:
				return(SIDE_BITS_BOTTOM);

		case	PATH_TILE_SOLID_LEFT:
				return(SIDE_BITS_LEFT);

		case	PATH_TILE_SOLID_TOPBOTTOM:
				return(SIDE_BITS_TOP|SIDE_BITS_BOTTOM);

		case	PATH_TILE_SOLID_LEFTRIGHT:
				return(SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOPRIGHT:
				return(SIDE_BITS_TOP|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_BOTTOMRIGHT:
				return(SIDE_BITS_BOTTOM|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_BOTTOMLEFT:
				return(SIDE_BITS_BOTTOM|SIDE_BITS_LEFT);

		case	PATH_TILE_SOLID_TOPLEFT:
				return(SIDE_BITS_TOP|SIDE_BITS_LEFT);

		case	PATH_TILE_SOLID_TOPLEFTRIGHT:
				return(SIDE_BITS_TOP|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOPRIGHTBOTTOM:
				return(SIDE_BITS_TOP|SIDE_BITS_RIGHT|SIDE_BITS_BOTTOM);

		case	PATH_TILE_SOLID_BOTTOMLEFTRIGHT:
				return(SIDE_BITS_BOTTOM|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOPBOTTOMLEFT:
				return(SIDE_BITS_TOP|SIDE_BITS_LEFT|SIDE_BITS_BOTTOM);
	}
	
	
				/* CHECK SECONDARY */
	if (checkAlt)
	{
		switch(tile)
		{
		 	case	PATH_TILE_SOLID_ALL2:
					return(SIDE_BITS_TOP|SIDE_BITS_BOTTOM|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);
	
			case	PATH_TILE_SOLID_TOP2:
					return(SIDE_BITS_TOP);
	
			case	PATH_TILE_SOLID_RIGHT2:
					return(SIDE_BITS_RIGHT);
	
			case	PATH_TILE_SOLID_BOTTOM2:
					return(SIDE_BITS_BOTTOM);
	
			case	PATH_TILE_SOLID_LEFT2:
					return(SIDE_BITS_LEFT);
	
			case	PATH_TILE_SOLID_TOPBOTTOM2:
					return(SIDE_BITS_TOP|SIDE_BITS_BOTTOM);
	
			case	PATH_TILE_SOLID_LEFTRIGHT2:
					return(SIDE_BITS_LEFT|SIDE_BITS_RIGHT);
	
			case	PATH_TILE_SOLID_TOPRIGHT2:
					return(SIDE_BITS_TOP|SIDE_BITS_RIGHT);
	
			case	PATH_TILE_SOLID_BOTTOMRIGHT2:
					return(SIDE_BITS_BOTTOM|SIDE_BITS_RIGHT);
	
			case	PATH_TILE_SOLID_BOTTOMLEFT2:
					return(SIDE_BITS_BOTTOM|SIDE_BITS_LEFT);
	
			case	PATH_TILE_SOLID_TOPLEFT2:
					return(SIDE_BITS_TOP|SIDE_BITS_LEFT);
	
			case	PATH_TILE_SOLID_TOPLEFTRIGHT2:
					return(SIDE_BITS_TOP|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);
	
			case	PATH_TILE_SOLID_TOPRIGHTBOTTOM2:
					return(SIDE_BITS_TOP|SIDE_BITS_RIGHT|SIDE_BITS_BOTTOM);
	
			case	PATH_TILE_SOLID_BOTTOMLEFTRIGHT2:
					return(SIDE_BITS_BOTTOM|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);
	
			case	PATH_TILE_SOLID_TOPBOTTOMLEFT2:
					return(SIDE_BITS_TOP|SIDE_BITS_LEFT|SIDE_BITS_BOTTOM);
		}
	}

	return(0);
}


/******************** GET TILE COLLISION BITS AT ROW COL 2 *************************/
//
// Version #2 here only checks the secondary collision tiles.
//

u_short	GetTileCollisionBitsAtRowCol2(short row, short col)
{
u_short	tile;

	tile = gPathMap[row][col];						// get path data from map
	tile = tile&TILENUM_MASK; 							  		// filter out tile #
	if (tile == 0)
		return(0);

	switch(tile)
	{
	 	case	PATH_TILE_SOLID_ALL2:
				return(SIDE_BITS_TOP|SIDE_BITS_BOTTOM|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOP2:
				return(SIDE_BITS_TOP);

		case	PATH_TILE_SOLID_RIGHT2:
				return(SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_BOTTOM2:
				return(SIDE_BITS_BOTTOM);

		case	PATH_TILE_SOLID_LEFT2:
				return(SIDE_BITS_LEFT);

		case	PATH_TILE_SOLID_TOPBOTTOM2:
				return(SIDE_BITS_TOP|SIDE_BITS_BOTTOM);

		case	PATH_TILE_SOLID_LEFTRIGHT2:
				return(SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOPRIGHT2:
				return(SIDE_BITS_TOP|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_BOTTOMRIGHT2:
				return(SIDE_BITS_BOTTOM|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_BOTTOMLEFT2:
				return(SIDE_BITS_BOTTOM|SIDE_BITS_LEFT);

		case	PATH_TILE_SOLID_TOPLEFT2:
				return(SIDE_BITS_TOP|SIDE_BITS_LEFT);

		case	PATH_TILE_SOLID_TOPLEFTRIGHT2:
				return(SIDE_BITS_TOP|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOPRIGHTBOTTOM2:
				return(SIDE_BITS_TOP|SIDE_BITS_RIGHT|SIDE_BITS_BOTTOM);

		case	PATH_TILE_SOLID_BOTTOMLEFTRIGHT2:
				return(SIDE_BITS_BOTTOM|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOPBOTTOMLEFT2:
				return(SIDE_BITS_TOP|SIDE_BITS_LEFT|SIDE_BITS_BOTTOM);
	}
	return(0);
}

#endif

/**************** IS SUPERTILE VISIBLE *******************/
//
// Returns false if is not in current camera's viewing frustum
//

static Boolean IsSuperTileVisible(short superTileNum, Byte layer)
{
float				w1,w2,radius;
float				rx,ry,rz,px,py;
TQ3Point3D			points[2];					// [0] = point, [1] = radius
TQ3RationalPoint4D	outPoint4D[2];
	
	rx = gSuperTileMemoryList[superTileNum].x; 
	rz = gSuperTileMemoryList[superTileNum].z;
	ry = gSuperTileMemoryList[superTileNum].y[layer];
	
	radius = gSuperTileMemoryList[superTileNum].radius[layer];							// get radius

	points[0].x = 	(rx*gCameraWorldToViewMatrix.value[0][0]) + 
					(ry*gCameraWorldToViewMatrix.value[1][0]) + 
					(rz*gCameraWorldToViewMatrix.value[2][0]) + 
					(gCameraWorldToViewMatrix.value[3][0]); 

	points[0].y = 	(rx*gCameraWorldToViewMatrix.value[0][1]) + 
					(ry*gCameraWorldToViewMatrix.value[1][1]) + 
					(rz*gCameraWorldToViewMatrix.value[2][1]) + 
					(gCameraWorldToViewMatrix.value[3][1]); 

	points[0].z = 	(rx*gCameraWorldToViewMatrix.value[0][2]) + 
					(ry*gCameraWorldToViewMatrix.value[1][2]) + 
					(rz*gCameraWorldToViewMatrix.value[2][2]) + 
					(gCameraWorldToViewMatrix.value[3][2]); 
				
				/* SEE IF BEHIND CAMERA */
				
	if (points[0].z >= -HITHER_DISTANCE)									
	{
		if ((points[0].z - radius) > -HITHER_DISTANCE)							// is entire sphere behind camera?
			goto draw_off;
			
				/* PARTIALLY BEHIND */
				
		points[0].z -= radius;													// move edge over hither plane so cone calc will work
	}
	else
	{
			/* SEE IF BEYOND YON PLANE */
		
		if ((points[0].z + radius) < (-gYon))							// see if too far away
			goto draw_off;
	}

#if 0 // Source port removal: X/Y culling is buggy

			/*****************************/
			/* SEE IF WITHIN VISION CONE */
			/*****************************/

	points[1].x = points[1].y = radius;
	points[1].z = points[0].z;	
	
	Q3Point3D_To4DTransformArray(&points[0],&gCameraViewToFrustumMatrix,
								&outPoint4D[0],	2,sizeof(TQ3Point3D),
								sizeof(TQ3RationalPoint4D));
	
	
	w1 = outPoint4D[0].w;
	w2 = outPoint4D[1].w;
	
	px = w1*outPoint4D[0].x;
	py = w1*outPoint4D[0].y;
	rx = w2*outPoint4D[1].x;
	ry = w2*outPoint4D[1].y;

	if ((px + rx) < -1.0f)						// see if sphere "would be" out of bounds
		goto draw_off;
	if ((px - rx) > 1.0f)
		goto draw_off;
	
	if ((py + ry) < -1.0f)						
		goto draw_off;
	if ((py - ry) > 1.0f)
		goto draw_off;
				
#endif

	return(true);

draw_off:
	return(false);
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











