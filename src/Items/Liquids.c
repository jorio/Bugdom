/****************************/
/*   	LIQUIDS.C		    */
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

static void DrawWaterPatch(ObjNode *theNode);
static void DrawWaterPatchTesselated(ObjNode *theNode);
static void UpdateWaterTextureAnimation(void);
static void UpdateHoneyTextureAnimation(void);
static void UpdateSlimeTextureAnimation(void);
static void UpdateLavaTextureAnimation(void);

static Boolean AddLiquidPatch(TerrainItemEntryType *itemPtr, long x, long z, int kind);
static void MoveLiquidPatch(ObjNode *theNode);
static void DrawSolidLiquidPatchTesselated(ObjNode *theNode);
static void ApplyJitterToLiquidVertex(TQ3Point3D* p, int type);


/****************************/
/*    CONSTANTS             */
/****************************/

#define MAX_LIQUID_MESHES		32
#define MAX_LIQUID_SIZE			20
#define MAX_LIQUID_TRIANGLES	(MAX_LIQUID_SIZE*MAX_LIQUID_SIZE*2)
#define MAX_LIQUID_VERTS		((MAX_LIQUID_SIZE+1)*(MAX_LIQUID_SIZE+1))

#define	RISING_WATER_YOFF		200.0f

static const bool gLiquidOnThisLevel[NUM_LEVEL_TYPES][NUM_LIQUID_TYPES] =
{
	//								Water	Honey	Slime	Lava
	[LEVEL_TYPE_LAWN	]	=	{	1,		0,		0,		0,		},
	[LEVEL_TYPE_POND	]	=	{	1,		0,		0,		0,		},
	[LEVEL_TYPE_FOREST	]	=	{	1,		0,		0,		0,		},
	[LEVEL_TYPE_HIVE	]	=	{	0,		1,		0,		0,		},
	[LEVEL_TYPE_NIGHT	]	=	{	0,		0,		1,		0,		},
	[LEVEL_TYPE_ANTHILL	]	=	{	1,		0,		1,		1,		},
};

static const float gLiquidYTable[NUM_LIQUID_TYPES][6] =
{
	[LIQUID_WATER	] = {	900,	950,	0,		0,		0,		0	},
	[LIQUID_HONEY	] = {	-620,	-580,	-550,	-600,	0,		0	},
	[LIQUID_SLIME	] = {	0,		-200,	0,		0,		0,		0	},
	[LIQUID_LAVA	] = {	-230,	-230,	-230,	0,		0,		0	},
};

// offset from top where water collision volume starts
const float gLiquidCollisionTopOffset[NUM_LIQUID_TYPES] =
{
	[LIQUID_WATER	] = 75.0f,
	[LIQUID_HONEY	] = 70.0f,
	[LIQUID_SLIME	] = 60.0f,
	[LIQUID_LAVA	] = 60.0f,
};

/*********************/
/*    VARIABLES      */
/*********************/

#define PatchWidth		SpecialL[0]
#define PatchDepth		SpecialL[1]
#define	PatchValveID	SpecialL[2]
#define PatchMeshID		SpecialL[3]
#define	TesselatePatch	Flag[0]
#define	PatchHasRisen	Flag[1]				// true when water has flooded up


static TQ3TriMeshData	*gLiquidMeshPtrs[MAX_LIQUID_MESHES];
static int				gFreeLiquidMeshes[MAX_LIQUID_MESHES];
static int				gNumActiveLiquidMeshes = 0;

static GLuint			gLiquidShaders[NUM_LIQUID_TYPES];
static TQ3Param2D		gLiquidUVOffsets[NUM_LIQUID_TYPES];
static TQ3Param2D		gWaterUVOffset2;		// extra uv offsets for second plane of non-tesselated water

/****************** HELPER: DELETE TEXTURE **********************/

static void DeleteTexture(GLuint* textureName)
{
	if (*textureName)
	{
		glDeleteTextures(1, textureName);
		*textureName = 0;
	}
}


/****************** ALLOCATE LIQUID MESH FROM POOL **********************/

static int AllocLiquidMesh(void)
{
	GAME_ASSERT(gNumActiveLiquidMeshes < MAX_LIQUID_MESHES);

	int newLiquidMeshID = gFreeLiquidMeshes[gNumActiveLiquidMeshes];
	gFreeLiquidMeshes[gNumActiveLiquidMeshes] = -1;
	gNumActiveLiquidMeshes++;

	GAME_ASSERT(newLiquidMeshID >= 0);
	GAME_ASSERT(newLiquidMeshID < MAX_LIQUID_MESHES);

	return newLiquidMeshID;
}


