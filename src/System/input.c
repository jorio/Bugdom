/****************************/
/*   	  INPUT.C	   	    */
/* By Brian Greenstone      */
/* (c)2006 Pangea Software  */
/* (c)2021 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"
#include <stdio.h>
#include <string.h>

#if __APPLE__
#include "killmacmouseacceleration.h"
#endif


/**********************/
/*     PROTOTYPES     */
/**********************/

typedef uint8_t KeyState;

static Boolean WeAreFrontProcess(void);

static void ClearMouseState(void);

typedef struct KeyBinding
{
	const char* name;
	int key1;
	int key2;
	int mouseButton;
	int gamepadButton;
} KeyBinding;

static struct
{
	int mouseX;
	int mouseY;
	float thumbX;
	float thumbY;
	bool didMove;
	bool steeredByThumbstick;
} gAnalogCursor;


/****************************/
/*    CONSTANTS             */
/****************************/

#define NUM_MOUSE_BUTTONS 6

#define kJoystickDeadZoneFrac .33f
#define kJoystickDeadZoneFracSquared (kJoystickDeadZoneFrac*kJoystickDeadZoneFrac)

#define MOUSE_DELTA_MAX 250
#define MOUSE_DELTA_MAX_SQUARED (MOUSE_DELTA_MAX*MOUSE_DELTA_MAX)

#define SDLKEYSTATEBUF_SIZE SDL_NUM_SCANCODES

static const float kMouseSensitivityTable[NUM_MOUSE_SENSITIVITY_LEVELS] =
{
	 12 * 0.001f,
	 25 * 0.001f,
	 50 * 0.001f,
	 75 * 0.001f,
	100 * 0.001f,
};

enum
{
	KEYSTATE_ACTIVE_BIT		= 0b001,
	KEYSTATE_CHANGE_BIT		= 0b010,
	KEYSTATE_IGNORE_BIT		= 0b100,

	KEYSTATE_OFF			= 0b000,
	KEYSTATE_PRESSED		= KEYSTATE_ACTIVE_BIT | KEYSTATE_CHANGE_BIT,
	KEYSTATE_HELD			= KEYSTATE_ACTIVE_BIT,
	KEYSTATE_UP				= KEYSTATE_OFF | KEYSTATE_CHANGE_BIT,
	KEYSTATE_IGNOREHELD		= KEYSTATE_OFF | KEYSTATE_IGNORE_BIT,
};


/**********************/
/*     VARIABLES      */
/**********************/

long				gEatMouse = 0;
static KeyState		gMouseButtonState[NUM_MOUSE_BUTTONS];
static KeyState		gKeyStates[kKey_MAX];
static KeyState		gRawKeyboardState[SDLKEYSTATEBUF_SIZE];

Boolean				gPlayerUsingKeyControl 	= false;

TQ3Vector2D			gCameraControlDelta;

SDL_GameController	*gSDLController = NULL;
SDL_JoystickID		gSDLJoystickInstanceID = -1;		// ID of the joystick bound to gSDLController

#if __APPLE__
	#define DEFAULT_JUMP_SCANCODE1 SDL_SCANCODE_LGUI
	#define DEFAULT_JUMP_SCANCODE2 SDL_SCANCODE_RGUI
	#define DEFAULT_KICK_SCANCODE1 SDL_SCANCODE_LALT
	#define DEFAULT_KICK_SCANCODE2 SDL_SCANCODE_RALT
#else
	#define DEFAULT_JUMP_SCANCODE1 SDL_SCANCODE_LALT
	#define DEFAULT_JUMP_SCANCODE2 SDL_SCANCODE_RALT
	#define DEFAULT_KICK_SCANCODE1 SDL_SCANCODE_LCTRL
	#define DEFAULT_KICK_SCANCODE2 SDL_SCANCODE_RCTRL
#endif

