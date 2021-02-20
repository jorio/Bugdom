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
extern	TQ3Vector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	QD3DSetupOutputType	*gGameViewInfoPtr;
extern	u_short				gLevelTypeMask;
extern	u_short				gLevelType;
extern	ObjNode				*gPlayerObj;
extern	Byte				gPlayerMode;
extern	SplineDefType		**gSplineList;
extern	Boolean				gDetonatorBlown[];
extern	u_long				gAutoFadeStatusBits;
extern	Boolean				gValveIsOpen[];
extern	ObjNode				*gFirstNodePtr;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void InitWaterPatch(void);
static void InitHoneyPatch(void);
static void InitLavaPatch(void);


static void MoveWaterPatch(ObjNode *theNode);
static void DrawWaterPatch(ObjNode *theNode, TQ3ViewObject view);
static void DrawWaterPatchTesselated(ObjNode *theNode, TQ3ViewObject view);
static void UpdateWaterTextureAnimation(void);


static void MoveHoneyPatch(ObjNode *theNode);
static void DrawHoneyPatchTesselated(ObjNode *theNode, TQ3ViewObject view);
static void UpdateHoneyTextureAnimation(void);

static void InitSlimePatch(void);
static void MoveSlimePatch(ObjNode *theNode);
static void DrawSlimePatchTesselated(ObjNode *theNode, TQ3ViewObject view);
static void UpdateSlimeTextureAnimation(void);

static void MoveLavaPatch(ObjNode *theNode);
static void UpdateLavaTextureAnimation(void);
static void DrawLavaPatchTesselated(ObjNode *theNode, TQ3ViewObject view);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_WATER_SIZE			20
#define MAX_WATER_TRIANGLES		(MAX_WATER_SIZE*MAX_WATER_SIZE*2)
#define MAX_WATER_VERTS			((MAX_WATER_SIZE+1)*(MAX_WATER_SIZE+1))

#define	RISING_WATER_YOFF		200.0f



#define	MAX_HONEY_SIZE			20
#define MAX_HONEY_TRIANGLES		(MAX_HONEY_SIZE*MAX_HONEY_SIZE*2)
#define MAX_HONEY_VERTS			((MAX_HONEY_SIZE+1)*(MAX_HONEY_SIZE+1))


#define	MAX_SLIME_SIZE			20
#define MAX_SLIME_TRIANGLES		(MAX_SLIME_SIZE*MAX_SLIME_SIZE*2)
#define MAX_SLIME_VERTS			((MAX_SLIME_SIZE+1)*(MAX_SLIME_SIZE+1))

#define	MAX_LAVA_SIZE			20
#define MAX_LAVA_TRIANGLES		(MAX_LAVA_SIZE*MAX_LAVA_SIZE*2)
#define MAX_LAVA_VERTS			((MAX_LAVA_SIZE+1)*(MAX_LAVA_SIZE+1))


/*********************/
/*    VARIABLES      */
/*********************/

#define PatchWidth		SpecialL[0]
#define PatchDepth		SpecialL[1]


		/* WATER */

static TQ3ShaderObject			gWaterShader = nil;
static TQ3TriMeshData			gWaterMesh;									// the quad version
static TQ3TriMeshData			gWaterMeshTess;								// the tesselated version

static TQ3TriMeshTriangleData	gWaterTriangles[4];							// the 2-layer quad version
static TQ3TriMeshTriangleData	gWaterTrianglesTess[MAX_WATER_TRIANGLES];	// the tesselated version
static TQ3Point3D				gWaterPoints[MAX_WATER_VERTS];
static TQ3TriMeshAttributeData	gWaterVertexAttribTypes[1];
static TQ3Param2D				gWaterUVs[MAX_WATER_VERTS];

static float gWaterU, gWaterV;				// uv offsets values for Water
static float gWaterU2, gWaterV2;
static short gNumWaterPatches;