/****************** PUT LIQUID MESH BACK INTO POOL **********************/

static void DisposeLiquidMesh(ObjNode* theNode)
{
	GAME_ASSERT(gNumActiveLiquidMeshes >= 0);
	GAME_ASSERT(theNode->PatchMeshID >= 0 && theNode->PatchMeshID < MAX_LIQUID_MESHES);

	gNumActiveLiquidMeshes--;
	gFreeLiquidMeshes[gNumActiveLiquidMeshes] = theNode->PatchMeshID;	// put mesh back into pool

	theNode->PatchMeshID = -1;
}


/****************** INIT LIQUIDS **********************/

void InitLiquids(void)
{
	gNumActiveLiquidMeshes = 0;

	// Initialize trimeshes
	for (int i = 0; i < MAX_LIQUID_MESHES; i++)
	{
		gLiquidMeshPtrs[i] = Q3TriMeshData_New(
				MAX_LIQUID_TRIANGLES,
				MAX_LIQUID_VERTS,
				kQ3TriMeshDataFeatureVertexUVs);
		gFreeLiquidMeshes[i] = i;
	}

	// Clear UV offsets
	for (int i = 0; i < NUM_LIQUID_TYPES; i++)
	{
		gLiquidUVOffsets[i] = (TQ3Param2D) { 0, 0 };
	}
	gWaterUVOffset2 = (TQ3Param2D) { 0, 0 };

	// Load textures
	for (int i = 0; i < NUM_LIQUID_TYPES; i++)
	{
		DeleteTexture(&gLiquidShaders[i]);

		if (!gLiquidOnThisLevel[gLevelType][i])
			continue;

		int textureFile;
		switch (i)
		{
			case LIQUID_WATER:	textureFile = gLevelType == LEVEL_TYPE_POND ? 129 : 128; break;
			case LIQUID_HONEY:	textureFile = 200; break;
			case LIQUID_SLIME:	textureFile = 201; break;
			case LIQUID_LAVA:	textureFile = 202; break;
			default:			textureFile = 128; break;
		}

		gLiquidShaders[i] = QD3D_LoadTextureFile(textureFile, kRendererTextureFlags_None);
	}
}


/**************** DISPOSE LIQUIDS ********************/

void DisposeLiquids(void)
{
	for (int i = 0; i < NUM_LIQUID_TYPES; i++)
	{
		DeleteTexture(&gLiquidShaders[i]);
	}

	for (int i = 0; i < MAX_LIQUID_MESHES; i++)
	{
		if (gLiquidMeshPtrs[i] != nil)
		{
			Q3TriMeshData_Dispose(gLiquidMeshPtrs[i]);
			gLiquidMeshPtrs[i] = nil;
		}

		gFreeLiquidMeshes[i] = i;
	}

	gNumActiveLiquidMeshes = 0;
}

/*************** UPDATE LIQUID ANIMATION ****************/

void UpdateLiquidAnimation(void)
{
	UpdateWaterTextureAnimation();
	UpdateHoneyTextureAnimation();
	UpdateSlimeTextureAnimation();
	UpdateLavaTextureAnimation();
}




/***************** FIND LIQUID Y **********************/

Boolean FindLiquidY(float x, float z, float* y)
{
	ObjNode* thisNodePtr = gFirstNodePtr;
	do
	{
		if (thisNodePtr->CType & CTYPE_LIQUID)
		{
			if (thisNodePtr->CollisionBoxes)
			{
				if (x < thisNodePtr->CollisionBoxes[0].left)
					goto next;
				if (x > thisNodePtr->CollisionBoxes[0].right)
					goto next;
				if (z > thisNodePtr->CollisionBoxes[0].front)
					goto next;
				if (z < thisNodePtr->CollisionBoxes[0].back)
					goto next;

				if (y)
				{
					*y = thisNodePtr->CollisionBoxes[0].top;
					*y += gLiquidCollisionTopOffset[thisNodePtr->Kind];
				}
				return true;
			}
		}		
next:					
		thisNodePtr = (ObjNode *)thisNodePtr->NextNode;		// next node
	}
	while (thisNodePtr != nil);

	return false;
}


#pragma mark -

/************************* ADD WATER PATCH *********************************/
//
// parm[0] = # tiles wide
// parm[1] = # tiles deep
// parm[2] = depth of water or ID# for anthill or index
// parm[3]:bit0 = tesselate flag
//		   bit1 = underground
//		   bit2 = indexed y
//