KeyBinding gKeyBindings[kKey_MAX] =
{
[kKey_Pause				] = { "Pause",				SDL_SCANCODE_ESCAPE,	0,						0,					SDL_CONTROLLER_BUTTON_START, },
[kKey_ToggleMusic		] = { "Toggle Music",		SDL_SCANCODE_M,			0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_SwivelCameraLeft	] = { "Swivel Camera Left",	SDL_SCANCODE_COMMA,		0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_SwivelCameraRight	] = { "Swivel Camera Right",SDL_SCANCODE_PERIOD,	0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_ZoomIn			] = { "Zoom In",			SDL_SCANCODE_2,			0,						0,					SDL_CONTROLLER_BUTTON_LEFTSHOULDER, },
[kKey_ZoomOut			] = { "Zoom Out",			SDL_SCANCODE_1,			0,						0,					SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, },
[kKey_MorphPlayer		] = { "Morph Into Ball",	SDL_SCANCODE_SPACE,		0,						SDL_BUTTON_MIDDLE,	SDL_CONTROLLER_BUTTON_B, },
[kKey_BuddyAttack		] = { "Buddy Attack",		SDL_SCANCODE_TAB,		0,						0,					SDL_CONTROLLER_BUTTON_Y, },
[kKey_Jump				] = { "Jump / Boost",		DEFAULT_JUMP_SCANCODE1,	DEFAULT_JUMP_SCANCODE2,	SDL_BUTTON_RIGHT,	SDL_CONTROLLER_BUTTON_A, },
[kKey_Kick				] = { "Kick / Boost",		DEFAULT_KICK_SCANCODE1, DEFAULT_KICK_SCANCODE2, SDL_BUTTON_LEFT,	SDL_CONTROLLER_BUTTON_X, },
[kKey_AutoWalk			] = { "Auto-Walk",			SDL_SCANCODE_LSHIFT,	SDL_SCANCODE_RSHIFT,	0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_Forward			] = { "Forward",			SDL_SCANCODE_UP,		SDL_SCANCODE_W,			0,					SDL_CONTROLLER_BUTTON_DPAD_UP, },
[kKey_Backward			] = { "Backward",			SDL_SCANCODE_DOWN,		SDL_SCANCODE_S,			0,					SDL_CONTROLLER_BUTTON_DPAD_DOWN, },
[kKey_Left				] = { "Left",				SDL_SCANCODE_LEFT,		SDL_SCANCODE_A,			0,					SDL_CONTROLLER_BUTTON_DPAD_LEFT, },
[kKey_Right				] = { "Right",				SDL_SCANCODE_RIGHT,		SDL_SCANCODE_D,			0,					SDL_CONTROLLER_BUTTON_DPAD_RIGHT, },
[kKey_UI_Confirm		] = { "DO_NOT_REBIND",		SDL_SCANCODE_RETURN,	SDL_SCANCODE_KP_ENTER,	0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_UI_Cancel			] = { "DO_NOT_REBIND",		SDL_SCANCODE_ESCAPE,	0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_UI_Skip			] = { "DO_NOT_REBIND",		SDL_SCANCODE_SPACE,		0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_UI_Start			] = { "DO_NOT_REBIND",		0,						0,						0,					SDL_CONTROLLER_BUTTON_START, },
[kKey_UI_PadConfirm		] = { "DO_NOT_REBIND",		0,						0,						0,					SDL_CONTROLLER_BUTTON_A, },
[kKey_UI_PadCancel		] = { "DO_NOT_REBIND",		0,						0,						0,					SDL_CONTROLLER_BUTTON_B, },
[kKey_UI_PadBack		] = { "DO_NOT_REBIND",		0,						0,						0,					SDL_CONTROLLER_BUTTON_BACK, },
[kKey_UI_CharMM			] = { "DO_NOT_REBIND",		SDL_SCANCODE_UP,		0,						0,					SDL_CONTROLLER_BUTTON_DPAD_UP, },
[kKey_UI_CharPP			] = { "DO_NOT_REBIND",		SDL_SCANCODE_DOWN,		0,						0,					SDL_CONTROLLER_BUTTON_DPAD_DOWN, },
[kKey_UI_CharLeft		] = { "DO_NOT_REBIND",		SDL_SCANCODE_LEFT,		0,						0,					SDL_CONTROLLER_BUTTON_DPAD_LEFT, },
[kKey_UI_CharRight		] = { "DO_NOT_REBIND",		SDL_SCANCODE_RIGHT,		0,						0,					SDL_CONTROLLER_BUTTON_DPAD_RIGHT, },
[kKey_UI_CharDelete		] = { "DO_NOT_REBIND",		SDL_SCANCODE_BACKSPACE,	0,						0,					SDL_CONTROLLER_BUTTON_B, },
[kKey_UI_CharDeleteFwd	] = { "DO_NOT_REBIND",		SDL_SCANCODE_DELETE,	0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_UI_CharOK			] = { "DO_NOT_REBIND",		SDL_SCANCODE_DELETE,	0,						0,					SDL_CONTROLLER_BUTTON_A, },
};



#pragma mark -


/************************* WE ARE FRONT PROCESS ******************************/

