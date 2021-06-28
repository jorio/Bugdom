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
#include <stdio.h>


#pragma mark -

/****************************/
/*    PROTOTYPES            */
/****************************/

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
	bool		wantFog;
	const TQ3Matrix4x4*	currentTransform;
} RendererState;

typedef struct MeshQueueEntry
{
	const TQ3TriMeshData*	mesh;
	const TQ3Matrix4x4*		transform;	// may be NULL
	const RenderModifiers*	mods;		// may be NULL
	float					depth;		// used to determine draw order
	bool					meshIsTransparent;
} MeshQueueEntry;

#define MESHQUEUE_MAX_SIZE 4096

static MeshQueueEntry		gMeshQueueBuffer[MESHQUEUE_MAX_SIZE];
static MeshQueueEntry*		gMeshQueuePtrs[MESHQUEUE_MAX_SIZE];
static int					gMeshQueueSize = 0;
static bool					gFrameStarted = false;

static float				gBackupVertexColors[4*65536];

static int DrawOrderComparator(void const* a_void, void const* b_void);

static void DrawMesh(const MeshQueueEntry* entry, bool (*preDrawFunc)(const MeshQueueEntry* entry));
static bool PreDrawMesh_DepthPass(const MeshQueueEntry* entry);
static bool PreDrawMesh_ColorPass(const MeshQueueEntry* entry);


#pragma mark -

/****************************/
/*    CONSTANTS             */
/****************************/

const TQ3Point3D kQ3Point3D_Zero = {0, 0, 0};

static const RenderModifiers kDefaultRenderMods =
{
	.statusBits = 0,
	.diffuseColor = {1,1,1,1},
	.autoFadeFactor = 1.0f,
	.drawOrder = 0,
};

const RenderModifiers kDefaultRenderMods_UI =
{
	.statusBits = STATUS_BIT_NULLSHADER | STATUS_BIT_NOFOG | STATUS_BIT_NOZWRITE,
	.diffuseColor = {1,1,1,1},
	.autoFadeFactor = 1.0f,
	.drawOrder = kDrawOrder_UI
};

static const RenderModifiers kDefaultRenderMods_FadeOverlay =
{
	.statusBits = STATUS_BIT_NULLSHADER | STATUS_BIT_NOFOG | STATUS_BIT_NOZWRITE,
	.diffuseColor = {1,1,1,1},
	.autoFadeFactor = 1.0f,
	.drawOrder = kDrawOrder_FadeOverlay
};

const RenderModifiers kDefaultRenderMods_DebugUI =
{
	.statusBits = STATUS_BIT_NULLSHADER | STATUS_BIT_NOFOG | STATUS_BIT_NOZWRITE | STATUS_BIT_KEEPBACKFACES | STATUS_BIT_DONTCULL,
	.diffuseColor = {1,1,1,1},
	.autoFadeFactor = 1.0f,
	.drawOrder = kDrawOrder_DebugUI
};

#pragma mark -

/****************************/
/*    VARIABLES             */
/****************************/

static RendererState gState;

static PFNGLDRAWRANGEELEMENTSPROC __glDrawRangeElements;

float gGammaFadeFactor = 1.0f;

static TQ3TriMeshData* gFullscreenQuad = nil;

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

#if _DEBUG
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
#endif

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
	SetInitialState(GL_FOG,				false);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gState.blendFuncIsAdditive = false;

	gState.hasFlag_glDepthMask = true;		// initially active on a fresh context

	gState.boundTexture = 0;
	gState.wantFog = false;
	gState.currentTransform = NULL;

	gMeshQueueSize = 0;
	for (int i = 0; i < MESHQUEUE_MAX_SIZE; i++)
		gMeshQueuePtrs[i] = &gMeshQueueBuffer[i];

	if (!gFullscreenQuad)
		gFullscreenQuad = MakeQuadMesh_UI(0, 0, 640, 480, 0, 0, 1, 1);
}