Boolean AddWaterPatch(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode				*newObj;
float				y,yOff;
TQ3BoundingSphere	bSphere;
int					width,depth,id = 0;
Boolean				tesselateFlag,hasRisen = true;
Boolean				putUnderGround;
static const float	yTable[] = {900,950,0,0,0,0};
static const float	yTable2[] = {-540,-540,-540,-540,-370,-540,-540,-540};

	GAME_ASSERT_MESSAGE(gLiquidShaders[LIQUID_WATER], "water not activated on this level");
		
	x -= (int)x % (int)TERRAIN_POLYGON_SIZE;				// round down to nearest tile & center
	x += TERRAIN_POLYGON_SIZE/2;
	z -= (int)z % (int)TERRAIN_POLYGON_SIZE;
	z += TERRAIN_POLYGON_SIZE/2;
		
		
	tesselateFlag = itemPtr->parm[3] & 1;					// get tesselate flag
	putUnderGround = itemPtr->parm[3] & (1<<1);				// get underground flag

	if (gLevelType == LEVEL_TYPE_ANTHILL)					// always tesselate on ant hill
		tesselateFlag = true;

			/***********************/
			/* DETERMINE WATER'S Y */
			/***********************/
			
				/* POND Y */
			
	if (gLevelType == LEVEL_TYPE_POND)						// pond water is at fixed y
	{
		y = WATER_Y;
		tesselateFlag = true;								// always tesselate on pond level
	}
	
			/* ANTHILL UNDERGROUND Y */
	else
	if (putUnderGround)										// anthill has water below surface for flooding
	{
		y = yTable2[itemPtr->parm[2]];						// get underground y from table2
		id = itemPtr->parm[2];								// get valve ID#
		if (gValveIsOpen[id])								// if valve is open, then move water up
			y += RISING_WATER_YOFF;
		else
			hasRisen = false;

	}
	
			/* USER Y */
			
	else													// other levels put water on terrain curve
	{
		if (itemPtr->parm[3]  & (1<<2))						// see if used table index y coord
		{
			y = yTable[itemPtr->parm[2]];					// get y from table		
		}
		
			/* USE PARM Y OFFSET */
			
		else
		{
			yOff = itemPtr->parm[2];							// get y offset
			yOff *= 4.0f;										// multiply by n since parm is only a Byte 0..255
			if (yOff == 0.0f)
				yOff = 3.0f;
	
			y = GetTerrainHeightAtCoord(x,z,FLOOR)+yOff;		// get y coord of patch
		}
	}

		/* CALC WIDTH & DEPTH OF PATCH */		

	width = itemPtr->parm[0];
	depth = itemPtr->parm[1];
	if (width == 0)
		width = 4;
	if (depth == 0)
		depth = 4;
		
	GAME_ASSERT(width <= MAX_LIQUID_SIZE);
	GAME_ASSERT(depth <= MAX_LIQUID_SIZE);


			/* CALC BSPHERE */

	bSphere.origin.x = x;
	bSphere.origin.y = y;
	bSphere.origin.z = z;
	bSphere.radius = (width + depth) / 2 * TERRAIN_POLYGON_SIZE;

			
			/***************/
			/* MAKE OBJECT */
			/***************/
			
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = y;
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags 	= STATUS_BIT_NOTRICACHE|STATUS_BIT_NULLSHADER|STATUS_BIT_NOZWRITE;
	gNewObjectDefinition.slot 	= SLOT_OF_DUMB-1;
	gNewObjectDefinition.moveCall = MoveLiquidPatch;
	gNewObjectDefinition.rot 	= 0;
	gNewObjectDefinition.scale = 1.0;
	if (tesselateFlag)
		newObj = MakeNewCustomDrawObject(&gNewObjectDefinition, &bSphere, DrawWaterPatchTesselated);	
	else
		newObj = MakeNewCustomDrawObject(&gNewObjectDefinition, &bSphere, DrawWaterPatch);
		
	if (newObj == nil)
		return(false);

			/* SET OBJECT INFO */
			
	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Kind = LIQUID_WATER;

	newObj->PatchWidth 		= width;
	newObj->PatchDepth 		= depth;
	newObj->TesselatePatch 	= tesselateFlag;
	newObj->PatchMeshID		= AllocLiquidMesh();					// allocate liquid mesh
	newObj->PatchValveID	= id;									// save valve ID for anthill level
	newObj->PatchHasRisen	= hasRisen;

			/* SET COLLISION */
			//
			// Water patches have a water volume collision area.
			// The collision volume starts a little under the top of the water so that
			// no collision is detected in shallow water, but if player sinks deep
			// then a collision will be detected.
			//
				
	newObj->CType = CTYPE_LIQUID|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_TOUCHABLE;
	SetObjectCollisionBounds(newObj,
							-gLiquidCollisionTopOffset[LIQUID_WATER],
							-2000,
							-(width*.5f) * TERRAIN_POLYGON_SIZE,
							width*.5f * TERRAIN_POLYGON_SIZE,
							depth*.5f * TERRAIN_POLYGON_SIZE,
							-(depth*.5f) * TERRAIN_POLYGON_SIZE);

			/* SET MESH PROPERTIES */

	TQ3TriMeshData* tmd = gLiquidMeshPtrs[newObj->PatchMeshID];

	if (gLevelType != LEVEL_TYPE_POND)		// any water in non-pond levels
		tmd->diffuseColor.a = .5f;
	else if (tesselateFlag)					// tesselated water in pond level
		tmd->diffuseColor.a = .6f;
	else									// non-tesselated water in pond level (is there even any?)
		tmd->diffuseColor.a = .7f;

	tmd->texturingMode = kQ3TexturingModeAlphaBlend;
	tmd->glTextureName = gLiquidShaders[LIQUID_WATER];

	return(true);													// item was added
}


