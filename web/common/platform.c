// Browser integration. Desktop builds deliberately compile an empty file.
#ifdef __EMSCRIPTEN__
#include "platform.h"
#include "Pomme.h"
#include <SDL3/SDL.h>
#include <math.h>
extern SDL_Window* gSDLWindow;
#include <emscripten.h>

static bool held[WEB_BUTTON_COUNT];
static bool pressed[WEB_BUTTON_COUNT];
static float stickX, stickY;
static float cameraYaw, cameraPitch, cameraZoom;
static float pointerX = -1, pointerY = -1;
static bool pointerHeld, pointerPressed;
static int webViewport[6] = {960, 540, 0, 0, 0, 0};
static bool gameplayActive;
void WebSetGameplayActive(int active) { gameplayActive = active != 0; }
int WebIsGameplayActive(void) { return gameplayActive; }
static bool soundMuted;
static bool audioReady;
static long normalOutputVolume;
static bool pauseRequested;

EMSCRIPTEN_KEEPALIVE void WebSetMuted(int muted)
{
	soundMuted = muted != 0;
	if (audioReady) SetDefaultOutputVolume(soundMuted ? 0 : normalOutputVolume);
}

EMSCRIPTEN_KEEPALIVE int WebIsMuted(void)
{
	if (!audioReady) return soundMuted;
	long volume;
	GetDefaultOutputVolume(&volume);
	return volume == 0;
}

EM_JS(void, ReadBrowserViewport, (int* values), {
	if (Module.webViewport) HEAP32.set(Module.webViewport, values >> 2);
});

void WebPrepareFrame(void)
{
	if (!audioReady)
	{
		GetDefaultOutputVolume(&normalOutputVolume);
		audioReady = true;
		WebSetMuted(soundMuted);
	}
	ReadBrowserViewport(webViewport);
	int width = webViewport[0], height = webViewport[1];
	if (width <= 0 || height <= 0) return;
	int oldWidth, oldHeight;
	SDL_GetWindowSize(gSDLWindow, &oldWidth, &oldHeight);
	if (width != oldWidth || height != oldHeight)
		SDL_SetWindowSize(gSDLWindow, width, height);
	SDL_GetWindowSizeInPixels(gSDLWindow, &width, &height);
	WebGameResize(width, height);
}

void WebGetSafeArea(float* top, float* right, float* bottom, float* left)
{
	*top = webViewport[2]; *right = webViewport[3];
	*bottom = webViewport[4]; *left = webViewport[5];
}

void WebGetHUDInsets(float logicalWidth, float logicalHeight, float* top, float* right, float* bottom, float* left)
{
    float sx = logicalWidth / fmaxf(1, webViewport[0]);
    float sy = logicalHeight / fmaxf(1, webViewport[1]);
    bool portrait = webViewport[1] > webViewport[0];
    *top = (webViewport[2] + (portrait ? 64 : 4)) * sy;
    *right = (webViewport[3] + (portrait ? 4 : 224)) * sx;
    *bottom = (webViewport[4] + 4) * sy;
    *left = (webViewport[5] + 4) * sx;
}

EMSCRIPTEN_KEEPALIVE void WebSetPointer(float x, float y, int down)
{
	pointerX = x;
	pointerY = y;
	if (down && !pointerHeld) pointerPressed = true;
	pointerHeld = down != 0;
}

int WebGetPointer(float* x, float* y)
{
	if (pointerX < 0 || pointerY < 0) return false;
	*x = pointerX * webViewport[0];
	*y = pointerY * webViewport[1];
	return true;
}

int WebGetClick(void) { return pointerHeld || pointerPressed; }

EMSCRIPTEN_KEEPALIVE void WebSetStick(float x, float y)
{
	if (!isfinite(x) || !isfinite(y)) return;
	float length = sqrtf(x*x + y*y);
	float scale = length > 1 ? 1 / length : 1;
	stickX = x * scale;
	stickY = y * scale;
}

EMSCRIPTEN_KEEPALIVE void WebAddCamera(float yaw, float pitch, float zoom)
{
	if (!isfinite(yaw) || !isfinite(pitch) || !isfinite(zoom)) return;
	cameraYaw += yaw;
	cameraPitch += pitch;
	cameraZoom += zoom;
}

void WebGetMovement(float* x, float* y)
{
	*x = stickX;
	*y = stickY;
}

void WebConsumeCamera(float* yaw, float* pitch, float* zoom)
{
	*yaw = cameraYaw;
	*pitch = cameraPitch;
	*zoom = cameraZoom;
	cameraYaw = cameraPitch = cameraZoom = 0;
}

EMSCRIPTEN_KEEPALIVE void WebSetButton(int button, int down)
{
	if (button < 0 || button >= WEB_BUTTON_COUNT) return;
	if (down && !held[button]) pressed[button] = true;
	held[button] = down != 0;
}

EMSCRIPTEN_KEEPALIVE void WebResetInput(void)
{
	SDL_memset(held, 0, sizeof(held));
	SDL_memset(pressed, 0, sizeof(pressed));
	stickX = stickY = cameraYaw = cameraPitch = cameraZoom = 0;
	pointerHeld = pointerPressed = false;
}

// A dedicated request, rather than a synthetic pause-button press: presses are
// wiped by blur/resize resets and are not read while the window is unfocused.
EMSCRIPTEN_KEEPALIVE void WebRequestPause(void) { pauseRequested = true; }

int WebConsumePauseRequest(void)
{
	bool requested = pauseRequested;
	pauseRequested = false;
	return requested;
}

int WebGetButton(int button)
{
	return button >= 0 && button < WEB_BUTTON_COUNT && (held[button] || pressed[button]);
}

void WebClearPresses(void)
{
	SDL_memset(pressed, 0, sizeof(pressed));
	pointerPressed = false;
}

EM_JS(void, WebSyncFiles, (), {
	if (Module.syncSaves) Module.syncSaves();
});

EM_JS(void, UpdateBrowserUI, (int playing, int paused), {
	if (Module.updateGameState) Module.updateGameState(!!playing, !!paused);
});

// Yield until the browser's next repaint so frames align with the display.
// The timer covers hidden tabs, where requestAnimationFrame never fires, and
// keeps the game near 30 fps if a slow device's compositor falls behind.
EM_ASYNC_JS(void, WaitForNextFrame, (), {
	await new Promise(resolve => {
		let done = false;
		const finish = () => { if (!done) { done = true; resolve(); } };
		requestAnimationFrame(finish);
		setTimeout(finish, 34);
	});
});

void WebPresent(int playing, int paused)
{
	UpdateBrowserUI(playing, paused);
	if (!playing || paused) pauseRequested = false;	// nothing left to pause
	// Every screen (including fades and loading screens) presents through here.
	// Asyncify preserves the original nested loops while the browser paints.
	WaitForNextFrame();
}
#endif
