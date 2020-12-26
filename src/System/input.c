/****************************/
/*   	  INPUT.C	   	    */
/* (c)2006 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include <string.h>

extern	float	gFramesPerSecondFrac;
extern	PrefsType	gGamePrefs;
extern	SDL_Window* gSDLWindow;
extern 	SDL_GameController* gSDLController;

/**********************/
/*     PROTOTYPES     */
/**********************/

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


/****************************/
/*    CONSTANTS             */
/****************************/

#define NUM_MOUSE_BUTTONS 6

#define JOYSTICK_DEAD_ZONE .1f
#define JOYSTICK_DEAD_ZONE_SQUARED (JOYSTICK_DEAD_ZONE*JOYSTICK_DEAD_ZONE)

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
	KEY_OFF,
	KEY_DOWN,
	KEY_HELD,
	KEY_UP,
};


/**********************/
/*     VARIABLES      */
/**********************/

long				gEatMouse = 0;
static Byte			gMouseButtonState[NUM_MOUSE_BUTTONS];

Boolean				gPlayerUsingKeyControl 	= false;

TQ3Vector2D			gCameraControlDelta;

Byte				gKeyStates[kKey_MAX];
Byte				gRawKeyboardState[SDLKEYSTATEBUF_SIZE];

SDL_GameController	*gSDLController = NULL;
SDL_JoystickID		gSDLJoystickInstanceID = -1;		// ID of the joystick bound to gSDLController
SDL_Haptic			*gSDLHaptic = NULL;

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
[kKey_ToggleFullscreen	] = { "Toggle Fullscreen",	SDL_SCANCODE_F11,		0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_RaiseVolume		] = { "Raise Volume",		SDL_SCANCODE_EQUALS,	0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_LowerVolume		] = { "Lower Volume",		SDL_SCANCODE_MINUS,		0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_SwivelCameraLeft	] = { "Swivel Camera Left",	SDL_SCANCODE_COMMA,		0,						0,					SDL_CONTROLLER_BUTTON_LEFTSHOULDER, },
[kKey_SwivelCameraRight	] = { "Swivel Camera Right",SDL_SCANCODE_PERIOD,	0,						0,					SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, },
[kKey_ZoomIn			] = { "Zoom In",			SDL_SCANCODE_2,			0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_ZoomOut			] = { "Zoom Out",			SDL_SCANCODE_1,			0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_MorphPlayer		] = { "Morph Into Ball",	SDL_SCANCODE_SPACE,		0,						SDL_BUTTON_MIDDLE,	SDL_CONTROLLER_BUTTON_B, },
[kKey_BuddyAttack		] = { "Buddy Attack",		SDL_SCANCODE_TAB,		0,						0,					SDL_CONTROLLER_BUTTON_Y, },
[kKey_Jump				] = { "Jump",				DEFAULT_JUMP_SCANCODE1,	DEFAULT_JUMP_SCANCODE2,	SDL_BUTTON_RIGHT,	SDL_CONTROLLER_BUTTON_A, },
[kKey_KickBoost			] = { "Kick / Boost",		DEFAULT_KICK_SCANCODE1,	DEFAULT_KICK_SCANCODE2,	SDL_BUTTON_LEFT,	SDL_CONTROLLER_BUTTON_X, },
[kKey_AutoWalk			] = { "Auto-Walk",			SDL_SCANCODE_LSHIFT,	SDL_SCANCODE_RSHIFT,	0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_Forward			] = { "Forward",			SDL_SCANCODE_UP,		SDL_SCANCODE_W,			0,					SDL_CONTROLLER_BUTTON_DPAD_UP, },
[kKey_Backward			] = { "Backward",			SDL_SCANCODE_DOWN,		SDL_SCANCODE_S,			0,					SDL_CONTROLLER_BUTTON_DPAD_DOWN, },
[kKey_Left				] = { "Left",				SDL_SCANCODE_LEFT,		SDL_SCANCODE_A,			0,					SDL_CONTROLLER_BUTTON_DPAD_LEFT, },
[kKey_Right				] = { "Right",				SDL_SCANCODE_RIGHT,		SDL_SCANCODE_D,			0,					SDL_CONTROLLER_BUTTON_DPAD_RIGHT, },
[kKey_UI_Confirm		] = { "DO_NOT_REBIND",		SDL_SCANCODE_RETURN,	SDL_SCANCODE_KP_ENTER,	0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_UI_Cancel			] = { "DO_NOT_REBIND",		SDL_SCANCODE_ESCAPE,	0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kKey_UI_Skip			] = { "DO_NOT_REBIND",		SDL_SCANCODE_SPACE,		0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
};



