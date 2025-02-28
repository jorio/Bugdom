/****************************/
/*   	EFFECTS.C		    */
/* By Brian Greenstone      */
/* (c)1997-99 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/


static void MoveRipple(ObjNode *theNode);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_PARTICLE_GROUPS		50
#define	MAX_PARTICLES			200		// (note change Byte below if > 255)
#define	NUM_PARTICLE_TEXTURES	8

_Static_assert(MAX_PARTICLES <= 255, "rewrite ParticleGroupType to support > 255 particles");
_Static_assert(MAX_PARTICLE_GROUPS <= 255, "particle group IDs currently assume the group index will fit in 8 bits");

typedef struct
{
	int32_t			magicNum;
	Pool			*pool;
	Byte			type;
	uint8_t			flags;
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
	TQ3TriMeshData	*mesh;
}ParticleGroupType;

static inline ParticleGroupType* GetValidParticleGroup(int32_t groupID);


/*********************/
/*    VARIABLES      */
/*********************/

static ParticleGroupType	gParticleGroups[MAX_PARTICLE_GROUPS];
static bool					gParticleGroupsInitialized = false;
Pool*						gParticleGroupPool = NULL;
static int32_t				gParticleGroupMagicAllocator = 0;

static GLuint				gParticleTextureNames[NUM_PARTICLE_TEXTURES];
static bool					gParticleTexturesLoaded = false;

static float	gGravitoidDistBuffer[MAX_PARTICLES][MAX_PARTICLES];

static RenderModifiers kParticleGroupRenderingMods;


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

	newObj->RenderModifiers.drawOrder = kDrawOrder_Ripples;				// draw ripples after water

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


static void InitParticleGroup(ParticleGroupType* pg)
{
			/* INITIALIZE THE GROUP */

	memset(pg, 0, sizeof(ParticleGroupType));

	pg->pool = Pool_New(MAX_PARTICLES);

			/* INIT THE GROUP'S TRIMESH STRUCTURE */

	pg->mesh = Q3TriMeshData_New(MAX_PARTICLES*2, MAX_PARTICLES*4, kQ3TriMeshDataFeatureVertexUVs | kQ3TriMeshDataFeatureVertexColors);

	pg->mesh->texturingMode = kQ3TexturingModeAlphaBlend;
	pg->mesh->glTextureName = 0;

	pg->mesh->bBox.isEmpty 			= kQ3False;

			/* INIT UV ARRAYS */

	for (int j = 0; j < (MAX_PARTICLES*4); j += 4)
	{
		pg->mesh->vertexUVs[j+0] = (TQ3Param2D) { 0, 1 };			// upper left
		pg->mesh->vertexUVs[j+1] = (TQ3Param2D) { 0, 0 };			// lower left
		pg->mesh->vertexUVs[j+2] = (TQ3Param2D) { 1, 0 };			// lower right
		pg->mesh->vertexUVs[j+3] = (TQ3Param2D) { 1, 1 };			// upper right
	}

			/* INIT TRIANGLE ARRAYS */

	for (int j = 0, k = 0; j < (MAX_PARTICLES*2); j += 2, k += 4)
	{
		pg->mesh->triangles[j].pointIndices[0] = k;				// triangle A
		pg->mesh->triangles[j].pointIndices[1] = k+1;
		pg->mesh->triangles[j].pointIndices[2] = k+2;

		pg->mesh->triangles[j+1].pointIndices[0] = k;				// triangle B
		pg->mesh->triangles[j+1].pointIndices[1] = k+2;
		pg->mesh->triangles[j+1].pointIndices[2] = k+3;
	}
}

/************************ INIT PARTICLE SYSTEM **************************/

void InitParticleSystem(void)
{
			/* INIT GROUP ARRAY */

	if (!gParticleGroupsInitialized)
	{
		for (int i = 0; i < MAX_PARTICLE_GROUPS; i++)
		{
			InitParticleGroup(&gParticleGroups[i]);
		}

		gParticleGroupPool = Pool_New(MAX_PARTICLE_GROUPS);
		gParticleGroupMagicAllocator = 0;
		gParticleGroupsInitialized = true;
	}

			/* LOAD PARTICLE TEXTURES */

	if (!gParticleTexturesLoaded)
	{
		for (int i = 0; i < NUM_PARTICLE_TEXTURES; i++)
		{
			gParticleTextureNames[i] = QD3D_LoadTextureFile(130 + i, kRendererTextureFlags_ClampBoth);
		}

		gParticleTexturesLoaded = true;
	}


	Render_SetDefaultModifiers(&kParticleGroupRenderingMods);
	kParticleGroupRenderingMods.statusBits
			|= STATUS_BIT_NOFOG
			| STATUS_BIT_NOZWRITE
			| STATUS_BIT_NULLSHADER
			| STATUS_BIT_GLOW
			;
	kParticleGroupRenderingMods.drawOrder = kDrawOrder_GlowyParticles;
}