#define	PatchID			SpecialL[2]
#define	TesselatePatch	Flag[0]
#define	PatchHasRisen	Flag[1]				// true when water has flooded up


		/* HONEY */
			
static TQ3ShaderObject			gHoneyShader = nil;
static TQ3TriMeshData			gHoneyMesh;						

static TQ3TriMeshTriangleData	gHoneyTriangles[MAX_HONEY_TRIANGLES];
static TQ3Point3D				gHoneyPoints[MAX_HONEY_VERTS];
static TQ3TriMeshAttributeData	gHoneyVertexAttribTypes[1];
static TQ3Param2D				gHoneyUVs[MAX_HONEY_VERTS];

static float gHoneyU, gHoneyV;				


		/* SLIME */
			
static TQ3ShaderObject			gSlimeShader = nil;
static TQ3TriMeshData			gSlimeMesh;						

static TQ3TriMeshTriangleData	gSlimeTriangles[MAX_SLIME_TRIANGLES];
static TQ3Point3D				gSlimePoints[MAX_SLIME_VERTS];
static TQ3TriMeshAttributeData	gSlimeVertexAttribTypes[1];
static TQ3Param2D				gSlimeUVs[MAX_SLIME_VERTS];

static float gSlimeU, gSlimeV;				


		/* LAVA */
			
static TQ3ShaderObject			gLavaShader = nil;
static TQ3TriMeshData			gLavaMesh;						

static TQ3TriMeshTriangleData	gLavaTriangles[MAX_LAVA_TRIANGLES];
static TQ3Point3D				gLavaPoints[MAX_LAVA_VERTS];
static TQ3TriMeshAttributeData	gLavaVertexAttribTypes[1];
static TQ3Param2D				gLavaUVs[MAX_LAVA_VERTS];

static float gLavaU, gLavaV;				


/****************** INIT LIQUIDS **********************/

void InitLiquids(void)
{
	InitWaterPatch();
	InitHoneyPatch();
	InitSlimePatch();
	InitLavaPatch();
}


/**************** DISPOSE LIQUIDS ********************/

void DisposeLiquids(void)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
	if (gWaterShader)
	{
		Q3Object_Dispose(gWaterShader);
		gWaterShader = nil;
	}

	if (gHoneyShader)
	{
		Q3Object_Dispose(gHoneyShader);
		gHoneyShader = nil;
	}

	if (gSlimeShader)
	{
		Q3Object_Dispose(gSlimeShader);
		gSlimeShader = nil;
	}

	if (gLavaShader)
	{
		Q3Object_Dispose(gLavaShader);
		gLavaShader = nil;
	}
