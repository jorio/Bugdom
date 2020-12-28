/****************************/
/*   	EFFECTS.C		    */
/* (c)1997-99 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	TQ3Point3D			gCoord;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	TQ3Vector3D			gDelta;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	u_short				**gFloorMap;
extern	Byte				gPlayerMode;
extern	TQ3Vector3D		gRecentTerrainNormal[2];
extern	u_short				**gCeilingMap;
extern	Boolean				gTorchPlayer,gPlayerGotKilledFlag;

/****************************/
/*    PROTOTYPES            */
/****************************/


static void MoveRipple(ObjNode *theNode);
static void DeleteParticleGroup(long groupNum);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_PARTICLE_GROUPS		50
#define	MAX_PARTICLES			200		// (note change Byte below if > 255)
#define	NUM_PARTICLE_TEXTURES	8

typedef struct
{
	u_long			magicNum;
	Byte			isUsed[MAX_PARTICLES];
	Byte			type;
	Byte			flags;
	Byte			particleTextureNum;
	float			gravity;
	float			magnetism;
	float			baseScale;
	float			decayRate;			// shrink speed
	float			fadeRate;
	
	float			alpha[MAX_PARTICLES];
	float			scale[MAX_PARTICLES];
	TQ3Point3D		coord[MAX_PARTICLES];
	TQ3Vector3D		delta[MAX_PARTICLES];
	TQ3TriMeshData	triMesh;
}ParticleGroupType;



/*********************/
/*    VARIABLES      */
/*********************/

static ParticleGroupType	*gParticleGroups[MAX_PARTICLE_GROUPS];

static Boolean				gParticleShadersLoaded = false;
static TQ3AttributeSet		gParticleShader[NUM_PARTICLE_TEXTURES];

static float	gGravitoidDistBuffer[MAX_PARTICLES][MAX_PARTICLES];


#pragma mark -


/************************* MAKE RIPPLE *********************************/

ObjNode *MakeRipple(float x, float y, float z, float startScale)
{
ObjNode	*newObj;

	gNewObjectDefinition.group = GLOBAL1_MGroupNum_Ripple;	
	gNewObjectDefinition.type = GLOBAL1_MObjType_Ripple;	
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = y;
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags 	= STATUS_BIT_NOFOG|STATUS_BIT_GLOW|STATUS_BIT_NOZWRITE|STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 	= SLOT_OF_DUMB+2;
	gNewObjectDefinition.moveCall = MoveRipple;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = startScale;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(nil);

	newObj->Health = .8;										// transparency value
	
	MakeObjectTransparent(newObj, newObj->Health);
	
	return(newObj);
}



/******************** MOVE RIPPLE ************************/

static void MoveRipple(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Health -= fps * .8f;
	if (theNode->Health < 0)
	{
		DeleteObject(theNode);
		return;
	}
	
	MakeObjectTransparent(theNode, theNode->Health);

	theNode->Scale.x += fps*6.0f;
	theNode->Scale.y += fps*6.0f;
	theNode->Scale.z += fps*6.0f;

	UpdateObjectTransforms(theNode);
}


//=============================================================================================================
//=============================================================================================================
//=============================================================================================================

#pragma mark -

/************************ INIT PARTICLE SYSTEM **************************/

void InitParticleSystem(void)
{
short	i;

			/* INIT GROUP ARRAY */

	for (i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		gParticleGroups[i] = nil;
	}
	
			/* LOAD PARTICLE TEXTURES */
			
	if (!gParticleShadersLoaded)
	{
		for (i = 0; i < NUM_PARTICLE_TEXTURES; i++)
		{
			TQ3ShaderObject	shader;
		
			shader = QD3D_GetTextureMapFromPICTResource(130+i, false);	// make PICT into shader
			GAME_ASSERT(shader);
			
			gParticleShader[i] = Q3AttributeSet_New();					// new attrib set
			GAME_ASSERT(gParticleShader[i]);

			Q3AttributeSet_Add(gParticleShader[i], kQ3AttributeTypeSurfaceShader, &shader);	// put shader in attrib set
			Q3Object_Dispose(shader);														// nuke extra ref
		}
		
		gParticleShadersLoaded = true;
	}
}


/******************** DELETE ALL PARTICLE GROUPS *********************/

void DeleteAllParticleGroups(void)
{
long	i;

	for (i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		DeleteParticleGroup(i);
	}
}


/******************* DELETE PARTICLE GROUP ***********************/