/********************* MOVE LIQUID PATCH **********************/

static void MoveLiquidPatch(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DisposeLiquidMesh(theNode);
		DeleteObject(theNode);
		return;
	}

		/* ON ANTHILL LEVEL, SEE IF TIME TO RAISE THE WATER */

	if (gLevelType == LEVEL_TYPE_ANTHILL &&
		theNode->Kind == LIQUID_WATER &&
		!theNode->PatchHasRisen)
	{
		int	id = theNode->PatchValveID;				// get valve ID

		if (gValveIsOpen[id])						// see if valve is open
		{
			float	targetY = theNode->InitCoord.y + RISING_WATER_YOFF;	
			
			GetObjectInfo(theNode);
			
			gCoord.y += 30.0f * gFramesPerSecondFrac;
			if (gCoord.y >= targetY)				// see if full
			{
				gCoord.y = targetY;
				theNode->PatchHasRisen = true;
			}
			
			UpdateObject(theNode);
		}
	}
}


/**************** UPDATE WATER TEXTURE ANIMATION ********************/

static void UpdateWaterTextureAnimation(void)
{
	gLiquidUVOffsets[LIQUID_WATER].u += gFramesPerSecondFrac * .05f;
	gLiquidUVOffsets[LIQUID_WATER].v += gFramesPerSecondFrac * .1f;
	gWaterUVOffset2.u -= gFramesPerSecondFrac * .12f;
	gWaterUVOffset2.v -= gFramesPerSecondFrac * .07f;
}


/********************* DRAW WATER PATCH **********************/
//
// This is the simple version which just draws a giant quad
//