#endif
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
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
	gWaterU = gWaterV = 0.0f;
	gWaterU2 = gWaterV2 = 0.0f;
	gNumWaterPatches = 0;
	
	
			/**********************/
			/* INIT WATER TEXTURE */
			/**********************/
		
			/* DISPOSE OF OLD WATER */
			
	if (gWaterShader)
	{
		Q3Object_Dispose(gWaterShader);
		gWaterShader = nil;		
	}		
		
			/* SEE IF THIS LEVEL NEEDS WATER */
				
	if (gLevelTypeMask & ((1<<LEVEL_TYPE_LAWN) | (1<<LEVEL_TYPE_POND) | (1<<LEVEL_TYPE_FOREST) |
		(1<<LEVEL_TYPE_ANTHILL)))
	{
				/* LOAD WATER TEXTURE */
				
	
		if (gLevelType == LEVEL_TYPE_POND)
			gWaterShader = QD3D_GetTextureMapFromPICTResource(129, false);
		else
			gWaterShader = QD3D_GetTextureMapFromPICTResource(128, false);


			/**********************/
			/* INIT WATER TRIMESH */
			/**********************/
			
		gWaterMesh.triMeshAttributeSet 			= nil;
		gWaterMesh.numTriangles 				= 4;
		gWaterMesh.triangles 					= &gWaterTriangles[0];
		gWaterMesh.numTriangleAttributeTypes 	= 0;
		gWaterMesh.triangleAttributeTypes 		= nil;
		gWaterMesh.numEdges						= 0;
		gWaterMesh.edges						= nil;
		gWaterMesh.numEdgeAttributeTypes		= 0;
		gWaterMesh.edgeAttributeTypes			= nil;
		gWaterMesh.numPoints 					= 8;
		gWaterMesh.points						= &gWaterPoints[0];
		gWaterMesh.numVertexAttributeTypes		= 1;
		gWaterMesh.vertexAttributeTypes			= &gWaterVertexAttribTypes[0];
		gWaterMesh.bBox.isEmpty 				= kQ3False;
		
		gWaterVertexAttribTypes[0].attributeType = kQ3AttributeTypeSurfaceUV;
		gWaterVertexAttribTypes[0].data			 = &gWaterUVs[0];
		gWaterVertexAttribTypes[0].attributeUseArray = nil;

				/* PRE-INIT TRIANGLE INDECIES */
				
		gWaterTriangles[0].pointIndices[0] = 0;
		gWaterTriangles[0].pointIndices[1] = 1;
		gWaterTriangles[0].pointIndices[2] = 2;
		gWaterTriangles[1].pointIndices[0] = 0;
		gWaterTriangles[1].pointIndices[1] = 2;
		gWaterTriangles[1].pointIndices[2] = 3;

		gWaterTriangles[2].pointIndices[0] = 4;
		gWaterTriangles[2].pointIndices[1] = 5;
		gWaterTriangles[2].pointIndices[2] = 6;
		gWaterTriangles[3].pointIndices[0] = 4;
		gWaterTriangles[3].pointIndices[1] = 6;
		gWaterTriangles[3].pointIndices[2] = 7;

		
		/*********************************/
		/* INIT TESSELATED WATER TRIMESH */
		/*********************************/
			
		gWaterMeshTess.triMeshAttributeSet 			= nil;
		gWaterMeshTess.triangles 					= &gWaterTrianglesTess[0];
		gWaterMeshTess.numTriangleAttributeTypes 	= 0;
		gWaterMeshTess.triangleAttributeTypes 		= nil;
		gWaterMeshTess.numEdges						= 0;
		gWaterMeshTess.edges						= nil;
		gWaterMeshTess.numEdgeAttributeTypes		= 0;
		gWaterMeshTess.edgeAttributeTypes			= nil;
		gWaterMeshTess.points						= &gWaterPoints[0];
		gWaterMeshTess.numVertexAttributeTypes		= 1;
		gWaterMeshTess.vertexAttributeTypes			= &gWaterVertexAttribTypes[0];
		gWaterMeshTess.bBox.isEmpty 				= kQ3False;				
	}
#endif
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
		
	GAME_ASSERT(width <= MAX_WATER_SIZE);
	GAME_ASSERT(depth <= MAX_WATER_SIZE);


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
	newObj->PatchID 		= id;									// save valve ID for anthill level
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


	gNumWaterPatches++;												// keep count of these
		
	return(true);													// item was added
}


/********************* MOVE WATER PATCH **********************/