void Render_Shutdown(void)
{
	if (gFullscreenQuad)
	{
		Q3TriMeshData_Dispose(gFullscreenQuad);
		gFullscreenQuad = NULL;
	}
}

void Render_EnableFog(
		float camHither,
		float camYon,
		float fogHither,
		float fogYon,
		TQ3ColorRGBA fogColor)
{
	glHint(GL_FOG_HINT,		GL_NICEST);
	glFogi(GL_FOG_MODE,		GL_LINEAR);
	glFogf(GL_FOG_START,	fogHither * camYon);
	glFogf(GL_FOG_END,		fogYon * camYon);
	glFogfv(GL_FOG_COLOR,	(float *)&fogColor);
	gState.wantFog = true;
}

void Render_DisableFog(void)
{
	gState.wantFog = false;
}

#pragma mark -

void Render_BindTexture(GLuint textureName)
{
	if (gState.boundTexture != textureName)
	{
		glBindTexture(GL_TEXTURE_2D, textureName);
		gState.boundTexture = textureName;
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
	GAME_ASSERT(gGLContext);

	GLuint textureName;

	glGenTextures(1, &textureName);
	CHECK_GL_ERROR();

	Render_BindTexture(textureName);				// this is now the currently active texture
	CHECK_GL_ERROR();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, !gGamePrefs.lowDetail? GL_LINEAR: GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, !gGamePrefs.lowDetail? GL_LINEAR: GL_NEAREST);

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

void Render_UpdateTexture(
		GLuint textureName,
		int x,
		int y,
		int width,
		int height,
		GLenum bufferFormat,
		GLenum bufferType,
		const GLvoid* pixels,
		int rowBytesInInput)
{
	GLint pUnpackRowLength = 0;

	Render_BindTexture(textureName);

	// Set unpack row length (if valid rowbytes input given)
	if (rowBytesInInput > 0)
	{
		glGetIntegerv(GL_UNPACK_ROW_LENGTH, &pUnpackRowLength);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, rowBytesInInput);
	}

	glTexSubImage2D(
			GL_TEXTURE_2D,
			0,
			x,
			y,
			width,
			height,
			bufferFormat,
			bufferType,
			pixels);
	CHECK_GL_ERROR();

	// Restore unpack row length
	if (rowBytesInInput > 0)
	{
		glPixelStorei(GL_UNPACK_ROW_LENGTH, pUnpackRowLength);
	}
}

void Render_Load3DMFTextures(TQ3MetaFile* metaFile, GLuint* outTextureNames, bool forceClampUVs)
{
	for (int i = 0; i < metaFile->numTextures; i++)
	{
		TQ3TextureShader* textureShader = &metaFile->textures[i];

		GAME_ASSERT(textureShader->pixmap);

		TQ3TexturingMode meshTexturingMode = kQ3TexturingModeOff;
		GLenum internalFormat;
		GLenum format;
		GLenum type;
		switch (textureShader->pixmap->pixelType)
		{
			case kQ3PixelTypeRGB32:
				meshTexturingMode = kQ3TexturingModeOpaque;
				internalFormat = GL_RGB;
				format = GL_BGRA;
				type = GL_UNSIGNED_INT_8_8_8_8_REV;
				break;
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
			case kQ3PixelTypeRGB24:
				meshTexturingMode = kQ3TexturingModeOpaque;
				internalFormat = GL_RGB;
				format = GL_BGR;
				type = GL_UNSIGNED_BYTE;
				break;
			default:
				DoAlert("3DMF texture: Unsupported kQ3PixelType");
				continue;
		}

		int clampFlags = forceClampUVs ? kRendererTextureFlags_ClampBoth : 0;
		if (textureShader->boundaryU == kQ3ShaderUVBoundaryClamp)
			clampFlags |= kRendererTextureFlags_ClampU;
		if (textureShader->boundaryV == kQ3ShaderUVBoundaryClamp)
			clampFlags |= kRendererTextureFlags_ClampV;

		outTextureNames[i] = Render_LoadTexture(
					 internalFormat,						// format in OpenGL
					 textureShader->pixmap->width,			// width in pixels
					 textureShader->pixmap->height,			// height in pixels
					 format,								// what my format is
					 type,									// size of each r,g,b
					 textureShader->pixmap->image,			// pointer to the actual texture pixels
					 clampFlags);

		// Set glTextureName on meshes
		for (int j = 0; j < metaFile->numMeshes; j++)
		{
			if (metaFile->meshes[j]->internalTextureID == i)
			{
				metaFile->meshes[j]->glTextureName = outTextureNames[i];
				metaFile->meshes[j]->texturingMode = meshTexturingMode;
			}
		}
	}
}

