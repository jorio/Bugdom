//
// input.h
//

#pragma once

#include <SDL.h>

#define NUM_MOUSE_SENSITIVITY_LEVELS		5
#define DEFAULT_MOUSE_SENSITIVITY_LEVEL		(NUM_MOUSE_SENSITIVITY_LEVELS/2)

enum
{
	kDragonflySteering_Normal,
	kDragonflySteering_InvertY,
	kDragonflySteering_InvertX,
	kDragonflySteering_InvertXY,
};


			/* ASCII */
			
#define	CHAR_RETURN			0x0d	/* ASCII code for Return key */
#define CHAR_UP				0x1e
#define CHAR_DOWN			0x1f
#define	CHAR_LEFT			0x1c
#define	CHAR_RIGHT			0x1d
#define	CHAR_DELETE			0x08
#define CHAR_FORWARD_DELETE	0x7f


	/* KEYBOARD EQUATE */

enum
{
	kKey_Pause,
	kKey_ToggleMusic,

	kKey_SwivelCameraLeft,
	kKey_SwivelCameraRight,
	kKey_ZoomIn,
	kKey_ZoomOut,

	kKey_MorphPlayer,
	kKey_BuddyAttack,
	kKey_Jump,
	kKey_Kick,

	kKey_AutoWalk,
	kKey_Forward,
	kKey_Backward,
	kKey_Left,
	kKey_Right,

	kKey_UI_Confirm,
	kKey_UI_Skip,
	kKey_UI_Cancel,
	kKey_UI_PadConfirm,
	kKey_UI_PadCancel,
	kKey_UI_PadBack,

	kKey_UI_CharMM,
	kKey_UI_CharPP,
	kKey_UI_CharLeft,
	kKey_UI_CharRight,
	kKey_UI_CharDelete,
	kKey_UI_CharDeleteFwd,
	kKey_UI_CharOK,

	kKey_MAX
};



//============================================================================================


void UpdateInput(void);
Boolean GetNewKeyState(unsigned short key);
Boolean GetKeyState_SDL(unsigned short sdlScanCode);
Boolean GetKeyState(unsigned short key);
Boolean GetNewKeyState_SDL(unsigned short sdlScanCode);
Boolean GetSkipScreenInput(void);
Boolean IsCmdQPressed(void);
void ResetInputState(void);
void UpdateKeyMap(void);

Boolean FlushMouseButtonPress(void);
void EatMouseEvents(void);
void GetMouseDelta(float *dx, float *dy);

void CaptureMouse(Boolean doCapture);

SDL_GameController* TryOpenController(bool showMessageOnFailure);
void OnJoystickRemoved(SDL_JoystickID which);
TQ3Vector2D GetThumbStickVector(bool rightStick);

TQ3Point2D GetMousePosition(void);
void InitAnalogCursor(void);
void ShutdownAnalogCursor(void);
bool MoveAnalogCursor(int* outMouseX, int* outMouseY);
bool IsAnalogCursorClicked(void);
void WarpMouseToCenter(void);

void SetMacLinearMouse(int linear);
