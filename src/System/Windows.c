/****************************/
/*        WINDOWS           */
/* (c)1997-99 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/



extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode	*gCurrentNode,*gFirstNodePtr;
extern	float	gFramesPerSecondFrac,gAdditionalClipping;
extern	short	gPrefsFolderVRefNum;
extern	long	gPrefsFolderDirID;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void PrepDrawSprockets(void);
static void MoveFadeEvent(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	ALLOW_FADE		0

#if 0
#define MONITORS_TO_FADE	gDisplayContext
#else
#define MONITORS_TO_FADE	nil
#endif

/**********************/
/*     VARIABLES      */
/**********************/

long					gScreenXOffset,gScreenYOffset;
extern WindowPtr		gCoverWindow;
#if 0  // Source port removal
DSpContextReference 	gDisplayContext = nil;
#endif
Boolean					gLoadedDrawSprocket = false;


float		gGammaFadePercent;

/****************  INIT WINDOW STUFF *******************/

void InitWindowStuff(void)
{
OSErr			iErr;


#if 1
	SOURCE_PORT_MINOR_PLACEHOLDER();
#else
#if 0
			/* INIT WITH DRAW SPROCKETS */
			
	PrepDrawSprockets();


				/* CLEAR SCREEN & MAKE WINDOW */
				
	iErr = DSpContext_GetFrontBuffer(gDisplayContext,(CGrafPtr *)&gCoverWindow);
	if (iErr)
		DoFatalAlert("InitWindowStuff: DSpContext_GetFrontBuffer failed!");
#else

	{
		Rect	r;
		
		SetRect(&r, 0,0, 1024, 768);
		gCoverWindow = NewCWindow(0L, &r, "Bugdom", true, noGrowDocProc, (WindowPtr) - 1L, true, 0);

	}


#endif		
		
#endif  // SOURCE_PORT_PLACEHOLDER
		
	gScreenXOffset = 0;										// calc offsets to center it
	gScreenYOffset = 0;	
		
	SetPort(GetWindowPort(gCoverWindow));
	ForeColor(whiteColor);
	BackColor(blackColor);
		

			
}




/****************** PREP DRAW SPROCKETS *********************/

static void PrepDrawSprockets(void)
{
#if 1
	SOURCE_PORT_PLACEHOLDER();
#else
DSpContextAttributes 	displayConfig,realConfig;
OSStatus 				theError;
Boolean					confirmIt = false;

		/* startup DrawSprocket */

	theError = DSpStartup();
	if( theError )
	{
		DoFatalAlert("DSpStartup failed!");
	}
	gLoadedDrawSprocket = true;


				/*************************/
				/* SETUP A REQUEST BLOCK */
				/*************************/
		
	displayConfig.frequency					= 0;
	displayConfig.displayWidth				= GAME_VIEW_WIDTH;
	displayConfig.displayHeight				= GAME_VIEW_HEIGHT;
	displayConfig.reserved1					= 0;
	displayConfig.reserved2					= 0;
	displayConfig.colorNeeds				= kDSpColorNeeds_Request;
	displayConfig.colorTable				= NULL;
	displayConfig.contextOptions			= 0; //kDSpContextOption_QD3DAccel;
	displayConfig.backBufferDepthMask		= kDSpDepthMask_1;
	displayConfig.displayDepthMask			= kDSpDepthMask_16;
	displayConfig.backBufferBestDepth		= 1;
	displayConfig.displayBestDepth			= 16;
	displayConfig.pageCount					= 1;
	displayConfig.gameMustConfirmSwitch		= false;
	displayConfig.reserved3[0]				= 0;
	displayConfig.reserved3[1]				= 0;
	displayConfig.reserved3[2]				= 0;
	displayConfig.reserved3[3]				= 0;
			
				/* SEE IF LET USER SELECT DISPLAY */

#if 0
	DSpCanUserSelectContext(&displayConfig, &canSelect);							// see if can do it
	if (canSelect)
	{
		FlushEvents (everyEvent, REMOVE_ALL_EVENTS);	
		InitCursor();
		theError = DSpUserSelectContext(&displayConfig,nil,nil,&gDisplayContext);	// let user select it
		if (theError)									
			CleanQuit();
		HideCursor();
		if (gDisplayContext == nil)													// see if something went horribly wrong
			CleanQuit();
	}
				/* AUTOMATICALLY FIND BEST CONTEXT */
	else
#endif	
	{
		theError = DSpFindBestContext( &displayConfig, &gDisplayContext );
		if (theError)
		{
			DoFatalAlert("PrepDrawSprockets: DSpFindBestContext failed");
		}
	}
				/* RESERVE IT */

	theError = DSpContext_Reserve( gDisplayContext, &displayConfig );
	if( theError )
		DoFatalAlert("PrepDrawSprockets: DSpContext_Reserve failed");
		
		
			/* MAKE STATE ACTIVE */
	
	theError = DSpContext_SetState( gDisplayContext, kDSpContextState_Active );
	if (theError == kDSpConfirmSwitchWarning)
	{
		confirmIt = true;
	}
	else
	if (theError)
	{
		DSpContext_Release( gDisplayContext );
		gDisplayContext = nil;
		DoFatalAlert("PrepDrawSprockets: DSpContext_SetState failed");
		return;
	}

			/* GET ATTRIBS OF THE CONTEXT */

	DSpContext_GetAttributes(gDisplayContext, &realConfig);
	confirmIt = displayConfig.gameMustConfirmSwitch;			//--- override setting from above cuz Carry has bug

#if ALLOW_FADE	
	DSpContext_FadeGamma(MONITORS_TO_FADE,100,nil);
#endif	
#endif
}