/******************** DELETE ALL PARTICLE GROUPS *********************/

void DeleteAllParticleGroups(void)
{
	if (gParticleGroupsInitialized)
	{
		for (int i = 0; i < MAX_PARTICLE_GROUPS; i++)
		{
			// Free mesh memory
			if (gParticleGroups[i].mesh)
			{
				Q3TriMeshData_Dispose(gParticleGroups[i].mesh);
				gParticleGroups[i].mesh = nil;
			}

			// Free particle pool
			Pool_Free(gParticleGroups[i].pool);
			gParticleGroups[i].pool = NULL;
		}

		// Free particle group pool
		Pool_Free(gParticleGroupPool);
		gParticleGroupPool = NULL;

		gParticleGroupsInitialized = false;
	}

	// Also delete textures
	if (gParticleTexturesLoaded)
	{
		for (int i = 0; i < NUM_PARTICLE_TEXTURES; i++)
		{
			glDeleteTextures(1, &gParticleTextureNames[i]);
			gParticleTextureNames[i] = 0;
		}
		gParticleTexturesLoaded = false;
	}
}


/********************** NEW PARTICLE GROUP *************************/
//
// INPUT:	type 	=	group type to create
//
// OUTPUT:	group ID#
//			Bits 0-7: group number (8 bits)
//			Bits 8-30: magic number (23 bits)
//			Bit 31: reserved for negative values (illegal)

int32_t NewParticleGroup(
		Byte type,
		Byte flags,
		float gravity,
		float magnetism,
		float baseScale,
		float decayRate,
		float fadeRate,
		Byte particleTextureNum)
{
	GAME_ASSERT(gParticleGroupsInitialized);

			/*************************/
			/* SCAN FOR A FREE GROUP */
			/*************************/

	int32_t pgNum = Pool_AllocateIndex(gParticleGroupPool);

	if (pgNum < 0)				// nothing free
		return -1;

	ParticleGroupType* pg = &gParticleGroups[pgNum];


			/* INITIALIZE THE GROUP */

	pg->type = type;
	pg->flags = flags;
	pg->gravity = gravity;
	pg->magnetism = magnetism;
	pg->baseScale = baseScale;
	pg->decayRate = decayRate;
	pg->fadeRate = fadeRate;
	pg->magicNum = gParticleGroupMagicAllocator;
	pg->particleTextureNum = particleTextureNum;

			/* INIT THE GROUP'S TRIMESH STRUCTURE */

	pg->mesh->texturingMode = kQ3TexturingModeAlphaBlend;
	pg->mesh->glTextureName = gParticleTextureNames[particleTextureNum];

			/* MAKE ID */

	int32_t pgID = pgNum | (pg->magicNum << 8);

			/* BUMP MAGIC ALLOCATOR FOR NEXT PGROUP */

	gParticleGroupMagicAllocator++;
	if (gParticleGroupMagicAllocator == 0x7FFFFF)	// max value that fits in 23 bits
	{
		gParticleGroupMagicAllocator = 0;			// wrap around cleanly
	}

			/* DONE */

	return pgID;
}


/******************** ADD PARTICLE TO GROUP **********************/
//
// Returns true if particle group was invalid or is full.
//

bool AddParticleToGroup(int32_t groupID, TQ3Point3D *where, TQ3Vector3D *delta, float scale, float alpha)
{
	GAME_ASSERT(groupID >= 0);

	ParticleGroupType* pg = GetValidParticleGroup(groupID);

	if (!pg)
	{
		return true;
	}


			/* SCAN FOR FREE SLOT */

	int p = Pool_AllocateIndex(pg->pool);

			/* NO FREE SLOTS */

	if (p < 0)
		return true;

			/* INIT PARAMETERS */

	pg->alpha[p] = alpha;
	pg->scale[p] = scale;
	pg->coord[p] = *where;
	pg->delta[p] = *delta;

	return(false);
}


/****************** MOVE PARTICLE GROUPS *********************/