#pragma mark -

void Render_StartFrame(void)
{
	// Clear rendering statistics
	memset(&gRenderStats, 0, sizeof(gRenderStats));

	// Clear mesh queue
	gMeshQueueSize = 0;

	// Clear stats
	gRenderStats.meshes = 0;
	gRenderStats.triangles = 0;

	// Clear color & depth buffers.
	SetFlag(glDepthMask, true);	// The depth mask must be re-enabled so we can clear the depth buffer.
	glDepthMask(true);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GAME_ASSERT(gState.currentTransform == NULL);

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

	// Flush mesh draw queue
	if (gMeshQueueSize == 0)
		return;

	// Sort mesh draw queue.
	// Note that this sorts the opaque meshes front-to-back,
	// then the transparent meshes back-to-front.
	qsort(
			gMeshQueuePtrs,
			gMeshQueueSize,
			sizeof(gMeshQueuePtrs[0]),
			DrawOrderComparator
	);

	// PASS 1: build depth buffer
	glColor4f(1,1,1,1);
	glDepthMask(true);
	glDepthFunc(GL_LESS);
	glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
	DisableClientState(GL_COLOR_ARRAY);
	DisableClientState(GL_NORMAL_ARRAY);
	EnableState(GL_DEPTH_TEST);
	for (int i = 0; i < gMeshQueueSize; i++)
		DrawMesh(gMeshQueuePtrs[i], PreDrawMesh_DepthPass);

	// PASS 2: draw opaque then transparent meshes
	// (pre-sorted in the most efficient way by DrawOrderComparator)
	glDepthMask(false);
	glDepthFunc(GL_LEQUAL);
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	for (int i = 0; i < gMeshQueueSize; i++)
		DrawMesh(gMeshQueuePtrs[i], PreDrawMesh_ColorPass);

	// Clear mesh draw queue
	gMeshQueueSize = 0;

	// Clear transform
	if (NULL != gState.currentTransform)
	{
		glPopMatrix();
		gState.currentTransform = NULL;
	}
}

void Render_EndFrame(void)
{
	GAME_ASSERT(gFrameStarted);

	Render_FlushQueue();

	DisableState(GL_SCISSOR_TEST);

	gFrameStarted = false;
}

#pragma mark -

static inline float WorldPointToDepth(const TQ3Point3D p)
{
	// This is a simplification of:
	//
	//     Q3Point3D_Transform(&p, &gCameraWorldToFrustumMatrix, &p);
	//     return p.z;
	//
	// This is enough to give us an idea of the depth of a point relative to another.

#define M(x,y) gCameraWorldToFrustumMatrix.value[x][y]
	return p.x*M(0,2) + p.y*M(1,2) + p.z*M(2,2);
#undef M
}