static void MoveWaterPatch(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteObject(theNode);
		gNumWaterPatches--;
		return;
	}
	
		/* ON ANTHILL LEVEL, SEE IF TIME TO RAISE THE WATER */
		
	if ((gLevelType == LEVEL_TYPE_ANTHILL) && (!theNode->PatchHasRisen))
	{
		int	id = theNode->PatchID;					// get valve ID
		
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

static void DrawWaterPatch(ObjNode *theNode, TQ3ViewObject view)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
float			patchW,patchD;
float			x,y,z,left,back, right, front, width, depth;
TQ3Point3D		*p;
TQ3ColorRGB		color;

			/* SETUP ENVIRONMENT */
			
	Q3Push_Submit(view);
	Q3Attribute_Submit(kQ3AttributeTypeSurfaceShader, &gWaterShader, view);
	
	switch(gLevelType)
	{
		case	LEVEL_TYPE_POND:
				color.r = color.g = color.b = .7;
				break;
				
		default:
				color.r = color.g = color.b = .5;
	
	}
	Q3Attribute_Submit(kQ3AttributeTypeTransparencyColor, &color, view);


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

			/* MAKE POINTS ARRAY */
				
	p = gWaterMesh.points;
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
			
	gWaterMesh.bBox.min.x = left;
	gWaterMesh.bBox.min.y = y;
	gWaterMesh.bBox.min.z = back;
	gWaterMesh.bBox.max.x = right;
	gWaterMesh.bBox.max.y = y;
	gWaterMesh.bBox.max.z = front;


		/* SET VERTEX UVS */
		
	gWaterUVs[0].u = gWaterU;	
	gWaterUVs[0].v = gWaterV + patchD*(1.0/2.0);	
	gWaterUVs[1].u = gWaterU;	
	gWaterUVs[1].v = gWaterV;	
	gWaterUVs[2].u = gWaterU + patchW*(1.0/2.0);	
	gWaterUVs[2].v = gWaterV;	
	gWaterUVs[3].u = gWaterU + patchW*(1.0/2.0);	
	gWaterUVs[3].v = gWaterV + patchD*(1.0/2.0);	
	
	gWaterUVs[4].u = gWaterU2;	
	gWaterUVs[4].v = gWaterV2 + patchD*(1.0/3.0);	
	gWaterUVs[5].u = gWaterU2;	
	gWaterUVs[5].v = gWaterV2;	
	gWaterUVs[6].u = gWaterU2 + patchW*(1.0/3.0);	
	gWaterUVs[6].v = gWaterV2;	
	gWaterUVs[7].u = gWaterU2 + patchW*(1.0/3.0);	
	gWaterUVs[7].v = gWaterV2 + patchD*(1.0/3.0);	
	


			/*************/
			/* SUBMIT IT */
			/*************/
	
	Q3TriMesh_Submit(&gWaterMesh, view);
	
	
			/* CLEANUP */
			
	Q3Pop_Submit(view);
#endif
}

/********************* DRAW WATER PATCH TESSELATED **********************/
//
// This version tesselates the water patch so fog will look better on it.
//

static void DrawWaterPatchTesselated(ObjNode *theNode, TQ3ViewObject view)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
float			x,y,z,left,back, right, front, width, depth;
float			u,v;
TQ3Point3D		*p;
TQ3ColorRGB		color;
int				width2,depth2,w,h,i;
TQ3TriMeshTriangleData	*t;

			/*********************/
			/* SETUP ENVIRONMENT */
			/*********************/
			
	Q3Push_Submit(view);
	Q3Attribute_Submit(kQ3AttributeTypeSurfaceShader, &gWaterShader, view);
	
	switch(gLevelType)
	{
		case	LEVEL_TYPE_POND:
				color.r = color.g = color.b = .6;
				break;
				
		default:
				color.r = color.g = color.b = .5;
	
	}
	Q3Attribute_Submit(kQ3AttributeTypeTransparencyColor, &color, view);


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
				
	p = gWaterMeshTess.points;							// get ptr to points array
	gWaterMeshTess.numPoints = (width2+1) * (depth2+1);	// calc # points in geometry	
	i = 0;
	
	z = back;
	v = 1.0;
	
	for (h = 0; h <= depth2; h++)
	{
		x = left;
		u = 0;
		for (w = 0; w <= width2; w++)
		{
				/* SET POINT */
				
			if ((h > 0) && (h < depth2) && (w > 0) && (w < width2))		// add random jitter to inner vertices
			{
				p[i].x = x + sin(x*z)*60.0f;
				p[i].y = y;
				p[i].z = z + cos(z*-x)*60.0f;
			}
			else
			{
				p[i].x = x;
				p[i].y = y;
				p[i].z = z;
			}
			
				/* SET UV */
				
			gWaterUVs[i].u = gWaterU+u;
			gWaterUVs[i].v = gWaterV+v;
			
			i++;
			x += TERRAIN_POLYGON_SIZE*2.0f;	
			u += .4f;
		}
		v -= .4f;
		z += TERRAIN_POLYGON_SIZE*2.0f;
	}
	

			/* UPDATE BBOX */
			
	gWaterMeshTess.bBox.min.x = left;
	gWaterMeshTess.bBox.min.y = y;
	gWaterMeshTess.bBox.min.z = back;
	gWaterMeshTess.bBox.max.x = right;
	gWaterMeshTess.bBox.max.y = y;
	gWaterMeshTess.bBox.max.z = front;


		/**************************/
		/* MAKE TRIANGLE INDECIES */
		/**************************/

	t = gWaterMeshTess.triangles;								// get ptr to triangle array
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
	gWaterMeshTess.numTriangles = i;						// set # triangles in geometry	


			/*************/
			/* SUBMIT IT */
			/*************/
	
	Q3TriMesh_Submit(&gWaterMeshTess, view);
	
	
			/* CLEANUP */
			
	Q3Pop_Submit(view);
