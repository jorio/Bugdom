/****************************/
/*   	LIQUIDS.C		    */
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	TQ3Point3D			gCoord,gMyCoord;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	QD3DSetupOutputType	*gGameViewInfoPtr;
extern	u_short				gLevelTypeMask;
extern	u_short				gLevelType;
extern	Boolean				gValveIsOpen[];
extern	ObjNode				*gFirstNodePtr;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void InitWaterPatch(void);
static void InitHoneyPatch(void);
static void InitLavaPatch(void);


static void MoveWaterPatch(ObjNode *theNode);
static void DrawWaterPatch(ObjNode *theNode);
static void DrawWaterPatchTesselated(ObjNode *theNode);
static void UpdateWaterTextureAnimation(void);


static void MoveHoneyPatch(ObjNode *theNode);
static void DrawHoneyPatchTesselated(ObjNode *theNode);
static void UpdateHoneyTextureAnimation(void);

static void InitSlimePatch(void);
static void MoveSlimePatch(ObjNode *theNode);
static void DrawSlimePatchTesselated(ObjNode *theNode);
static void UpdateSlimeTextureAnimation(void);

static void MoveLavaPatch(ObjNode *theNode);
static void UpdateLavaTextureAnimation(void);
static void DrawLavaPatchTesselated(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define MAX_LIQUID_MESHES		16
#define MAX_LIQUID_SIZE			20
#define MAX_LIQUID_TRIANGLES	(MAX_LIQUID_SIZE*MAX_LIQUID_SIZE*2)
#define MAX_LIQUID_VERTS		((MAX_LIQUID_SIZE+1)*(MAX_LIQUID_SIZE+1))

#define	RISING_WATER_YOFF		200.0f


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


		/* WATER */

static GLuint			gWaterShader = 0;
static float gWaterU, gWaterV;				// uv offsets values for Water
static float gWaterU2, gWaterV2;


		/* HONEY */
			
static GLuint			gHoneyShader = 0;
static float			gHoneyU, gHoneyV;


		/* SLIME */

static GLuint			gSlimeShader = 0;
static float			gSlimeU, gSlimeV;


		/* LAVA */

static GLuint			gLavaShader = 0;
static float			gLavaU, gLavaV;


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
static bool liquidMeshesAllocated = false;

	gNumActiveLiquidMeshes = 0;

	if (!liquidMeshesAllocated)
	{
		for (int i = 0; i < MAX_LIQUID_MESHES; i++)
		{
			gLiquidMeshPtrs[i] = Q3TriMeshData_New(MAX_LIQUID_TRIANGLES, MAX_LIQUID_VERTS, false);
			gFreeLiquidMeshes[i] = i;
		}
		liquidMeshesAllocated = true;
	}

	InitWaterPatch();
	InitHoneyPatch();
	InitSlimePatch();
	InitLavaPatch();
}


/**************** DISPOSE LIQUIDS ********************/

void DisposeLiquids(void)
{
	DeleteTexture(&gWaterShader);
	DeleteTexture(&gHoneyShader);
	DeleteTexture(&gSlimeShader);
	DeleteTexture(&gLavaShader);

	for (int i = 0; i < MAX_LIQUID_MESHES; i++)
	{
		gFreeLiquidMeshes[i] = i;
	}
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

float FindLiquidY(float x, float z)
{
ObjNode *thisNodePtr;
float		y;

	thisNodePtr = gFirstNodePtr;
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

				y = thisNodePtr->CollisionBoxes[0].top;
				
				switch(thisNodePtr->Kind)
				{
					case	LIQUID_WATER:
							y += WATER_COLLISION_TOPOFF;
							break;
							
					case	LIQUID_SLIME:
							y += SLIME_COLLISION_TOPOFF;
							break;

					case	LIQUID_HONEY:
							y += HONEY_COLLISION_TOPOFF;
							break;

					case	LIQUID_LAVA:
							y += LAVA_COLLISION_TOPOFF;
							break;						
				}				
				return(y);				
			}
		}		
next:					
		thisNodePtr = (ObjNode *)thisNodePtr->NextNode;		// next node
	}
	while (thisNodePtr != nil);

	return(0);
}