static float GetDepth(
		int						numMeshes,
		TQ3TriMeshData**		meshList,
		const TQ3Point3D*		centerCoord)
{
	if (centerCoord)
	{
		return WorldPointToDepth(*centerCoord);
	}
	else
	{
		// Average centers of all bounding boxes
		float mult = (float) numMeshes / 2.0f;
		TQ3Point3D center = (TQ3Point3D) { 0, 0, 0 };
		for (int i = 0; i < numMeshes; i++)
		{
			center.x += (meshList[i]->bBox.min.x + meshList[i]->bBox.max.x) * mult;
			center.y += (meshList[i]->bBox.min.y + meshList[i]->bBox.max.y) * mult;
			center.z += (meshList[i]->bBox.min.z + meshList[i]->bBox.max.z) * mult;
		}
		return WorldPointToDepth(center);
	}
}

static bool IsMeshTransparent(const TQ3TriMeshData* mesh, const RenderModifiers* mods)
{
	return	mesh->texturingMode == kQ3TexturingModeAlphaBlend
			|| mesh->diffuseColor.a < .999f
			|| mods->diffuseColor.a < .999f
			|| mods->autoFadeFactor < .999f
			|| (mods->statusBits & STATUS_BIT_GLOW)
	;
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
	GAME_ASSERT(gMeshQueueSize + numMeshes <= MESHQUEUE_MAX_SIZE);

	float depth = GetDepth(numMeshes, meshList, centerCoord);

	for (int i = 0; i < numMeshes; i++)
	{
		MeshQueueEntry* entry = gMeshQueuePtrs[gMeshQueueSize++];
		entry->mesh				= meshList[i];
		entry->transform		= transform;
		entry->mods				= mods ? mods : &kDefaultRenderMods;
		entry->depth			= depth;
		entry->meshIsTransparent= IsMeshTransparent(entry->mesh, entry->mods);

		gRenderStats.meshes++;
		gRenderStats.triangles += entry->mesh->numTriangles;
	}
}

void Render_SubmitMesh(
		const TQ3TriMeshData*	mesh,
		const TQ3Matrix4x4*		transform,
		const RenderModifiers*	mods,
		const TQ3Point3D*		centerCoord)
{
	GAME_ASSERT(gFrameStarted);
	GAME_ASSERT(gMeshQueueSize < MESHQUEUE_MAX_SIZE);

	MeshQueueEntry* entry = gMeshQueuePtrs[gMeshQueueSize++];
	entry->mesh				= mesh;
	entry->transform		= transform;
	entry->mods				= mods ? mods : &kDefaultRenderMods;
	entry->depth			= GetDepth(1, (TQ3TriMeshData **) &mesh, centerCoord);
	entry->meshIsTransparent= IsMeshTransparent(entry->mesh, entry->mods);

	gRenderStats.meshes++;
	gRenderStats.triangles += entry->mesh->numTriangles;
}

#pragma mark -

static int DrawOrderComparator(const void* a_void, const void* b_void)
{
	static const int AFirst		= -1;
	static const int BFirst		= +1;
	static const int DontCare	= 0;

	const MeshQueueEntry* a = *(MeshQueueEntry**) a_void;
	const MeshQueueEntry* b = *(MeshQueueEntry**) b_void;

	// First check manual priority

	if (a->mods->drawOrder < b->mods->drawOrder)
		return AFirst;

	if (a->mods->drawOrder > b->mods->drawOrder)
		return BFirst;

	// A and B have the same manual priority
	// Compare their transparencies (opaque meshes go first)

	if (a->meshIsTransparent != b->meshIsTransparent)
	{
		return b->meshIsTransparent? AFirst: BFirst;
	}

	// A and B have the same manual priority AND transparency
	// Compare their depths

	if (!a->meshIsTransparent)					// both A and B are OPAQUE meshes: order them front-to-back
	{
		if (a->depth < b->depth)				// A is closer to the camera, draw it first
			return AFirst;

		if (a->depth > b->depth)				// B is closer to the camera, draw it first
			return BFirst;
	}
	else										// both A and B are TRANSPARENT meshes: order them back-to-front
	{
		if (a->depth < b->depth)				// A is closer to the camera, draw it last
			return BFirst;

		if (a->depth > b->depth)				// B is closer to the camera, draw it last
			return AFirst;
	}

	return DontCare;
}