static Boolean WeAreFrontProcess(void)
{
	//return 0 != (SDL_GetWindowFlags(gSDLWindow) & SDL_WINDOW_INPUT_FOCUS);
    return true; // always true on vita (TODO: check....)
}

static inline void UpdateKeyState(KeyState* state, bool downNow)
{
	switch (*state)	// look at prev state
	{
		case KEYSTATE_HELD:
		case KEYSTATE_PRESSED:
			*state = downNow ? KEYSTATE_HELD : KEYSTATE_UP;
			break;

		case KEYSTATE_OFF:
		case KEYSTATE_UP:
		default:
			*state = downNow ? KEYSTATE_PRESSED : KEYSTATE_OFF;
			break;

		case KEYSTATE_IGNOREHELD:
			*state = downNow ? KEYSTATE_IGNOREHELD : KEYSTATE_OFF;
			break;
	}
}

TQ3Vector2D GetThumbStickVector(bool rightStick)
{
	Sint16 dxRaw = SDL_GameControllerGetAxis(gSDLController, rightStick ? SDL_CONTROLLER_AXIS_RIGHTX : SDL_CONTROLLER_AXIS_LEFTX);
	Sint16 dyRaw = SDL_GameControllerGetAxis(gSDLController, rightStick ? SDL_CONTROLLER_AXIS_RIGHTY : SDL_CONTROLLER_AXIS_LEFTY);

	float dx = dxRaw / 32767.0f;
	float dy = dyRaw / 32767.0f;

	float magnitudeSquared = dx*dx + dy*dy;

	if (magnitudeSquared < kJoystickDeadZoneFracSquared)
	{
		return (TQ3Vector2D) { 0, 0 };
	}
	else
	{
		float magnitude;
		
		if (magnitudeSquared > 1.0f)
		{
			// Cap magnitude -- what's returned by the controller actually lies within a square
			magnitude = 1.0f;
		}
		else
		{
			magnitude = sqrtf(magnitudeSquared);

			// Avoid magnitude bump when thumbstick is pushed past dead zone:
			// Bring magnitude from [kJoystickDeadZoneFrac, 1.0] to [0.0, 1.0].
			magnitude = (magnitude - kJoystickDeadZoneFrac) / (1.0f - kJoystickDeadZoneFrac);
		}

		float angle = atan2f(dy, dx);

		//angle = SnapAngle(angle, kDefaultSnapAngle);

		return (TQ3Vector2D) { cosf(angle) * magnitude, sinf(angle) * magnitude };
	}
}

/********************** UPDATE INPUT ******************************/

void UpdateInput(void)
{

	SDL_PumpEvents();

		/* CHECK FOR NEW MOUSE BUTTONS */

	if (gEatMouse)
	{
		gEatMouse--;
		ClearMouseState();
	}
	else
	{
		MouseSmoothing_StartFrame();

		uint32_t mouseButtons = SDL_GetMouseState(NULL, NULL);

		for (int i = 1; i < NUM_MOUSE_BUTTONS; i++)
		{
			bool downNow = mouseButtons & SDL_BUTTON(i);
			UpdateKeyState(&gMouseButtonState[i], downNow);
		}
	}

		/* UPDATE KEYMAP */
		
	if (WeAreFrontProcess())								// only read keys if we're the front process
		UpdateKeyMap();
	else													// otherwise, just clear it out
		ResetInputState();

	// Assume player using key control if any arrow keys are pressed,
	// otherwise assume mouse movement 
	gPlayerUsingKeyControl =
			   GetKeyState(kKey_Forward)
			|| GetKeyState(kKey_Backward)
			|| GetKeyState(kKey_Left)
			|| GetKeyState(kKey_Right);


		/* UPDATE SWIVEL CAMERA */

	gCameraControlDelta.x = 0;
	gCameraControlDelta.y = 0;

    float xReverser = 1.0;
    if (gGamePrefs.flipCameraHorizontal)
    {
        xReverser = -1.0;
    }

	if (gSDLController)
	{
		TQ3Vector2D rsVec = GetThumbStickVector(true);
		gCameraControlDelta.x -= xReverser*rsVec.x * 3.0f;
		gCameraControlDelta.y += rsVec.y * 3.0f;
	}

	if (GetKeyState(kKey_SwivelCameraLeft))
		gCameraControlDelta.x -= xReverser*2.0f;

	if (GetKeyState(kKey_SwivelCameraRight))
		gCameraControlDelta.x += xReverser*2.0f;
}