#pragma mark -

/********************** INIT WATER PATCH ************************/

static void InitWaterPatch(void)
{
	gWaterU = gWaterV = 0.0f;
	gWaterU2 = gWaterV2 = 0.0f;


			/**********************/
			/* INIT WATER TEXTURE */
			/**********************/

			/* DISPOSE OF OLD WATER */

	DeleteTexture(&gWaterShader);

			/* SEE IF THIS LEVEL NEEDS WATER */

	const uint16_t kLevelsWithWater =
			(1<<LEVEL_TYPE_LAWN) | (1<<LEVEL_TYPE_POND) | (1<<LEVEL_TYPE_FOREST) | (1<<LEVEL_TYPE_ANTHILL);

	if (!(gLevelTypeMask & kLevelsWithWater))
		return;

				/* LOAD WATER TEXTURE */

	long waterFile = gLevelType == LEVEL_TYPE_POND ? 129 : 128;

	gWaterShader = QD3D_NumberedTGAToTexture(waterFile, false, kRendererTextureFlags_None);
}

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

	if (!gWaterShader)
		DoFatalAlert("AddWaterPatch: water not activated on this level");
		
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
	gNewObjectDefinition.moveCall = MoveWaterPatch;
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
	SetObjectCollisionBounds(newObj,-WATER_COLLISION_TOPOFF,-2000,
							-(width*.5f) * TERRAIN_POLYGON_SIZE,width*.5f * TERRAIN_POLYGON_SIZE,
							depth*.5f * TERRAIN_POLYGON_SIZE,-(depth*.5f) * TERRAIN_POLYGON_SIZE);


	return(true);													// item was added
}


/********************* MOVE WATER PATCH **********************/

static void MoveWaterPatch(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DisposeLiquidMesh(theNode);
		DeleteObject(theNode);
		return;
	}
	
		/* ON ANTHILL LEVEL, SEE IF TIME TO RAISE THE WATER */

	if ((gLevelType == LEVEL_TYPE_ANTHILL) && (!theNode->PatchHasRisen))
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
	gWaterU += gFramesPerSecondFrac * .05f;
	gWaterV += gFramesPerSecondFrac * .1f;
	gWaterU2 -= gFramesPerSecondFrac * .12f;
	gWaterV2 -= gFramesPerSecondFrac * .07f;
}


/********************* DRAW WATER PATCH **********************/
//
// This is the simple version which just draws a giant quad
//

static void DrawWaterPatch(ObjNode *theNode)
{
float			patchW,patchD;
float			x,y,z,left,back, right, front, width, depth;
TQ3Point3D		*p;

	TQ3TriMeshData* tmd = gLiquidMeshPtrs[theNode->PatchMeshID];

			/* SETUP ENVIRONMENT */

	tmd->diffuseColor.a = gLevelType == LEVEL_TYPE_POND ? .7f : .5f;
	tmd->texturingMode = kQ3TexturingModeAlphaBlend;
	tmd->glTextureName = gWaterShader;
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

	width = right - left;
	depth = front - back;

	// Source port mod: water is made of 2 overlapping faces to achieve a UV
	// scrolling effect in 2 directions. Prevent the water faces from Z-fighting
	// (well, actually, Y-fighting) by moving them apart slightly.
	float y1 = y - 0.0f;
	float y2 = y - 1.0f;

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
	p[0].y = y1;
	p[0].z = back;

	p[1].x = left;
	p[1].y = y1;
	p[1].z = front;

	p[2].x = right;
	p[2].y = y1;
	p[2].z = front;

	p[3].x = right;
	p[3].y = y1;
	p[3].z = back;

	p[4].x = left;
	p[4].y = y2;
	p[4].z = back;

	p[5].x = left;
	p[5].y = y2;
	p[5].z = front;

	p[6].x = right;
	p[6].y = y2;
	p[6].z = front;

	p[7].x = right;
	p[7].y = y2;
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

	tmd->vertexUVs[0].u = gWaterU;
	tmd->vertexUVs[0].v = gWaterV + patchD*(1.0/2.0);
	tmd->vertexUVs[1].u = gWaterU;
	tmd->vertexUVs[1].v = gWaterV;
	tmd->vertexUVs[2].u = gWaterU + patchW*(1.0/2.0);
	tmd->vertexUVs[2].v = gWaterV;
	tmd->vertexUVs[3].u = gWaterU + patchW*(1.0/2.0);
	tmd->vertexUVs[3].v = gWaterV + patchD*(1.0/2.0);

	tmd->vertexUVs[4].u = gWaterU2;
	tmd->vertexUVs[4].v = gWaterV2 + patchD*(1.0/3.0);
	tmd->vertexUVs[5].u = gWaterU2;
	tmd->vertexUVs[5].v = gWaterV2;
	tmd->vertexUVs[6].u = gWaterU2 + patchW*(1.0/3.0);
	tmd->vertexUVs[6].v = gWaterV2;
	tmd->vertexUVs[7].u = gWaterU2 + patchW*(1.0/3.0);
	tmd->vertexUVs[7].v = gWaterV2 + patchD*(1.0/3.0);

			/*************/
			/* SUBMIT IT */
			/*************/

	Render_SubmitMesh(tmd, nil, nil, nil);
}

