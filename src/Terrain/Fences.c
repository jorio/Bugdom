/**********************/
/*   	FENCES.C      */
/**********************/


#include "3dmath.h"

/***************/
/* EXTERNALS   */
/***************/

extern	u_char	gTerrainScrollBuffer[MAX_SUPERTILES_DEEP][MAX_SUPERTILES_WIDE];
extern	TQ3Point3D	gCoord;
extern	TQ3Vector3D	gDelta;
extern	float		gAutoFadeStartDist;
extern	u_short		gLevelType;
extern	ObjNode		*gCurrentDragonFly,*gPlayerObj;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SubmitFence(int f, TQ3ViewObject viewObj, float camX, float camZ);


/****************************/
/*    CONSTANTS             */
/****************************/

#define MAX_FENCES			60
#define	MAX_NUBS_IN_FENCE	40

#define	NUM_FENCE_SHADERS	9


#define	FENCE_SINK_FACTOR	40.0f

enum
{
	FENCE_TYPE_THORN,
	FENCE_TYPE_WHEAT,
	FENCE_TYPE_GRASS,
	FENCE_TYPE_FOREST2,
	FENCE_TYPE_NIGHT,
	FENCE_TYPE_POND,
	FENCE_TYPE_MOSS,
	FENCE_TYPE_WOOD,
	FENCE_TYPE_HIVE
};


/**********************/
/*     VARIABLES      */
/**********************/

long			gNumFences = 0;
FenceDefType	*gFenceList = nil;
Boolean			gIsFenceVisible[MAX_FENCES];

static TQ3AttributeSet	gFenceShaderAttribs[NUM_FENCE_SHADERS];


static Boolean gFenceOnThisLevel[NUM_LEVELS][NUM_FENCE_SHADERS] =
{
	//			Hay										AntHill
	//	Thorn	weed	Grass	????	Night	Pond	moss	Wood	Hive
	{	1,		0,		1,		0,		0,		0,		0,		0,		0,		},	// LAWN
	{	0,		0,		1,		0,		0,		1,		0,		0,		0,		},	// POND
	{	0,		1,		0,		1,		0,		0,		0,		1,		0,		},	// FOREST
	{	0,		0,		0,		0,		0,		0,		0,		0,		1,		},	// HIVE
	{	0,		0,		0,		0,		1,		0,		0,		0,		0,		},	// NIGHT
	{	0,		0,		0,		0,		0,		0,		1,		0,		0,		},	// ANTHILL
};


float			gFenceHeight[NUM_FENCE_SHADERS] =
{
	600,			// thorn
	1000,			// wheat
	1000,			// grass
	500,			// ????
	1000,			// night
	2000,			// pond
	200,			// moss
	6000,			// wood
	1200,			// hive
};

double			gFenceTextureW[NUM_FENCE_SHADERS] =		// smaller denom == smaller width
{
	1.0f/600.0f,			// thorn
	1.0f/500.0f,			// wheat
	1.0f/500.0f,			// grass
	1.0f/500.0f,			// ????
	1.0f/500.0f,			// night
	1.0f/500.0f,			// pond
	1.0f/500.0f,			// moss
	1.0f/1000.0f,			// wood
	1.0f/1200.0f,			// hive
};

Boolean			gFenceUsesNullShader[NUM_FENCE_SHADERS] =
{
	true,			// thorn
	true,			// wheat
	true,			// grass
	true,			// ????
	true,			// night
	true,			// pond
	false,			// moss
	true,			// wood
	true			// hive
};

static TQ3TriMeshData			gFenceTriMeshData;
static TQ3TriMeshAttributeData	gFenceVertexAttribs[2];
static TQ3TriMeshTriangleData	gFenceTriangles[MAX_NUBS_IN_FENCE*2];
static TQ3Point3D				gFencePoints[MAX_NUBS_IN_FENCE*2];
static TQ3Param2D				gFenceUVs[MAX_NUBS_IN_FENCE*2];
static TQ3ColorRGB				gFenceTransp[MAX_NUBS_IN_FENCE*2];