/**************** CLEAR STATE *************/

static void ClearMouseState(void)
{
	MouseSmoothing_ResetState();
	memset(gMouseButtonState, KEYSTATE_IGNOREHELD, sizeof(gMouseButtonState));
}

void ResetInputState(void)
{
	_Static_assert(1 == sizeof(KeyState), "sizeof(KeyState) has changed -- Rewrite this function without memset()!");

	memset(gKeyStates, KEYSTATE_IGNOREHELD, kKey_MAX);
	memset(gRawKeyboardState, KEYSTATE_IGNOREHELD, SDL_NUM_SCANCODES);
	memset(gMouseButtonState, KEYSTATE_IGNOREHELD, sizeof(gMouseButtonState));

	MouseSmoothing_ResetState();
	EatMouseEvents();
}

void InvalidateKeyState(int need)
{
	gKeyStates[need] = KEYSTATE_IGNOREHELD;
}



/**************** UPDATE KEY MAP *************/
//
// This reads the KeyMap and sets a bunch of new/old stuff.
//

void UpdateKeyMap(void)
{
	SDL_PumpEvents();
	int numkeys = 0;
	const UInt8* keystate = SDL_GetKeyboardState(&numkeys);
	uint32_t mouseButtons = SDL_GetMouseState(NULL, NULL);

	{
		int minNumKeys = numkeys < SDLKEYSTATEBUF_SIZE ? numkeys : SDLKEYSTATEBUF_SIZE;
		for (int i = 0; i < minNumKeys; i++)
			UpdateKeyState(&gRawKeyboardState[i], keystate[i]);
		for (int i = minNumKeys; i < SDLKEYSTATEBUF_SIZE; i++)
			UpdateKeyState(&gRawKeyboardState[i], false);
	}

	for (int i = 0; i < kKey_MAX; i++)
	{
		const KeyBinding* kb = &gKeyBindings[i];
		
		bool downNow = false;

		if (kb->key1 && kb->key1 < numkeys)
			downNow |= 0 != keystate[kb->key1];
		
		if (kb->key2 && kb->key2 < numkeys)
			downNow |= 0 != keystate[kb->key2];

		if (kb->mouseButton)
			downNow |= 0 != (mouseButtons & SDL_BUTTON(kb->mouseButton));

		if (gSDLController && kb->gamepadButton != SDL_CONTROLLER_BUTTON_INVALID)
			downNow |= 0 != SDL_GameControllerGetButton(gSDLController, kb->gamepadButton);

		UpdateKeyState(&gKeyStates[i], downNow);
	}


		/*****************************************************/
		/* WHILE WE'RE HERE LET'S DO SOME SPECIAL KEY CHECKS */
		/*****************************************************/

	if (GetNewKeyState_SDL(SDL_SCANCODE_RETURN)
		&& (GetKeyState_SDL(SDL_SCANCODE_LALT) || GetKeyState_SDL(SDL_SCANCODE_RALT)))
	{
		gGamePrefs.fullscreen = gGamePrefs.fullscreen ? 0 : 1;
		SetFullscreenMode();

		ResetInputState();
	}

}


/****************** GET KEY STATE ***********/

Boolean GetKeyState(unsigned short key)
{
	return 0 != (gKeyStates[key] & KEYSTATE_ACTIVE_BIT);
}

Boolean GetKeyState_SDL(unsigned short key)
{
	if (key >= SDLKEYSTATEBUF_SIZE)
		return false;
	return 0 != (gRawKeyboardState[key] & KEYSTATE_ACTIVE_BIT);
}


/****************** GET NEW KEY STATE ***********/

Boolean GetNewKeyState(unsigned short key)
{
	GAME_ASSERT(key < kKey_MAX);
	return gKeyStates[key] == KEYSTATE_PRESSED;
}

Boolean GetNewKeyState_SDL(unsigned short key)
{
	if (key >= SDLKEYSTATEBUF_SIZE)
		return false;
	return gRawKeyboardState[key] == KEYSTATE_PRESSED;
}

/******* DOES USER WANT TO SKIP TO NEXT SCREEN *******/

Boolean GetSkipScreenInput(void)
{
	return GetNewKeyState(kKey_UI_Confirm)
		|| GetNewKeyState(kKey_UI_Cancel)
		|| GetNewKeyState(kKey_UI_Skip)
		|| GetNewKeyState(kKey_UI_PadConfirm)
		|| GetNewKeyState(kKey_UI_Start)
//		|| GetNewKeyState(kKey_UI_PadBack)
		|| GetNewKeyState(kKey_UI_PadCancel)
		|| FlushMouseButtonPress();
}