/********************* DRAW WATER PATCH TESSELATED **********************/
//
// This version tesselates the water patch so fog will look better on it.
//

static void DrawWaterPatchTesselated(ObjNode *theNode)
{
float			x,y,z,left,back, right, front, width, depth;
float			u,v;
TQ3Point3D		*p;
int				width2,depth2,w,h,i;
TQ3TriMeshTriangleData	*t;

	TQ3TriMeshData* tmd = gLiquidMeshPtrs[theNode->PatchMeshID];

			/*********************/
			/* SETUP ENVIRONMENT */
			/*********************/

	tmd->diffuseColor.a = gLevelType == LEVEL_TYPE_POND ? .6f : .5f;
	tmd->texturingMode = kQ3TexturingModeAlphaBlend;
	tmd->glTextureName = gWaterShader;

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

	width = right - left;
	depth = front - back;
	

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

			if ((h > 0) && (h < depth2) && (w > 0) && (w < width2))		// add random jitter to inner vertices
			{
				p[i].x = x + sinf(x*z)*60.0f;
				p[i].y = y;
				p[i].z = z + cosf(z*-x)*60.0f;
			}
			else
			{
				p[i].x = x;
				p[i].y = y;
				p[i].z = z;
			}

				/* SET UV */

			tmd->vertexUVs[i].u = gWaterU+u;
			tmd->vertexUVs[i].v = gWaterV+v;

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

	Render_SubmitMesh(tmd, nil, nil, nil);
}



#pragma mark -


/********************** INIT HONEY PATCH ************************/
//
// NOTE: Also does Toxic Waste on AntHill & Night Level
//

static void InitHoneyPatch(void)
{
	gHoneyU = gHoneyV = 0.0f;

			/**********************/
			/* INIT HONEY TEXTURE */
			/**********************/

			/* NUKE OLD SHADER */

	DeleteTexture(&gHoneyShader);


		/* SEE IF THIS LEVEL NEEDS HONEY OR LAVA */

	if (gLevelType == LEVEL_TYPE_HIVE)
	{
			/* LOAD HONEY TEXTURE */

		gHoneyShader = QD3D_NumberedTGAToTexture(200, false, kRendererTextureFlags_None);
	}
}


/************************* ADD HONEY PATCH *********************************/
//
// parm[0] = # tiles wide
// parm[1] = # tiles deep
// parm[2] = y off or y coord index
// parm[3]: bit 0 = use y coord index for fixed ht.
//

