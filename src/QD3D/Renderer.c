// RENDERER.C
// (C)2021 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#include <QD3D.h>
#include <stdlib.h>		// qsort


#pragma mark -

/****************************/
/*    PROTOTYPES            */
/****************************/

enum
{
	kRenderPass_Opaque,
	kRenderPass_Transparent
};

typedef struct RendererState
{
	GLuint		boundTexture;
	bool		hasClientState_GL_TEXTURE_COORD_ARRAY;
	bool		hasClientState_GL_VERTEX_ARRAY;
	bool		hasClientState_GL_COLOR_ARRAY;
	bool		hasClientState_GL_NORMAL_ARRAY;
	bool		hasState_GL_CULL_FACE;
	bool		hasState_GL_ALPHA_TEST;
	bool		hasState_GL_DEPTH_TEST;
	bool		hasState_GL_SCISSOR_TEST;
	bool		hasState_GL_COLOR_MATERIAL;
	bool		hasState_GL_TEXTURE_2D;
	bool		hasState_GL_BLEND;
	bool		hasState_GL_LIGHTING;
	bool		hasState_GL_FOG;
	bool		hasFlag_glDepthMask;
	bool		blendFuncIsAdditive;
} RendererState;

typedef struct MeshQueueEntry
{
	int						numMeshes;
	TQ3TriMeshData**		meshPtrList;
	TQ3TriMeshData*			mesh0;
	const TQ3Matrix4x4*		transform;
	const RenderModifiers*	mods;
	float					depthSortZ;
} MeshQueueEntry;

#define MESHQUEUE_MAX_SIZE 4096

static MeshQueueEntry		gMeshQueueBuffer[MESHQUEUE_MAX_SIZE];
static MeshQueueEntry*		gMeshQueuePtrs[MESHQUEUE_MAX_SIZE];
static int					gMeshQueueSize = 0;
static bool					gFrameStarted = false;

static int DepthSortCompare(void const* a_void, void const* b_void);
static void DrawMeshList(int renderPass, const MeshQueueEntry* entry);
static void DrawFadeOverlay(float opacity);

#pragma mark -

/****************************/
/*    CONSTANTS             */
/****************************/

static const RenderModifiers kDefaultRenderMods =
{
	.statusBits = 0,
	.diffuseColor = {1,1,1,1},
};

static const float kFreezeFrameFadeOutDuration = .33f;

//		2----3
//		| \  |
//		|  \ |
//		0----1
static const TQ3Point2D kFullscreenQuadPointsNDC[4] =
{
	{-1.0f, -1.0f},
	{ 1.0f, -1.0f},
	{-1.0f,  1.0f},
	{ 1.0f,  1.0f},
};

static const uint8_t kFullscreenQuadTriangles[2][3] =
{
	{0, 1, 2},
	{1, 3, 2},
};

static const TQ3Param2D kFullscreenQuadUVs[4] =
{
	{0, 1},
	{1, 1},
	{0, 0},
	{1, 0},
};

static const TQ3Param2D kFullscreenQuadUVsFlipped[4] =
{
	{0, 0},
	{1, 0},
	{0, 1},
	{1, 1},
};


#pragma mark -

/****************************/
/*    VARIABLES             */
/****************************/

static RendererState gState;

static PFNGLDRAWRANGEELEMENTSPROC __glDrawRangeElements;

static	GLuint			gCoverWindowTextureName = 0;
static	GLuint			gCoverWindowTextureWidth = 0;
static	GLuint			gCoverWindowTextureHeight = 0;

static	float			gFadeOverlayOpacity = 0;

#pragma mark -

/****************************/
/*    MACROS/HELPERS        */
/****************************/

static void Render_GetGLProcAddresses(void)
{
	__glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)SDL_GL_GetProcAddress("glDrawRangeElements");  // missing link with something...?
	GAME_ASSERT(__glDrawRangeElements);
}

static void __SetInitialState(GLenum stateEnum, bool* stateFlagPtr, bool initialValue)
{
	*stateFlagPtr = initialValue;
	if (initialValue)
		glEnable(stateEnum);
	else
		glDisable(stateEnum);
	CHECK_GL_ERROR();
}

static void __SetInitialClientState(GLenum stateEnum, bool* stateFlagPtr, bool initialValue)
{
	*stateFlagPtr = initialValue;
	if (initialValue)
		glEnableClientState(stateEnum);
	else
		glDisableClientState(stateEnum);
	CHECK_GL_ERROR();
}