void MoveParticleGroups(void)
{
Byte		flags;
float		fps = gFramesPerSecondFrac;
float		y,baseScale,oneOverBaseScaleSquared,gravity;
float		decayRate,magnetism,fadeRate;
TQ3Point3D	*coord;
TQ3Vector3D	*delta;

	if (!gParticleGroupsInitialized)
		return;

	int g = Pool_First(gParticleGroupPool);
	while (g >= 0)
	{
		GAME_ASSERT(Pool_IsUsed(gParticleGroupPool, g));

		ParticleGroupType* pg = &gParticleGroups[g];

		int nextGroupIndex = Pool_Next(gParticleGroupPool, g);

		baseScale 	= pg->baseScale;					// get base scale
		oneOverBaseScaleSquared = 1.0f/(baseScale*baseScale);
		gravity 	= pg->gravity;						// get gravity
		decayRate 	= pg->decayRate;					// get decay rate
		fadeRate 	= pg->fadeRate;						// get fade rate
		magnetism 	= pg->magnetism;					// get magnetism
		flags 		= pg->flags;

		int n = 0;										// init counter
		int p = Pool_First(pg->pool);
		while (p >= 0)
		{
			GAME_ASSERT(Pool_IsUsed(pg->pool, p));
			int nextParticleIndex = Pool_Next(pg->pool, p);

			n++;										// inc counter
			delta = &pg->delta[p];						// get ptr to deltas
			coord = &pg->coord[p];						// get ptr to coords

							/* ADD GRAVITY */

			delta->y -= gravity * fps;									// add gravity

			switch (pg->type)
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
						for (int q = Pool_Last(pg->pool); q >= 0; q = Pool_Prev(pg->pool, q))
						{
							GAME_ASSERT(Pool_IsUsed(pg->pool, q));

							float		dist;
							float		x,z;
							TQ3Vector3D	v;

							if (p == q)									// don't check against self
								continue;

							x = pg->coord[q].x;
							y = pg->coord[q].y;
							z = pg->coord[q].z;

									/* calc 1/(dist2) */

							if (p < q)									// see if calc or get from buffer
							{
								float dx = coord->x - x;
								float dy = coord->y - y;
								float dz = coord->z - z;
								dist = sqrtf(dx*dx + dy*dy + dz*dz);
								if (dist != 0.0f)
									dist = 1.0f / (dist*dist);

								if (dist > oneOverBaseScaleSquared)		// adjust if closer than radius
									dist = oneOverBaseScaleSquared;

								gGravitoidDistBuffer[p][q] = dist;		// remember it
							}
							else
							{
								dist = gGravitoidDistBuffer[q][p];		// use from buffer
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

			pg->scale[p] -= decayRate * fps;			// shrink it
			if (pg->scale[p] <= 0.0f)					// see if gone
				goto deleteParticle;

					/* DO FADE */

			pg->alpha[p] -= fadeRate * fps;				// fade it
			if (pg->alpha[p] <= 0.0f)					// see if gone
				goto deleteParticle;


			goto nextParticle;

					/* IF GONE RELEASE INDEX */

deleteParticle:
			Pool_ReleaseIndex(pg->pool, p);


nextParticle:
			p = nextParticleIndex;
		}


				/* SEE IF GROUP WAS EMPTY, THEN DELETE */

		if (n == 0)
		{
			Pool_ReleaseIndex(gParticleGroupPool, g);
		}


		g = nextGroupIndex;
	}
}


/**************** DRAW PARTICLE GROUPS *********************/

void DrawParticleGroup(const QD3DSetupOutputType *setupInfo)
{
float			baseScale;
TQ3TriMeshData	*tm;
TQ3Point3D		v[4],*coord;
TQ3Matrix4x4	m;
static const TQ3Vector3D up = {0,1,0};


	(void) setupInfo;

	if (!gParticleGroupsInitialized)
		return;

	const TQ3Point3D* camCoords = &gGameViewInfoPtr->currentCameraCoords;

	for (int g = Pool_First(gParticleGroupPool); g >= 0; g = Pool_Next(gParticleGroupPool, g))
	{
		GAME_ASSERT(Pool_IsUsed(gParticleGroupPool, g));

		float	minX,minY,minZ,maxX,maxY,maxZ;

		ParticleGroupType* pg = &gParticleGroups[g];

		tm = pg->mesh;									// get pointer to trimesh data
		baseScale = pg->baseScale;						// get base scale

					/********************************/
					/* ADD ALL PARTICLES TO TRIMESH */
					/********************************/

		minX = minY = minZ = 1e9f;						// init bbox
		maxX = maxY = maxZ = -minX;

		int numParticlesDrawn = 0;
		for (int p = Pool_First(pg->pool); p >= 0; p = Pool_Next(pg->pool, p))
		{
			GAME_ASSERT(Pool_IsUsed(pg->pool, p));

					/* TRANSFORM PARTICLE POSITION */

			coord = &pg->coord[p];
			SetLookAtMatrixAndTranslate(&m, &up, coord, camCoords);

					/* CULL PARTICLE TO AVOID OVERDRAW (SOURCE PORT ADD) */

			if (!IsSphereInFrustum_XYZ(coord, pg->baseScale))
				continue;

					/* TRANSFORM PARTICLE VERTICES & ADD TO TRIMESH */

			const float S = baseScale * pg->scale[p];
			v[0] = (TQ3Point3D) {  S, S, 0 };
			v[1] = (TQ3Point3D) {  S,-S, 0 };
			v[2] = (TQ3Point3D) { -S,-S, 0 };
			v[3] = (TQ3Point3D) { -S, S, 0 };

			Q3Point3D_To3DTransformArray(&v[0], &m, &tm->points[numParticlesDrawn * 4], 4);

					/* UPDATE BBOX */

			for (int i = 0; i < 4; i++)
			{
				const TQ3Point3D* pt = &tm->points[numParticlesDrawn * 4 + i];
				if (pt->x < minX) minX = pt->x;
				if (pt->x > maxX) maxX = pt->x;
				if (pt->y < minY) minY = pt->y;
				if (pt->y > maxY) maxY = pt->y;
				if (pt->z < minZ) minZ = pt->z;
				if (pt->z > maxZ) maxZ = pt->z;
			}

					/* UPDATE FACE TRANSPARENCY */

			tm->vertexColors[numParticlesDrawn * 4 + 0].a = pg->alpha[p];
			tm->vertexColors[numParticlesDrawn * 4 + 1].a = pg->alpha[p];
			tm->vertexColors[numParticlesDrawn * 4 + 2].a = pg->alpha[p];
			tm->vertexColors[numParticlesDrawn * 4 + 3].a = pg->alpha[p];

			numParticlesDrawn++;										// inc particle count
		}

		if (numParticlesDrawn == 0)										// if no particles, then skip
			continue;

				/* UPDATE FINAL VALUES */

		tm->numTriangles = numParticlesDrawn * 2;
		tm->numPoints = numParticlesDrawn * 4;
		tm->bBox.min.x = minX;
		tm->bBox.min.y = minY;
		tm->bBox.min.z = minZ;
		tm->bBox.max.x = maxX;
		tm->bBox.max.y = maxY;
		tm->bBox.max.z = maxZ;

					/* DRAW IT */

		Render_SubmitMesh(tm, nil, &kParticleGroupRenderingMods, nil);
	}
}

/**************** GET ACTIVE PARTICLE GROUP FROM ID ******************/

static inline ParticleGroupType* GetValidParticleGroup(int32_t groupID)
{
	if (groupID < 0)
		return NULL;

	int32_t group = groupID & 0xFF;
	int32_t magic = groupID >> 8;

	if (group >= MAX_PARTICLE_GROUPS)
		return NULL;

	if (!Pool_IsUsed(gParticleGroupPool, group))
		return NULL;

	if (gParticleGroups[group].magicNum != magic)
		return NULL;

	return &gParticleGroups[group];
}


/**************** VERIFY PARTICLE GROUP MAGIC NUM ******************/

bool VerifyParticleGroup(int32_t groupID)
{
	return NULL != GetValidParticleGroup(groupID);
}


/************* PARTICLE HIT OBJECT *******************/
//
// INPUT:	inFlags = flags to check particle types against
//

bool ParticleHitObject(ObjNode *theNode, uint8_t inFlags)
{
	for (int g = Pool_First(gParticleGroupPool); g >= 0; g = Pool_Next(gParticleGroupPool, g))
	{
		GAME_ASSERT(Pool_IsUsed(gParticleGroupPool, g));

		const ParticleGroupType* pg = &gParticleGroups[g];

		if (inFlags && !(inFlags & pg->flags))				// see if check flags
			continue;

		for (int p = Pool_First(pg->pool); p >= 0; p = Pool_Next(pg->pool, p))
		{
			GAME_ASSERT(Pool_IsUsed(pg->pool, p));

			if (pg->alpha[p] < .4f)							// if particle is too decayed, then skip
				continue;

			const TQ3Point3D* coord = &pg->coord[p];		// get ptr to coords
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
TQ3Vector3D	delta;
TQ3Point3D	pt;

	int n = 30.0f * force;

	pt.y = y;

			/* white sparks */

	int32_t pg = NewParticleGroup(
							PARTICLE_TYPE_FALLINGSPARKS,// type
							0,							// flags
							800,						// gravity
							0,							// magnetism
							40,							// base scale
							-.6,						// decay rate
							.8,							// fade rate
							PARTICLE_TEXTURE_PATCHY);	// texture

	if (pg != -1)
	{
		for (int i = 0; i < n; i++)
		{
			pt.x = x + (RandomFloat()-.5f) * 130.0f * force;
			pt.z = z + (RandomFloat()-.5f) * 130.0f * force;

			delta.x = (RandomFloat()-.5f) * 400.0f * force;
			delta.y = 500.0f + RandomFloat() * 300.0f * force;
			delta.z = (RandomFloat()-.5f) * 400.0f * force;
			AddParticleToGroup(pg, &pt, &delta, RandomFloat() + 1.0f, FULL_ALPHA);
		}
	}


			/* PLAY SPLASH SOUND */

	pt.x = x;
	pt.z = z;
	PlayEffect_Parms3D(EFFECT_SPLASH, &pt, kMiddleC, volume);
}