Boolean AddHoneyPatch(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode				*newObj;
float				y,yOff;
TQ3BoundingSphere	bSphere;
float				width,depth;
//static const float	yTable[] = {-230,-200,0,0,0,0};
static const float	yTable[] = {-620,-580,-550,-600,0,0};

	if (!gHoneyShader)
		DoFatalAlert("AddHoneyPatch: honey not activated on this level");
		
	x -= (int)x % (int)TERRAIN_POLYGON_SIZE;				// round down to nearest tile & center
	x += TERRAIN_POLYGON_SIZE/2;
	z -= (int)z % (int)TERRAIN_POLYGON_SIZE;
	z += TERRAIN_POLYGON_SIZE/2;
		
	if (itemPtr->parm[3]&1)									// see if use indexed y
	{
		y = yTable[itemPtr->parm[2]];						// get y from table
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
	gNewObjectDefinition.flags 	= STATUS_BIT_NULLSHADER;
	gNewObjectDefinition.slot 	= SLOT_OF_DUMB-1;
	gNewObjectDefinition.moveCall = MoveHoneyPatch;
	gNewObjectDefinition.rot 	= 0;
	gNewObjectDefinition.scale = 1.0;
	newObj = MakeNewCustomDrawObject(&gNewObjectDefinition, &bSphere, DrawHoneyPatchTesselated);	
		
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Kind = LIQUID_HONEY;

	newObj->PatchWidth 		= width;
	newObj->PatchDepth 		= depth;
	newObj->PatchMeshID		= AllocLiquidMesh();

			/* SET COLLISION */
			//
			// Honey patches have a water volume collision area.
			// The collision volume starts a little under the top of the water so that
			// no collision is detected in shallow water, but if player sinks deep
			// then a collision will be detected.
			//
				
	newObj->CType = CTYPE_LIQUID|CTYPE_BLOCKCAMERA|CTYPE_BLOCKSHADOW;
	newObj->CBits = CBITS_TOUCHABLE;
	SetObjectCollisionBounds(newObj,
							-HONEY_COLLISION_TOPOFF,
							-2000,
							-(width*.5f) * TERRAIN_POLYGON_SIZE,
							(width*.5f) * TERRAIN_POLYGON_SIZE,
							(depth*.5f) * TERRAIN_POLYGON_SIZE,
							-(depth*.5f) * TERRAIN_POLYGON_SIZE);

	return(true);													// item was added
}


/********************* MOVE HONEY PATCH **********************/

static void MoveHoneyPatch(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DisposeLiquidMesh(theNode);
		DeleteObject(theNode);
		return;
	}
}


/**************** UPDATE HONEY TEXTURE ANIMATION ********************/

static void UpdateHoneyTextureAnimation(void)
{
	gHoneyU += gFramesPerSecondFrac * .09f;
	gHoneyV += gFramesPerSecondFrac * .1f;
}



/********************* DRAW HONEY PATCH TESSELATED **********************/
//
// This version tesselates the water patch so fog will look better on it.
//

static void DrawHoneyPatchTesselated(ObjNode *theNode)
{
float			x,y,z,left,back, right, front, width, depth;
float			u,v;
TQ3Point3D		*p;
int				width2,depth2,w,h,i;
TQ3TriMeshTriangleData	*t;

	TQ3TriMeshData* tmd = gLiquidMeshPtrs[theNode->PatchMeshID];

			/*********************/
			/* SETUP ENVIRONMENT */
			/*********************/

	tmd->diffuseColor.a = 1.0f;
	tmd->texturingMode = kQ3TexturingModeOpaque;
	tmd->glTextureName = gHoneyShader;

			/************************/
			/* CALC BOUNDS OF HONEY */
			/************************/
			
	x = theNode->Coord.x;
	y = theNode->Coord.y;
	z = theNode->Coord.z;
	
	width2 = theNode->PatchWidth;
	depth2 = theNode->PatchDepth;
	
	left = x - (width2*.5f) * TERRAIN_POLYGON_SIZE;
	right = left + width2 * TERRAIN_POLYGON_SIZE;
	back = z - (depth2*.5f) * TERRAIN_POLYGON_SIZE;
	front = back + depth2 * TERRAIN_POLYGON_SIZE;

	width = right - left;
	depth = front - back;
	

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
				
			if ((h > 0) && (h < depth2) && (w > 0) && (w < width2))		// add random jitter to inner vertices
			{
				p[i].x = x + sinf(x*z)*40.0f;
				p[i].y = y + sinf(z*x)*10.0f;
				p[i].z = z + cosf(z*-x)*40.0f;
			}
			else
			{
				p[i].x = x;
				p[i].y = y;
				p[i].z = z;
			}
			
				/* SET UV */

			tmd->vertexUVs[i].u = gHoneyU+u;
			tmd->vertexUVs[i].v = gHoneyV+v;

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

	Render_SubmitMesh(tmd, nil, nil, nil);
}



