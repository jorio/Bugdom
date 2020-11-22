/****************************/
/*   	  INPUT.C	   	    */
/* (c)2006 Pangea Software  */
/* By Brian Greenstone      */
/****************************/

#if 0  // Source port removal
#include <mach/mach_port.h>
#endif

/***************/
/* EXTERNALS   */
/***************/

extern	float	gFramesPerSecondFrac;

/**********************/
/*     PROTOTYPES     */
/**********************/

#if 0
static pascal OSStatus MyKeyboardEventHandler(EventHandlerCallRef eventhandler, EventRef pEventRef, void *userdata);
static pascal OSStatus MyMouseEventHandler(EventHandlerCallRef eventhandler, EventRef pEventRef, void *userdata);
#endif
static Boolean WeAreFrontProcess(void);



/****************************/
/*    CONSTANTS             */
/****************************/



/**********************/
/*     VARIABLES      */
/**********************/

#if 0
static	EventHandlerUPP			gMouseEventHandlerUPP = nil;
static	EventHandlerRef			gMouseEventHandlerRef = 0;

static	EventHandlerUPP			gKeyboardEventHandlerUPP = nil;
static	EventHandlerRef			gKeyboardEventHandlerRef = 0;
#endif

long					gMouseDeltaX = 0;
long					gMouseDeltaY = 0;


Boolean		gMouseButtonState[3] = {0,0,0};
Boolean		gNewMouseButtonState[3] = {0,0,0};
static Boolean		gOldMouseButtonState[3] = {0,0,0};


static KeyMapByteArray gKeyMap,gNewKeys,gOldKeys;


Boolean	gPlayerUsingKeyControl 	= true;

Boolean	gKeyStates[255];


/******************* INSTALL MOUSE EVENT HANDLER ***********************/

void InstallMouseEventHandler(void)
{
#if 1
	SOURCE_PORT_MINOR_PLACEHOLDER();
#else
EventTypeSpec			mouseEvents[5] ={{kEventClassMouse, kEventMouseMoved},
										{kEventClassMouse, kEventMouseDragged},
										{kEventClassMouse, kEventMouseUp},
										{kEventClassMouse, kEventMouseDown},
										{kEventClassMouse, kEventMouseWheelMoved},
										};



	// Set up the handler to capture mouse movements.

	if (!gMouseEventHandlerUPP)
		gMouseEventHandlerUPP = NewEventHandlerUPP(MyMouseEventHandler);

	
				/* INSTALL THE EVENT HANDLER */

	InstallEventHandler(GetApplicationEventTarget(), gMouseEventHandlerUPP,	5,
						mouseEvents, nil, &gMouseEventHandlerRef);
#endif
}


/******************* INSTALL KEYBOARD EVENT HANDLER ***********************/

void InstallKeyboardEventHandler(void)
{
#if 1
	SOURCE_PORT_MINOR_PLACEHOLDER();
#else
EventTypeSpec			events[3] ={
										{kEventClassKeyboard, kEventRawKeyDown},
										{kEventClassKeyboard, kEventRawKeyRepeat},
										{kEventClassKeyboard, kEventRawKeyUp},
										
										};



	// Set up the handler to capture mouse movements.

	if (!gKeyboardEventHandlerUPP)
		gKeyboardEventHandlerUPP = NewEventHandlerUPP(MyKeyboardEventHandler);

	
				/* INSTALL THE EVENT HANDLER */

	InstallEventHandler(GetApplicationEventTarget(), gKeyboardEventHandlerUPP,	3,
						events, nil, &gKeyboardEventHandlerRef);
#endif
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


/******************* REMOVE MOUSE EVENT HANDLER *************************/

void RemoveMouseEventHandler(void)
{

#if 1
	SOURCE_PORT_MINOR_PLACEHOLDER();
#else
	//	if the handler has been installed, remove it

	if (gMouseEventHandlerRef != 0)
	{
		RemoveEventHandler(gMouseEventHandlerRef);
		gMouseEventHandlerRef = 0;
		
		DisposeEventHandlerUPP(gMouseEventHandlerUPP);
		gMouseEventHandlerUPP = nil;
	}
	
#endif
}


/******************* REMOVE KEYBOARD EVENT HANDLER *************************/

void RemoveKeyboardEventHandler(void)
{

#if 1
	SOURCE_PORT_MINOR_PLACEHOLDER();
#else
	//	if the handler has been installed, remove it

	if (gKeyboardEventHandlerRef != 0)
	{
		RemoveEventHandler(gKeyboardEventHandlerRef);
		gKeyboardEventHandlerRef = 0;
		
		DisposeEventHandlerUPP(gKeyboardEventHandlerUPP);
		gKeyboardEventHandlerUPP = nil;
	}
	
#endif
}




#pragma mark -


/************************* WE ARE FRONT PROCESS ******************************/

static Boolean WeAreFrontProcess(void)
{
#if 1
	static Boolean once = false;
	if (!once)
	{
		SOURCE_PORT_MINOR_PLACEHOLDER();
		once = true;
	}
	return true;
#else
ProcessSerialNumber	frontProcess, myProcess;
Boolean				same;

	GetFrontProcess(&frontProcess); 					// get the front process
	MacGetCurrentProcess(&myProcess);					// get the current process

	SameProcess(&frontProcess, &myProcess, &same);		// if they're the same then we're in front

	return(same);
#endif
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
	{
		for (i = 0; i < 16; i++)
			gKeyMap[i] = 0;
	}
	

	

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

	for (i = 0; i <  16; i++)
		gOldKeys[i] = gKeyMap[i];
	
	
	
#if 0
		/*****************************************************/
		/* WHILE WE'RE HERE LET'S DO SOME SPECIAL KEY CHECKS */
		/*****************************************************/
	
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

	*dx = gMouseDeltaX;
	*dy = gMouseDeltaY;
}