#pragma mark -

static void DrawMesh(const MeshQueueEntry* entry, bool (*preDrawFunc)(const MeshQueueEntry* entry))
{
	uint32_t statusBits = entry->mods->statusBits;

	const TQ3TriMeshData* mesh = entry->mesh;

	// Skip if hidden
	if (statusBits & STATUS_BIT_HIDDEN)
		return;

	// Call the preDraw function for this pass
	if (!preDrawFunc(entry))
		return;

	// Cull backfaces or not
	SetState(GL_CULL_FACE, !(statusBits & STATUS_BIT_KEEPBACKFACES));

	// To keep backfaces on a transparent mesh, draw backfaces first, then frontfaces.
	// This enhances the appearance of e.g. translucent spheres,
	// without the need to depth-sort individual faces.
	if (statusBits & STATUS_BIT_KEEPBACKFACES_2PASS)
		glCullFace(GL_FRONT);		// Pass 1: draw backfaces (cull frontfaces)

	// Submit vertex data
	glVertexPointer(3, GL_FLOAT, 0, mesh->points);

	// Submit transformation matrix if any
	if (gState.currentTransform != entry->transform)
	{
		if (gState.currentTransform)	// nuke old transform
			glPopMatrix();

		if (entry->transform)			// apply new transform
		{
			glPushMatrix();
			glMultMatrixf((float*)entry->transform->value);
		}

		gState.currentTransform = entry->transform;
	}

	// Draw the mesh
	__glDrawRangeElements(GL_TRIANGLES, 0, mesh->numPoints-1, mesh->numTriangles*3, GL_UNSIGNED_SHORT, mesh->triangles);
	CHECK_GL_ERROR();

	// Pass 2 to draw transparent meshes without face culling (see above for an explanation)
	if (statusBits & STATUS_BIT_KEEPBACKFACES_2PASS)
	{
		// Restored glCullFace to GL_BACK, which is the default for all other meshes.
		glCullFace(GL_BACK);	// pass 2: draw frontfaces (cull backfaces)

		// Draw the mesh again
		__glDrawRangeElements(GL_TRIANGLES, 0, mesh->numPoints - 1, mesh->numTriangles * 3, GL_UNSIGNED_SHORT, mesh->triangles);
		CHECK_GL_ERROR();
	}

}

static bool PreDrawMesh_DepthPass(const MeshQueueEntry* entry)
{
	const TQ3TriMeshData* mesh = entry->mesh;
	uint32_t statusBits = entry->mods->statusBits;

	if (statusBits & STATUS_BIT_NOZWRITE)
		return false;

	// Texture mapping
	if ((mesh->texturingMode & kQ3TexturingModeExt_OpacityModeMask) == kQ3TexturingModeAlphaTest ||
		(mesh->texturingMode & kQ3TexturingModeExt_OpacityModeMask) == kQ3TexturingModeAlphaBlend)
	{
		GAME_ASSERT(mesh->vertexUVs);

		EnableState(GL_ALPHA_TEST);
		EnableState(GL_TEXTURE_2D);
		EnableClientState(GL_TEXTURE_COORD_ARRAY);
		Render_BindTexture(mesh->glTextureName);
		glTexCoordPointer(2, GL_FLOAT, 0, mesh->vertexUVs);
		CHECK_GL_ERROR();
	}
	else
	{
		DisableState(GL_ALPHA_TEST);
		DisableState(GL_TEXTURE_2D);
		DisableClientState(GL_TEXTURE_COORD_ARRAY);
		CHECK_GL_ERROR();
	}

	return true;
}