static void DrawWaterPatch(ObjNode *theNode)
{
float			patchW,patchD;
float			x,y,z,left,back, right, front;
TQ3Point3D		*p;

	TQ3TriMeshData* tmd = gLiquidMeshPtrs[theNode->PatchMeshID];

			/* SETUP ENVIRONMENT */

	tmd->numPoints = 8;
	tmd->numTriangles = 4;


			/* CALC BOUNDS OF WATER */

	x = theNode->Coord.x;
	y = theNode->Coord.y;
	z = theNode->Coord.z;

	patchW = theNode->PatchWidth;
	patchD = theNode->PatchDepth;

	left = x - patchW * (TERRAIN_POLYGON_SIZE/2);
	right = left + patchW * TERRAIN_POLYGON_SIZE;
	back = z - patchD * (TERRAIN_POLYGON_SIZE/2);
	front = back + patchD * TERRAIN_POLYGON_SIZE;

//	width = right - left;
//	depth = front - back;

			/******************/
			/* BUILD GEOMETRY */
			/******************/

	tmd->triangles[0].pointIndices[0] = 0;
	tmd->triangles[0].pointIndices[1] = 1;
	tmd->triangles[0].pointIndices[2] = 2;
	tmd->triangles[1].pointIndices[0] = 0;
	tmd->triangles[1].pointIndices[1] = 2;
	tmd->triangles[1].pointIndices[2] = 3;

	tmd->triangles[2].pointIndices[0] = 4;
	tmd->triangles[2].pointIndices[1] = 5;
	tmd->triangles[2].pointIndices[2] = 6;
	tmd->triangles[3].pointIndices[0] = 4;
	tmd->triangles[3].pointIndices[1] = 6;
	tmd->triangles[3].pointIndices[2] = 7;

			/* MAKE POINTS ARRAY */

	p = tmd->points;
	p[0].x = left;
	p[0].y = y;
	p[0].z = back;

	p[1].x = left;
	p[1].y = y;
	p[1].z = front;

	p[2].x = right;
	p[2].y = y;
	p[2].z = front;

	p[3].x = right;
	p[3].y = y;
	p[3].z = back;

	p[4].x = left;
	p[4].y = y;
	p[4].z = back;

	p[5].x = left;
	p[5].y = y;
	p[5].z = front;

	p[6].x = right;
	p[6].y = y;
	p[6].z = front;

	p[7].x = right;
	p[7].y = y;
	p[7].z = back;

			/* UPDATE BBOX */

	tmd->bBox.min.x = left;
	tmd->bBox.min.y = y;
	tmd->bBox.min.z = back;
	tmd->bBox.max.x = right;
	tmd->bBox.max.y = y;
	tmd->bBox.max.z = front;
	tmd->bBox.isEmpty = false;


		/* SET VERTEX UVS */

	const TQ3Param2D scroll = gLiquidUVOffsets[LIQUID_WATER];

	tmd->vertexUVs[0].u = scroll.u;
	tmd->vertexUVs[0].v = scroll.v + patchD*(1.0/2.0);
	tmd->vertexUVs[1].u = scroll.u;
	tmd->vertexUVs[1].v = scroll.v;
	tmd->vertexUVs[2].u = scroll.u + patchW*(1.0/2.0);
	tmd->vertexUVs[2].v = scroll.v;
	tmd->vertexUVs[3].u = scroll.u + patchW*(1.0/2.0);
	tmd->vertexUVs[3].v = scroll.v + patchD*(1.0/2.0);

	tmd->vertexUVs[4].u = gWaterUVOffset2.u;
	tmd->vertexUVs[4].v = gWaterUVOffset2.v + patchD * (1.0 / 3.0);
	tmd->vertexUVs[5].u = gWaterUVOffset2.u;
	tmd->vertexUVs[5].v = gWaterUVOffset2.v;
	tmd->vertexUVs[6].u = gWaterUVOffset2.u + patchW * (1.0 / 3.0);
	tmd->vertexUVs[6].v = gWaterUVOffset2.v;
	tmd->vertexUVs[7].u = gWaterUVOffset2.u + patchW * (1.0 / 3.0);
	tmd->vertexUVs[7].v = gWaterUVOffset2.v + patchD * (1.0 / 3.0);

			/*************/
			/* SUBMIT IT */
			/*************/

	Render_SubmitMesh(tmd, nil, &theNode->RenderModifiers, &theNode->Coord);
}

/********************* DRAW WATER PATCH TESSELATED **********************/
//
// This version tesselates the water patch so fog will look better on it.
//

