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
	TQ3Point2D pos;
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

#define SDLKEYSTATEBUF_SIZE SDL_SCANCODE_COUNT

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

SDL_Gamepad			*gSDLGamepad = NULL;

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
[kKey_Pause				] = { "Pause",				SDL_SCANCODE_ESCAPE,	0,						0,					SDL_GAMEPAD_BUTTON_START, },
[kKey_ToggleMusic		] = { "Toggle Music",		SDL_SCANCODE_M,			0,						0,					SDL_GAMEPAD_BUTTON_INVALID, },
[kKey_SwivelCameraLeft	] = { "Swivel Camera Left",	SDL_SCANCODE_COMMA,		0,						0,					SDL_GAMEPAD_BUTTON_INVALID, },
[kKey_SwivelCameraRight	] = { "Swivel Camera Right",SDL_SCANCODE_PERIOD,	0,						0,					SDL_GAMEPAD_BUTTON_INVALID, },
[kKey_ZoomIn			] = { "Zoom In",			SDL_SCANCODE_2,			0,						0,					SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, },
[kKey_ZoomOut			] = { "Zoom Out",			SDL_SCANCODE_1,			0,						0,					SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, },
[kKey_MorphPlayer		] = { "Morph Into Ball",	SDL_SCANCODE_SPACE,		0,						SDL_BUTTON_MIDDLE,	SDL_GAMEPAD_BUTTON_EAST, },
[kKey_BuddyAttack		] = { "Buddy Attack",		SDL_SCANCODE_TAB,		0,						0,					SDL_GAMEPAD_BUTTON_NORTH, },
[kKey_Jump				] = { "Jump / Boost",		DEFAULT_JUMP_SCANCODE1,	DEFAULT_JUMP_SCANCODE2,	SDL_BUTTON_RIGHT,	SDL_GAMEPAD_BUTTON_SOUTH, },
[kKey_Kick				] = { "Kick / Boost",		DEFAULT_KICK_SCANCODE1, DEFAULT_KICK_SCANCODE2, SDL_BUTTON_LEFT,	SDL_GAMEPAD_BUTTON_WEST, },
[kKey_AutoWalk			] = { "Auto-Walk",			SDL_SCANCODE_LSHIFT,	SDL_SCANCODE_RSHIFT,	0,					SDL_GAMEPAD_BUTTON_INVALID, },
[kKey_Forward			] = { "Forward",			SDL_SCANCODE_UP,		SDL_SCANCODE_W,			0,					SDL_GAMEPAD_BUTTON_DPAD_UP, },
[kKey_Backward			] = { "Backward",			SDL_SCANCODE_DOWN,		SDL_SCANCODE_S,			0,					SDL_GAMEPAD_BUTTON_DPAD_DOWN, },
[kKey_Left				] = { "Left",				SDL_SCANCODE_LEFT,		SDL_SCANCODE_A,			0,					SDL_GAMEPAD_BUTTON_DPAD_LEFT, },
[kKey_Right				] = { "Right",				SDL_SCANCODE_RIGHT,		SDL_SCANCODE_D,			0,					SDL_GAMEPAD_BUTTON_DPAD_RIGHT, },
[kKey_UI_Confirm		] = { "DO_NOT_REBIND",		SDL_SCANCODE_RETURN,	SDL_SCANCODE_KP_ENTER,	0,					SDL_GAMEPAD_BUTTON_INVALID, },
[kKey_UI_Cancel			] = { "DO_NOT_REBIND",		SDL_SCANCODE_ESCAPE,	0,						0,					SDL_GAMEPAD_BUTTON_INVALID, },
[kKey_UI_Skip			] = { "DO_NOT_REBIND",		SDL_SCANCODE_SPACE,		0,						0,					SDL_GAMEPAD_BUTTON_INVALID, },
[kKey_UI_PadConfirm		] = { "DO_NOT_REBIND",		0,						0,						0,					SDL_GAMEPAD_BUTTON_SOUTH, },
[kKey_UI_PadCancel		] = { "DO_NOT_REBIND",		0,						0,						0,					SDL_GAMEPAD_BUTTON_EAST, },
[kKey_UI_PadBack		] = { "DO_NOT_REBIND",		0,						0,						0,					SDL_GAMEPAD_BUTTON_BACK, },
[kKey_UI_CharMM			] = { "DO_NOT_REBIND",		SDL_SCANCODE_UP,		0,						0,					SDL_GAMEPAD_BUTTON_DPAD_UP, },
[kKey_UI_CharPP			] = { "DO_NOT_REBIND",		SDL_SCANCODE_DOWN,		0,						0,					SDL_GAMEPAD_BUTTON_DPAD_DOWN, },
[kKey_UI_CharLeft		] = { "DO_NOT_REBIND",		SDL_SCANCODE_LEFT,		0,						0,					SDL_GAMEPAD_BUTTON_DPAD_LEFT, },
[kKey_UI_CharRight		] = { "DO_NOT_REBIND",		SDL_SCANCODE_RIGHT,		0,						0,					SDL_GAMEPAD_BUTTON_DPAD_RIGHT, },
[kKey_UI_CharDelete		] = { "DO_NOT_REBIND",		SDL_SCANCODE_BACKSPACE,	0,						0,					SDL_GAMEPAD_BUTTON_EAST, },
[kKey_UI_CharDeleteFwd	] = { "DO_NOT_REBIND",		SDL_SCANCODE_DELETE,	0,						0,					SDL_GAMEPAD_BUTTON_INVALID, },
[kKey_UI_CharOK			] = { "DO_NOT_REBIND",		SDL_SCANCODE_DELETE,	0,						0,					SDL_GAMEPAD_BUTTON_SOUTH, },
};



