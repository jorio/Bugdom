#include "MyPCH_Normal.pch"
#include "PommeInit.h"


extern "C"
{
	extern TQ3Matrix4x4 gCameraWorldToViewMatrix;
	extern TQ3Matrix4x4 gCameraViewToFrustumMatrix;
	extern long gNodesDrawn;
	extern SDL_Window* gSDLWindow;
	extern	QD3DSetupOutputType		*gGameViewInfoPtr;
	extern float gAdditionalClipping;
	extern float gFramesPerSecond;
	extern PrefsType gGamePrefs;
	extern short gPrefsFolderVRefNum;
	extern long gPrefsFolderDirID;
}

static struct
{
	UInt32 lastUpdateAt = 0;
	const UInt32 updateInterval = 250;
	UInt32 frameAccumulator = 0;
	char titleBuffer[1024];
} debugText;

TQ3Area GetAdjustedPane(int windowWidth, int windowHeight, Rect paneClip)
{
	TQ3Area pane;

	pane.min.x = paneClip.left;					// set bounds?
	pane.max.x = GAME_VIEW_WIDTH - paneClip.right;
	pane.min.y = paneClip.top;
	pane.max.y = GAME_VIEW_HEIGHT - paneClip.bottom;

	pane.min.x += gAdditionalClipping;						// offset bounds by user clipping
	pane.max.x -= gAdditionalClipping;
	pane.min.y += gAdditionalClipping*.75;
	pane.max.y -= gAdditionalClipping*.75;

	// Source port addition
	pane.min.x *= windowWidth / (float)(GAME_VIEW_WIDTH);					// scale clip pane to window size
	pane.max.x *= windowWidth / (float)(GAME_VIEW_WIDTH);
	pane.min.y *= windowHeight / (float)(GAME_VIEW_HEIGHT);
	pane.max.y *= windowHeight / (float)(GAME_VIEW_HEIGHT);

	return pane;
}

// Called when the game window gets resized.
// Adjusts the clipping pane and camera aspect ratio.
static void QD3D_OnWindowResized(int windowWidth, int windowHeight)
{
	if (!gGameViewInfoPtr)
		return;

	TQ3Area pane = GetAdjustedPane(windowWidth, windowHeight, gGameViewInfoPtr->paneClip);
	Q3DrawContext_SetPane(gGameViewInfoPtr->drawContext, &pane);

	float aspectRatioXToY = (pane.max.x-pane.min.x)/(pane.max.y-pane.min.y);

	Q3ViewAngleAspectCamera_SetAspectRatio(gGameViewInfoPtr->cameraObject, aspectRatioXToY);
}

void DoSDLMaintenance()
{
	static int holdFramerateCap = 0;

	// Cap frame rate.
	if (gFramesPerSecond > 200 || holdFramerateCap > 0)
	{
		SDL_Delay(5);
		// Keep framerate cap for a while to avoid jitter in game physics
		holdFramerateCap = 10;
	}
	else
	{
		holdFramerateCap--;
	}

#if 0
#if _DEBUG
	UInt32 now = SDL_GetTicks();
	UInt32 ticksElapsed = now - debugText.lastUpdateAt;
	if (ticksElapsed >= debugText.updateInterval) {
		float fps = 1000 * debugText.frameAccumulator / (float)ticksElapsed;
		snprintf(debugText.titleBuffer, 1024, "Bugdom - %d fps - %ld nodes drawn", (int)round(fps), gNodesDrawn);
		SDL_SetWindowTitle(gSDLWindow, debugText.titleBuffer);
		debugText.frameAccumulator = 0;
		debugText.lastUpdateAt = now;
	}
	debugText.frameAccumulator++;
#endif

	if (GetNewKeyState(kKey_ToggleFullscreen))
	{
		gGamePrefs.fullscreen = gGamePrefs.fullscreen ? 0 : 1;
		SetFullscreenMode();
	}
#endif

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				throw Pomme::QuitRequest();
				break;

			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_CLOSE:
						throw Pomme::QuitRequest();
						break;

					case SDL_WINDOWEVENT_RESIZED:
						QD3D_OnWindowResized(event.window.data1, event.window.data2);
						break;
				}
				break;
		}
	}
}