static void DrawWaterPatchTesselated(ObjNode *theNode)
{
float			x,y,z,left,back, right, front;
float			u,v;
TQ3Point3D		*p;
int				width2,depth2,w,h,i;
TQ3TriMeshTriangleData	*t;


			/*********************/
			/* SETUP ENVIRONMENT */
			/*********************/

	TQ3TriMeshData* tmd = gLiquidMeshPtrs[theNode->PatchMeshID];

	const TQ3Param2D uvScroll = gLiquidUVOffsets[LIQUID_WATER];

			/************************/
			/* CALC BOUNDS OF WATER */
			/************************/
			
	x = theNode->Coord.x;
	y = theNode->Coord.y;
	z = theNode->Coord.z;
	
	if ((theNode->PatchWidth & 1) || (theNode->PatchDepth & 1))		// widths cannot be odd!
		DoFatalAlert("DrawWaterPatchTesselated: water patches cannot be odd sizes! Must be even numbers.");
	
	width2 = theNode->PatchWidth/2;
	depth2 = theNode->PatchDepth/2;
	
	left = x - (float)(width2) * TERRAIN_POLYGON_SIZE;
	right = left + (float)theNode->PatchWidth * TERRAIN_POLYGON_SIZE;
	back = z - (float)(depth2) * TERRAIN_POLYGON_SIZE;
	front = back + (float)theNode->PatchDepth * TERRAIN_POLYGON_SIZE;

//	width = right - left;
//	depth = front - back;
	

			/******************/
			/* BUILD GEOMETRY */
			/******************/

		/**************************/
		/* MAKE POINTS & UV ARRAY */
		/**************************/

	p = tmd->points;							// get ptr to points array
	tmd->numPoints = (width2+1) * (depth2+1);	// calc # points in geometry
	i = 0;

	z = back;
	v = 1.0f;

	for (h = 0; h <= depth2; h++)
	{
		x = left;
		u = 0;
		for (w = 0; w <= width2; w++)
		{
				/* SET POINT */

			p[i].x = x;
			p[i].y = y;
			p[i].z = z;
			if ((h > 0) && (h < depth2) && (w > 0) && (w < width2))		// add random jitter to inner vertices
			{
				ApplyJitterToLiquidVertex(&p[i], LIQUID_WATER);
			}

				/* SET UV */

			tmd->vertexUVs[i].u = uvScroll.u + u;
			tmd->vertexUVs[i].v = uvScroll.v + v;

			i++;
			x += TERRAIN_POLYGON_SIZE*2.0f;	
			u += .4f;
		}

		v -= .4f;
		z += TERRAIN_POLYGON_SIZE*2.0f;
	}


			/* UPDATE BBOX */

	tmd->bBox.min.x = left;
	tmd->bBox.min.y = y;
	tmd->bBox.min.z = back;
	tmd->bBox.max.x = right;
	tmd->bBox.max.y = y;
	tmd->bBox.max.z = front;
	tmd->bBox.isEmpty = false;


		/**************************/
		/* MAKE TRIANGLE INDECIES */
		/**************************/

	t = tmd->triangles;								// get ptr to triangle array
	i = 0;
	for (h = 0; h < depth2; h++)
	{
		for (w = 0; w < width2; w++)
		{
			int rw = width2+1;

				/* TRIANGLE A */

			t[i].pointIndices[0] = (h * rw) + w;			// far left
			t[i].pointIndices[1] = ((h+1) * rw) + w;		// near left
			t[i].pointIndices[2] = ((h+1) * rw) + w + 1;	// near right
			i++;

				/* TRIANGLE B */

			t[i].pointIndices[0] = (h * rw) + w;			// far left
			t[i].pointIndices[1] = ((h+1) * rw) + w + 1; 	// near right
			t[i].pointIndices[2] = (h * rw) + w + 1;		// far right
			i++;
		}
	}
	tmd->numTriangles = i;						// set # triangles in geometry


			/*************/
			/* SUBMIT IT */
			/*************/

	Render_SubmitMesh(tmd, nil, &theNode->RenderModifiers, &theNode->Coord);
}



#pragma mark -


/************************* ADD HONEY PATCH *********************************/

Boolean AddHoneyPatch(TerrainItemEntryType *itemPtr, long x, long z)
{
	return AddLiquidPatch(itemPtr, x, z, LIQUID_HONEY);
}

/************************* ADD SLIME PATCH *********************************/

Boolean AddSlimePatch(TerrainItemEntryType *itemPtr, long x, long z)
{
	return AddLiquidPatch(itemPtr, x, z, LIQUID_SLIME);
}

/************************* ADD LAVA PATCH *********************************/

Boolean AddLavaPatch(TerrainItemEntryType *itemPtr, long x, long z)
{
	return AddLiquidPatch(itemPtr, x, z, LIQUID_LAVA);
}


#pragma mark -


/**************** UPDATE HONEY TEXTURE ANIMATION ********************/

static void UpdateHoneyTextureAnimation(void)
{
	gLiquidUVOffsets[LIQUID_HONEY].u += gFramesPerSecondFrac * .09f;
	gLiquidUVOffsets[LIQUID_HONEY].v += gFramesPerSecondFrac * .1f;
}

/**************** UPDATE SLIME TEXTURE ANIMATION ********************/

static void UpdateSlimeTextureAnimation(void)
{
	gLiquidUVOffsets[LIQUID_SLIME].u += gFramesPerSecondFrac * .09f;
	gLiquidUVOffsets[LIQUID_SLIME].v += gFramesPerSecondFrac * .1f;
}

/**************** UPDATE LAVA TEXTURE ANIMATION ********************/

static void UpdateLavaTextureAnimation(void)
{
	gLiquidUVOffsets[LIQUID_LAVA].u += gFramesPerSecondFrac * .09f;
	gLiquidUVOffsets[LIQUID_LAVA].v += gFramesPerSecondFrac * .1f;
}