static inline void __SetState(GLenum stateEnum, bool* stateFlagPtr, bool enable)
{
	if (enable != *stateFlagPtr)
	{
		if (enable)
			glEnable(stateEnum);
		else
			glDisable(stateEnum);
		*stateFlagPtr = enable;
	}
	else
		gRenderStats.batchedStateChanges++;
}

static inline void __SetClientState(GLenum stateEnum, bool* stateFlagPtr, bool enable)
{
	if (enable != *stateFlagPtr)
	{
		if (enable)
			glEnableClientState(stateEnum);
		else
			glDisableClientState(stateEnum);
		*stateFlagPtr = enable;
	}
	else
		gRenderStats.batchedStateChanges++;
}

#define SetInitialState(stateEnum, initialValue) __SetInitialState(stateEnum, &gState.hasState_##stateEnum, initialValue)
#define SetInitialClientState(stateEnum, initialValue) __SetInitialClientState(stateEnum, &gState.hasClientState_##stateEnum, initialValue)

#define SetState(stateEnum, value) __SetState(stateEnum, &gState.hasState_##stateEnum, (value))

#define EnableState(stateEnum) __SetState(stateEnum, &gState.hasState_##stateEnum, true)
#define EnableClientState(stateEnum) __SetClientState(stateEnum, &gState.hasClientState_##stateEnum, true)

#define DisableState(stateEnum) __SetState(stateEnum, &gState.hasState_##stateEnum, false)
#define DisableClientState(stateEnum) __SetClientState(stateEnum, &gState.hasClientState_##stateEnum, false)

#define RestoreStateFromBackup(stateEnum, backup) __SetState(stateEnum, &gState.hasState_##stateEnum, (backup)->hasState_##stateEnum)
#define RestoreClientStateFromBackup(stateEnum, backup) __SetClientState(stateEnum, &gState.hasClientState_##stateEnum, (backup)->hasClientState_##stateEnum)

#define SetFlag(glFunction, value) do {				\
	if ((value) != gState.hasFlag_##glFunction) {	\
		glFunction((value)? GL_TRUE: GL_FALSE);		\
		gState.hasFlag_##glFunction = (value);		\
	} } while(0)


#pragma mark -

//=======================================================================================================

/****************************/
/*    API IMPLEMENTATION    */
/****************************/

void DoFatalGLError(GLenum error, const char* file, int line)
{
	static char alertbuf[1024];
	snprintf(alertbuf, sizeof(alertbuf), "OpenGL error 0x%x: %s\nin %s:%d",
		error,
		(const char*)gluErrorString(error),
		file,
		line);
	DoFatalAlert(alertbuf);
}

void Render_SetDefaultModifiers(RenderModifiers* dest)
{
	memcpy(dest, &kDefaultRenderMods, sizeof(RenderModifiers));
}

void Render_InitState(void)
{
	// On Windows, proc addresses are only valid for the current context, so we must get fetch everytime we recreate the context.
	Render_GetGLProcAddresses();

	SetInitialClientState(GL_VERTEX_ARRAY,				true);
	SetInitialClientState(GL_NORMAL_ARRAY,				true);
	SetInitialClientState(GL_COLOR_ARRAY,				false);
	SetInitialClientState(GL_TEXTURE_COORD_ARRAY,		true);
	SetInitialState(GL_CULL_FACE,		true);
	SetInitialState(GL_ALPHA_TEST,		true);
	SetInitialState(GL_DEPTH_TEST,		true);
	SetInitialState(GL_SCISSOR_TEST,	false);
	SetInitialState(GL_COLOR_MATERIAL,	true);
	SetInitialState(GL_TEXTURE_2D,		false);
	SetInitialState(GL_BLEND,			false);
	SetInitialState(GL_LIGHTING,		true);
//	SetInitialState(GL_FOG,				true);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gState.blendFuncIsAdditive = false;

	gState.hasFlag_glDepthMask = true;		// initially active on a fresh context

	gState.boundTexture = 0;

	gMeshQueueSize = 0;
	for (int i = 0; i < MESHQUEUE_MAX_SIZE; i++)
		gMeshQueuePtrs[i] = &gMeshQueueBuffer[i];
}

#pragma mark -

void Render_BindTexture(GLuint textureName)
{
	if (gState.boundTexture != textureName)
	{
		glBindTexture(GL_TEXTURE_2D, textureName);
		gState.boundTexture = textureName;
	}
	else
	{
		gRenderStats.batchedStateChanges++;
	}
}