#endif
}



#pragma mark -


/********************** INIT HONEY PATCH ************************/
//
// NOTE: Also does Toxic Waste on AntHill & Night Level
//

static void InitHoneyPatch(void)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
	gHoneyU = gHoneyV = 0.0f;
	
			/**********************/
			/* INIT HONEY TEXTURE */
			/**********************/
	
			/* NUKE OLD SHADER */
			
	if (gHoneyShader)	
	{
		Q3Object_Dispose(gHoneyShader);
		gHoneyShader = nil;
	}
	
		
		/* SEE IF THIS LEVEL NEEDS HONEY OR LAVA */
				
	if (gLevelType == LEVEL_TYPE_HIVE)
	{
		gHoneyShader = QD3D_GetTextureMapFromPICTResource(200, false);				// load texture


			/****************/
			/* INIT TRIMESH */
			/****************/
					
		gHoneyVertexAttribTypes[0].attributeType = kQ3AttributeTypeSurfaceUV;
		gHoneyVertexAttribTypes[0].data			 = &gHoneyUVs[0];
		gHoneyVertexAttribTypes[0].attributeUseArray = nil;
		
		/*********************************/
		/* INIT TESSELATED WATER TRIMESH */
		/*********************************/
			
		gHoneyMesh.triMeshAttributeSet 			= nil;
		gHoneyMesh.triangles 					= &gHoneyTriangles[0];
		gHoneyMesh.numTriangleAttributeTypes 	= 0;
		gHoneyMesh.triangleAttributeTypes 		= nil;
		gHoneyMesh.numEdges						= 0;
		gHoneyMesh.edges						= nil;
		gHoneyMesh.numEdgeAttributeTypes		= 0;
		gHoneyMesh.edgeAttributeTypes			= nil;
		gHoneyMesh.points						= &gHoneyPoints[0];
		gHoneyMesh.numVertexAttributeTypes		= 1;
		gHoneyMesh.vertexAttributeTypes			= &gHoneyVertexAttribTypes[0];
		gHoneyMesh.bBox.isEmpty 				= kQ3False;				
	}
#endif
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
		
	GAME_ASSERT(width <= MAX_HONEY_SIZE);
	GAME_ASSERT(depth <= MAX_HONEY_SIZE);


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