#pragma mark -


/************************* WE ARE FRONT PROCESS ******************************/

static Boolean WeAreFrontProcess(void)
{
	return 0 != (SDL_GetWindowFlags(gSDLWindow) & SDL_WINDOW_INPUT_FOCUS);
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
	if (!gSDLGamepad)
	{
		return (TQ3Vector2D) { 0, 0 };
	}

	Sint16 dxRaw = SDL_GetGamepadAxis(gSDLGamepad, rightStick ? SDL_GAMEPAD_AXIS_RIGHTX : SDL_GAMEPAD_AXIS_LEFTX);
	Sint16 dyRaw = SDL_GetGamepadAxis(gSDLGamepad, rightStick ? SDL_GAMEPAD_AXIS_RIGHTY : SDL_GAMEPAD_AXIS_LEFTY);

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
			bool downNow = mouseButtons & SDL_BUTTON_MASK(i);
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

	if (gSDLGamepad)
	{
		TQ3Vector2D rsVec = GetThumbStickVector(true);
		gCameraControlDelta.x -= rsVec.x * 3.0f;
		gCameraControlDelta.y += rsVec.y * 3.0f;
	}

	if (GetKeyState(kKey_SwivelCameraLeft))
		gCameraControlDelta.x -= 2.0f;

	if (GetKeyState(kKey_SwivelCameraRight))
		gCameraControlDelta.x += 2.0f;
}


/**************** CLEAR STATE *************/

static void ClearMouseState(void)
{
	MouseSmoothing_ResetState();
	SDL_memset(gMouseButtonState, KEYSTATE_IGNOREHELD, sizeof(gMouseButtonState));
}

void ResetInputState(void)
{
	_Static_assert(1 == sizeof(KeyState), "sizeof(KeyState) has changed -- Rewrite this function without SDL_memset()!");

	SDL_memset(gKeyStates, KEYSTATE_IGNOREHELD, kKey_MAX);
	SDL_memset(gRawKeyboardState, KEYSTATE_IGNOREHELD, SDL_SCANCODE_COUNT);
	SDL_memset(gMouseButtonState, KEYSTATE_IGNOREHELD, sizeof(gMouseButtonState));

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
	const bool* keystate = SDL_GetKeyboardState(&numkeys);
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
			downNow |= 0 != (mouseButtons & SDL_BUTTON_MASK(kb->mouseButton));

		if (gSDLGamepad && kb->gamepadButton != SDL_GAMEPAD_BUTTON_INVALID)
			downNow |= 0 != SDL_GetGamepadButton(gSDLGamepad, kb->gamepadButton);

		UpdateKeyState(&gKeyStates[i], downNow);
	}


		/*****************************************************/
		/* WHILE WE'RE HERE LET'S DO SOME SPECIAL KEY CHECKS */
		/*****************************************************/

	if (GetNewKeyState_SDL(SDL_SCANCODE_RETURN)
		&& (GetKeyState_SDL(SDL_SCANCODE_LALT) || GetKeyState_SDL(SDL_SCANCODE_RALT)))
	{
		gGamePrefs.fullscreen = gGamePrefs.fullscreen ? 0 : 1;
		SetFullscreenMode(false);

		ResetInputState();
	}

	if ((!gIsInGame || gIsGamePaused) && IsCmdQPressed())
	{
		CleanQuit();
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
//		|| GetNewKeyState(kKey_UI_PadBack)
		|| GetNewKeyState(kKey_UI_PadCancel)
		|| FlushMouseButtonPress();
}