#pragma mark -


/********************** INIT SLIME PATCH ************************/
//
// NOTE: Also does Toxic Waste on AntHill & Night Level
//

static void InitSlimePatch(void)
{
	gSlimeU = gSlimeV = 0.0f;
	
			/**********************/
			/* INIT SLIME TEXTURE */
			/**********************/

			/* NUKE OLD SHADER */

	DeleteTexture(&gSlimeShader);


		/* SEE IF THIS LEVEL NEEDS SLIME OR LAVA */

	if (gLevelTypeMask & ((1<<LEVEL_TYPE_ANTHILL) | (1<<LEVEL_TYPE_NIGHT)))		// ant hill & night
	{
			/* LOAD SLIME TEXTURE */

		gSlimeShader = QD3D_NumberedTGAToTexture(201, false, kRendererTextureFlags_None);
	}
}


/************************* ADD SLIME PATCH *********************************/
//
// parm[0] = # tiles wide
// parm[1] = # tiles deep
// parm[2] = y off or y coord index
// parm[3]: bit 0 = use y coord index for fixed ht.
//

Boolean AddSlimePatch(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode				*newObj;
float				y,yOff;
TQ3BoundingSphere	bSphere;
float				width,depth;
static const float	yTable[] = {0,-200,0,0,0,0};

	if (!gSlimeShader)
		DoFatalAlert("AddSlimePatch: slime not activated on this level");
		
	x -= (int)x % (int)TERRAIN_POLYGON_SIZE;				// round down to nearest tile & center
	x += TERRAIN_POLYGON_SIZE/2;
	z -= (int)z % (int)TERRAIN_POLYGON_SIZE;
	z += TERRAIN_POLYGON_SIZE/2;
		
	
	if (itemPtr->parm[3]&1)									// see if use indexed y
	{
		y = yTable[itemPtr->parm[2]];						// get y from table
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
	gNewObjectDefinition.flags 	= STATUS_BIT_NOTRICACHE|STATUS_BIT_NULLSHADER|STATUS_BIT_NOZWRITE;
	gNewObjectDefinition.slot 	= SLOT_OF_DUMB-1;
	gNewObjectDefinition.moveCall = MoveSlimePatch;
	gNewObjectDefinition.rot 	= 0;
	gNewObjectDefinition.scale = 1.0;
	newObj = MakeNewCustomDrawObject(&gNewObjectDefinition, &bSphere, DrawSlimePatchTesselated);	
		
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Kind = LIQUID_SLIME;


	newObj->PatchWidth 		= width;
	newObj->PatchDepth 		= depth;
	newObj->PatchMeshID		= AllocLiquidMesh();

			/* SET COLLISION */
			//
			// Slime patches have a water volume collision area.
			// The collision volume starts a little under the top of the water so that
			// no collision is detected in shallow water, but if player sinks deep
			// then a collision will be detected.
			//
				
	newObj->CType = CTYPE_LIQUID|CTYPE_BLOCKCAMERA|CTYPE_BLOCKSHADOW;
	newObj->CBits = CBITS_TOUCHABLE;
	SetObjectCollisionBounds(newObj,
							-SLIME_COLLISION_TOPOFF,
							-2000,
							-(width*.5f) * TERRAIN_POLYGON_SIZE,
							(width*.5f) * TERRAIN_POLYGON_SIZE,
							(depth*.5f) * TERRAIN_POLYGON_SIZE,
							-(depth*.5f) * TERRAIN_POLYGON_SIZE);

	return(true);													// item was added
}


/********************* MOVE SLIME PATCH **********************/

static void MoveSlimePatch(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DisposeLiquidMesh(theNode);
		DeleteObject(theNode);
		return;
	}
}