static void DrawHoneyPatchTesselated(ObjNode *theNode, TQ3ViewObject view)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
float			x,y,z,left,back, right, front, width, depth;
float			u,v;
TQ3Point3D		*p;
int				width2,depth2,w,h,i;
TQ3TriMeshTriangleData	*t;

			/*********************/
			/* SETUP ENVIRONMENT */
			/*********************/
			
	Q3Push_Submit(view);
	Q3Attribute_Submit(kQ3AttributeTypeSurfaceShader, &gHoneyShader, view);


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
				
	p = gHoneyMesh.points;							// get ptr to points array
	gHoneyMesh.numPoints = (width2+1) * (depth2+1);	// calc # points in geometry	
	i = 0;
	
	z = back;
	v = 1.0;
	
	for (h = 0; h <= depth2; h++)
	{
		x = left;
		u = 0;
		for (w = 0; w <= width2; w++)
		{
				/* SET POINT */
				
			if ((h > 0) && (h < depth2) && (w > 0) && (w < width2))		// add random jitter to inner vertices
			{
				p[i].x = x + sin(x*z)*40.0f;
				p[i].y = y + sin(z*x)*10.0f;
				p[i].z = z + cos(z*-x)*40.0f;
			}
			else
			{
				p[i].x = x;
				p[i].y = y;
				p[i].z = z;
			}
			
				/* SET UV */
				
			gHoneyUVs[i].u = gHoneyU+u;
			gHoneyUVs[i].v = gHoneyV+v;
			
			i++;
			x += TERRAIN_POLYGON_SIZE;	
			u += .4f;
		}
		v -= .4f;
		z += TERRAIN_POLYGON_SIZE;
	}
	

			/* UPDATE BBOX */
			
	gHoneyMesh.bBox.min.x = left;
	gHoneyMesh.bBox.min.y = y;
	gHoneyMesh.bBox.min.z = back;
	gHoneyMesh.bBox.max.x = right;
	gHoneyMesh.bBox.max.y = y;
	gHoneyMesh.bBox.max.z = front;


		/**************************/
		/* MAKE TRIANGLE INDECIES */
		/**************************/

	t = gHoneyMesh.triangles;								// get ptr to triangle array
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
	gHoneyMesh.numTriangles = i;						// set # triangles in geometry	


			/*************/
			/* SUBMIT IT */
			/*************/
	
	Q3TriMesh_Submit(&gHoneyMesh, view);
	
	
			/* CLEANUP */
			
	Q3Pop_Submit(view);
#endif
}



#pragma mark -


/********************** INIT SLIME PATCH ************************/
//
// NOTE: Also does Toxic Waste on AntHill & Night Level
//

static void InitSlimePatch(void)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
	gSlimeU = gSlimeV = 0.0f;
	
			/**********************/
			/* INIT SLIME TEXTURE */
			/**********************/
	
			/* NUKE OLD SHADER */
			
	if (gSlimeShader)	
	{
		Q3Object_Dispose(gSlimeShader);
		gSlimeShader = nil;
	}
	
		
		/* SEE IF THIS LEVEL NEEDS SLIME OR LAVA */
				
	if (gLevelTypeMask & ((1<<LEVEL_TYPE_ANTHILL) | (1<<LEVEL_TYPE_NIGHT)))		// ant hill & night
	{
			/* LOAD SLIME TEXTURE */
				
		gSlimeShader = QD3D_GetTextureMapFromPICTResource(201, false);
		

			/****************/
			/* INIT TRIMESH */
			/****************/
					
		gSlimeVertexAttribTypes[0].attributeType 		= kQ3AttributeTypeSurfaceUV;
		gSlimeVertexAttribTypes[0].data			 		= &gSlimeUVs[0];
		gSlimeVertexAttribTypes[0].attributeUseArray 	= nil;
		
		/*********************************/
		/* INIT TESSELATED WATER TRIMESH */
		/*********************************/
			
		gSlimeMesh.triMeshAttributeSet 			= nil;
		gSlimeMesh.triangles 					= &gSlimeTriangles[0];
		gSlimeMesh.numTriangleAttributeTypes 	= 0;
		gSlimeMesh.triangleAttributeTypes 		= nil;
		gSlimeMesh.numEdges						= 0;
		gSlimeMesh.edges						= nil;
		gSlimeMesh.numEdgeAttributeTypes		= 0;
		gSlimeMesh.edgeAttributeTypes			= nil;
		gSlimeMesh.points						= &gSlimePoints[0];
		gSlimeMesh.numVertexAttributeTypes		= 1;
		gSlimeMesh.vertexAttributeTypes			= &gSlimeVertexAttribTypes[0];
		gSlimeMesh.bBox.isEmpty 				= kQ3False;				
	}
