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

/**********************/
/*     PROTOTYPES     */
/**********************/

static Boolean WeAreFrontProcess(void);

static void ClearMouseState(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define NUM_MOUSE_BUTTONS 5

static const float kMouseSensitivityTable[NUM_MOUSE_SENSITIVITY_LEVELS] =
{
	 12 * 0.001f,
	 25 * 0.001f,
	 50 * 0.001f,
	 75 * 0.001f,
	100 * 0.001f,
};

static const int kMouseDeltaMax = 250;

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
static Byte			gMouseButtonState[NUM_MOUSE_BUTTONS] = {KEY_OFF, KEY_OFF, KEY_OFF, KEY_OFF, KEY_OFF};


static KeyMapByteArray gKeyMap,gNewKeys,gOldKeys;


Boolean	gPlayerUsingKeyControl 	= false;






#pragma mark -


/************************* WE ARE FRONT PROCESS ******************************/

static Boolean WeAreFrontProcess(void)
{
	return 0 != (SDL_GetWindowFlags(gSDLWindow) & SDL_WINDOW_INPUT_FOCUS);
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

		for (int i = 0; i < NUM_MOUSE_BUTTONS; i++)
		{
			Byte prevState = gMouseButtonState[i];
			bool downNow = mouseButtons & SDL_BUTTON(i+1);
			switch (prevState)
			{
			case KEY_HELD:
			case KEY_DOWN:
				gMouseButtonState[i] = downNow ? KEY_HELD : KEY_UP;
				break;
			case KEY_OFF:
			case KEY_UP:
			default:
				gMouseButtonState[i] = downNow ? KEY_DOWN : KEY_OFF;
				break;
			}
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
}


/**************** CLEAR STATE *************/

static void ClearMouseState(void)
{
	MouseSmoothing_ResetState();
	memset(gMouseButtonState, KEY_OFF, sizeof(gMouseButtonState));
}

void ResetInputState(void)
{
	memset(gKeyMap, 0, sizeof(gKeyMap));
	memset(gNewKeys, 0, sizeof(gNewKeys));
	ClearMouseState();
	EatMouseEvents();
}


/**************** UPDATE KEY MAP *************/
//
// This reads the KeyMap and sets a bunch of new/old stuff.
//

void UpdateKeyMap(void)
{
int	i;

	GetKeys((void *)gKeyMap);										

			/* CALC WHICH KEYS ARE NEW THIS TIME */
		
	for (i = 0; i <  16; i++)
		gNewKeys[i] = (gOldKeys[i] ^ gKeyMap[i]) & gKeyMap[i];


			/* REMEMBER AS OLD MAP */
	
	memcpy(gOldKeys, gKeyMap, sizeof(gKeyMap));
	
	
	

		/*****************************************************/
		/* WHILE WE'RE HERE LET'S DO SOME SPECIAL KEY CHECKS */
		/*****************************************************/

	if (GetNewKeyState(kKey_ToggleFullscreen))
	{
		ResetInputState();
		gGamePrefs.fullscreen = gGamePrefs.fullscreen ? 0 : 1;
		SetFullscreenMode();
	}

#if 0
				/* SEE IF QUIT GAME */
				
	if (GetKeyState(KEY_Q) && GetKeyState(KEY_APPLE))			// see if real key quit
		CleanQuit();	
#endif

}


/****************** GET KEY STATE ***********/

Boolean GetKeyState(unsigned short key)
{
	return ( ( gKeyMap[key>>3] >> (key & 7) ) & 1);
}


/****************** GET NEW KEY STATE ***********/

Boolean GetNewKeyState(unsigned short key)
{
	if (key == kKey_KickBoost   && KEY_DOWN == gMouseButtonState[SDL_BUTTON_LEFT-1]) return true;
	if (key == kKey_Jump        && KEY_DOWN == gMouseButtonState[SDL_BUTTON_RIGHT-1]) return true;
	if (key == kKey_MorphPlayer && KEY_DOWN == gMouseButtonState[SDL_BUTTON_MIDDLE-1]) return true;

	return ( ( gNewKeys[key>>3] >> (key & 7) ) & 1);
}

/***************** ARE ANY NEW KEYS PRESSED ****************/

Boolean AreAnyNewKeysPressed(void)
{
int		i;

	for (i = 0; i < 16; i++)
	{
		if (gNewKeys[i])
			return(true);
	}

	return(false);
}


#pragma mark -


Boolean FlushMouseButtonPress()
{
	Boolean gotPress = KEY_DOWN == gMouseButtonState[SDL_BUTTON_LEFT - 1];
	if (gotPress)
		gMouseButtonState[SDL_BUTTON_LEFT - 1] = KEY_HELD;
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

	const float mouseSensitivity = 1600.0f * kMouseSensitivityTable[gGamePrefs.mouseSensitivityLevel];
	int mdx, mdy;
	MouseSmoothing_GetDelta(&mdx, &mdy);

	if (mdx != 0 && mdy != 0)
	{
		int mouseDeltaMagnitudeSquared = mdx*mdx + mdy*mdy;
		if (mouseDeltaMagnitudeSquared > kMouseDeltaMax*kMouseDeltaMax)
		{
#if _DEBUG
			printf("Capping mouse delta %f\n", sqrtf(mouseDeltaMagnitudeSquared));
#endif
			TQ3Vector2D mouseDeltaVector = { mdx, mdy };
			Q3Vector2D_Normalize(&mouseDeltaVector, &mouseDeltaVector);
			Q3Vector2D_Scale(&mouseDeltaVector, kMouseDeltaMax, &mouseDeltaVector);
			mdx = mouseDeltaVector.x;
			mdy = mouseDeltaVector.y;
		}
	}

	*dx = gFramesPerSecondFrac * mdx * mouseSensitivity;
	*dy = gFramesPerSecondFrac * mdy * mouseSensitivity;
}