/******************** INIT FENCE MANAGER ***********************/
//
// Called once at boot
//

void InitFenceManager(void)
{
int	i;
	
	for (i = 0; i < NUM_FENCE_SHADERS; i++)
		gFenceShaderAttribs[i] = nil;

}

/******************** DISPOSE FENCE SHADERS ********************/

void DisposeFenceShaders(void)
{
int	i;
			/* DISPOSE OLD SHADER ATTRIBS */
			
	for (i = 0; i < NUM_FENCE_SHADERS; i++)
	{
		if (gFenceShaderAttribs[i])
		{
			Q3Object_Dispose(gFenceShaderAttribs[i]);
			gFenceShaderAttribs[i] = nil;
		}
	}

}


/********************* PRIME FENCES ***********************/
//
// Called during terrain prime function to initialize 
//

void PrimeFences(void)
{
long					f,i,j,numNubs;
FenceDefType			*fence;
FencePointType			*nubs;

	GAME_ASSERT(gNumFences <= MAX_FENCES);


			/* DISPOSE OLD SHADER ATTRIBS */
			
	DisposeFenceShaders();

	
			/******************************/
			/* ADJUST TO GAME COORDINATES */
			/******************************/
			
	for (f = 0; f < gNumFences; f++)
	{
		gIsFenceVisible[f] = false;							// assume invisible
		fence = &gFenceList[f];								// point to this fence
		HLockHi((Handle)fence->nubList);
		nubs = (*fence->nubList);							// point to nub list
		numNubs = fence->numNubs;							// get # nubs in fence
		
		GAME_ASSERT(numNubs != 1);

		GAME_ASSERT(numNubs <= MAX_NUBS_IN_FENCE);

		for (i = 0; i < numNubs; i++)						// adjust nubs
		{
			nubs[i].x *= MAP2UNIT_VALUE;
			nubs[i].z *= MAP2UNIT_VALUE;			
		}

		fence->bBox.left *= MAP2UNIT_VALUE;					// do bbox
		fence->bBox.right *= MAP2UNIT_VALUE;
		fence->bBox.top *= MAP2UNIT_VALUE;
		fence->bBox.bottom *= MAP2UNIT_VALUE;
		
		
		/* CALCULATE VECTOR FOR EACH SECTION */
		
		fence->sectionVectors = (TQ3Vector2D *)AllocPtr(sizeof(TQ3Vector2D) * (numNubs-1));
		GAME_ASSERT(fence->sectionVectors);

		for (i = 0; i < (numNubs-1); i++)
		{
			float	dx,dz;
			
			dx = nubs[i+1].x - nubs[i].x;
			dz = nubs[i+1].z - nubs[i].z;
			
			FastNormalizeVector2D(dx, dz, &fence->sectionVectors[i]);		
		}
		
	}
	
			/***********************************************************/
			/* LOAD FENCE SHADER TEXTURES & CONVERT INTO ATTRIBUTE SET */
			/***********************************************************/
					
	for (i = 0; i < NUM_FENCE_SHADERS; i++)
	{
		TQ3ShaderObject	shader;
		
		if (gFenceOnThisLevel[gLevelType][i])										// see if this fence exists on this level
		{
			shader = QD3D_GetTextureMap(2000+i, nil, true);							// create shader object
			GAME_ASSERT(shader);

			// Source port addition: clamp texture vertically to avoid ugly line at top
			Q3Shader_SetUBoundary(shader, kQ3ShaderUVBoundaryWrap);
			Q3Shader_SetVBoundary(shader, kQ3ShaderUVBoundaryClamp);

			gFenceShaderAttribs[i] = Q3AttributeSet_New();							// create new attribute set
			GAME_ASSERT(gFenceShaderAttribs[i]);

			Q3AttributeSet_Add(gFenceShaderAttribs[i], kQ3AttributeTypeSurfaceShader, &shader);	// put shader into attrib
	
			Q3Object_Dispose(shader);												// nuke extra ref
		}
		else
		{
			gFenceShaderAttribs[i] = nil;
		}
	}

			/*****************************/
			/* SETUP COMMON TRIMESH INFO */
			/*****************************/
			
	gFenceTriMeshData.triMeshAttributeSet 		= nil;
	gFenceTriMeshData.numEdges 					= 0;
	gFenceTriMeshData.edges 					= nil;
	gFenceTriMeshData.numEdgeAttributeTypes 	= 0;
	gFenceTriMeshData.edgeAttributeTypes 		= 0;	
	gFenceTriMeshData.points 					= &gFencePoints[0];
	gFenceTriMeshData.triangles					= &gFenceTriangles[0];
	gFenceTriMeshData.numTriangleAttributeTypes = 0;
	gFenceTriMeshData.triangleAttributeTypes	= nil;
	gFenceTriMeshData.numVertexAttributeTypes 	= 2;
	gFenceTriMeshData.vertexAttributeTypes		= &gFenceVertexAttribs[0];
	gFenceTriMeshData.bBox.isEmpty 				= kQ3True;

	gFenceVertexAttribs[0].attributeType 		= kQ3AttributeTypeSurfaceUV;
	gFenceVertexAttribs[0].attributeUseArray 	= nil;
	gFenceVertexAttribs[0].data					= &gFenceUVs[0];

	gFenceVertexAttribs[1].attributeType 		= kQ3AttributeTypeTransparencyColor;
	gFenceVertexAttribs[1].attributeUseArray 	= nil;
	gFenceVertexAttribs[1].data					= &gFenceTransp[0];


			/* PREBUILD TRIANGLE INFO */
			
	for (i = j = 0; i < MAX_NUBS_IN_FENCE; i++, j+=2)
	{
		gFenceTriangles[j].pointIndices[0] = 1 + j;
		gFenceTriangles[j].pointIndices[1] = 0 + j;
		gFenceTriangles[j].pointIndices[2] = 3 + j;

		gFenceTriangles[j+1].pointIndices[0] = 3 + j;
		gFenceTriangles[j+1].pointIndices[1] = 0 + j;
		gFenceTriangles[j+1].pointIndices[2] = 2 + j;	
	}
	
}