GLuint Render_LoadTexture(
		GLenum internalFormat,
		int width,
		int height,
		GLenum bufferFormat,
		GLenum bufferType,
		const GLvoid* pixels,
		RendererTextureFlags flags)
{
	GLuint textureName;

	glGenTextures(1, &textureName);
	CHECK_GL_ERROR();

	Render_BindTexture(textureName);				// this is now the currently active texture
	CHECK_GL_ERROR();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gGamePrefs.textureFiltering? GL_LINEAR: GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gGamePrefs.textureFiltering? GL_LINEAR: GL_NEAREST);

	if (flags & kRendererTextureFlags_ClampU)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

	if (flags & kRendererTextureFlags_ClampV)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(
			GL_TEXTURE_2D,
			0,						// mipmap level
			internalFormat,			// format in OpenGL
			width,					// width in pixels
			height,					// height in pixels
			0,						// border
			bufferFormat,			// what my format is
			bufferType,				// size of each r,g,b
			pixels);				// pointer to the actual texture pixels
	CHECK_GL_ERROR();

	return textureName;
}

void Render_Load3DMFTextures(TQ3MetaFile* metaFile)
{
	for (int i = 0; i < metaFile->numTextures; i++)
	{
		TQ3Pixmap* textureDef = metaFile->textures[i];
		GAME_ASSERT_MESSAGE(textureDef->glTextureName == 0, "GL texture already allocated");

		TQ3TexturingMode meshTexturingMode = kQ3TexturingModeOff;
		GLenum internalFormat;
		GLenum format;
		GLenum type;
		switch (textureDef->pixelType)
		{
			case kQ3PixelTypeARGB32:
				meshTexturingMode = kQ3TexturingModeAlphaBlend;
				internalFormat = GL_RGBA;
				format = GL_BGRA;
				type = GL_UNSIGNED_INT_8_8_8_8_REV;
				break;
			case kQ3PixelTypeRGB16:
				meshTexturingMode = kQ3TexturingModeOpaque;
				internalFormat = GL_RGB;
				format = GL_BGRA;
				type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
				break;
			case kQ3PixelTypeARGB16:
				meshTexturingMode = kQ3TexturingModeAlphaTest;
				internalFormat = GL_RGBA;
				format = GL_BGRA;
				type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
				break;
			default:
				DoAlert("3DMF texture: Unsupported kQ3PixelType");
				continue;
		}

		textureDef->glTextureName = Render_LoadTexture(
					 internalFormat,						// format in OpenGL
					 textureDef->width,						// width in pixels
					 textureDef->height,					// height in pixels
					 format,								// what my format is
					 type,									// size of each r,g,b
					 textureDef->image,						// pointer to the actual texture pixels
					 0);

		// Set glTextureName on meshes
		for (int j = 0; j < metaFile->numMeshes; j++)
		{
			if (metaFile->meshes[j]->internalTextureID == i)
			{
				metaFile->meshes[j]->glTextureName = textureDef->glTextureName;
				metaFile->meshes[j]->texturingMode = meshTexturingMode;
			}
		}
	}
}

void Render_Dispose3DMFTextures(TQ3MetaFile* metaFile)
{
	for (int i = 0; i < metaFile->numTextures; i++)
	{
		TQ3Pixmap* textureDef = metaFile->textures[i];

		if (textureDef->glTextureName != 0)
		{
			glDeleteTextures(1, &textureDef->glTextureName);
			textureDef->glTextureName = 0;
		}
	}
}

#pragma mark -

void Render_StartFrame(void)
{
	// Clear rendering statistics
	memset(&gRenderStats, 0, sizeof(gRenderStats));

	// Clear transparent queue
	gMeshQueueSize = 0;
	gRenderStats.meshQueueSize = 0;

	// Clear color & depth buffers.
	SetFlag(glDepthMask, true);	// The depth mask must be re-enabled so we can clear the depth buffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	GAME_ASSERT(!gFrameStarted);
	gFrameStarted = true;
}

