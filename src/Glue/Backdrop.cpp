#include <SDL.h>
#include "Pomme.h"
#include "PommeGraphics.h"
#include "MyPCH_Normal.pch"
#include "GLBackdrop.h"

#include <memory>

extern "C"
{
	extern SDL_Window* gSDLWindow;
	extern PrefsType gGamePrefs;
}

constexpr const bool ALLOW_BACKDROP_TEXTURE = true;
std::unique_ptr<GLBackdrop> glBackdrop = nullptr;

static SDL_GLContext exclusiveGLContext = nullptr;
static bool exclusiveGLContextValid = false;

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

void SetWindowGamma(int percent)
{
	SDL_SetWindowBrightness(gSDLWindow, percent / 100.0f);
}

static UInt32* GetBackdropPixPtr()
{
	return (UInt32*) GetPixBaseAddr(GetGWorldPixMap(Pomme::Graphics::GetScreenPort()));
}

static bool IsBackdropAllocated()
{
	return glBackdrop != nullptr;
}

void AllocBackdropTexture()
{
	if (!ALLOW_BACKDROP_TEXTURE
		|| IsBackdropAllocated())
	{
		return;
	}

	PortGuard portGuard;

	glBackdrop = std::make_unique<GLBackdrop>(
		GAME_VIEW_WIDTH,
		GAME_VIEW_HEIGHT,
		(unsigned char*) GetBackdropPixPtr());

	ClearPortDamage();
}

void ClearBackdrop(UInt32 argb)
{
	PortGuard portGuard;

	UInt32 bgra = ByteswapScalar(argb);

	auto backdropPixPtr = GetBackdropPixPtr();

	for (int i = 0; i < GAME_VIEW_WIDTH * GAME_VIEW_HEIGHT; i++)
	{
		*(backdropPixPtr++) = bgra;
	}

	GrafPtr port;
	GetPort(&port);
	DamagePortRegion(&port->portRect);
}

void DisposeBackdropTexture()
{
	if (!ALLOW_BACKDROP_TEXTURE
		|| !IsBackdropAllocated())
	{
		return;
	}

	glBackdrop.reset(nullptr);
}

void SetBackdropClipRegion(int regionWidth, int regionHeight)
{
	if (!ALLOW_BACKDROP_TEXTURE
		|| !IsBackdropAllocated())
	{
		return;
	}

	glBackdrop->SetClipRegion(regionWidth, regionHeight);
}

void RenderBackdropQuad(int fit)
{
	if (!ALLOW_BACKDROP_TEXTURE
		|| !IsBackdropAllocated())
	{
		return;
	}

	PortGuard portGuard;

	int windowWidth, windowHeight;
	GLint vp[4];

	SDL_GetWindowSize(gSDLWindow, &windowWidth, &windowHeight);
	glGetIntegerv(GL_VIEWPORT, vp);

	glDisable(GL_SCISSOR_TEST);

	if (IsPortDamaged())
	{
		Rect damageRect;
		GetPortDamageRegion(&damageRect);
		glBackdrop->UpdateTexture(damageRect.left, damageRect.top, damageRect.right - damageRect.left, damageRect.bottom - damageRect.top);
		ClearPortDamage();
	}

	glBackdrop->UpdateQuad(windowWidth, windowHeight, fit);

	glBackdrop->Render(windowWidth, windowHeight, gGamePrefs.textureFiltering, !(fit & BACKDROP_CLEAR_BLACK));

	if (exclusiveGLContextValid) // in exclusive GL mode, force swap buffer
	{
		SDL_GL_SwapWindow(gSDLWindow);
	}

	glEnable(GL_SCISSOR_TEST);
	glViewport(vp[0], vp[1], (GLsizei) vp[2], (GLsizei) vp[3]);
}

void ExclusiveOpenGLMode_Begin()
{
	if (!ALLOW_BACKDROP_TEXTURE)
		return;

	if (exclusiveGLContextValid)
		throw std::runtime_error("already in exclusive GL mode");

	exclusiveGLContext = SDL_GL_CreateContext(gSDLWindow);
	exclusiveGLContextValid = true;
	SDL_GL_MakeCurrent(gSDLWindow, exclusiveGLContext);

	SDL_GL_SetSwapInterval(gGamePrefs.vsync ? 1 : 0);

	AllocBackdropTexture();
}

void ExclusiveOpenGLMode_End()
{
	if (!ALLOW_BACKDROP_TEXTURE)
		return;

	if (!exclusiveGLContextValid)
		throw std::runtime_error("not in exclusive GL mode");

	DisposeBackdropTexture();

	exclusiveGLContextValid = false;
	SDL_GL_DeleteContext(exclusiveGLContext);
	exclusiveGLContext = nullptr;
}
