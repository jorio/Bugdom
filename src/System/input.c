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



/****************************/
/*    CONSTANTS             */
/****************************/

static const float kMouseSensitivityTable[NUM_MOUSE_SENSITIVITY_LEVELS] =
{
	 12 * 0.001f,
	 25 * 0.001f,
	 50 * 0.001f,
	 75 * 0.001f,
	100 * 0.001f,
};

static const int kMouseDeltaMax = 250;



/**********************/
/*     VARIABLES      */
/**********************/

long					gEatMouse = 0;


Boolean		gMouseButtonState[3] = {0,0,0};
Boolean		gNewMouseButtonState[3] = {0,0,0};
static Boolean		gOldMouseButtonState[3] = {0,0,0};


static KeyMapByteArray gKeyMap,gNewKeys,gOldKeys;


Boolean	gPlayerUsingKeyControl 	= false;

Boolean	gKeyStates[255];




void CaptureMouse(Boolean doCapture)
{
	SDL_SetRelativeMouseMode(doCapture ? SDL_TRUE : SDL_FALSE);
	SDL_ShowCursor(doCapture ? 0 : 1);
	gEatMouse = 5; // eat mouse events for a couple frames
}




#if 0
/**************** MY MOUSE EVENT HANDLER *************************/
//
// Every time WaitNextEvent() is called this callback will be invoked.
//

static pascal OSStatus MyMouseEventHandler(EventHandlerCallRef eventhandler, EventRef pEventRef, void *userdata)
{
OSStatus	result = eventNotHandledErr;
OSStatus	theErr = noErr;
Point		qdPoint;
SInt32		lDelta;
UInt16 		whichButton;

#pragma unused (eventhandler, userdata)

	switch (GetEventClass(pEventRef))
	{
						/*****************************/
						/* HANDLE MOUSE CLASS EVENTS */
						/*****************************/
						
		case	kEventClassMouse:
				switch (GetEventKind(pEventRef))
				{	 
							/* MOUSE DELTA */
							
					case	kEventMouseMoved:
					case	kEventMouseDragged:
							theErr = GetEventParameter(pEventRef, kEventParamMouseDelta, typeQDPoint,
													nil, sizeof(Point), nil, &qdPoint);
						
							gMouseDeltaX = qdPoint.h;
							gMouseDeltaY = qdPoint.v;

							result = noErr;
							break;	
							
							
							/* SCROLL WHEEL */
							
					case	kEventMouseWheelMoved:										
							theErr = GetEventParameter(pEventRef, kEventParamMouseWheelDelta, typeSInt32, nil, sizeof(lDelta), nil, &lDelta);		// Get the delta


							result = noErr;
							break;
							
							
							/* BUTTON DOWN */
							
					case	kEventMouseDown:
							theErr = GetEventParameter(pEventRef, kEventParamMouseButton, typeMouseButton, nil ,sizeof(whichButton), nil, &whichButton); 	// see if 2nd or 3rd button was pressed
							if (theErr == noErr)
							{
								switch(whichButton)
								{
									case	kEventMouseButtonPrimary:
											gMouseButtonState[0] = true;						// left (default) button?
											break;
											
									case	kEventMouseButtonSecondary:							// right button?
											gMouseButtonState[1] = true;
											break;
											
									case	kEventMouseButtonTertiary:							// middle button?
											gMouseButtonState[2] = true;
											break;
								}								
							}

			//				result = noErr;	// NOTE:  DO *NOT* SET RESULT TO NOERR BECAUSE WE WANT THESE CLICKS TO GET PASSED TO THE SYSTEM FOR WINDOW MOVING ETC.
							break;


							/* BUTTON UP */

					case	kEventMouseUp:
							theErr = GetEventParameter(pEventRef, kEventParamMouseButton, typeMouseButton, nil ,sizeof(whichButton), nil, &whichButton); 	// see if 2nd or 3rd button was pressed
							if (theErr == noErr)
							{
								switch(whichButton)
								{
									case	kEventMouseButtonPrimary:
											gMouseButtonState[0] = false;						// left (default) button?
											break;
											
									case	kEventMouseButtonSecondary:							// right button?
											gMouseButtonState[1] = false;
											break;
											
									case	kEventMouseButtonTertiary:							// middle button?
											gMouseButtonState[2] = false;
											break;
								}								
							}
							
			//				result = noErr;	// NOTE:  DO *NOT* SET RESULT TO NOERR BECAUSE WE WANT THESE CLICKS TO GET PASSED TO THE SYSTEM FOR WINDOW MOVING ETC.
							break;
				}	 
				break;
				
				
	}
	
	return(result);
}