void Render_SetViewport(bool scissor, int x, int y, int w, int h)
{
	if (scissor)
	{
		EnableState(GL_SCISSOR_TEST);
		glScissor	(x,y,w,h);
		glViewport	(x,y,w,h);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	else
	{
		glViewport	(x,y,w,h);
	}
}

void Render_FlushQueue(void)
{
	GAME_ASSERT(gFrameStarted);

	// Keep track of transparent queue size for debug stats
	gRenderStats.meshQueueSize += gMeshQueueSize;

	// Flush mesh draw queue
	if (gMeshQueueSize == 0)
		return;

	// Sort mesh draw queue, front to back
	qsort(
			gMeshQueuePtrs,
			gMeshQueueSize,
			sizeof(gMeshQueuePtrs[0]),
			DepthSortCompare
	);

	// PASS 1: draw opaque meshes, front to back
	for (int i = 0; i < gMeshQueueSize; i++)
	{
		DrawMeshList(kRenderPass_Opaque, gMeshQueuePtrs[i]);
	}

	// PASS 2: draw transparent meshes, back to front
	for (int i = gMeshQueueSize-1; i >= 0; i--)
	{
		DrawMeshList(kRenderPass_Transparent, gMeshQueuePtrs[i]);
	}

	// Clear mesh draw queue
	gMeshQueueSize = 0;
}

void Render_EndFrame(void)
{
	GAME_ASSERT(gFrameStarted);

	Render_FlushQueue();

	if (gState.hasState_GL_SCISSOR_TEST)
	{
		DisableState(GL_SCISSOR_TEST);
	}

#if ALLOW_FADE
	// Draw fade overlay
	if (gFadeOverlayOpacity > 0.01f)
	{
		DrawFadeOverlay(gFadeOverlayOpacity);
	}
#endif

	gFrameStarted = false;
}

#pragma mark -

static float GetDepthSortZ(
		int						numMeshes,
		TQ3TriMeshData**		meshList,
		const TQ3Point3D*		centerCoord)
{
	TQ3Point3D altCenter;

	if (!centerCoord)
	{
		float mult = (float) numMeshes / 2.0f;
		altCenter = (TQ3Point3D) { 0, 0, 0 };
		for (int i = 0; i < numMeshes; i++)
		{
			altCenter.x += (meshList[i]->bBox.min.x + meshList[i]->bBox.max.x) * mult;
			altCenter.y += (meshList[i]->bBox.min.y + meshList[i]->bBox.max.y) * mult;
			altCenter.z += (meshList[i]->bBox.min.z + meshList[i]->bBox.max.z) * mult;
		}
		centerCoord = &altCenter;
	}

	TQ3Point3D coordInFrustum;
	Q3Point3D_Transform(centerCoord, &gCameraWorldToFrustumMatrix, &coordInFrustum);

	return coordInFrustum.z;
}

void Render_SubmitMeshList(
		int						numMeshes,
		TQ3TriMeshData**		meshList,
		const TQ3Matrix4x4*		transform,
		const RenderModifiers*	mods,
		const TQ3Point3D*		centerCoord)
{
	if (numMeshes <= 0)
		printf("not drawing this!\n");

	GAME_ASSERT(gFrameStarted);
	GAME_ASSERT(gMeshQueueSize < MESHQUEUE_MAX_SIZE);

	MeshQueueEntry* entry = gMeshQueuePtrs[gMeshQueueSize++];
	entry->numMeshes		= numMeshes;
	entry->meshPtrList		= meshList;
	entry->mesh0			= nil;
	entry->transform		= transform;
	entry->mods				= mods ? mods : &kDefaultRenderMods;
	entry->depthSortZ		= GetDepthSortZ(numMeshes, meshList, centerCoord);
}

void Render_SubmitMesh(
		TQ3TriMeshData*			mesh,
		const TQ3Matrix4x4*		transform,
		const RenderModifiers*	mods,
		const TQ3Point3D*		centerCoord)
{
	GAME_ASSERT(gFrameStarted);
	GAME_ASSERT(gMeshQueueSize < MESHQUEUE_MAX_SIZE);

	MeshQueueEntry* entry = gMeshQueuePtrs[gMeshQueueSize++];
	entry->numMeshes		= 1;
	entry->meshPtrList		= &entry->mesh0;
	entry->mesh0			= mesh;
	entry->transform		= transform;
	entry->mods				= mods ? mods : &kDefaultRenderMods;
	entry->depthSortZ		= GetDepthSortZ(1, &mesh, centerCoord);
}

#pragma mark -

static int DepthSortCompare(void const* a_void, void const* b_void)
{
	static const int AFirst		= -1;
	static const int BFirst		= +1;
	static const int DontCare	= 0;

	const MeshQueueEntry* a = *(MeshQueueEntry**) a_void;
	const MeshQueueEntry* b = *(MeshQueueEntry**) b_void;

	// First check manual priority
	if (a->mods->sortPriority < b->mods->sortPriority)
		return AFirst;

	if (a->mods->sortPriority > b->mods->sortPriority)
		return BFirst;

	// Next, if both A and B have the same manual priority, compare their depth coord
	if (a->depthSortZ < b->depthSortZ)		// A is closer to the camera
		return AFirst;

	if (a->depthSortZ > b->depthSortZ)		// B is closer to the camera
		return BFirst;

	return DontCare;
}

static void DrawMeshList(int renderPass, const MeshQueueEntry* entry)
{
	bool applyEnvironmentMap = entry->mods->statusBits & STATUS_BIT_REFLECTIONMAP;

	bool matrixPushedYet = false;

	for (int i = 0; i < entry->numMeshes; i++)
	{
		const TQ3TriMeshData* mesh = entry->meshPtrList[i];

		bool meshIsTransparent = mesh->texturingMode == kQ3TexturingModeAlphaBlend
				|| mesh->diffuseColor.a < .999f
				|| entry->mods->diffuseColor.a < .999f
				|| (entry->mods->statusBits & STATUS_BIT_GLOW)
				;

		// Decide whether or not to draw this mesh in this pass, depending on which pass we're in
		// (opaque or transparent), and whether the mesh has transparency.
		if ((renderPass == kRenderPass_Opaque		&& meshIsTransparent) ||
			(renderPass == kRenderPass_Transparent	&& !meshIsTransparent))
		{
			// Skip this mesh in this pass
			continue;
		}

		// Enable alpha blending if the mesh has transparency
		SetState(GL_BLEND, meshIsTransparent);

		// Set blending function for transparent meshes
		if (meshIsTransparent)
		{
			bool wantAdditive = !!(entry->mods->statusBits & STATUS_BIT_GLOW);
			if (gState.blendFuncIsAdditive != wantAdditive)
			{
				if (wantAdditive)
					glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				else
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				gState.blendFuncIsAdditive = wantAdditive;
			}
		}

		// Enable alpha testing if the mesh's texture calls for it
		SetState(GL_ALPHA_TEST, !meshIsTransparent && mesh->texturingMode == kQ3TexturingModeAlphaTest);

		// Environment map effect
		if (applyEnvironmentMap)
		{
			EnvironmentMapTriMesh(mesh, entry->transform);
		}

		// Cull backfaces or not
		if (entry->mods->statusBits & STATUS_BIT_KEEPBACKFACES)
		{
			if (meshIsTransparent)
			{
				// If we want to keep backfaces on a transparent mesh, keep GL_CULL_FACE enabled,
				// draw the backfaces first, then the frontfaces. This enhances the appearance of
				// e.g. Nanosaur shield spheres, without the need to depth-sort individual faces.
				
				EnableState(GL_CULL_FACE);
				glCullFace(GL_FRONT);		// Pass 1: draw backfaces (cull frontfaces)
				
				// Pass 2 (at the end of the function) will draw the mesh again with backfaces culled.
				// It will restore glCullFace to GL_BACK.
			}
			else
			{
				DisableState(GL_CULL_FACE);		// opaque mesh -- don't cull faces
			}
		}
		else
		{
			EnableState(GL_CULL_FACE);
		}

		// Apply gouraud or null illumination
		SetState(GL_LIGHTING, !(entry->mods->statusBits & STATUS_BIT_NULLSHADER));

		// Write geometry to depth buffer or not
		SetFlag(glDepthMask, !(meshIsTransparent || entry->mods->statusBits & STATUS_BIT_NOZWRITE));

		// Texture mapping
		if (mesh->texturingMode != kQ3TexturingModeOff)
		{
			EnableState(GL_TEXTURE_2D);
			EnableClientState(GL_TEXTURE_COORD_ARRAY);
			Render_BindTexture(mesh->glTextureName);

			glTexCoordPointer(2, GL_FLOAT, 0, applyEnvironmentMap ? gEnvMapUVs : mesh->vertexUVs);
			CHECK_GL_ERROR();
		}
		else
		{
			DisableState(GL_TEXTURE_2D);
			DisableClientState(GL_TEXTURE_COORD_ARRAY);
			CHECK_GL_ERROR();
		}

		// Per-vertex colors (unused in Nanosaur, will be in Bugdom)
		if (mesh->hasVertexColors)
		{
			EnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4, GL_FLOAT, 0, mesh->vertexColors);
		}
		else
		{
			DisableClientState(GL_COLOR_ARRAY);
		}

		// Apply diffuse color for the entire mesh
		glColor4f(
				mesh->diffuseColor.r * entry->mods->diffuseColor.r,
				mesh->diffuseColor.g * entry->mods->diffuseColor.g,
				mesh->diffuseColor.b * entry->mods->diffuseColor.b,
				mesh->diffuseColor.a * entry->mods->diffuseColor.a
				);

		// Submit transformation matrix if any
		if (!matrixPushedYet && entry->transform)
		{
			glPushMatrix();
			glMultMatrixf((float*)entry->transform->value);
			matrixPushedYet = true;
		}

		// Submit vertex and normal data
		glVertexPointer(3, GL_FLOAT, 0, mesh->points);
		glNormalPointer(GL_FLOAT, 0, mesh->vertexNormals);
		CHECK_GL_ERROR();

		// Draw the mesh
		__glDrawRangeElements(GL_TRIANGLES, 0, mesh->numPoints-1, mesh->numTriangles*3, GL_UNSIGNED_SHORT, mesh->triangles);
		CHECK_GL_ERROR();

		/*
		// Axis lines for debugging
		glColor4f(1, 1, 1, 1);
		glBegin(GL_LINES);
		glVertex3i(0,0,0);glVertex3i(0,1,0);
		glVertex3i(0,0,0);glVertex3i(1,0,0);
		glVertex3i(0,0,0);glVertex3i(0,0,1);
		glEnd();
		*/

		// Pass 2 to draw transparent meshes without face culling (see above for an explanation)
		if (meshIsTransparent && entry->mods->statusBits & STATUS_BIT_KEEPBACKFACES)
		{
			glCullFace(GL_BACK);	// pass 2: draw frontfaces (cull backfaces)
			// We've restored glCullFace to GL_BACK, which is the default for all other meshes.
			
			// Draw the mesh again
			__glDrawRangeElements(GL_TRIANGLES, 0, mesh->numPoints - 1, mesh->numTriangles * 3, GL_UNSIGNED_SHORT, mesh->triangles);
			CHECK_GL_ERROR();
		}

		// Update stats
		gRenderStats.trianglesDrawn += mesh->numTriangles;
	}

	if (matrixPushedYet)
	{
		glPopMatrix();
	}
}