/******* DID USER PRESS CMD+Q (MAC ONLY) *******/

Boolean IsCmdQPressed(void)
{
#if __APPLE__
	return (GetKeyState_SDL(SDL_SCANCODE_LGUI) || GetKeyState_SDL(SDL_SCANCODE_RGUI))
		&& GetNewKeyState_SDL(SDL_GetScancodeFromKey(SDLK_Q, NULL));
#else
	return false;
#endif
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
	SDL_SetWindowMouseGrab(gSDLWindow, doCapture);
	SDL_SetWindowRelativeMouseMode(gSDLWindow, doCapture);

	if (doCapture)
		SDL_HideCursor();

	ClearMouseState();
	EatMouseEvents();

	SetMacLinearMouse(doCapture);
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

	if (gSDLGamepad)
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
	float mdx, mdy;
	MouseSmoothing_GetDelta(&mdx, &mdy);

	if (mdx != 0 && mdy != 0)
	{
		int mouseDeltaMagnitudeSquared = mdx*mdx + mdy*mdy;
		if (mouseDeltaMagnitudeSquared > MOUSE_DELTA_MAX_SQUARED)
		{
#if _DEBUG
			SDL_Log("Capping mouse delta %f\n", sqrtf(mouseDeltaMagnitudeSquared));
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

static SDL_Gamepad* TryOpenGamepadFromJoystick(SDL_JoystickID joystickID)
{
	// First, check that it's not in use already
	if (gSDLGamepad && SDL_GetGamepadID(gSDLGamepad) == joystickID)
	{
		return gSDLGamepad;
	}

	// Don't open if we already have a gamepad
	if (gSDLGamepad)
	{
		return NULL;
	}

	// If we can't get an SDL_Gamepad from that joystick, don't bother
	if (!SDL_IsGamepad(joystickID))
	{
		return NULL;
	}

	// Use this one
	gSDLGamepad = SDL_OpenGamepad(joystickID);

	SDL_Log("Opened joystick %d as gamepad: \"%s\"", joystickID, SDL_GetGamepadName(gSDLGamepad));

//	gUserPrefersGamepad = true;

	return gSDLGamepad;
}

SDL_Gamepad* TryOpenGamepad(bool showMessage)
{
	int numJoysticks = 0;
	int numJoysticksAlreadyInUse = 0;

	SDL_JoystickID* joysticks = SDL_GetJoysticks(&numJoysticks);
	SDL_Gamepad* newGamepad = NULL;

	for (int i = 0; i < numJoysticks; ++i)
	{
		SDL_JoystickID joystickID = joysticks[i];

		// Usable as an SDL_Gamepad?
		if (!SDL_IsGamepad(joystickID))
		{
			continue;
		}

		// Already in use?
		if (gSDLGamepad && SDL_GetGamepadID(gSDLGamepad) == joystickID)
		{
			numJoysticksAlreadyInUse++;
			continue;
		}

		// Use this one
		newGamepad = TryOpenGamepadFromJoystick(joystickID);
		if (newGamepad)
		{
			break;
		}
	}

	if (newGamepad)
	{
		// OK
	}
	else if (numJoysticksAlreadyInUse == numJoysticks)
	{
		// No-op; All joysticks already in use (or there might be zero joysticks)
	}
	else
	{
		SDL_Log("%d joysticks found, but none is suitable as an SDL_Gamepad.", numJoysticks);
		if (showMessage)
		{
			char messageBuf[1024];
			SDL_snprintf(messageBuf, sizeof(messageBuf),
						 "The game does not support your controller yet (\"%s\").\n\n"
						 "You can play with the keyboard and mouse instead. Sorry!",
						 SDL_GetJoystickNameForID(joysticks[0]));
			SDL_ShowSimpleMessageBox(
				SDL_MESSAGEBOX_WARNING,
				"Controller not supported",
				messageBuf,
				gSDLWindow);
		}
	}

	SDL_free(joysticks);

	return newGamepad;
}

void OnJoystickRemoved(SDL_JoystickID which)
{
	if (NULL == gSDLGamepad)		// don't care, I didn't open any controller
		return;

	if (which != SDL_GetGamepadID(gSDLGamepad))	// don't care, this isn't the joystick I'm using
		return;

	SDL_Log("Current joystick was removed: %d", which);

	// Nuke reference to this controller+joystick
	SDL_CloseGamepad(gSDLGamepad);
	gSDLGamepad = NULL;

	// Try to open another joystick if any is connected.
	TryOpenGamepad(false);
}

#pragma mark -

/***************** ANALOG MOUSE CURSOR *****************/

TQ3Point2D GetMousePosition(void)
{
	float windowX = 0;
	float windowY = 0;
	SDL_GetMouseState(&windowX, &windowY);

	// On macOS, the mouse position is relative to the window's "point size" on Retina screens.
	int windowW = 1;
	int windowH = 1;
	SDL_GetWindowSize(gSDLWindow, &windowW, &windowH);		// NOT InPixels!
	float dpiScaleX = (float) gWindowWidth / windowW;		// gWindowWidth is in actual pixels
	float dpiScaleY = (float) gWindowHeight / windowH;		// gWindowHeight is in actual pixels

	return (TQ3Point2D) { windowX * dpiScaleX, windowY * dpiScaleY };
}

void InitAnalogCursor(void)
{
	SDL_memset(&gAnalogCursor, 0, sizeof(gAnalogCursor));

	gAnalogCursor.pos = GetMousePosition();
}

void ShutdownAnalogCursor(void)
{
	SDL_HideCursor();
}

bool MoveAnalogCursor(int* outMouseX, int* outMouseY)
{
	SDL_ShowCursor();

	gAnalogCursor.didMove = false;

	TQ3Vector2D thumbstick = GetThumbStickVector(false);

	TQ3Point2D newMouse = GetMousePosition();
	bool mouseMoved = gAnalogCursor.pos.x != newMouse.x || gAnalogCursor.pos.y != newMouse.y;

	float speed = 300;
	if (gWindowWidth > gWindowHeight)
		speed *= (gWindowHeight / 480.0f);
	else
		speed *= (gWindowWidth / 640.0f);

	if (mouseMoved)
	{
		gAnalogCursor.steeredByThumbstick = false;
		gAnalogCursor.didMove = true;
		gAnalogCursor.pos = newMouse;
	}
	else if (thumbstick.x != 0 && thumbstick.y != 0)
	{
		gAnalogCursor.steeredByThumbstick = true;
		gAnalogCursor.didMove = true;

		gAnalogCursor.pos.x += thumbstick.x * gFramesPerSecondFrac * speed;
		gAnalogCursor.pos.y += thumbstick.y * gFramesPerSecondFrac * speed;

		if (gAnalogCursor.pos.x < 0) gAnalogCursor.pos.x = 0;
		if (gAnalogCursor.pos.y < 0) gAnalogCursor.pos.y = 0;

		if (gAnalogCursor.pos.x > gWindowWidth-1) gAnalogCursor.pos.x = gWindowWidth-1;
		if (gAnalogCursor.pos.y > gWindowHeight-1) gAnalogCursor.pos.y = gWindowHeight-1;

		SDL_WarpMouseInWindow(gSDLWindow, (int) gAnalogCursor.pos.x, (int) gAnalogCursor.pos.y);
	}

	if (outMouseX)
		*outMouseX = (int) gAnalogCursor.pos.x;

	if (outMouseY)
		*outMouseY = (int) gAnalogCursor.pos.y;

	return gAnalogCursor.didMove;
}

bool IsAnalogCursorClicked(void)
{
	return FlushMouseButtonPress()
		|| (gAnalogCursor.steeredByThumbstick && GetNewKeyState(kKey_UI_PadConfirm));
}

void WarpMouseToCenter(void)
{
	// Can't use gWindowWidth/Height because that's the GL drawable size in physical pixels.
	// Mouse cursor coords must be given in the "point" coordinate system on macOS.
	int windowPointWidth = 1;
	int windowPointHeight = 1;
	SDL_GetWindowSize(gSDLWindow, &windowPointWidth, &windowPointHeight);		// get window size in "device points"

	SDL_WarpMouseInWindow(gSDLWindow, windowPointWidth/2, windowPointHeight/2);
}
