#pragma once

#include <QD3D.h>

#if __APPLE__
#include <OpenGL/glu.h>		// gluErrorString, gluPerspective, gluLookAt
#else
#include <GL/glu.h>			// gluErrorString, gluPerspective, gluLookAt
#endif

#if !defined(ALLOW_FADE) && !_DEBUG
#define ALLOW_FADE		1
#endif

#define CHECK_GL_ERROR()												\
	do {					 											\
		GLenum err = glGetError();										\
		if (err != GL_NO_ERROR)											\
			DoFatalGLError(err, __func__, __LINE__);					\
	} while(0)

#pragma mark -

typedef struct RenderStats
{
	int			trianglesDrawn;
	int			meshQueueSize;
	int 		batchedStateChanges;
} RenderStats;

typedef struct RenderModifiers
{
	// Copy of the status bits from ObjNode.
	uint32_t 				statusBits;

	// Diffuse color applied to the entire mesh.
	TQ3ColorRGBA			diffuseColor;

	// Set this to override the order in which meshes are drawn.
	// The default value is 0.
	// Positive values will cause the mesh to be drawn as if it were further back in the scene than it really is.
	// Negative values will cause the mesh to be drawn as if it were closer to the camera than it really is.
	// When several meshes have the same priority, they are sorted according to their depth relative to the camera.
	// Note that opaque meshes are drawn front-to-back, and transparent meshes are drawn back-to-front.
	int						sortPriority;
} RenderModifiers;


enum
{
	kCoverQuadFill						= 0,
	kCoverQuadPillarbox					= 1,
	kCoverQuadLetterbox					= 2,
	kCoverQuadFit						= kCoverQuadPillarbox | kCoverQuadLetterbox,
};

typedef enum
{
	kRendererTextureFlags_None			= 0,
	kRendererTextureFlags_ClampU		= (1 << 0),
	kRendererTextureFlags_ClampV		= (1 << 1),
	kRendererTextureFlags_ClampBoth		= kRendererTextureFlags_ClampU | kRendererTextureFlags_ClampV,
} RendererTextureFlags;

#pragma mark -

void DoFatalGLError(GLenum error, const char* function, int line);

#pragma mark -

// Fills the argument with the default mesh rendering modifiers.
void Render_SetDefaultModifiers(RenderModifiers* dest);

// Sets up the initial renderer state.
// Call this function after creating the OpenGL context.
void Render_InitState(void);

#pragma mark -

void Render_BindTexture(GLuint textureName);

// Wrapper for glTexImage that takes care of all the boilerplate associated with texture creation.
// Returns an OpenGL texture name.
// Aborts the game on failure.
GLuint Render_LoadTexture(
		GLenum internalFormat,
		int width,
		int height,
		GLenum bufferFormat,
		GLenum bufferType,
		const GLvoid* pixels,
		RendererTextureFlags flags
);

// Uploads all textures from a 3DMF file to the GPU.
// Requires an OpenGL context to be active.
void Render_Load3DMFTextures(TQ3MetaFile* metaFile);

// Deletes OpenGL texture names previously loaded from a 3DMF file.
void Render_Dispose3DMFTextures(TQ3MetaFile* metaFile);

#pragma mark -

// Instructs the renderer to get ready to draw a new frame.
// Call this function before any draw/submit calls.
void Render_StartFrame(void);

// Flushes the rendering queue.
void Render_EndFrame(void);

void Render_SetViewport(bool scissor, int x, int y, int w, int h);

#pragma mark -

// Submits a list of trimeshes for drawing.
// Arguments transform and mods may be nil.
// Rendering will actually occur in Render_EndFrame(), after all meshes have been submitted.
// IMPORTANT: the pointers must remain valid until you call Render_EndFrame(),
// INCLUDING THE POINTER TO THE LIST OF MESHES!
void Render_SubmitMeshList(
		int numMeshes,
		TQ3TriMeshData** meshList,
		const TQ3Matrix4x4* transform,
		const RenderModifiers* mods,
		const TQ3Point3D* centerCoord);

// Submits one trimesh for drawing.
// Arguments transform and mods may be nil.
// Rendering will actually occur in Render_EndFrame(), after all meshes have been submitted.
// IMPORTANT: the pointers must remain valid until you call Render_EndFrame().
void Render_SubmitMesh(
		TQ3TriMeshData* mesh,
		const TQ3Matrix4x4* transform,
		const RenderModifiers* mods,
		const TQ3Point3D* centerCoord);

#pragma mark -

void Render_Enter2D(void);

void Render_Exit2D(void);

#pragma mark -

void Render_Alloc2DCover(int width, int height);

void Render_Dispose2DCover(void);

void Render_Clear2DCover(uint32_t argb);

void Render_Draw2DCover(int fit);

#pragma mark -

void Render_SetWindowGamma(float percent);

void Render_FreezeFrameFadeOut(void);