#pragma mark -

//=======================================================================================================

/****************************/
/*    2D    */
/****************************/

static void Render_EnterExit2D(bool enter)
{
	static RendererState backup3DState;

	if (enter)
	{
		backup3DState = gState;
		DisableState(GL_LIGHTING);
		DisableState(GL_FOG);
		DisableState(GL_DEPTH_TEST);
		DisableState(GL_ALPHA_TEST);
//		DisableState(GL_TEXTURE_2D);
//		DisableClientState(GL_TEXTURE_COORD_ARRAY);
		DisableClientState(GL_COLOR_ARRAY);
		DisableClientState(GL_NORMAL_ARRAY);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(-1, 1,  -1, 1, 0, 1000);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	}
	else
	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		RestoreStateFromBackup(GL_LIGHTING,		&backup3DState);
		RestoreStateFromBackup(GL_FOG,			&backup3DState);
		RestoreStateFromBackup(GL_DEPTH_TEST,	&backup3DState);
		RestoreStateFromBackup(GL_ALPHA_TEST,	&backup3DState);
//		RestoreStateFromBackup(GL_TEXTURE_2D,	&backup3DState);
//		RestoreClientStateFromBackup(GL_TEXTURE_COORD_ARRAY, &backup3DState);
		RestoreClientStateFromBackup(GL_COLOR_ARRAY,	&backup3DState);
		RestoreClientStateFromBackup(GL_NORMAL_ARRAY,	&backup3DState);
	}
}