/********************* DRAW FENCES ***********************/

void DrawFences(const QD3DSetupOutputType *setupInfo)
{
long			f,n,row,col,numNubs,type;
FencePointType	*nubs;
TQ3ViewObject 	view = setupInfo->viewObject;
float			cameraX, cameraZ;
Boolean			isNullShader = false;

			/* SET TAGS */
			
	Q3BackfacingStyle_Submit(kQ3BackfacingStyleBoth, view);


			/* GET CAMERA COORDS */
			
	cameraX = setupInfo->currentCameraCoords.x;
	cameraZ = setupInfo->currentCameraCoords.z;


			/*******************/
			/* DRAW EACH FENCE */
			/*******************/			

	for (f = 0; f < gNumFences; f++)
	{
		type = gFenceList[f].type;							// get type
		
			/* VERIFY THAT THIS FENCE TYPE EXISTS ON THIS LEVEL */
			
		if (!gFenceOnThisLevel[gLevelType][type])
		{
			DoAlert("DrawFences: illegal fence type for this level!");
			ShowSystemErr(gFenceList[f].type);
		}	
	
			/* SEE IF THIS FENCE IS VISIBLE AT ALL */
			
		numNubs = gFenceList[f].numNubs;
		nubs = *gFenceList[f].nubList;
		
		for (n = 0; n < numNubs; n++)
		{
			row = nubs[n].z / TERRAIN_SUPERTILE_UNIT_SIZE;	// calc supertile row,col
			col = nubs[n].x / TERRAIN_SUPERTILE_UNIT_SIZE;
			
			if (gTerrainScrollBuffer[row][col] != EMPTY_SUPERTILE)
				goto drawit;
		}
		gIsFenceVisible[f] = false;
		continue;											// not visible, so skip it
		
				/*********************/
				/* SUBMIT THIS FENCE */
				/*********************/
drawit:	
		gIsFenceVisible[f] = true;
		
				/* SEE IF USE NULL SHADER */
				
		if (gFenceUsesNullShader[type])
		{
			if (!isNullShader)
			{
				Q3Shader_Submit(setupInfo->nullShaderObject, view);		// use null shader
				isNullShader = true;
			}
		}
		else
		{
			if (isNullShader)
			{
				Q3Shader_Submit(setupInfo->shaderObject, view);			// use lambert shader
				isNullShader = false;
			}		
		}
		
				/* SUBMIT GEOMETRY */
				
		SubmitFence(f, view, cameraX, cameraZ);
	}

			/* RESET TAGS */
			
	if (isNullShader)
		Q3Shader_Submit(setupInfo->shaderObject, view);			// restore lambert shader
	Q3BackfacingStyle_Submit(kQ3BackfacingStyleRemove, view);
}