/**************** UPDATE SLIME TEXTURE ANIMATION ********************/

static void UpdateSlimeTextureAnimation(void)
{
	gSlimeU += gFramesPerSecondFrac * .09f;
	gSlimeV += gFramesPerSecondFrac * .1f;
}



/********************* DRAW SLIME PATCH TESSELATED **********************/
//
// This version tesselates the water patch so fog will look better on it.
//

static void DrawSlimePatchTesselated(ObjNode *theNode)
{
float			x,y,z,left,back, right, front, width, depth;
float			u,v;
TQ3Point3D		*p;
int				width2,depth2,w,h,i;
TQ3TriMeshTriangleData	*t;

	TQ3TriMeshData* tmd = gLiquidMeshPtrs[theNode->PatchMeshID];

			/*********************/
			/* SETUP ENVIRONMENT */
			/*********************/

	tmd->diffuseColor.a = 1.0f;
	tmd->texturingMode = kQ3TexturingModeOpaque;
	tmd->glTextureName = gSlimeShader;

			/************************/
			/* CALC BOUNDS OF SLIME */
			/************************/
			
	x = theNode->Coord.x;
	y = theNode->Coord.y;
	z = theNode->Coord.z;
	
	width2 = theNode->PatchWidth;
	depth2 = theNode->PatchDepth;
	
	left = x - (width2*.5f) * TERRAIN_POLYGON_SIZE;
	right = left + width2 * TERRAIN_POLYGON_SIZE;
	back = z - (depth2*.5f) * TERRAIN_POLYGON_SIZE;
	front = back + depth2 * TERRAIN_POLYGON_SIZE;

	width = right - left;
	depth = front - back;
	

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
				
			if ((h > 0) && (h < depth2) && (w > 0) && (w < width2))		// add random jitter to inner vertices
			{
				p[i].x = x + sinf(x*z)*40.0f;
				p[i].y = y;
				p[i].z = z + cosf(z*-x)*40.0f;
			}
			else
			{
				p[i].x = x;
				p[i].y = y;
				p[i].z = z;
			}
			
				/* SET UV */

			tmd->vertexUVs[i].u = gSlimeU+u;
			tmd->vertexUVs[i].v = gSlimeV+v;

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

	Render_SubmitMesh(tmd, nil, nil, nil);
}

#pragma mark -



/********************** INIT LAVA PATCH ************************/

static void InitLavaPatch(void)
{
	gLavaU = gLavaV = 0.0f;
	
			/**********************/
			/* INIT LAVA TEXTURE */
			/**********************/

			/* NUKE OLD SHADER */

	DeleteTexture(&gLavaShader);


		/* SEE IF THIS LEVEL NEEDS LAVA */

	if (gLevelType == LEVEL_TYPE_ANTHILL)
	{
			/* LOAD LAVA TEXTURE */

		gLavaShader = QD3D_NumberedTGAToTexture(202, false, kRendererTextureFlags_None);
	}
}


/************************* ADD LAVA PATCH *********************************/
//
// parm[0] = # tiles wide
// parm[1] = # tiles deep
// parm[2] = y off or y coord index
// parm[3]: bit 0 = use y coord index for fixed ht.
//