static void DeleteParticleGroup(long groupNum)
{
TQ3TriMeshData	*tm;

	if (gParticleGroups[groupNum])
	{
			/* NUKE TRIMESH DATA */
			
		tm = &gParticleGroups[groupNum]->triMesh;			// get pointer to trimesh data
			
		DisposePtr((Ptr)tm->triangles);
		DisposePtr((Ptr)tm->points);
		DisposePtr((Ptr)tm->vertexAttributeTypes[0].data);
		DisposePtr((Ptr)tm->vertexAttributeTypes);
		DisposePtr((Ptr)tm->triangleAttributeTypes[0].data);
		DisposePtr((Ptr)tm->triangleAttributeTypes);

				/* NUKE GROUP ITSELF */
					
		DisposePtr((Ptr)gParticleGroups[groupNum]);
		gParticleGroups[groupNum] = nil;
	}
}


/********************** NEW PARTICLE GROUP *************************/
//
// INPUT:	type 	=	group type to create
//
// OUTPUT:	group ID#
//

short NewParticleGroup(u_long magicNum, Byte type, Byte flags, float gravity, float magnetism,
					 float baseScale, float decayRate, float fadeRate, Byte particleTextureNum)
{
short					p,i,j,k;
TQ3TriMeshData			*tm;
TQ3TriMeshTriangleData	*t;
TQ3Param2D				*uv;

			/*************************/
			/* SCAN FOR A FREE GROUP */
			/*************************/
			
	for (i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		if (gParticleGroups[i] == nil)
		{
				/* ALLOCATE NEW GROUP */
				
			gParticleGroups[i] = (ParticleGroupType *)AllocPtr(sizeof(ParticleGroupType));
			if (gParticleGroups[i] == nil)
				return(-1);									// out of memory

			
				/* INITIALIZE THE GROUP */
			
			gParticleGroups[i]->type = type;						// set type
			for (p = 0; p < MAX_PARTICLES; p++)						// mark all unused
				gParticleGroups[i]->isUsed[p] = false;
			
			gParticleGroups[i]->flags = flags;
			gParticleGroups[i]->gravity = gravity;
			gParticleGroups[i]->magnetism = magnetism;
			gParticleGroups[i]->baseScale = baseScale;
			gParticleGroups[i]->decayRate = decayRate;
			gParticleGroups[i]->fadeRate = fadeRate;
			gParticleGroups[i]->magicNum = magicNum;
			gParticleGroups[i]->particleTextureNum = particleTextureNum;
			
			
				/* INIT THE GROUP'S TRIMESH STRUCTURE */
			
			tm = &gParticleGroups[i]->triMesh;						// get pointer to trimesh data
			
			tm->triMeshAttributeSet 	= gParticleShader[particleTextureNum];
			tm->triangles 				= (TQ3TriMeshTriangleData *)AllocPtr(sizeof(TQ3TriMeshTriangleData) * MAX_PARTICLES * 2);
			tm->numTriangleAttributeTypes = 1;
			tm->numEdges 				= 0;
			tm->edges 					= nil;
			tm->numEdgeAttributeTypes	= 0;
			tm->edgeAttributeTypes 		= 0;
			tm->points 					= (TQ3Point3D *)AllocPtr(sizeof(TQ3Point3D) * MAX_PARTICLES * 4);
			tm->numVertexAttributeTypes = 1;

			tm->vertexAttributeTypes						= (TQ3TriMeshAttributeData *)AllocPtr(sizeof(TQ3TriMeshAttributeData) * tm->numVertexAttributeTypes);
			tm->vertexAttributeTypes[0].attributeType 		= kQ3AttributeTypeSurfaceUV;
			tm->vertexAttributeTypes[0].attributeUseArray 	= nil;
			tm->vertexAttributeTypes[0].data 				= (TQ3Param2D *)AllocPtr(sizeof(TQ3Param2D) * MAX_PARTICLES * 4);
			GAME_ASSERT(tm->vertexAttributeTypes[0].data);

			tm->triangleAttributeTypes						= (TQ3TriMeshAttributeData *)AllocPtr(sizeof(TQ3TriMeshAttributeData) * tm->numTriangleAttributeTypes);
			tm->triangleAttributeTypes[0].attributeType 	= kQ3AttributeTypeTransparencyColor;
			tm->triangleAttributeTypes[0].attributeUseArray = nil;
			tm->triangleAttributeTypes[0].data 				= (TQ3ColorRGB *)AllocPtr(sizeof(TQ3ColorRGB) * MAX_PARTICLES * 2);
			GAME_ASSERT(tm->triangleAttributeTypes[0].data);

			tm->bBox.isEmpty 			= kQ3False;

			
					/* INIT UV ARRAYS */
					
			uv = (TQ3Param2D *)tm->vertexAttributeTypes[0].data;
			for (j=0; j < (MAX_PARTICLES*4); j+=4)
			{
				uv[j].u = 0;									// upper left
				uv[j].v = 1;
				uv[j+1].u = 0;									// lower left
				uv[j+1].v = 0;	
				uv[j+2].u = 1;									// lower right
				uv[j+2].v = 0;	
				uv[j+3].u = 1;									// upper right
				uv[j+3].v = 1;				
			}
						
					/* INIT TRIANGLE ARRAYS */
					
			t = tm->triangles;
			for (j = k = 0; j < (MAX_PARTICLES*2); j+=2, k+=4)
			{
				t[j].pointIndices[0] = k;							// triangle A
				t[j].pointIndices[1] = k+1;
				t[j].pointIndices[2] = k+2;

				t[j+1].pointIndices[0] = k;							// triangle B
				t[j+1].pointIndices[1] = k+2;
				t[j+1].pointIndices[2] = k+3;			
			}
			
			return(i);
		}
	}
	
			/* NOTHING FREE */
			
//	DoFatalAlert("NewParticleGroup: no free groups!");
	return(-1);	
}