#pragma mark -

/******************** DO FENCE COLLISION **************************/
//
// INPUT:	radiusScale = tweak amount to scale radius by
//

void DoFenceCollision(ObjNode *theNode, float radiusScale)
{
double			fromX,fromZ,toX,toZ;
long			f,numFenceSegments,i,numReScans;
double			segFromX,segFromZ,segToX,segToZ;
FencePointType	*nubs;
Boolean			intersected,letGoOver;
double			intersectX,intersectZ;
TQ3Vector2D		lineNormal;
double			radius;
double			oldX,oldZ,newX,newZ;

	letGoOver = false;

			/* CALC MY MOTION LINE SEGMENT */
			
	oldX = theNode->OldCoord.x;						// from old coord
	oldZ = theNode->OldCoord.z;
	newX = gCoord.x;								// to new coord
	newZ = gCoord.z;
	radius = theNode->BoundingSphere.radius * radiusScale;



			/****************************************/
			/* SCAN THRU ALL FENCES FOR A COLLISION */
			/****************************************/
			
	for (f = 0; f < gNumFences; f++)
	{
		float	temp;
		int		type;
		
		if (!gIsFenceVisible[f])						// dont collide if it isnt visible
			continue;
			

				/* SPECIAL STUFF */
				
		type = gFenceList[f].type;
		switch(type)
		{
			case	FENCE_TYPE_WHEAT:					// things can go over this
			case	FENCE_TYPE_FOREST2:	
					letGoOver = true;
					break;
					
			case	FENCE_TYPE_MOSS:					// this isnt solid
					goto next_fence;
		}
			
			
		/* QUICK CHECK TO SEE IF OLD & NEW COORDS (PLUS RADIUS) ARE OUTSIDE OF FENCE'S BBOX */
#if 1	
		temp = gFenceList[f].bBox.left - radius;
		if ((oldX < temp) && (newX < temp))
			continue;
		temp = gFenceList[f].bBox.right + radius;
		if ((oldX > temp) && (newX > temp))
			continue;
			
		temp = gFenceList[f].bBox.top - radius;
		if ((oldZ < temp) && (newZ < temp))
			continue;
		temp = gFenceList[f].bBox.bottom + radius;
		if ((oldZ > temp) && (newZ > temp))
			continue;
#endif			
			
		nubs = *gFenceList[f].nubList;				// point to nub list
		numFenceSegments = gFenceList[f].numNubs-1;	// get # line segments in fence
		
				/**********************************/
				/* SCAN EACH SECTION OF THE FENCE */
				/**********************************/
			
		numReScans = 0;	
		for (i = 0; i < numFenceSegments; i++)
		{
					/* GET LINE SEG ENDPOINTS */
					
			segFromX = nubs[i].x;
			segFromZ = nubs[i].z;
			segToX = nubs[i+1].x;
			segToZ = nubs[i+1].z;				
	
	
					/* SEE IF ROUGHLY CAN GO OVER */
					
			if (letGoOver)
			{
				float y = GetTerrainHeightAtCoord(segFromX,segFromZ,FLOOR) + gFenceHeight[type];			
				if ((gCoord.y + theNode->BottomOff) >= y)
					continue;
			}
	
	
					/* CALC NORMAL TO THE LINE */
					//
					// We need to find the point on the bounding sphere which is closest to the line
					// in order to do good collision checks
					//
					
			CalcRayNormal2D(&gFenceList[f].sectionVectors[i], segFromX, segFromZ,
							 oldX, oldZ, &lineNormal);
	
	
					/* CALC FROM-TO POINTS OF MOTION */
					
			fromX = oldX - (lineNormal.x * radius);
			fromZ = oldZ - (lineNormal.y * radius);
			toX = newX - (lineNormal.x * radius);
			toZ = newZ - (lineNormal.y * radius);
			
	
					/* SEE IF THE LINES INTERSECT */
					
			intersected = IntersectLineSegments(fromX,  fromZ, toX, toZ,
						                     segFromX, segFromZ, segToX, segToZ,
				                             &intersectX, &intersectZ);
	
			if (intersected)
			{				
						/***************************/
						/* HANDLE THE INTERSECTION */
						/***************************/
						//
						// Move so edge of sphere would be tangent, but also a bit
						// farther so it isnt tangent.
						//
											
				gCoord.x = intersectX + lineNormal.x*radius + (lineNormal.x*2.0);
				gCoord.z = intersectZ + lineNormal.y*radius + (lineNormal.y*2.0);
				
					
						/* BOUNCE OFF WALL */
				
				{
					TQ3Vector2D deltaV;
					
					deltaV.x = gDelta.x;
					deltaV.y = gDelta.z;		
					ReflectVector2D(&deltaV, &lineNormal);
					gDelta.x = deltaV.x * .8f;
					gDelta.z = deltaV.y * .8f;
				}					
				
						/* UPDATE COORD & SCAN AGAIN */
						
				newX = gCoord.x;
				newZ = gCoord.z;
				if (++numReScans < 5)
					i = 0;							// reset segment index to scan all again
			}
			
			/**********************************************/			
			/* NO INTERSECT, DO SAFETY CHECK FOR /\ CASES */
			/**********************************************/
			//
			// The above check may fail when the sphere is going thru
			// the tip of a tee pee /\ intersection, so this is a hack
			// to get around it.
			//			
						
			else
			{
					/* SEE IF EITHER ENDPOINT IS IN SPHERE */
					
				if ((CalcQuickDistance(segFromX, segFromZ, newX, newZ) <= radius) ||
					(CalcQuickDistance(segToX, segToZ, newX, newZ) <= radius))
				{
					TQ3Vector2D deltaV;
					
					gCoord.x = oldX;
					gCoord.z = oldZ;
					
						/* BOUNCE OFF WALL */
				
					deltaV.x = gDelta.x;
					deltaV.y = gDelta.z;		
					ReflectVector2D(&deltaV, &lineNormal);
					gDelta.x = deltaV.x * .8f;
					gDelta.z = deltaV.y * .8f;
					return;
				}
				else
					continue;
			}
		} // for i
next_fence:;
	}
}