/**************** GAMMA FADE IN *************************/

void GammaFadeIn(void)
{
#if ALLOW_FADE	
	if (gDisplayContext)
	{
		while(gGammaFadePercent < 100)
		{
			gGammaFadePercent += 5;
			if (gGammaFadePercent > 100)
				gGammaFadePercent = 100;

			DSpContext_FadeGamma(gDisplayContext,gGammaFadePercent,nil);
		
			Wait(1);
		}
	}
#endif		
}

/**************** GAMMA FADE OUT *************************/

void GammaFadeOut(void)
{
#if ALLOW_FADE	
	if (gDisplayContext)
	{
		while(gGammaFadePercent > 0)
		{
			gGammaFadePercent -= 5;
			if (gGammaFadePercent < 0)
				gGammaFadePercent = 0;

			DSpContext_FadeGamma(gDisplayContext,gGammaFadePercent,nil);
		
			Wait(1);
		}
	}
#endif	
}

/********************** GAMMA ON *********************/

void GammaOn(void)
{
#if ALLOW_FADE	

	if (gDisplayContext)
	{
		if (gGammaFadePercent != 100)
		{
			DSpContext_FadeGamma(MONITORS_TO_FADE,100,nil);
			gGammaFadePercent = 100;
		}
	}
#endif		
}



/****************** CLEANUP DISPLAY *************************/

void CleanupDisplay(void)
{
#if 1
	SOURCE_PORT_MINOR_PLACEHOLDER();
#else
OSStatus 		theError;
	
	if(gDisplayContext != nil)
	{	
#if ALLOW_FADE		
		DSpContext_FadeGammaOut(gDisplayContext,nil);						// fade out	ours
#endif		
		DSpContext_SetState( gDisplayContext, kDSpContextState_Inactive );	// deactivate
#if ALLOW_FADE			
		DSpContext_FadeGamma(MONITORS_TO_FADE,100,nil);						// gamme on all
#endif		
		DSpContext_Release( gDisplayContext );								// release
	
		gDisplayContext = nil;
	}
	
	
	/* shutdown draw sprocket */
	
	if (gLoadedDrawSprocket)
	{
		theError = DSpShutdown();
		gLoadedDrawSprocket = false;
	}
#endif
}


/******************** MAKE FADE EVENT *********************/
//
// INPUT:	fadeIn = true if want fade IN, otherwise fade OUT.
//

void MakeFadeEvent(Boolean	fadeIn)
{
#if ALLOW_FADE			
ObjNode	*newObj;
ObjNode		*thisNodePtr;

		/* SCAN FOR OLD FADE EVENTS STILL IN LIST */

	thisNodePtr = gFirstNodePtr;
	
	while (thisNodePtr)
	{
		if ((thisNodePtr->Slot == SLOT_OF_DUMB) &&
			(thisNodePtr->MoveCall == MoveFadeEvent))
		{
			thisNodePtr->Flag[0] = fadeIn;								// set new mode
			return;
		}
		thisNodePtr = thisNodePtr->NextNode;							// next node
	}


		/* MAKE NEW FADE EVENT */
			
	gNewObjectDefinition.genre = EVENT_GENRE;
	gNewObjectDefinition.coord.x = 0;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.z = 0;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall = MoveFadeEvent;
	newObj = MakeNewObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;

	newObj->Flag[0] = fadeIn;
#endif		
}


/***************** MOVE FADE EVENT ********************/

static void MoveFadeEvent(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
		
			/* SEE IF FADE IN */
			
	if (theNode->Flag[0])
	{
		if (gGammaFadePercent >= 100.0f)										// see if @ 100%
		{
			gGammaFadePercent = 100;
			DeleteObject(theNode);
		}
#if ALLOW_FADE			
		if (gDisplayContext)
			DSpContext_FadeGamma(gDisplayContext,gGammaFadePercent,nil);
#endif		
		gGammaFadePercent += 300.0f*fps;
		if (gGammaFadePercent >= 100.0f)										// see if @ 100%
			gGammaFadePercent = 100;
	}
	
			/* FADE OUT */
	else
	{
		if (gGammaFadePercent <= 0.0f)													// see if @ 0%
		{
			gGammaFadePercent = 0;
			DeleteObject(theNode);
		}
#if ALLOW_FADE		
		if (gDisplayContext)
			DSpContext_FadeGamma(gDisplayContext,gGammaFadePercent,nil);	
#endif		
		gGammaFadePercent -= 300.0f*fps;
		if (gGammaFadePercent <= 0.0f)													// see if @ 0%
			gGammaFadePercent = 0;
	}
}