/******* ARE THE CHEAT CODE KEYS PRESSED *******/

Boolean GetCheatKeysInput(void)
{
	return (GetKeyState(kKey_ZoomIn) && GetKeyState(kKey_ZoomOut));
}


#pragma mark -


Boolean FlushMouseButtonPress()
{
	Boolean gotPress = KEYSTATE_PRESSED == gMouseButtonState[SDL_BUTTON_LEFT];
	if (gotPress)
		gMouseButtonState[SDL_BUTTON_LEFT] = KEYSTATE_HELD;
	return gotPress;
}

void EatMouseEvents(void)
{
	gEatMouse = 5;
}

void CaptureMouse(Boolean doCapture)
{
	SDL_PumpEvents();	// Prevent SDL from thinking mouse buttons are stuck as we switch into relative mode
	SDL_SetRelativeMouseMode(doCapture ? SDL_TRUE : SDL_FALSE);

	if (doCapture)
		SDL_ShowCursor(0);

	ClearMouseState();
	EatMouseEvents();

#if __APPLE__
    if (doCapture)
        KillMacMouseAcceleration();
    else
        RestoreMacMouseAcceleration();
#endif
}

/***************** GET MOUSE DELTA *****************/

void GetMouseDelta(float *dx, float *dy)
{
	
		/* SEE IF OVERRIDE MOUSE WITH KEY MOVEMENT */
			
	if (gPlayerUsingKeyControl)
	{
		if (GetKeyState(kKey_Left))
			*dx = -1600.0f * gFramesPerSecondFrac;
		else
		if (GetKeyState(kKey_Right))
			*dx = 1600.0f * gFramesPerSecondFrac;
		else
			*dx = 0;

		if (GetKeyState(kKey_Forward))
			*dy = -1600.0f * gFramesPerSecondFrac;
		else
		if (GetKeyState(kKey_Backward))
			*dy = 1600.0f * gFramesPerSecondFrac;
		else
			*dy = 0;
	
		return;
	}

		/* SEE IF OVERRIDE MOUSE WITH JOYSTICK MOVEMENT */

	if (gSDLController)
	{
		TQ3Vector2D lsVec = GetThumbStickVector(false);
		if (lsVec.x != 0 || lsVec.y != 0)
		{
			*dx = gFramesPerSecondFrac * 1600.0f * lsVec.x;
			*dy = gFramesPerSecondFrac * 1600.0f * lsVec.y;
			return;
		}
	}

		/* GET MOUSE MOVEMENT */

	const float mouseSensitivity = 1600.0f * kMouseSensitivityTable[gGamePrefs.mouseSensitivityLevel];
	int mdx, mdy;
	MouseSmoothing_GetDelta(&mdx, &mdy);

	if (mdx != 0 && mdy != 0)
	{
		int mouseDeltaMagnitudeSquared = mdx*mdx + mdy*mdy;
		if (mouseDeltaMagnitudeSquared > MOUSE_DELTA_MAX_SQUARED)
		{
#if _DEBUG
			printf("Capping mouse delta %f\n", sqrtf(mouseDeltaMagnitudeSquared));
#endif
			TQ3Vector2D mouseDeltaVector = { mdx, mdy };
			Q3Vector2D_Normalize(&mouseDeltaVector, &mouseDeltaVector);
			Q3Vector2D_Scale(&mouseDeltaVector, MOUSE_DELTA_MAX, &mouseDeltaVector);
			mdx = mouseDeltaVector.x;
			mdy = mouseDeltaVector.y;
		}
	}

	*dx = gFramesPerSecondFrac * mdx * mouseSensitivity;
	*dy = gFramesPerSecondFrac * mdy * mouseSensitivity;
}