#endif
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
		
	GAME_ASSERT(width <= MAX_SLIME_SIZE);
	GAME_ASSERT(depth <= MAX_SLIME_SIZE);


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

static void DrawSlimePatchTesselated(ObjNode *theNode, TQ3ViewObject view)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
float			x,y,z,left,back, right, front, width, depth;
float			u,v;
TQ3Point3D		*p;
int				width2,depth2,w,h,i;
TQ3TriMeshTriangleData	*t;

			/*********************/
			/* SETUP ENVIRONMENT */
			/*********************/
			
	Q3Push_Submit(view);
	Q3Attribute_Submit(kQ3AttributeTypeSurfaceShader, &gSlimeShader, view);

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
				
	p = gSlimeMesh.points;							// get ptr to points array
	gSlimeMesh.numPoints = (width2+1) * (depth2+1);	// calc # points in geometry	
	i = 0;
	
	z = back;
	v = 1.0;
	
	for (h = 0; h <= depth2; h++)
	{
		x = left;
		u = 0;
		for (w = 0; w <= width2; w++)
		{
				/* SET POINT */
				
			if ((h > 0) && (h < depth2) && (w > 0) && (w < width2))		// add random jitter to inner vertices
			{
				p[i].x = x + sin(x*z)*40.0f;
				p[i].y = y;
				p[i].z = z + cos(z*-x)*40.0f;
			}
			else
			{
				p[i].x = x;
				p[i].y = y;
				p[i].z = z;
			}
			
				/* SET UV */
				
			gSlimeUVs[i].u = gSlimeU+u;
			gSlimeUVs[i].v = gSlimeV+v;
			
			i++;
			x += TERRAIN_POLYGON_SIZE;	
			u += .4f;
		}
		v -= .4f;
		z += TERRAIN_POLYGON_SIZE;
	}
	

			/* UPDATE BBOX */
			
	gSlimeMesh.bBox.min.x = left;
	gSlimeMesh.bBox.min.y = y;
	gSlimeMesh.bBox.min.z = back;
	gSlimeMesh.bBox.max.x = right;
	gSlimeMesh.bBox.max.y = y;
	gSlimeMesh.bBox.max.z = front;


		/**************************/
		/* MAKE TRIANGLE INDECIES */
		/**************************/

	t = gSlimeMesh.triangles;								// get ptr to triangle array
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
	gSlimeMesh.numTriangles = i;						// set # triangles in geometry	


			/*************/
			/* SUBMIT IT */
			/*************/
	
	Q3TriMesh_Submit(&gSlimeMesh, view);
	
	
			/* CLEANUP */
			
	Q3Pop_Submit(view);
#endif
}

#pragma mark -



/********************** INIT LAVA PATCH ************************/
//
// NOTE: Also does Toxic Waste on AntHill & Night Level
//