/******************** ADD PARTICLE TO GROUP **********************/
//
// Returns true if particle group was invalid or is full.
//

Boolean AddParticleToGroup(short groupNum, TQ3Point3D *where, TQ3Vector3D *delta, float scale, float alpha)
{
short	p;

	GAME_ASSERT((groupNum >= 0) && (groupNum < MAX_PARTICLE_GROUPS));

	if (gParticleGroups[groupNum] == nil)
	{
		return(true);
//		DoAlert("AddParticleToGroup: this group is nil");
//		DebugStr("debug me!");
	}


			/* SCAN FOR FREE SLOT */
			
	for (p = 0; p < MAX_PARTICLES; p++)
	{
		if (!gParticleGroups[groupNum]->isUsed[p])
			goto got_it;
	}
	
			/* NO FREE SLOTS */
			
	return(true);


			/* INIT PARAMETERS */
got_it:
	gParticleGroups[groupNum]->alpha[p] = alpha;
	gParticleGroups[groupNum]->scale[p] = scale;
	gParticleGroups[groupNum]->coord[p] = *where;
	gParticleGroups[groupNum]->delta[p] = *delta;	
	gParticleGroups[groupNum]->isUsed[p] = true;
	
	return(false);
}


/****************** MOVE PARTICLE GROUPS *********************/