void Render_Enter2D(void)
{
	Render_EnterExit2D(true);
}

void Render_Exit2D(void)
{
	Render_EnterExit2D(false);
}

static void Render_Draw2DFullscreenQuad(int fit)
{
	//		2----3
	//		| \  |
	//		|  \ |
	//		0----1
	TQ3Point2D pts[4] = {
			{-1,	-1},
			{ 1,	-1},
			{-1,	 1},
			{ 1,	 1},
	};

	float screenLeft   = 0.0f;
	float screenRight  = (float)gWindowWidth;
	float screenTop    = 0.0f;
	float screenBottom = (float)gWindowHeight;
	bool needClear = false;

	// Adjust screen coordinates if we want to pillarbox/letterbox the image.
	if (fit & (kCoverQuadLetterbox | kCoverQuadPillarbox))
	{
		const float targetAspectRatio = (float) gWindowWidth / gWindowHeight;
		const float sourceAspectRatio = (float) gCoverWindowTextureWidth / gCoverWindowTextureHeight;

		if (fabsf(sourceAspectRatio - targetAspectRatio) < 0.1)
		{
			// source and window have nearly the same aspect ratio -- fit (no-op)
		}
		else if ((fit & kCoverQuadLetterbox) && sourceAspectRatio > targetAspectRatio)
		{
			// source is wider than window -- letterbox
			needClear = true;
			float letterboxedHeight = gWindowWidth / sourceAspectRatio;
			screenTop = (gWindowHeight - letterboxedHeight) / 2;
			screenBottom = screenTop + letterboxedHeight;
		}
		else if ((fit & kCoverQuadPillarbox) && sourceAspectRatio < targetAspectRatio)
		{
			// source is narrower than window -- pillarbox
			needClear = true;
			float pillarboxedWidth = sourceAspectRatio * gWindowWidth / targetAspectRatio;
			screenLeft = (gWindowWidth / 2.0f) - (pillarboxedWidth / 2.0f);
			screenRight = screenLeft + pillarboxedWidth;
		}
	}

	// Compute normalized device coordinates for the quad vertices.
	float ndcLeft   = 2.0f * screenLeft  / gWindowWidth - 1.0f;
	float ndcRight  = 2.0f * screenRight / gWindowWidth - 1.0f;
	float ndcTop    = 1.0f - 2.0f * screenTop    / gWindowHeight;
	float ndcBottom = 1.0f - 2.0f * screenBottom / gWindowHeight;

	pts[0] = (TQ3Point2D) { ndcLeft, ndcBottom };
	pts[1] = (TQ3Point2D) { ndcRight, ndcBottom };
	pts[2] = (TQ3Point2D) { ndcLeft, ndcTop };
	pts[3] = (TQ3Point2D) { ndcRight, ndcTop };


	glColor4f(1, 1, 1, 1);
	EnableState(GL_TEXTURE_2D);
	EnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, pts);
	glTexCoordPointer(2, GL_FLOAT, 0, kFullscreenQuadUVs);
	__glDrawRangeElements(GL_TRIANGLES, 0, 3*2, 3*2, GL_UNSIGNED_BYTE, kFullscreenQuadTriangles);
}