static bool PreDrawMesh_ColorPass(const MeshQueueEntry* entry)
{
	const TQ3TriMeshData* mesh = entry->mesh;
	uint32_t statusBits = entry->mods->statusBits;

	TQ3TexturingMode texturingMode = (mesh->texturingMode & kQ3TexturingModeExt_OpacityModeMask);

	// Set alpha blending according to mesh transparency
	if (entry->meshIsTransparent)
	{
		EnableState(GL_BLEND);
		DisableState(GL_ALPHA_TEST);

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
	else
	{
		DisableState(GL_BLEND);

		// Enable alpha testing if the mesh's texture calls for it
		SetState(GL_ALPHA_TEST, texturingMode == kQ3TexturingModeAlphaTest);
	}

	// Environment map effect
	if (statusBits & STATUS_BIT_REFLECTIONMAP)
		EnvironmentMapTriMesh(mesh, entry->transform);

	// Apply gouraud or null illumination
	SetState(GL_LIGHTING,
			!( (statusBits & STATUS_BIT_NULLSHADER) || (mesh->texturingMode & kQ3TexturingModeExt_NullShaderFlag) ));

	// Apply fog or not
	SetState(GL_FOG, gState.wantFog && !(statusBits & STATUS_BIT_NOFOG));

	// Texture mapping
	if (texturingMode != kQ3TexturingModeOff)
	{
		EnableState(GL_TEXTURE_2D);
		EnableClientState(GL_TEXTURE_COORD_ARRAY);
		Render_BindTexture(mesh->glTextureName);
		glTexCoordPointer(2, GL_FLOAT, 0,
				statusBits & STATUS_BIT_REFLECTIONMAP ? gEnvMapUVs : mesh->vertexUVs);
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

		if (entry->mods->autoFadeFactor < .999f)
		{
			// Old-school OpenGL will ignore the diffuse color (used for transparency) if we also send
			// per-vertex colors. So, apply transparency to the per-vertex color array.
			GAME_ASSERT(4 * mesh->numPoints <= (int)(sizeof(gBackupVertexColors) / sizeof(gBackupVertexColors[0])));
			int j = 0;
			for (int v = 0; v < mesh->numPoints; v++)
			{
				gBackupVertexColors[j++] = mesh->vertexColors[v].r;
				gBackupVertexColors[j++] = mesh->vertexColors[v].g;
				gBackupVertexColors[j++] = mesh->vertexColors[v].b;
				gBackupVertexColors[j++] = mesh->vertexColors[v].a * entry->mods->autoFadeFactor;
			}
			glColorPointer(4, GL_FLOAT, 0, gBackupVertexColors);
		}
		else
		{
			glColorPointer(4, GL_FLOAT, 0, mesh->vertexColors);
		}
	}
	else
	{
		DisableClientState(GL_COLOR_ARRAY);

		// Apply diffuse color for the entire mesh
		glColor4f(
			mesh->diffuseColor.r * entry->mods->diffuseColor.r,
			mesh->diffuseColor.g * entry->mods->diffuseColor.g,
			mesh->diffuseColor.b * entry->mods->diffuseColor.b,
			mesh->diffuseColor.a * entry->mods->diffuseColor.a * entry->mods->autoFadeFactor
		);
	}

	// Submit normal data if any
	if (mesh->hasVertexNormals && !(statusBits & STATUS_BIT_NULLSHADER))
	{
		EnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, mesh->vertexNormals);
	}
	else
	{
		DisableClientState(GL_NORMAL_ARRAY);
	}

	return true;
}

void Render_ResetColor(void)
{
	DisableState(GL_BLEND);
	DisableState(GL_ALPHA_TEST);
	DisableState(GL_LIGHTING);
	DisableState(GL_TEXTURE_2D);
	DisableClientState(GL_NORMAL_ARRAY);
	DisableClientState(GL_COLOR_ARRAY);
	glColor4f(1, 1, 1, 1);
}

#pragma mark -

//=======================================================================================================

/****************************/
/*    2D    */
/****************************/

void Render_Enter2D_Full640x480(void)
{
	DisableState(GL_SCISSOR_TEST);
	glViewport(0, 0, gWindowWidth, gWindowHeight);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0,640,480,0,0,1000);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}