#pragma mark - Generic liquid routines


/************************* ADD HONEY/SLIME/LAVA PATCH *********************************/
//
// parm[0] = # tiles wide
// parm[1] = # tiles deep
// parm[2] = y off or y coord index
// parm[3]: bit 0 = use y coord index for fixed ht.
//

static Boolean AddLiquidPatch(TerrainItemEntryType *itemPtr, long x, long z, int kind)
{
ObjNode				*newObj;
float				y,yOff;
TQ3BoundingSphere	bSphere;
float				width,depth;

	GAME_ASSERT_MESSAGE(gLiquidShaders[kind], "this kind of liquid is not activated on this level");

	x -= (int)x % (int)TERRAIN_POLYGON_SIZE;				// round down to nearest tile & center
	x += TERRAIN_POLYGON_SIZE/2;
	z -= (int)z % (int)TERRAIN_POLYGON_SIZE;
	z += TERRAIN_POLYGON_SIZE/2;

	if (itemPtr->parm[3]&1)									// see if use indexed y
	{
		GAME_ASSERT(itemPtr->parm[2] < 6);
		y = gLiquidYTable[kind][itemPtr->parm[2]];			// get y from table
	}
	else
	{
		yOff = itemPtr->parm[2];							// get y offset
		yOff *= 10.0f;										// multiply by n since parm is only a Byte 0..255
		if (yOff == 0.0f)
			yOff = 40.0f;
		y = GetTerrainHeightAtCoord(x,z,FLOOR)+yOff;		// get y coord of patch
	}

	width = itemPtr->parm[0];								// get width & depth of water patch
	depth = itemPtr->parm[1];
	if (width == 0)
		width = 4;
	if (depth == 0)
		depth = 4;

	GAME_ASSERT(width <= MAX_LIQUID_SIZE);
	GAME_ASSERT(depth <= MAX_LIQUID_SIZE);


			/* CALC BSPHERE */

	bSphere.origin.x = x;
	bSphere.origin.y = y;
	bSphere.origin.z = z;
	bSphere.radius = (((float)width + (float)depth) * .5f) * TERRAIN_POLYGON_SIZE;


			/* MAKE OBJECT */

	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = y;
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags 	= STATUS_BIT_NULLSHADER;	// note: Slime/Lava also had notricache|nozwrite. Honey just had nullshader.
	gNewObjectDefinition.slot 	= SLOT_OF_DUMB-1;
	gNewObjectDefinition.moveCall = MoveLiquidPatch;
	gNewObjectDefinition.rot 	= 0;
	gNewObjectDefinition.scale = 1.0;
	newObj = MakeNewCustomDrawObject(&gNewObjectDefinition, &bSphere, DrawSolidLiquidPatchTesselated);

	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Kind = kind;

	newObj->PatchWidth 		= width;
	newObj->PatchDepth 		= depth;
	newObj->PatchMeshID		= AllocLiquidMesh();					// allocate liquid mesh

			/* SET COLLISION */
			//
			// Liquid patches have a liquid volume collision area.
			// The collision volume starts a little under the top of the liquid so that
			// no collision is detected in shallow liquid, but if player sinks deep
			// then a collision will be detected.
			//

	newObj->CType = CTYPE_LIQUID|CTYPE_BLOCKCAMERA|CTYPE_BLOCKSHADOW;
	newObj->CBits = CBITS_TOUCHABLE;
	SetObjectCollisionBounds(newObj,
							-gLiquidCollisionTopOffset[kind],
							-2000,
							-(width*.5f) * TERRAIN_POLYGON_SIZE,
							(width*.5f) * TERRAIN_POLYGON_SIZE,
							(depth*.5f) * TERRAIN_POLYGON_SIZE,
							-(depth*.5f) * TERRAIN_POLYGON_SIZE);

			/* SET MESH PROPERTIES */

	TQ3TriMeshData* tmd = gLiquidMeshPtrs[newObj->PatchMeshID];

	tmd->diffuseColor.a = 1.0f;
	tmd->texturingMode = kQ3TexturingModeOpaque;
	tmd->glTextureName = gLiquidShaders[kind];


	return(true);													// item was added
}

/********************* DRAW LIQUID PATCH TESSELATED **********************/
//
// This version tesselates the water patch so fog will look better on it.
//

