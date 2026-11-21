// Browser integration. Desktop builds deliberately compile an empty file.
#ifdef __EMSCRIPTEN__
#include "game.h"
#include <emscripten.h>

// This order is the public interface used by web/controls.js.
enum { WEB_UP, WEB_DOWN, WEB_LEFT, WEB_RIGHT, WEB_JUMP, WEB_KICK,
	WEB_ROLL, WEB_BUDDY, WEB_PAUSE, WEB_CONFIRM, WEB_CAMERA_LEFT,
	WEB_CAMERA_RIGHT, WEB_ZOOM_IN, WEB_ZOOM_OUT, WEB_BUTTON_COUNT };
static bool held[WEB_BUTTON_COUNT];
static bool pressed[WEB_BUTTON_COUNT];
static float stickX, stickY;
static float cameraYaw, cameraPitch, cameraZoom;
static float pointerX = -1, pointerY = -1;
static bool pointerHeld, pointerPressed;
static int webViewport[6] = {960, 540, 0, 0, 0, 0};
static bool soundMuted;
static bool audioReady;
static long normalOutputVolume;

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
	QD3D_OnWindowResized(width, height);
}

void WebGetSafeArea(float* top, float* right, float* bottom, float* left)
{
	*top = webViewport[2]; *right = webViewport[3];
	*bottom = webViewport[4]; *left = webViewport[5];
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
	*x = pointerX * gWindowWidth;
	*y = pointerY * gWindowHeight;
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

int WebGetKey(int key)
{
	int button;
	switch (key)
	{
		case kKey_Forward: case kKey_UI_CharMM: button = WEB_UP; break;
		case kKey_Backward: case kKey_UI_CharPP: button = WEB_DOWN; break;
		case kKey_Left: case kKey_UI_CharLeft: button = WEB_LEFT; break;
		case kKey_Right: case kKey_UI_CharRight: button = WEB_RIGHT; break;
		case kKey_Jump: button = WEB_JUMP; break;
		case kKey_Kick: button = WEB_KICK; break;
		case kKey_MorphPlayer: button = WEB_ROLL; break;
		case kKey_BuddyAttack: button = WEB_BUDDY; break;
		case kKey_Pause: case kKey_UI_Cancel: button = WEB_PAUSE; break;
		case kKey_UI_Confirm: case kKey_UI_Skip: case kKey_UI_PadConfirm:
			button = WEB_CONFIRM; break;
		case kKey_SwivelCameraLeft: button = WEB_CAMERA_LEFT; break;
		case kKey_SwivelCameraRight: button = WEB_CAMERA_RIGHT; break;
		case kKey_ZoomIn: button = WEB_ZOOM_IN; break;
		case kKey_ZoomOut: button = WEB_ZOOM_OUT; break;
		default: return false;
	}
	return held[button] || pressed[button];
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

void WebEndFrame(void)
{
	UpdateBrowserUI(gIsInGame, gIsGamePaused);
	// Every screen (including fades and loading screens) presents through here.
	// Asyncify preserves the original nested loops while the browser paints.
	emscripten_sleep(16);
}
#endif