void MoveParticleGroups(void)
{
Byte		flags;
long		i,n,p,j;
float		fps = gFramesPerSecondFrac;
float		y,baseScale,oneOverBaseScaleSquared,gravity;
float		decayRate,magnetism,fadeRate;
TQ3Point3D	*coord;
TQ3Vector3D	*delta;

	for (i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		if (gParticleGroups[i])
		{
			baseScale 	= gParticleGroups[i]->baseScale;					// get base scale
			oneOverBaseScaleSquared = 1.0f/(baseScale*baseScale);
			gravity 	= gParticleGroups[i]->gravity;						// get gravity
			decayRate 	= gParticleGroups[i]->decayRate;					// get decay rate
			fadeRate 	= gParticleGroups[i]->fadeRate;						// get fade rate
			magnetism 	= gParticleGroups[i]->magnetism;					// get magnetism
			flags 		= gParticleGroups[i]->flags;
			
			n = 0;															// init counter
			for (p = 0; p < MAX_PARTICLES; p++)
			{
				if (!gParticleGroups[i]->isUsed[p])							// make sure this particle is used
					continue;
				
				n++;														// inc counter
				delta = &gParticleGroups[i]->delta[p];						// get ptr to deltas
				coord = &gParticleGroups[i]->coord[p];						// get ptr to coords
				
							/* ADD GRAVITY */
							
				delta->y -= gravity * fps;									// add gravity
				
				
				
				switch(gParticleGroups[i]->type)
				{
							/* FALLING SPARKS */
							
					case	PARTICLE_TYPE_FALLINGSPARKS:
							coord->x += delta->x * fps;						// move it
							coord->y += delta->y * fps;
							coord->z += delta->z * fps;
							break;


							/* GRAVITOIDS */
							//
							// Every particle has gravity pull on other particle
							//
							
					case	PARTICLE_TYPE_GRAVITOIDS:
							for (j = MAX_PARTICLES-1; j >= 0; j--)
							{
								float		dist,temp,x,z;
								TQ3Vector3D	v;
								
								if (p==j)									// dont check against self
									continue;
								if (!gParticleGroups[i]->isUsed[j])			// make sure this particle is used
									continue;
															
								x = gParticleGroups[i]->coord[j].x;
								y = gParticleGroups[i]->coord[j].y;
								z = gParticleGroups[i]->coord[j].z;
								
										/* calc 1/(dist2) */
							
								if (i < j)									// see if calc or get from buffer
								{
									temp = coord->x - x;					// dx squared
									dist = temp*temp;
									temp = coord->y - y;					// dy squared
									dist += temp*temp;
									temp = coord->z - z;					// dz squared
									dist += temp*temp;
									
									dist = sqrt(dist);						// 1/dist2
									if (dist != 0.0f)
										dist = 1.0f / (dist*dist);  

									if (dist > oneOverBaseScaleSquared)		// adjust if closer than radius
										dist = oneOverBaseScaleSquared;

									gGravitoidDistBuffer[i][j] = dist;		// remember it
								}
								else
								{
									dist = gGravitoidDistBuffer[j][i];		// use from buffer
								}
								
											/* calc vector to particle */
											
								if (dist != 0.0f)
								{
									x = x - coord->x;
									y = y - coord->y;
									z = z - coord->z;
									FastNormalizeVector(x, y, z, &v);
								}
								else
								{
									v.x = v.y = v.z = 0;
								}
																
								delta->x += v.x * (dist * magnetism * fps);		// apply gravity to particle
								delta->y += v.y * (dist * magnetism * fps);
								delta->z += v.z * (dist * magnetism * fps);
							}
														
							coord->x += delta->x * fps;						// move it
							coord->y += delta->y * fps;
							coord->z += delta->z * fps;
							break;							
				}
				
				
				if (gFloorMap)					// only do these checks if there's a terrain floor
				{
					/*****************/
					/* SEE IF BOUNCE */
					/*****************/
					
					if (flags & PARTICLE_FLAGS_BOUNCE)
					{
						if (delta->y < 0.0f)							// if moving down, see if hit floor
						{
							y = GetTerrainHeightAtCoord(coord->x, coord->z, FLOOR)+10.0f;	// see if hit floor
							if (coord->y < y)
							{
								coord->y = y;
								delta->y *= -.4f;
								
								delta->x += gRecentTerrainNormal[FLOOR].x * 300.0f;	// reflect off of surface
								delta->z += gRecentTerrainNormal[FLOOR].z * 300.0f;
							}
						}
					}
					
					
					/**********************/
					/* SEE IF HURT PLAYER */
					/**********************/
					
					if (flags & PARTICLE_FLAGS_HURTPLAYER)
					{					
						if (DoSimpleBoxCollisionAgainstPlayer(coord->y+30.0f,coord->y-30.0f,
															coord->x-30.0f, coord->x+30.0f,
															coord->z+30.0f, coord->z-30.0f))
						{
							if (flags & PARTICLE_FLAGS_HURTPLAYERBAD)					// hurt really bad!
							{
								PlayerGotHurt(nil, 1.0, false, false, false,.5);		// hurt enough to kill!
								if (gPlayerGotKilledFlag)
									gTorchPlayer = true;
							}
							else														// normal hurt
							{
								if (gPlayerMode == PLAYER_MODE_BALL)					// ball gets hurt less
									PlayerGotHurt(nil, .1, false, false, false,1.2);	
								else
									PlayerGotHurt(nil, .15, false, false, false,1.2);	
							}
						}
					}
				}
				
				if (gCeilingMap)
				{
						/* SEE IF HIT CEILING */
					
					if (flags & PARTICLE_FLAGS_ROOF)
					{
						if (delta->y > 0.0f)							// if moving up, see if hit ceiling
						{
							y = GetTerrainHeightAtCoord(coord->x, coord->z, CEILING)-10.0f;	// see if hit ceiling
							if (coord->y > y)
							{
								coord->y = y;
								delta->x += gRecentTerrainNormal[FLOOR].x * 1000.0f;	// reflect off of surface
								delta->z += gRecentTerrainNormal[FLOOR].z * 1000.0f;
							}
						}
					}
				}
				
				/***************/
				/* SEE IF GONE */
				/***************/
				
					/* DO SCALE */
					
				gParticleGroups[i]->scale[p] -= decayRate * fps;			// shrink it							
				if (gParticleGroups[i]->scale[p] <= 0.0f)					// see if gone
					gParticleGroups[i]->isUsed[p] = false;				

					/* DO FADE */
					
				gParticleGroups[i]->alpha[p] -= fadeRate * fps;				// fade it							
				if (gParticleGroups[i]->alpha[p] <= 0.0f)					// see if gone
					gParticleGroups[i]->isUsed[p] = false;				
					
					
			}
			
				/* SEE IF GROUP WAS EMPTY, THEN DELETE */
					
			if (n == 0)
			{
				DeleteParticleGroup(i);
			}
		}
	}
}