SDL_GameController* TryOpenController(bool showMessage)
{
	if (gSDLController)
	{
		printf("Already have a valid controller.\n");
		return gSDLController;
	}

	if (SDL_NumJoysticks() == 0)
	{
		return NULL;
	}

	for (int i = 0; gSDLController == NULL && i < SDL_NumJoysticks(); ++i)
	{
		if (SDL_IsGameController(i))
		{
			gSDLController = SDL_GameControllerOpen(i);
			gSDLJoystickInstanceID = SDL_JoystickGetDeviceInstanceID(i);
		}
	}

	if (!gSDLController)
	{
		printf("Joystick(s) found, but none is suitable as an SDL_GameController.\n");
		if (showMessage)
		{
			char messageBuf[1024];
			snprintf(messageBuf, sizeof(messageBuf),
					 "The game does not support your controller yet (\"%s\").\n\n"
					 "You can play with the keyboard and mouse instead. Sorry!",
					 SDL_JoystickNameForIndex(0));
			SDL_ShowSimpleMessageBox(
					SDL_MESSAGEBOX_WARNING,
					"Controller not supported",
					messageBuf,
					gSDLWindow);
		}
		return NULL;
	}

	printf("Opened joystick %d as controller: %s\n", gSDLJoystickInstanceID, SDL_GameControllerName(gSDLController));

	return gSDLController;
}

void OnJoystickRemoved(SDL_JoystickID which)
{
	if (NULL == gSDLController)		// don't care, I didn't open any controller
		return;

	if (which != gSDLJoystickInstanceID)	// don't care, this isn't the joystick I'm using
		return;

	printf("Current joystick was removed: %d\n", which);

	// Nuke reference to this controller+joystick
	SDL_GameControllerClose(gSDLController);
	gSDLController = NULL;
	gSDLJoystickInstanceID = -1;

	// Try to open another joystick if any is connected.
	TryOpenController(false);
}

#pragma mark -

/***************** ANALOG MOUSE CURSOR *****************/

void InitAnalogCursor(void)
{
	memset(&gAnalogCursor, 0, sizeof(gAnalogCursor));

	SDL_GetMouseState(&gAnalogCursor.mouseX, &gAnalogCursor.mouseY);
	gAnalogCursor.thumbX = gAnalogCursor.mouseX;
	gAnalogCursor.thumbY = gAnalogCursor.mouseY;
}

void ShutdownAnalogCursor(void)
{
	SDL_ShowCursor(0);
}

bool MoveAnalogCursor(int* outMouseX, int* outMouseY)
{
	SDL_ShowCursor(1);

	gAnalogCursor.didMove = false;

	TQ3Vector2D thumbstick = GetThumbStickVector(false);

	int oldMouseX = gAnalogCursor.mouseX;
	int oldMouseY = gAnalogCursor.mouseY;
	SDL_GetMouseState(&gAnalogCursor.mouseX, &gAnalogCursor.mouseY);
	bool mouseMoved = oldMouseX != gAnalogCursor.mouseX || oldMouseY != gAnalogCursor.mouseY;

	float speed = 300;
	if (gWindowWidth > gWindowHeight)
		speed *= (gWindowHeight / 480.0f);
	else
		speed *= (gWindowWidth / 640.0f);

	if (mouseMoved)
	{
		gAnalogCursor.steeredByThumbstick = false;
		gAnalogCursor.didMove = true;
		gAnalogCursor.thumbX = gAnalogCursor.mouseX;
		gAnalogCursor.thumbY = gAnalogCursor.mouseY;
	}
	else if (thumbstick.x != 0 && thumbstick.y != 0)
	{
		gAnalogCursor.steeredByThumbstick = true;
		gAnalogCursor.didMove = true;

		gAnalogCursor.thumbX += thumbstick.x * gFramesPerSecondFrac * speed;
		gAnalogCursor.thumbY += thumbstick.y * gFramesPerSecondFrac * speed;

		if (gAnalogCursor.thumbX < 0) gAnalogCursor.thumbX = 0;
		if (gAnalogCursor.thumbY < 0) gAnalogCursor.thumbY = 0;

		if (gAnalogCursor.thumbX > gWindowWidth-1) gAnalogCursor.thumbX = gWindowWidth-1;
		if (gAnalogCursor.thumbY > gWindowHeight-1) gAnalogCursor.thumbY = gWindowHeight-1;

		gAnalogCursor.mouseX = gAnalogCursor.thumbX;
		gAnalogCursor.mouseY = gAnalogCursor.thumbY;

		SDL_WarpMouseInWindow(gSDLWindow, gAnalogCursor.mouseX, gAnalogCursor.mouseY);
	}

	if (outMouseX)
		*outMouseX = (int) gAnalogCursor.mouseX;

	if (outMouseY)
		*outMouseY = (int) gAnalogCursor.mouseY;

	return gAnalogCursor.didMove;
}

bool IsAnalogCursorClicked(void)
{
	return FlushMouseButtonPress()
		|| (gAnalogCursor.steeredByThumbstick && GetNewKeyState(kKey_UI_PadConfirm));
}
