#ifdef __EMSCRIPTEN__
#include "game.h"

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
	return WebGetButton(button);
}

void WebGameResize(int width, int height) { QD3D_OnWindowResized(width, height); }
void WebEndFrame(void) { WebPresent(gIsInGame, gIsGamePaused); }
#endif