/**************** DRAW PARTICLE GROUPS *********************/

void DrawParticleGroup(const QD3DSetupOutputType *setupInfo)
{
float			scale,baseScale;
long			g,p,n,i;
TQ3ColorRGB		*faceColor;
TQ3TriMeshData	*tm;
TQ3Point3D		v[4],*camCoords,*coord;
TQ3Matrix4x4	m;
static const TQ3Vector3D up = {0,1,0};
TQ3ViewObject	view = setupInfo->viewObject;


				/* SETUP ENVIRONTMENT */
				
	QD3D_DisableFog(setupInfo);
	Q3BackfacingStyle_Submit(kQ3BackfacingStyleBoth, view);
	Q3Shader_Submit(setupInfo->nullShaderObject, view);							// use null shader
	QD3D_SetZWrite(false);
	QD3D_SetAdditiveBlending(true);
	QD3D_SetTriangleCacheMode(false);


	camCoords = &gGameViewInfoPtr->currentCameraCoords;	

	for (g = 0; g < MAX_PARTICLE_GROUPS; g++)
	{
		float	minX,minY,minZ,maxX,maxY,maxZ;
		int		temp;

		if (gParticleGroups[g])
		{
			
			tm = &gParticleGroups[g]->triMesh;								// get pointer to trimesh data
			faceColor = (TQ3ColorRGB *)tm->triangleAttributeTypes[0].data;	// get point to face transp attribute array
			baseScale = gParticleGroups[g]->baseScale;						// get base scale

					/********************************/
					/* ADD ALL PARTICLES TO TRIMESH */
					/********************************/
					
			minX = minY = minZ = 100000000;									// init bbox
			maxX = maxY = maxZ = -minX;
								
			for (p = n = 0; p < MAX_PARTICLES; p++)
			{
				if (!gParticleGroups[g]->isUsed[p])							// make sure this particle is used
					continue;
				
					/* TRANSFORM PARTICLE POSITION */
					
				coord = &gParticleGroups[g]->coord[p];
				SetLookAtMatrixAndTranslate(&m, &up, coord, camCoords);

					/* CULL PARTICLE TO AVOID OVERDRAW (SOURCE PORT ADD) */

				if (!IsSphereInFrustum_XYZ(coord, 0))						// radius 0: cull somewhat aggressively
				{															// (use negative radius to cull even more)
					continue;
				}

					/* TRANSFORM PARTICLE VERTICES & ADD TO TRIMESH */

				scale = gParticleGroups[g]->scale[p];
				
				v[0].x = -scale*baseScale;
				v[0].y = scale*baseScale;
				v[0].z = 0;

				v[1].x = -scale*baseScale;
				v[1].y = -scale*baseScale;
				v[1].z = 0;

				v[2].x = scale*baseScale;
				v[2].y = -scale*baseScale;
				v[2].z = 0;

				v[3].x = scale*baseScale;
				v[3].y = scale*baseScale;
				v[3].z = 0;

				Q3Point3D_To3DTransformArray(&v[0], &m, &tm->points[n*4], 4, sizeof(TQ3Point3D), sizeof(TQ3Point3D));

					/* UPDATE BBOX */
						
				for (i = 0; i < 4; i++)
				{			
					if (tm->points[n*4+i].x < minX)
						minX = tm->points[n*4+i].x;
					if (tm->points[n*4+i].x > maxX)
						maxX = tm->points[n*4+i].x;
					if (tm->points[n*4+i].y < minY)
						minY = tm->points[n*4+i].y;
					if (tm->points[n*4+i].y > maxY)
						maxY = tm->points[n*4+i].y;
					if (tm->points[n*4+i].z < minZ)
						minZ = tm->points[n*4+i].z;
					if (tm->points[n*4+i].z > maxZ)
						maxZ = tm->points[n*4+i].z;
				}
				
					/* UPDATE FACE TRANSPARENCY */
								
				temp = n*2;		
				faceColor[temp].r = faceColor[temp].g = faceColor[temp].b = gParticleGroups[g]->alpha[p];		// set transparency alpha tri A
				temp++;
				faceColor[temp].r = faceColor[temp].g = faceColor[temp].b = gParticleGroups[g]->alpha[p];		// set transparency alpha tri B

				n++;											// inc particle count
			}
	
			if (n == 0)											// if no particles, then skip
				continue;
	
				/* UPDATE FINAL VALUES */
						
			tm->numTriangles = n*2;
			tm->numPoints = n*4;
			tm->bBox.min.x = minX;
			tm->bBox.min.y = minY;
			tm->bBox.min.z = minZ;
			tm->bBox.max.x = maxX;
			tm->bBox.max.y = maxY;
			tm->bBox.max.z = maxZ;

					/* DRAW IT */
		
			if (Q3TriMesh_Submit(tm, view) == kQ3Failure)
				DoFatalAlert("DrawParticleGroup: Q3TriMesh_Submit failed!");
		}
	}
	
			/* RESTORE MODES */
			
	Q3Shader_Submit(setupInfo->shaderObject, view);								// restore shader
	QD3D_SetZWrite(true);
	QD3D_SetTriangleCacheMode(true);
	QD3D_SetAdditiveBlending(false);
	QD3D_ReEnableFog(setupInfo);
	Q3BackfacingStyle_Submit(kQ3BackfacingStyleRemove, view);
}

