#include <SDL.h>
#include "Pomme.h"
#include "PommeGraphics.h"
#include "MyPCH_Normal.pch"
#include "GLOverlay.h"

#include <memory>

extern "C"
{
	extern SDL_Window* gSDLWindow;
	extern PrefsType gGamePrefs;
}

constexpr const bool ALLOW_OVERLAY = true;
static std::unique_ptr<GLOverlay> glOverlay = nullptr;
static GLint viewportBackup[4];
static SDL_GLContext exclusiveGLContext = nullptr;

class PortGuard
{
	GrafPtr oldPort;

public:
	PortGuard()
	{
		GetPort(&oldPort);
		SetPort(Pomme::Graphics::GetScreenPort());
	}

	~PortGuard()
	{
		SetPort(oldPort);
	}
};

static UInt32* GetOverlayPixPtr()
{
	return (UInt32*) GetPixBaseAddr(GetGWorldPixMap(Pomme::Graphics::GetScreenPort()));
}

static bool IsOverlayAllocated()
{
	return glOverlay != nullptr;
}

void Overlay_Alloc()
{
	if (!ALLOW_OVERLAY
		|| IsOverlayAllocated())
	{
		return;
	}

	PortGuard portGuard;

	glOverlay = std::make_unique<GLOverlay>(
		GAME_VIEW_WIDTH,
		GAME_VIEW_HEIGHT,
		(unsigned char*) GetOverlayPixPtr());

	ClearPortDamage();
}

void Overlay_Clear(UInt32 argb)
{
	PortGuard portGuard;

	UInt32 bgra = ByteswapScalar(argb);

	auto pixel = GetOverlayPixPtr();

	for (int i = 0; i < GAME_VIEW_WIDTH * GAME_VIEW_HEIGHT; i++)
	{
		*(pixel++) = bgra;
	}

	GrafPtr port;
	GetPort(&port);
	DamagePortRegion(&port->portRect);
}

void Overlay_Dispose()
{
	if (!ALLOW_OVERLAY
		|| !IsOverlayAllocated())
	{
		return;
	}

	glOverlay.reset(nullptr);
}


bool Overlay_Begin()
{
	if (!ALLOW_OVERLAY
		|| !IsOverlayAllocated())
	{
		return false;
	}

	// Set screen port as current; guard will restore current port when the function exits
	PortGuard portGuard;

	// Update overlay texture if needed
	if (IsPortDamaged())
	{
		Rect damageRect;
		GetPortDamageRegion(&damageRect);
		glOverlay->UpdateTexture(damageRect.left, damageRect.top, damageRect.right - damageRect.left, damageRect.bottom - damageRect.top);
		ClearPortDamage();
	}

	// Back up viewport
	glGetIntegerv(GL_VIEWPORT, viewportBackup);

	// Use entire screen
	glDisable(GL_SCISSOR_TEST);

	// Set viewport
	int windowWidth, windowHeight;
	SDL_GetWindowSize(gSDLWindow, &windowWidth, &windowHeight);
	glViewport(0, 0, windowWidth, windowHeight);

	return true;
}

void Overlay_End()
{
	// Restore scissor test & viewport because Quesa doesn't reset it
	glEnable(GL_SCISSOR_TEST);
	glViewport(viewportBackup[0], viewportBackup[1], (GLsizei) viewportBackup[2], (GLsizei) viewportBackup[3]);
}

void Overlay_SubmitQuad(
		int srcX, int srcY, int srcW, int srcH,
		float dstX, float dstY, float dstW, float dstH)
{
	glOverlay->SubmitQuad(
			srcX, srcY, srcW, srcH,
			dstX, dstY, dstW, dstH);
}

void Overlay_Flush()
{
	if (!Overlay_Begin())
		return;

	glOverlay->FlushQuads(gGamePrefs.textureFiltering);


	Overlay_End();
}

void Overlay_BeginExclusive()
{
	if (!ALLOW_OVERLAY)
		return;

	if (exclusiveGLContext)
		throw std::runtime_error("already in exclusive GL mode");

	exclusiveGLContext = SDL_GL_CreateContext(gSDLWindow);
	SDL_GL_MakeCurrent(gSDLWindow, exclusiveGLContext);

	SDL_GL_SetSwapInterval(gGamePrefs.vsync ? 1 : 0);

	Overlay_Alloc();
}

void Overlay_EndExclusive()
{
	if (!ALLOW_OVERLAY)
		return;

	if (!exclusiveGLContext)
		throw std::runtime_error("not in exclusive GL mode");

	Overlay_Dispose();

	SDL_GL_DeleteContext(exclusiveGLContext);
	exclusiveGLContext = nullptr;
}