void Render_Enter2D_NormalizedCoordinates(float aspect)
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-aspect, aspect, -1, 1, 0, 1000);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}

void Render_Exit2D(void)
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

#pragma mark -

//=======================================================================================================

/*******************************************/
/*    BACKDROP/OVERLAY (COVER WINDOW)      */
/*******************************************/

void Render_DrawFadeOverlay(float opacity)
{
	GAME_ASSERT(gFullscreenQuad);

	gFullscreenQuad->texturingMode = kQ3TexturingModeOff;
	gFullscreenQuad->diffuseColor = (TQ3ColorRGBA) { 0,0,0,1.0f-opacity };

	Render_SubmitMesh(gFullscreenQuad, NULL, &kDefaultRenderMods_FadeOverlay, &kQ3Point3D_Zero);
}

GLuint Render_CaptureFrameIntoTexture(int* outTextureWidth, int* outTextureHeight)
{
	int width4rem = gWindowWidth % 4;
	int width4ceil = gWindowWidth - width4rem + (width4rem == 0? 0: 4);

	GLint textureWidth = width4ceil;
	GLint textureHeight = gWindowHeight;
	char* textureData = NewPtrClear(textureWidth * textureHeight * 3);

	//SDL_GL_SwapWindow(gSDLWindow);
//	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//	glPixelStorei(GL_UNPACK_ROW_LENGTH, textureWidth);
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

	DisposePtr(textureData);
	textureData = NULL;

	if (outTextureWidth)
		*outTextureWidth = textureWidth;

	if (outTextureHeight)
		*outTextureHeight = textureHeight;

	return textureName;
}

void Render_FreezeFrameFadeOut(float duration)
{
#if ALLOW_FADE
	//-------------------------------------------------------------------------
	// Capture window contents into texture

	int textureWidth, textureHeight;

	GLuint textureName = Render_CaptureFrameIntoTexture(&textureWidth, &textureHeight);

	gFullscreenQuad->texturingMode = kQ3TexturingModeOpaque;
	gFullscreenQuad->glTextureName = textureName;

	gFullscreenQuad->vertexUVs[0] = (TQ3Param2D) { 0, 0 };
	gFullscreenQuad->vertexUVs[1] = (TQ3Param2D) { (float)gWindowWidth/textureWidth, 0 };
	gFullscreenQuad->vertexUVs[2] = (TQ3Param2D) { (float)gWindowWidth/textureWidth, (float)gWindowHeight/textureHeight };
	gFullscreenQuad->vertexUVs[3] = (TQ3Param2D) { 0, (float)gWindowHeight/textureHeight };

	//-------------------------------------------------------------------------
	// Fade out

	Uint32 startTicks = SDL_GetTicks();

	gGammaFadeFactor = 1.0f;

	while (gGammaFadeFactor > 0)
	{
		Uint32 ticks = SDL_GetTicks();
		gGammaFadeFactor = 1.0f - ((ticks - startTicks) / 1000.0f / duration);
		if (gGammaFadeFactor < 0.0f)
			gGammaFadeFactor = 0.0f;

		gFullscreenQuad->diffuseColor = (TQ3ColorRGBA) { gGammaFadeFactor, gGammaFadeFactor, gGammaFadeFactor, 1.0f };

		Render_StartFrame();
		Render_Enter2D_Full640x480();
		Render_SubmitMesh(gFullscreenQuad, NULL, &kDefaultRenderMods_FadeOverlay, &kQ3Point3D_Zero);
		Render_FlushQueue();
		Render_Exit2D();
		Render_EndFrame();

		CHECK_GL_ERROR();
		SDL_GL_SwapWindow(gSDLWindow);
		SDL_Delay(15);
	}

	SDL_Delay(100);			// Hold full blackness for a little bit

	//-------------------------------------------------------------------------
	// Clean up

	glDeleteTextures(1, &textureName);

	gGammaFadeFactor = 1;
#endif
}

#pragma mark -

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