/**************** VERIFY PARTICLE GROUP MAGIC NUM ******************/

Boolean VerifyParticleGroupMagicNum(short group, u_long magicNum)
{
	if (gParticleGroups[group] == nil)
		return(false);

	if (gParticleGroups[group]->magicNum != magicNum)
		return(false);

	return(true);
}


/************* PARTICLE HIT OBJECT *******************/
//
// INPUT:	inFlags = flags to check particle types against
//

Boolean ParticleHitObject(ObjNode *theNode, u_short inFlags)
{
int		i,p;
u_short	flags;
TQ3Point3D	*coord;

	for (i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		if (!gParticleGroups[i])									// see if group active
			continue;
			
		if (inFlags)												// see if check flags
		{
			flags = gParticleGroups[i]->flags;
			if (!(inFlags & flags))
				continue;
		}
		
		for (p = 0; p < MAX_PARTICLES; p++)
		{
			if (!gParticleGroups[i]->isUsed[p])						// make sure this particle is used
				continue;
				
			if (gParticleGroups[i]->alpha[p] < .4f)				// if particle is too decayed, then skip
				continue;			 
			
			coord = &gParticleGroups[i]->coord[p];					// get ptr to coords					
			if (DoSimpleBoxCollisionAgainstObject(coord->y+40.0f,coord->y-40.0f,
												coord->x-40.0f, coord->x+40.0f,
												coord->z+40.0f, coord->z-40.0f,
												theNode))
			{
				return(true);
			}
		}
	}
	return(false);
}

#pragma mark -

/********************* MAKE SPLASH ***********************/

void MakeSplash(float x, float y, float z, float force, float volume)
{
long	pg,i,n;
TQ3Vector3D	delta;
TQ3Point3D	pt;

	n = 30.0f * force;

	pt.y = y;

			/* white sparks */
				
	pg = NewParticleGroup(	0,							// magic num
							PARTICLE_TYPE_FALLINGSPARKS,// type
							0,							// flags
							800,						// gravity
							0,							// magnetism
							40,							// base scale
							-.6,						// decay rate
							.8,							// fade rate
							PARTICLE_TEXTURE_PATCHY);	// texture
	
	for (i = 0; i < n; i++)
	{
		pt.x = x + (RandomFloat()-.5f) * 130.0f * force;
		pt.z = z + (RandomFloat()-.5f) * 130.0f * force;

		delta.x = (RandomFloat()-.5f) * 400.0f * force;
		delta.y = 500.0f + RandomFloat() * 300.0f * force;
		delta.z = (RandomFloat()-.5f) * 400.0f * force;
		AddParticleToGroup(pg, &pt, &delta, RandomFloat() + 1.0f, FULL_ALPHA);
	}


			/* PLAY SPLASH SOUND */

	pt.x = x;
	pt.z = z;			
	PlayEffect_Parms3D(EFFECT_SPLASH, &pt, kMiddleC, volume);
}