#pragma mark -

//=======================================================================================================

/*******************************************/
/*    BACKDROP/OVERLAY (COVER WINDOW)      */
/*******************************************/

void Render_Alloc2DCover(int width, int height)
{
	GAME_ASSERT_MESSAGE(gCoverWindowTextureName == 0, "cover texture already allocated");

	gCoverWindowTextureWidth = width;
	gCoverWindowTextureHeight = height;

	gCoverWindowTextureName = Render_LoadTexture(
			GL_RGBA,
			width,
			height,
			GL_BGRA,
			GL_UNSIGNED_INT_8_8_8_8,
			gCoverWindowPixPtr,
			kRendererTextureFlags_ClampBoth
	);

	ClearPortDamage();
}

void Render_Dispose2DCover(void)
{
	if (gCoverWindowTextureName == 0)
		return;

	glDeleteTextures(1, &gCoverWindowTextureName);
	gCoverWindowTextureName = 0;
}

void Render_Clear2DCover(UInt32 argb)
{
	UInt32 bgra = Byteswap32(&argb);

	UInt32* backdropPixPtr = gCoverWindowPixPtr;

	for (GLuint i = 0; i < gCoverWindowTextureWidth * gCoverWindowTextureHeight; i++)
	{
		*(backdropPixPtr++) = bgra;
	}

	GrafPtr port;
	GetPort(&port);
	DamagePortRegion(&port->portRect);
}

void Render_Draw2DCover(int fit)
{
	if (gCoverWindowTextureName == 0)
		return;

	Render_BindTexture(gCoverWindowTextureName);

	// If the screen port has dirty pixels ("damaged"), update the texture
	if (IsPortDamaged())
	{
		Rect damageRect;
		GetPortDamageRegion(&damageRect);

		// Set unpack row length to 640
		GLint pUnpackRowLength;
		glGetIntegerv(GL_UNPACK_ROW_LENGTH, &pUnpackRowLength);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, gCoverWindowTextureWidth);

		glTexSubImage2D(
				GL_TEXTURE_2D,
				0,
				damageRect.left,
				damageRect.top,
				damageRect.right - damageRect.left,
				damageRect.bottom - damageRect.top,
				GL_BGRA,
				GL_UNSIGNED_INT_8_8_8_8,
				gCoverWindowPixPtr + (damageRect.top * gCoverWindowTextureWidth + damageRect.left));
		CHECK_GL_ERROR();

		// Restore unpack row length
		glPixelStorei(GL_UNPACK_ROW_LENGTH, pUnpackRowLength);

		ClearPortDamage();
	}

	glViewport(0, 0, gWindowWidth, gWindowHeight);
	Render_Enter2D();
	Render_Draw2DFullscreenQuad(fit);
	Render_Exit2D();
}

static void DrawFadeOverlay(float opacity)
{
	glViewport(0, 0, gWindowWidth, gWindowHeight);
	Render_Enter2D();
	EnableState(GL_BLEND);
	DisableState(GL_TEXTURE_2D);
	DisableClientState(GL_TEXTURE_COORD_ARRAY);
	glColor4f(0, 0, 0, opacity);
	glVertexPointer(2, GL_FLOAT, 0, kFullscreenQuadPointsNDC);
	__glDrawRangeElements(GL_TRIANGLES, 0, 3*2, 3*2, GL_UNSIGNED_BYTE, kFullscreenQuadTriangles);
	Render_Exit2D();
}