Boolean AddLavaPatch(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode				*newObj;
float				y,yOff;
TQ3BoundingSphere	bSphere;
float				width,depth;
static const float	yTable[] = {-230,-230,-230,0,0,0};

	if (!gLavaShader)
		DoFatalAlert("AddLavaPatch: slime not activated on this level");
		
	x -= (int)x % (int)TERRAIN_POLYGON_SIZE;				// round down to nearest tile & center
	x += TERRAIN_POLYGON_SIZE/2;
	z -= (int)z % (int)TERRAIN_POLYGON_SIZE;
	z += TERRAIN_POLYGON_SIZE/2;
		
	if (itemPtr->parm[3]&1)									// see if use indexed y
	{
		y = yTable[itemPtr->parm[2]];						// get y from table
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
	bSphere.radius = (((float)width + (float)depth) / 2.0f) * TERRAIN_POLYGON_SIZE;


			/* MAKE OBJECT */
			
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = y;
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags 	= STATUS_BIT_NOTRICACHE|STATUS_BIT_NULLSHADER|STATUS_BIT_NOZWRITE;
	gNewObjectDefinition.slot 	= SLOT_OF_DUMB-1;
	gNewObjectDefinition.moveCall = MoveLavaPatch;
	gNewObjectDefinition.rot 	= 0;
	gNewObjectDefinition.scale = 1.0;
	newObj = MakeNewCustomDrawObject(&gNewObjectDefinition, &bSphere, DrawLavaPatchTesselated);	
		
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Kind = LIQUID_LAVA;

	newObj->PatchWidth 		= width;
	newObj->PatchDepth 		= depth;
	newObj->PatchMeshID		= AllocLiquidMesh();

			/* SET COLLISION */
			//
			// Lava patches have a water volume collision area.
			// The collision volume starts a little under the top of the water so that
			// no collision is detected in shallow water, but if player sinks deep
			// then a collision will be detected.
			//
				
	newObj->CType = CTYPE_LIQUID|CTYPE_BLOCKCAMERA|CTYPE_BLOCKSHADOW;
	newObj->CBits = CBITS_TOUCHABLE;
	SetObjectCollisionBounds(newObj,
							-LAVA_COLLISION_TOPOFF,
							-2000,
							-(width*.5f) * TERRAIN_POLYGON_SIZE,
							(width*.5f) * TERRAIN_POLYGON_SIZE,
							(depth*.5f) * TERRAIN_POLYGON_SIZE,
							-(depth*.5f) * TERRAIN_POLYGON_SIZE);

	return(true);													// item was added
}


/********************* MOVE LAVA PATCH **********************/

static void MoveLavaPatch(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DisposeLiquidMesh(theNode);
		DeleteObject(theNode);
		return;
	}
}


/**************** UPDATE LAVA TEXTURE ANIMATION ********************/

static void UpdateLavaTextureAnimation(void)
{
	gLavaU += gFramesPerSecondFrac * .09f;
	gLavaV += gFramesPerSecondFrac * .1f;
}



/********************* DRAW LAVA PATCH TESSELATED **********************/
//
// This version tesselates the water patch so fog will look better on it.
//

static void DrawLavaPatchTesselated(ObjNode *theNode)
{
float			x,y,z,left,back, right, front, width, depth;
float			u,v;
TQ3Point3D		*p;
int				width2,depth2,w,h,i;
TQ3TriMeshTriangleData	*t;

	TQ3TriMeshData* tmd = gLiquidMeshPtrs[theNode->PatchMeshID];

			/*********************/
			/* SETUP ENVIRONMENT */
			/*********************/

	tmd->diffuseColor.a = 1.0f;
	tmd->texturingMode = kQ3TexturingModeOpaque;
	tmd->glTextureName = gLavaShader;


			/************************/
			/* CALC BOUNDS OF LAVA */
			/************************/
			
	x = theNode->Coord.x;
	y = theNode->Coord.y;
	z = theNode->Coord.z;
	
	width2 = theNode->PatchWidth;
	depth2 = theNode->PatchDepth;
	
	left = x - (width2*.5f) * TERRAIN_POLYGON_SIZE;
	right = left + width2 * TERRAIN_POLYGON_SIZE;
	back = z - (depth2*.5f) * TERRAIN_POLYGON_SIZE;
	front = back + depth2 * TERRAIN_POLYGON_SIZE;

	width = right - left;
	depth = front - back;


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
				
			if ((h > 0) && (h < depth2) && (w > 0) && (w < width2))		// add random jitter to inner vertices
			{
				p[i].x = x + sinf(x*z)*30.0f;
				p[i].y = y;
				p[i].z = z + cosf(z*-x)*30.0f;
			}
			else
			{
				p[i].x = x;
				p[i].y = y;
				p[i].z = z;
			}
			
				/* SET UV */

			tmd->vertexUVs[i].u = gLavaU+u;
			tmd->vertexUVs[i].v = gLavaV+v;

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

	Render_SubmitMesh(tmd, nil, nil, nil);
}
