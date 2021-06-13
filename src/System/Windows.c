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

#if _DEBUG
#define	ALLOW_FADE		0
#else
#define	ALLOW_FADE		1
#endif


/**********************/
/*     VARIABLES      */
/**********************/

extern WindowPtr		gCoverWindow;


float		gGammaFadePercent;

/****************  INIT WINDOW STUFF *******************/

void InitWindowStuff(void)
{
}


/**************** GAMMA FADE OUT *************************/

void GammaFadeOut(void)
{
#if ALLOW_FADE
	printf("TODO NOQUESA: fade out!\n");//Overlay_FadeOutFrozenFrame(.3f);
	gGammaFadePercent = 0;
#endif
	FlushMouseButtonPress();
}

/********************** GAMMA ON *********************/

void GammaOn(void)
{
	// Note: the game used to fade gamma in smoothly if it wasn't at 100% already. Changed to instant 100%.
	gGammaFadePercent = 100;
}



/****************** CLEANUP DISPLAY *************************/

void CleanupDisplay(void)
{
	GammaFadeOut();
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

	FlushMouseButtonPress();
}

#pragma mark -



void QD3D_OnWindowResized(int windowWidth, int windowHeight)
{
	if (!gGameViewInfoPtr)
		return;

	gWindowWidth = windowWidth;
	gWindowHeight = windowHeight;
}


/******************* SET FULLSCREEN MODE FROM USER PREFS **************/

void SetFullscreenMode(void)
{
	SDL_SetWindowFullscreen(
			gSDLWindow,
			gGamePrefs.fullscreen? SDL_WINDOW_FULLSCREEN_DESKTOP: 0);

	// Ensure the clipping pane gets resized properly after switching in or out of fullscreen mode
	int width, height;
	SDL_GetWindowSize(gSDLWindow, &width, &height);
	QD3D_OnWindowResized(width, height);

//	SDL_ShowCursor(gGamePrefs.fullscreen? 0: 1);
}