static void DrawSolidLiquidPatchTesselated(ObjNode *theNode)
{
float			x,y,z,left,back, right, front;
float			u,v;
TQ3Point3D		*p;
int				width2,depth2,w,h,i;
TQ3TriMeshTriangleData	*t;


			/*********************/
			/* SETUP ENVIRONMENT */
			/*********************/

	TQ3TriMeshData* tmd = gLiquidMeshPtrs[theNode->PatchMeshID];

	const TQ3Param2D uvOffset = gLiquidUVOffsets[theNode->Kind];

			/*************************/
			/* CALC BOUNDS OF LIQUID */
			/*************************/

	x = theNode->Coord.x;
	y = theNode->Coord.y;
	z = theNode->Coord.z;

	width2 = theNode->PatchWidth;
	depth2 = theNode->PatchDepth;

	left = x - (width2*.5f) * TERRAIN_POLYGON_SIZE;
	right = left + width2 * TERRAIN_POLYGON_SIZE;
	back = z - (depth2*.5f) * TERRAIN_POLYGON_SIZE;
	front = back + depth2 * TERRAIN_POLYGON_SIZE;

//	width = right - left;
//	depth = front - back;


			/******************/
			/* BUILD GEOMETRY */
			/******************/

		/**************************/
		/* MAKE POINTS & UV ARRAY */
		/**************************/

	p = tmd->points;							// get ptr to points array
	tmd->numPoints = (width2+1) * (depth2+1);	// calc # points in geometry
	i = 0;

	z = back;
	v = 1.0f;

	for (h = 0; h <= depth2; h++)
	{
		x = left;
		u = 0;
		for (w = 0; w <= width2; w++)
		{
				/* SET POINT */

			p[i].x = x;
			p[i].y = y;
			p[i].z = z;
			if ((h > 0) && (h < depth2) && (w > 0) && (w < width2))		// add random jitter to inner vertices
			{
				ApplyJitterToLiquidVertex(&p[i], theNode->Kind);
			}

				/* SET UV */

			tmd->vertexUVs[i].u = uvOffset.u + u;
			tmd->vertexUVs[i].v = uvOffset.v + v;

			i++;
			x += TERRAIN_POLYGON_SIZE;
			u += .4f;
		}
		v -= .4f;
		z += TERRAIN_POLYGON_SIZE;
	}


			/* UPDATE BBOX */

	tmd->bBox.min.x = left;
	tmd->bBox.min.y = y;
	tmd->bBox.min.z = back;
	tmd->bBox.max.x = right;
	tmd->bBox.max.y = y;
	tmd->bBox.max.z = front;
	tmd->bBox.isEmpty = false;


		/**************************/
		/* MAKE TRIANGLE INDECIES */
		/**************************/

	t = tmd->triangles;								// get ptr to triangle array
	i = 0;
	for (h = 0; h < depth2; h++)
	{
		for (w = 0; w < width2; w++)
		{
			int rw = width2+1;
			
				/* TRIANGLE A */
				
			t[i].pointIndices[0] = (h * rw) + w;			// far left
			t[i].pointIndices[1] = ((h+1) * rw) + w;		// near left
			t[i].pointIndices[2] = ((h+1) * rw) + w + 1;	// near right
			i++;

				/* TRIANGLE B */

			t[i].pointIndices[0] = (h * rw) + w;			// far left
			t[i].pointIndices[1] = ((h+1) * rw) + w + 1; 	// near right
			t[i].pointIndices[2] = (h * rw) + w + 1;		// far right
			i++;		
		}
	}
	tmd->numTriangles = i;						// set # triangles in geometry


			/*************/
			/* SUBMIT IT */
			/*************/

	Render_SubmitMesh(tmd, nil, &theNode->RenderModifiers, &theNode->Coord);
}


static void ApplyJitterToLiquidVertex(TQ3Point3D* p, int type)
{
	float x = p->x;
	float y = p->y;
	float z = p->z;

	switch (type)
	{
		case LIQUID_WATER:
			p->x = x + sinf(x*z) * 60.0f;
			p->y = y;
			p->z = z + cosf(z*-x) * 60.0f;
			break;

		case LIQUID_HONEY:
			p->x = x + sinf(x*z) * 40.0f;
			p->y = y + sinf(z*x) * 10.0f;
			p->z = z + cosf(z*-x) * 40.0f;
			break;

		case LIQUID_SLIME:
			p->x = x + sinf(x*z) * 40.0f;
			p->y = y;
			p->z = z + cosf(z*-x) * 40.0f;
			break;

		case LIQUID_LAVA:
			p->x = x + sinf(x*z) * 30.0f;
			p->y = y;
			p->z = z + cosf(z*-x) * 30.0f;
			break;
	}
}