/************************ GAME SCREEN TO BLACK ************************/

void GameScreenToBlack(void)
{
	if (gCoverWindow)
	{
		Rect	r;
		SetPort(GetWindowPort(gCoverWindow));
		BackColor(blackColor);
		GetPortBounds(GetWindowPort(gCoverWindow), &r);
		EraseRect(&r);
	}
}


/*********************** CLEAN SCREEN BORDER ****************************/
//
// This clears to black the border around the QD3D view and the cover window's rect.
//

void CleanScreenBorder(void)
{
#if 0
Rect	r;

	if (gAdditionalClipping == 0)
		return;

	SetPort(GetWindowPort(gCoverWindow));
	BackColor(blackColor);
	
			/* TOP MARGIN */
			
	r = gCoverWindow->portRect;
	r.bottom = r.top + (gAdditionalClipping*.75);
	EraseRect(&r);

			/* BOTTOM MARGIN */
			
	r = gCoverWindow->portRect;
	r.top = r.bottom - (gAdditionalClipping*.75);
	EraseRect(&r);

			/* LEFT MARGIN */
			
	r = gCoverWindow->portRect;
	r.right = r.left + gAdditionalClipping;
	EraseRect(&r);
	
			/* RIGHT MARGIN */
			
	r = gCoverWindow->portRect;
	r.left = r.right - gAdditionalClipping;
	EraseRect(&r);
#endif	
}



#pragma mark -

/*********************** DUMP GWORLD 2 **********************/
//
//    copies to a destination RECT
//

void DumpGWorld2(GWorldPtr thisWorld, WindowPtr thisWindow,Rect *destRect)
{
PixMapHandle pm;
GDHandle		oldGD;
GWorldPtr		oldGW;
Rect			r;

	DoLockPixels(thisWorld);

	GetGWorld (&oldGW,&oldGD);
	pm = GetGWorldPixMap(thisWorld);	
	if ((pm == nil) | (*pm == nil) )
		DoAlert("PixMap Handle or Ptr = Null?!");

	SetPort(GetWindowPort(thisWindow));

	ForeColor(blackColor);
	BackColor(whiteColor);

	GetPortBounds(thisWorld, &r);
				
	CopyBits(/*(BitMap *)*/ *pm,
			GetPortBitMapForCopyBits(GetWindowPort(thisWindow)),
			&r,
			destRect,
			srcCopy,
			0);

	SetGWorld(oldGW,oldGD);								// restore gworld
	
	
	//QDFlushPortBuffer(GetWindowPort(thisWindow), nil);

}

/*********************** DUMP GWORLD 3 **********************/
//
//    copies from src RECT to a destination RECT
//

void DumpGWorld3(GWorldPtr thisWorld, WindowPtr thisWindow,Rect *srcRect, Rect *destRect)
{
PixMapHandle pm;
GDHandle		oldGD;
GWorldPtr		oldGW;

	DoLockPixels(thisWorld);

	GetGWorld (&oldGW,&oldGD);
	pm = GetGWorldPixMap(thisWorld);	
	if ((pm == nil) | (*pm == nil) )
		DoAlert("PixMap Handle or Ptr = Null?!");

	SetPort(GetWindowPort(thisWindow));

	ForeColor(blackColor);
	BackColor(whiteColor);
				
	CopyBits(/*(BitMap *)*/ *pm,
			GetPortBitMapForCopyBits(GetWindowPort(thisWindow)),
			srcRect,
			destRect,
			srcCopy,
			0);

	SetGWorld(oldGW,oldGD);								// restore gworld
	
//	QDFlushPortBuffer(GetWindowPort(thisWindow), nil);
	
}


/*********************** DUMP GWORLD TRANSPARENT **********************/
//
// copies to a destination RECT without copying black
//

void DumpGWorldTransparent(GWorldPtr thisWorld, WindowPtr thisWindow,Rect *destRect)
{
PixMapHandle pm;
GDHandle		oldGD;
GWorldPtr		oldGW;
Rect			r;

	DoLockPixels(thisWorld);

	GetGWorld (&oldGW,&oldGD);
	pm = GetGWorldPixMap(thisWorld);	
	if ((pm == nil) | (*pm == nil) )
		DoAlert("PixMap Handle or Ptr = Null?!");

	SetPort(GetWindowPort(thisWindow));

	ForeColor(whiteColor);
	BackColor(blackColor);

	GetPortBounds(thisWorld, &r);
				
	CopyBits(/*(BitMap *)*/ *pm,
			GetPortBitMapForCopyBits(GetWindowPort(thisWindow)),
			&r,
			destRect,
			srcCopy|transparent,
			0);

	SetGWorld(oldGW,oldGD);								// restore gworld

}


/******************* DO LOCK PIXELS **************/

void DoLockPixels(GWorldPtr world)
{
PixMapHandle pm;
	
	pm = GetGWorldPixMap(world);
	if (LockPixels(pm) == false)
		DoFatalAlert("PixMap Went Bye,Bye?!");
}