/**************** MY KEYBOARD EVENT HANDLER *************************/
//
// Every time WaitNextEvent() is called this callback will be invoked.
//

static pascal OSStatus MyKeyboardEventHandler(EventHandlerCallRef eventhandler, EventRef pEventRef, void *userdata)
{
OSStatus	result = eventNotHandledErr;
char		charCode;
UInt32		modifiers;

#pragma unused (eventhandler, userdata)

	switch (GetEventClass(pEventRef))
	{
					/*******************/
					/* KEYBOARD EVENTS */
					/*******************/
					
		case	kEventClassKeyboard:
				
				modifiers = GetCurrentEventKeyModifiers();			// get modifier bits in case we need them
				
				switch (GetEventKind(pEventRef))
				{	 
				
							/* KEYDOWN */
							
					case	kEventRawKeyDown:
					case	kEventRawKeyRepeat:
							GetEventParameter(pEventRef, kEventParamKeyMacCharCodes, typeChar, nil,
											sizeof(charCode), nil, &charCode);
											
							gKeyStates[charCode] = true;
	
							gPlayerUsingKeyControl = true;									// assume player using key control

							break;
				
						/* KEY UP */
						
					case	kEventRawKeyUp:
							GetEventParameter(pEventRef, kEventParamKeyMacCharCodes, typeChar, nil,
											sizeof(charCode), nil, &charCode);

							gKeyStates[charCode] = false;
							break;
										
																
				}
	}
	
	
	return(result);
}

#endif


#pragma mark -


/************************* WE ARE FRONT PROCESS ******************************/

static Boolean WeAreFrontProcess(void)
{
	return 0 != (SDL_GetWindowFlags(gSDLWindow) & SDL_WINDOW_INPUT_FOCUS);
}


/********************** UPDATE INPUT ******************************/

void UpdateInput(void)
{
short   i;


		/* CHECK FOR NEW MOUSE BUTTONS */

	for (i = 0; i < 3; i++)
	{
		gNewMouseButtonState[i] = gMouseButtonState[i] && (!gOldMouseButtonState[i]);
		gOldMouseButtonState[i] = gMouseButtonState[i];
	}


		/* UPDATE KEYMAP */
		
	if (WeAreFrontProcess())								// only read keys if we're the front process
		UpdateKeyMap();
	else													// otherwise, just clear it out
		ClearKeyState();

	// Assume player using key control if any arrow keys are pressed,
	// otherwise assume mouse movement 
	gPlayerUsingKeyControl =
			   GetKeyState(kKey_Forward)
			|| GetKeyState(kKey_Backward)
			|| GetKeyState(kKey_Left)
			|| GetKeyState(kKey_Right);
}


/**************** CLEAR KEY MAP *************/

void ClearKeyState(void)
{
	memset(gKeyMap, 0, sizeof(gKeyMap));
	memset(gNewKeys, 0, sizeof(gNewKeys));
	memset(gMouseButtonState, 0, sizeof(gMouseButtonState));
	memset(gNewMouseButtonState, 0, sizeof(gNewMouseButtonState));
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
		ClearKeyState();
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
	if (key == kKey_KickBoost   && gNewMouseButtonState[0]) return true;
	if (key == kKey_Jump        && gNewMouseButtonState[1]) return true;
	if (key == kKey_MorphPlayer && gNewMouseButtonState[2]) return true;

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


/***************** GET MOUSE COORD *****************/

void GetMouseCoord(Point *point)
{

		GetMouse(point);

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




