/****************************/
/*        WINDOWS           */
/* (c)1997-99 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveFadeEvent(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/

/****************  INIT WINDOW STUFF *******************/

void InitWindowStuff(void)
{
		/* SET FULLSCREEN MODE ACCORDING TO PREFS */

	SetFullscreenMode();

		/* SHOW A COUPLE BLACK FRAMES BEFORE WE BEGIN */

	QD3DSetupInputType viewDef;
	QD3D_NewViewDef(&viewDef);
	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);
	for (int i = 0; i < 45; i++)
	{
		QD3D_DrawScene(gGameViewInfoPtr, nil);
		DoSDLMaintenance();
	}
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);
	Pomme_FlushPtrTracking(true);
}


/**************** GAMMA FADE OUT *************************/

void GammaFadeOut(Boolean fadeSound)
{
	float duration = .3f;

#if _DEBUG
	duration = .05f;
#endif

	Uint32 startTicks = SDL_GetTicks();
//	gGammaFadeFactor = 1.0f;

	while (gGammaFadeFactor > 0)
	{
		Uint32 ticks = SDL_GetTicks();
		gGammaFadeFactor = 1.0f - ((ticks - startTicks) / 1000.0f / duration);
		if (gGammaFadeFactor < 0.0f)
			gGammaFadeFactor = 0.0f;

		UpdateInput();

		if (gSuperTileMemoryListExists)
			QD3D_DrawScene(gGameViewInfoPtr, DrawTerrain);
		else
			QD3D_DrawScene(gGameViewInfoPtr, DrawObjects);

		if (fadeSound)
		{
			FadeGlobalVolume(gGammaFadeFactor);
		}

		QD3D_CalcFramesPerSecond();
		DoSDLMaintenance();
	}

	gGammaFadeFactor = 1;

	if (fadeSound)
	{
		StopAllEffectChannels();
		KillSong();
		FadeGlobalVolume(1);
	}

	FlushMouseButtonPress();
}

/********************** GAMMA ON *********************/

void GammaOn(void)
{
	// Note: the game used to fade gamma in smoothly if it wasn't at 100% already. Changed to instant 100%.
	gGammaFadeFactor = 1.0f;
}



/******************** MAKE FADE EVENT *********************/
//
// INPUT:	fadeIn = true if want fade IN, otherwise fade OUT.
//

void MakeFadeEvent(Boolean	fadeIn)
{
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

	if (fadeIn)
	{
		gGammaFadeFactor = .01f;
	}
	else
	{
		gGammaFadeFactor = .99f;
	}
}


/***************** MOVE FADE EVENT ********************/

static void MoveFadeEvent(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
		
			/* SEE IF FADE IN */
			
	if (theNode->Flag[0])
	{
		if (gGammaFadeFactor >= 1.0f)										// see if @ 100%
		{
			gGammaFadeFactor = 1.0f;
			DeleteObject(theNode);
			return;
		}
		gGammaFadeFactor += 3.0f * fps;
		if (gGammaFadeFactor >= 1.0f)										// see if @ 100%
			gGammaFadeFactor = 1.0f;
	}
	
			/* FADE OUT */
	else
	{
		if (gGammaFadeFactor <= 0.0f)													// see if @ 0%
		{
			gGammaFadeFactor = 0;
			DeleteObject(theNode);
			return;
		}
		gGammaFadeFactor -= 3.0f * fps;
		if (gGammaFadeFactor <= 0.0f)													// see if @ 0%
			gGammaFadeFactor = 0;
	}
}


/************************ GAME SCREEN TO BLACK ************************/

void GameScreenToBlack(void)
{
	FlushMouseButtonPress();
}

#pragma mark -



void QD3D_OnWindowResized(int windowWidth, int windowHeight)
{
	gWindowWidth = windowWidth;
	gWindowHeight = windowHeight;
}


/******************* SET FULLSCREEN MODE FROM USER PREFS **************/

void SetFullscreenMode(void)
{
#if OSXPPC
	static bool didSwitchOnce = false;		// called at start of game, so allow switching once only

	if (didSwitchOnce)
	{
		puts("This version of the game does not allow hot-switching between windowed/fullscreen modes.");
		return;
	}

	didSwitchOnce = true;
#endif

	if (!gGamePrefs.fullscreen)
	{
		SDL_SetWindowFullscreen(gSDLWindow, 0);
	}
	else if (gCommandLine.fullscreenWidth > 0 && gCommandLine.fullscreenHeight > 0)
	{
		SDL_DisplayMode mode;
		mode.w = gCommandLine.fullscreenWidth;
		mode.h = gCommandLine.fullscreenHeight;
		mode.refresh_rate = gCommandLine.fullscreenRefreshRate;
		mode.driverdata = nil;
		mode.format = SDL_PIXELFORMAT_UNKNOWN;
		SDL_SetWindowDisplayMode(gSDLWindow, &mode);
		SDL_SetWindowFullscreen(gSDLWindow, SDL_WINDOW_FULLSCREEN);
	}
	else
	{
#if OSXPPC
		Byte* curatedModeIDs = NULL;
		int numCuratedModes = CurateDisplayModes(0, &curatedModeIDs);
		SDL_DisplayMode mode = {0};

		int sdlModeID = 0;
		if (gGamePrefs.curatedDisplayModeID < numCuratedModes)
		{
			sdlModeID = curatedModeIDs[gGamePrefs.curatedDisplayModeID];
		}

		if (0 == SDL_GetDisplayMode(0, sdlModeID, &mode))
		{
			SDL_SetWindowDisplayMode(gSDLWindow, &mode);
			SDL_SetWindowFullscreen(gSDLWindow, SDL_WINDOW_FULLSCREEN);
		}
		else
		{
			SDL_SetWindowFullscreen(gSDLWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
#else
		SDL_SetWindowFullscreen(gSDLWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
#endif
	}

	// Some systems may not display textures properly in the first GL context created by the game
	// unless we flush SDL events immediately after entering fullscreen mode.
	SDL_PumpEvents();
	SDL_FlushEvents(0, 0xFFFFFFFF);
}

/*************** GET CURATED LIST OF DISPLAY MODES **************/
//
// Returns the number of curated display modes that were found.
// Sets bestModeOut to an array of IDs of SDL display modes.
// You can those IDs to SDL_GetDisplayMode().
//

int CurateDisplayModes(int display, Byte** curatedModesOut)
{
	static Byte curatedModes[256] = {0};
	int numCuratedModes = 0;
	SDL_DisplayMode lastMode = {0};

	int numDisplayModes = SDL_GetNumDisplayModes(display);

	for (int i = numDisplayModes-1; i >= 0; i--)	// walk the list backwards so we get the lowest-quality modes first
	{
		SDL_DisplayMode mode;

		if (0 != SDL_GetDisplayMode(display, i, &mode))
			continue;

		if (mode.w != lastMode.w || mode.h != lastMode.h)
		{
			if (numCuratedModes >= 256)
				break;
			curatedModes[numCuratedModes] = i;
			numCuratedModes++;
		}
		else
		{
			// the resolution is already known, but this mode probably has a better refresh rate or bit depth
			curatedModes[numCuratedModes-1] = i;
		}

		lastMode = mode;
	}

	// pass result to caller
	if (curatedModesOut)
	{
		*curatedModesOut = curatedModes;
	}

	return numCuratedModes;
}

