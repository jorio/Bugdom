#include <SDL.h>
#include "Pomme.h"
#include "PommeGraphics.h"
#include "MyPCH_Normal.pch"
#include "GLOverlay.h"
#include "GLOverlayFade.h"

#include <memory>

extern "C"
{
	extern SDL_Window* gSDLWindow;
	extern PrefsType gGamePrefs;
	extern float gGammaFadePercent;
}

constexpr const bool ALLOW_OVERLAY = true;
static std::unique_ptr<GLOverlay> glOverlay = nullptr;
static std::unique_ptr<GLOverlayFade> glOverlayFade = nullptr;
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

	glOverlayFade = std::make_unique<GLOverlayFade>();

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
	glOverlayFade.reset(nullptr);
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

	float fadeBlackness = (100.0f - gGammaFadePercent) / 100.0f;
	glOverlayFade->DrawFade(0, 0, 0, fadeBlackness);

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

void Overlay_FadeOutFrozenFrame(float duration)
{
	SDL_GLContext currentContext = SDL_GL_GetCurrentContext();
	if (!currentContext)
	{
		printf("%s: no GL context; skipping fade out\n", __func__);
		return;
	}

	int windowWidth, windowHeight;
	SDL_GetWindowSize(gSDLWindow, &windowWidth, &windowHeight);

	GLOverlay overlay = GLOverlay::CapturePixels(windowWidth, windowHeight);

	const int durationTicks = duration * 1000.0f;
	uint32_t nowTicks = SDL_GetTicks();
	uint32_t startTicks = nowTicks;
	uint32_t endTicks = startTicks + durationTicks;


	// Back up viewport
	glGetIntegerv(GL_VIEWPORT, viewportBackup);

	// Use entire screen
	glDisable(GL_SCISSOR_TEST);

	// Set viewport
	glViewport(0, 0, windowWidth, windowHeight);


	while (nowTicks < endTicks)
	{
		float progress = (float) (nowTicks - startTicks) / (durationTicks);
		overlay.SubmitQuad(0, 0, windowWidth, windowHeight, 0, 1, 1, -1);
		overlay.FlushQuads(false);
		glOverlayFade->DrawFade(0, 0, 0, progress);
		SDL_GL_SwapWindow(gSDLWindow);
		SDL_Delay(5);
		nowTicks = SDL_GetTicks();
	}

	// Draw solid black frame before moving on
	glOverlayFade->DrawFade(0, 0, 0, 1.0);
	SDL_GL_SwapWindow(gSDLWindow);

	// Restore scissor test & viewport because Quesa doesn't reset it
	glEnable(GL_SCISSOR_TEST);
	glViewport(viewportBackup[0], viewportBackup[1], (GLsizei) viewportBackup[2], (GLsizei) viewportBackup[3]);
}