/******************** SUBMIT FENCE **************************/
//
// Visibility checks have already been done, so there's a good chance the fence is visible
//

static void SubmitFence(int f, TQ3ViewObject viewObj, float camX, float camZ)
{
u_short					type;
float					u,height;
long					i,numNubs,j;
FenceDefType			*fence;
FencePointType			*nubs;
			
			/* GET FENCE INFO */
			
	fence = &gFenceList[f];								// point to this fence
	nubs = (*fence->nubList);							// point to nub list	
	numNubs = fence->numNubs;							// get # nubs in fence


		/* SET TRIMESH SHADER ATTRIBUTE */
		
	type = fence->type;									// get fence type
	GAME_ASSERT_MESSAGE(type < NUM_FENCE_SHADERS, "illegal fence type");
	gFenceTriMeshData.triMeshAttributeSet = gFenceShaderAttribs[type];
	

		/* SET TRIMESH POINT AND TRI INFO */
			
	gFenceTriMeshData.numPoints = numNubs * 2;				// 2 vertices per nub
	gFenceTriMeshData.numTriangles = (numNubs-1) * 2;			// 2 faces per nub (minus 1st)


		/* SET TRIMESH BBOX X/Z (Y WILL BE COMPUTED LATER) */

	gFenceTriMeshData.bBox.isEmpty = false;
	gFenceTriMeshData.bBox.min.x = gFenceList[f].bBox.left;
	gFenceTriMeshData.bBox.max.x = gFenceList[f].bBox.right;
	gFenceTriMeshData.bBox.min.y = FLT_MAX;
	gFenceTriMeshData.bBox.max.y = -FLT_MAX;
	gFenceTriMeshData.bBox.min.z = gFenceList[f].bBox.top;
	gFenceTriMeshData.bBox.max.z = gFenceList[f].bBox.bottom;


			/************************************/
			/* BUILD POINTS, UV's & TRANSP LIST */
			/************************************/
		
	switch(type)
	{
		case	FENCE_TYPE_MOSS:
				height = -gFenceHeight[type];						// get height						
				break;
						
		default:
				height = gFenceHeight[type];
	}
			
	u = 0;
	
	for (i = j = 0; i < numNubs; i++, j+=2)
	{
		float		x,y,z,y2;
		float		dist;
		
		x = nubs[i].x;
		z = nubs[i].z;
		
		switch(type)
		{
			case	FENCE_TYPE_MOSS:
					y = GetTerrainHeightAtCoord(x, z, CEILING);			
					y += FENCE_SINK_FACTOR;										// sink into ceiling a little bit
					y2 = y + height;
					break;
					
			case	FENCE_TYPE_WOOD:
					y = -400;
					y2 = y + height;
					break;
		
			case	FENCE_TYPE_HIVE:
					y = GetTerrainHeightAtCoord(x, z, FLOOR);			
					y -= FENCE_SINK_FACTOR;	
					y2 = GetTerrainHeightAtCoord(x, z, CEILING);
					y2 += FENCE_SINK_FACTOR;
					break;
		
			default:
					y = GetTerrainHeightAtCoord(x, z, FLOOR);			
					y -= FENCE_SINK_FACTOR;										// sink into ground a little bit
					y2 = y + height;
		}
	
		gFencePoints[j].x = x;
		gFencePoints[j].y = y;
		gFencePoints[j].z = z;
		
		gFencePoints[j+1].x = x;
		gFencePoints[j+1].y = y2;
		gFencePoints[j+1].z = z;		

		// Update mesh bbox y min/max
		if (y  < gFenceTriMeshData.bBox.min.y) gFenceTriMeshData.bBox.min.y = y;
		if (y2 > gFenceTriMeshData.bBox.max.y) gFenceTriMeshData.bBox.max.y = y2;
		
		if (i > 0)
			u += Q3Point3D_Distance(&gFencePoints[j], &gFencePoints[j-2]) * gFenceTextureW[type];
					
		gFenceUVs[j].v 		= 0;									// bottom
		gFenceUVs[j+1].v 	= 1.0;									// top
		gFenceUVs[j].u 		= gFenceUVs[j+1].u = u;
		
		
				/* CALC & SET TRANSPARENCY */
		
		if (gAutoFadeStartDist != 0.0f)								// see if this level has xparency
		{
			dist = CalcQuickDistance(camX, camZ, x, z);				// see if in fade zone
			if (dist < gAutoFadeStartDist)	
				dist = 1.0;
			else
			{
				dist -= gAutoFadeStartDist;							// calc xparency %
				dist = 1.0f - (dist * (1.0f/AUTO_FADE_RANGE));				
				if (dist < 0.0f)
					dist = 0;
			}			
		}
		else
			dist = 1.0f;
				
		gFenceTransp[j].r = 										// set xparency value
		gFenceTransp[j].g = 
		gFenceTransp[j].b =		
		gFenceTransp[j+1].r = 
		gFenceTransp[j+1].g = 
		gFenceTransp[j+1].b = dist;				
	}
	
			
			
		/*******************/
		/* SUBMIT GEOMETRY */
		/*******************/
						
	Q3TriMesh_Submit(&gFenceTriMeshData, viewObj);
}	