#pragma mark -

void Render_SetWindowGamma(float percent)
{
	gFadeOverlayOpacity = (100.0f - percent) / 100.0f;
}

void Render_FreezeFrameFadeOut(void)
{
#if ALLOW_FADE
	//-------------------------------------------------------------------------
	// Capture window contents into texture

	int width4rem = gWindowWidth % 4;
	int width4ceil = gWindowWidth - width4rem + (width4rem == 0? 0: 4);

	GLint textureWidth = width4ceil;
	GLint textureHeight = gWindowHeight;
	char* textureData = NewPtrClear(textureWidth * textureHeight * 3);

	//SDL_GL_SwapWindow(gSDLWindow);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, textureWidth);
	glReadPixels(0, 0, textureWidth, textureHeight, GL_BGR, GL_UNSIGNED_BYTE, textureData);
	CHECK_GL_ERROR();

	GLuint textureName = Render_LoadTexture(
			GL_RGB,
			textureWidth,
			textureHeight,
			GL_BGR,
			GL_UNSIGNED_BYTE,
			textureData,
			kRendererTextureFlags_ClampBoth
			);
	CHECK_GL_ERROR();

	//-------------------------------------------------------------------------
	// Set up 2D viewport

	glViewport(0, 0, gWindowWidth, gWindowHeight);
	Render_Enter2D();
	DisableState(GL_BLEND);
	EnableState(GL_TEXTURE_2D);
	EnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, kFullscreenQuadPointsNDC);
	glTexCoordPointer(2, GL_FLOAT, 0, kFullscreenQuadUVsFlipped);

	//-------------------------------------------------------------------------
	// Fade out

	Uint32 startTicks = SDL_GetTicks();
	Uint32 endTicks = startTicks + kFreezeFrameFadeOutDuration * 1000.0f;

	for (Uint32 ticks = startTicks; ticks <= endTicks; ticks = SDL_GetTicks())
	{
		float gGammaFadePercent = 1.0f - ((ticks - startTicks) / 1000.0f / kFreezeFrameFadeOutDuration);
		if (gGammaFadePercent < 0.0f)
			gGammaFadePercent = 0.0f;

		glColor4f(gGammaFadePercent, gGammaFadePercent, gGammaFadePercent, 1.0f);
		__glDrawRangeElements(GL_TRIANGLES, 0, 3*2, 3*2, GL_UNSIGNED_BYTE, kFullscreenQuadTriangles);
		CHECK_GL_ERROR();
		SDL_GL_SwapWindow(gSDLWindow);
		SDL_Delay(15);
	}

	//-------------------------------------------------------------------------
	// Hold full blackness for a little bit

	startTicks = SDL_GetTicks();
	endTicks = startTicks + .1f * 1000.0f;
	glClearColor(0,0,0,1);
	for (Uint32 ticks = startTicks; ticks <= endTicks; ticks = SDL_GetTicks())
	{
		glClear(GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapWindow(gSDLWindow);
		SDL_Delay(15);
	}

	//-------------------------------------------------------------------------
	// Clean up

	Render_Exit2D();

	DisposePtr(textureData);
	glDeleteTextures(1, &textureName);

	gFadeOverlayOpacity = 1;
#endif
}

#pragma mark -

float Render_GetViewportAspectRatio(Rect paneClip)
{
	int w = gWindowWidth	- paneClip.left	- paneClip.right;
	int h = gWindowHeight	- paneClip.top	- paneClip.bottom;

	if (h == 0)
		return 1;
	else
		return (float)w / (float)h;
}

TQ3Area Render_GetAdjustedViewportRect(Rect paneClip, int logicalWidth, int logicalHeight)
{
	float scaleX = gWindowWidth / (float)logicalWidth;	// scale clip pane to window size
	float scaleY = gWindowHeight / (float)logicalHeight;

	// Floor min to avoid seam at edges of HUD if scale ratio is dirty
	float left = floorf( scaleX * paneClip.left );
	float top = floorf( scaleY * paneClip.top  );
	// Ceil max to avoid seam at edges of HUD if scale ratio is dirty
	float right = ceilf( scaleX * (logicalWidth  - paneClip.right ) );
	float bottom = ceilf( scaleY * (logicalHeight - paneClip.bottom) );


	return (TQ3Area) {{left,top},{right,bottom}};
}