#pragma mark -


/************************* WE ARE FRONT PROCESS ******************************/

static Boolean WeAreFrontProcess(void)
{
	return 0 != (SDL_GetWindowFlags(gSDLWindow) & SDL_WINDOW_INPUT_FOCUS);
}


static inline void UpdateKeyState(Byte* state, bool downNow)
{
	switch (*state)	// look at prev state
	{
	case KEY_HELD:
	case KEY_DOWN:
		*state = downNow ? KEY_HELD : KEY_UP;
		break;
	case KEY_OFF:
	case KEY_UP:
	default:
		*state = downNow ? KEY_DOWN : KEY_OFF;
		break;
	}
}


static TQ3Vector2D GetThumbStickVector(bool rightStick)
{
	Sint16 dxRaw = SDL_GameControllerGetAxis(gSDLController, rightStick ? SDL_CONTROLLER_AXIS_RIGHTX : SDL_CONTROLLER_AXIS_LEFTX);
	Sint16 dyRaw = SDL_GameControllerGetAxis(gSDLController, rightStick ? SDL_CONTROLLER_AXIS_RIGHTY : SDL_CONTROLLER_AXIS_LEFTY);

	float dx = dxRaw / 32767.0f;
	float dy = dyRaw / 32767.0f;

	float magnitudeSquared = dx*dx + dy*dy;
	if (magnitudeSquared < JOYSTICK_DEAD_ZONE_SQUARED)
		return (TQ3Vector2D) { 0, 0 };
	else
		return (TQ3Vector2D) { dx, dy };
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

	if (gSDLController)
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
	memset(gMouseButtonState, KEY_OFF, sizeof(gMouseButtonState));
}

void ResetInputState(void)
{
	memset(gKeyStates, KEY_OFF, sizeof(gKeyStates));
	ClearMouseState();
	EatMouseEvents();
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

	if (GetNewKeyState(kKey_ToggleFullscreen))
	{
		gGamePrefs.fullscreen = gGamePrefs.fullscreen ? 0 : 1;
		SetFullscreenMode();

		ResetInputState();
		gKeyStates[kKey_ToggleFullscreen] = KEY_HELD;
	}

}


/****************** GET KEY STATE ***********/

Boolean GetKeyState(unsigned short key)
{
	return gKeyStates[key] == KEY_HELD || gKeyStates[key] == KEY_DOWN;
}

Boolean GetKeyState_SDL(unsigned short key)
{
	if (key >= SDLKEYSTATEBUF_SIZE)
		return false;
	return gRawKeyboardState[key] == KEY_HELD || gRawKeyboardState[key] == KEY_DOWN;
}


/****************** GET NEW KEY STATE ***********/

Boolean GetNewKeyState(unsigned short key)
{
	return gKeyStates[key] == KEY_DOWN;
}

Boolean GetNewKeyState_SDL(unsigned short key)
{
	if (key >= SDLKEYSTATEBUF_SIZE)
		return false;
	return gRawKeyboardState[key] == KEY_DOWN;
}

Boolean GetSkipScreenInput(void)
{
	return GetNewKeyState(kKey_UI_Confirm)
		|| GetNewKeyState(kKey_UI_Cancel)
		|| GetNewKeyState(kKey_UI_Skip)
		|| FlushMouseButtonPress();
}

/***************** ARE ANY NEW KEYS PRESSED ****************/

Boolean AreAnyNewKeysPressed(void)
{
	for (int i = 0; i < kKey_MAX; i++)
		if (gKeyStates[i] == KEY_DOWN)
			return true;
	return(false);
}


#pragma mark -


Boolean FlushMouseButtonPress()
{
	Boolean gotPress = KEY_DOWN == gMouseButtonState[SDL_BUTTON_LEFT];
	if (gotPress)
		gMouseButtonState[SDL_BUTTON_LEFT] = KEY_HELD;
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
	SDL_ShowCursor(doCapture ? 0 : 1);
	ClearMouseState();
	EatMouseEvents();
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
		printf("No joysticks found.\n");
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

	gSDLHaptic = SDL_HapticOpenFromJoystick(SDL_GameControllerGetJoystick(gSDLController));
	if (!gSDLHaptic)
		printf("This joystick can't do haptic.\n");
	else
		printf("This joystick can do haptic!\n");

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

