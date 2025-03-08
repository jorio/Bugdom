#pragma once

#include <QD3D.h>
#include <SDL3/SDL_opengl.h>

#if _DEBUG
#define CHECK_GL_ERROR()												\
	do {					 											\
		GLenum err = glGetError();										\
		if (err != GL_NO_ERROR)											\
			DoFatalGLError(err, __func__, __LINE__);					\
	} while(0)
#else
#define CHECK_GL_ERROR() do {} while(0)
#endif

#pragma mark -

typedef struct RenderStats
{
	int			triangles;
	int			meshesPass1;
	int			meshesPass2;
} RenderStats;

typedef struct RenderModifiers
{
	// Copy of the status bits from ObjNode.
	uint32_t 				statusBits;

	// Diffuse color applied to the entire mesh.
	TQ3ColorRGBA			diffuseColor;

	// Auto-fade alpha factor (applied on top of the alpha in diffuseColor).
	float					autoFadeFactor;

	// Set this to override the order in which meshes are drawn.
	// The default value for most objects is 0.
	// Meshes are sorted by ascending draw order.
	// Meshes with the same draw order are sorted according to their depth relative to the camera.
	// Note that opaque meshes within the same draw order group are drawn front-to-back,
	// and transparent meshes are drawn back-to-front.
	int						drawOrder;
} RenderModifiers;

enum
{
	kDrawOrder_Terrain = -128,
	kDrawOrder_Cyclorama,
	kDrawOrder_Fences,
	kDrawOrder_Shadows,
	kDrawOrder_Default = 0,
	kDrawOrder_Ripples,
	kDrawOrder_GlowyParticles,
	kDrawOrder_UI,
	kDrawOrder_FadeOverlay,
	kDrawOrder_DebugUI,
};

typedef enum
{
	kRendererTextureFlags_None			= 0,
	kRendererTextureFlags_ClampU		= (1 << 0),
	kRendererTextureFlags_ClampV		= (1 << 1),
	kRendererTextureFlags_ClampBoth		= kRendererTextureFlags_ClampU | kRendererTextureFlags_ClampV,
	kRendererTextureFlags_SolidBlackIsAlpha	= 1 << 4,
	kRendererTextureFlags_GrayscaleIsAlpha	= 1 << 5,
	kRendererTextureFlags_KeepOriginalAlpha	= 1 << 6,
	kRendererTextureFlags_ForcePOT			= 1 << 7,
} RendererTextureFlags;

#define kQ3TexturingModeExt_OpacityModeMask		0x0000FFFF

// OR this flag to a mesh's texturingMode to force the mesh to be NULL-shaded.
#define kQ3TexturingModeExt_NullShaderFlag		0x00010000

#pragma mark -

void DoFatalGLError(GLenum error, const char* function, int line);

#pragma mark -

void Render_CreateContext(void);

void Render_DeleteContext(void);

// Fills the argument with the default mesh rendering modifiers.
void Render_SetDefaultModifiers(RenderModifiers* dest);

// Sets up the initial renderer state.
// Call this function after creating the OpenGL context.
void Render_InitState(const TQ3ColorRGBA* clearColor);

void Render_EndScene(void);

void Render_EnableFog(
		float camHither,
		float camYon,
		float fogHither,
		float fogYon,
		TQ3ColorRGBA fogColor
);

void Render_DisableFog(void);

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

void Render_UpdateTexture(
		GLuint textureName,
		int x,
		int y,
		int width,
		int height,
		GLenum bufferFormat,
		GLenum bufferType,
		const GLvoid* pixels,
		int rowBytesInInput
);

// Uploads all textures from a 3DMF file to the GPU.
// Requires an OpenGL context to be active.
// outTextureNames is an array with enough capacity to hold `metaFile->numTextures` texture names.
void Render_Load3DMFTextures(TQ3MetaFile* metaFile, GLuint* outTextureNames, bool forceClampUVs);

#pragma mark -

// Instructs the renderer to get ready to draw a new frame.
// Call this function before any draw/submit calls.
void Render_StartFrame(void);

// Flushes the rendering queue.
// May be called multiple times between Render_StartFrame and Render_EndFrame.
void Render_FlushQueue(void);

// Flushes the rendering queue and finishes the frame.
void Render_EndFrame(void);

void Render_SetViewport(int x, int y, int w, int h);

void Render_ResetColor(void);

#pragma mark -

// Submits a list of trimeshes for drawing.
// Arguments transform and mods may be nil.
// Rendering will actually occur in Render_FlushQueue(), after all meshes have been submitted.
// IMPORTANT: the pointers must remain valid until Render_FlushQueue().
void Render_SubmitMeshList(
		int numMeshes,
		TQ3TriMeshData** meshList,
		const TQ3Matrix4x4* transform,
		const RenderModifiers* mods,
		const TQ3Point3D* centerCoord);

// Submits one trimesh for drawing.
// Arguments transform and mods may be nil.
// Rendering will actually occur in Render_FlushQueue(), after all meshes have been submitted.
// IMPORTANT: the pointers must remain valid until Render_FlushQueue().
void Render_SubmitMesh(
		const TQ3TriMeshData* mesh,
		const TQ3Matrix4x4* transform,
		const RenderModifiers* mods,
		const TQ3Point3D* centerCoord);

#pragma mark -

void Render_Enter2D_Full640x480(void);

void Render_Enter2D_NormalizedCoordinates(float aspect);

void Render_Enter2D_NativeResolution(void);

void Render_Exit2D(void);

#pragma mark -

void Render_DrawFadeOverlay(float opacity);

#pragma mark -

TQ3Area Render_GetAdjustedViewportRect(Rect paneClip, int logicalWidth, int logicalHeight);

TQ3Vector2D FitRectKeepAR(
	int logicalWidth,
	int logicalHeight,
	float displayWidth,
	float displayHeight);