static void InitLavaPatch(void)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
	gLavaU = gLavaV = 0.0f;
	
			/**********************/
			/* INIT LAVA TEXTURE */
			/**********************/
	
			/* NUKE OLD SHADER */
			
	if (gLavaShader)	
	{
		Q3Object_Dispose(gLavaShader);
		gLavaShader = nil;
	}
	
		
		/* SEE IF THIS LEVEL NEEDS LAVA */
				
	if (gLevelType == LEVEL_TYPE_ANTHILL)
	{
			/* LOAD LAVA TEXTURE */
				
		gLavaShader = QD3D_GetTextureMapFromPICTResource(202, false);
		

			/****************/
			/* INIT TRIMESH */
			/****************/
					
		gLavaVertexAttribTypes[0].attributeType 		= kQ3AttributeTypeSurfaceUV;
		gLavaVertexAttribTypes[0].data			 		= &gLavaUVs[0];
		gLavaVertexAttribTypes[0].attributeUseArray 	= nil;
		
		/*********************************/
		/* INIT TESSELATED WATER TRIMESH */
		/*********************************/
			
		gLavaMesh.triMeshAttributeSet 			= nil;
		gLavaMesh.triangles 					= &gLavaTriangles[0];
		gLavaMesh.numTriangleAttributeTypes 	= 0;
		gLavaMesh.triangleAttributeTypes 		= nil;
		gLavaMesh.numEdges						= 0;
		gLavaMesh.edges						= nil;
		gLavaMesh.numEdgeAttributeTypes		= 0;
		gLavaMesh.edgeAttributeTypes			= nil;
		gLavaMesh.points						= &gLavaPoints[0];
		gLavaMesh.numVertexAttributeTypes		= 1;
		gLavaMesh.vertexAttributeTypes			= &gLavaVertexAttribTypes[0];
		gLavaMesh.bBox.isEmpty 				= kQ3False;				
	}
#endif
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
		
	GAME_ASSERT(width <= MAX_LAVA_SIZE);
	GAME_ASSERT(depth <= MAX_LAVA_SIZE);


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

static void DrawLavaPatchTesselated(ObjNode *theNode, TQ3ViewObject view)
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
float			x,y,z,left,back, right, front, width, depth;
float			u,v;
TQ3Point3D		*p;
int				width2,depth2,w,h,i;
TQ3TriMeshTriangleData	*t;

			/*********************/
			/* SETUP ENVIRONMENT */
			/*********************/
			
	Q3Push_Submit(view);
	Q3Attribute_Submit(kQ3AttributeTypeSurfaceShader, &gLavaShader, view);


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
				
	p = gLavaMesh.points;							// get ptr to points array
	gLavaMesh.numPoints = (width2+1) * (depth2+1);	// calc # points in geometry	
	i = 0;
	
	z = back;
	v = 1.0;
	
	for (h = 0; h <= depth2; h++)
	{
		x = left;
		u = 0;
		for (w = 0; w <= width2; w++)
		{
				/* SET POINT */
				
			if ((h > 0) && (h < depth2) && (w > 0) && (w < width2))		// add random jitter to inner vertices
			{
				p[i].x = x + sin(x*z)*30.0f;
				p[i].y = y;
				p[i].z = z + cos(z*-x)*30.0f;
			}
			else
			{
				p[i].x = x;
				p[i].y = y;
				p[i].z = z;
			}
			
				/* SET UV */
				
			gLavaUVs[i].u = gLavaU+u;
			gLavaUVs[i].v = gLavaV+v;
			
			i++;
			x += TERRAIN_POLYGON_SIZE;	
			u += .4f;
		}
		v -= .4f;
		z += TERRAIN_POLYGON_SIZE;
	}
	

			/* UPDATE BBOX */
			
	gLavaMesh.bBox.min.x = left;
	gLavaMesh.bBox.min.y = y;
	gLavaMesh.bBox.min.z = back;
	gLavaMesh.bBox.max.x = right;
	gLavaMesh.bBox.max.y = y;
	gLavaMesh.bBox.max.z = front;


		/**************************/
		/* MAKE TRIANGLE INDECIES */
		/**************************/

	t = gLavaMesh.triangles;								// get ptr to triangle array
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
	gLavaMesh.numTriangles = i;						// set # triangles in geometry	


			/*************/
			/* SUBMIT IT */
			/*************/
	
	Q3TriMesh_Submit(&gLavaMesh, view);
	
	
			/* CLEANUP */
			
	Q3Pop_Submit(view);
#endif
}
