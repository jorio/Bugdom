#include "bugdom.h"
#include "PommeInit.h"
#include "PommeFiles.h"
#include "PommeGraphics.h"
#include "GLOverlay.h"

extern "C"
{
	extern TQ3Matrix4x4 gCameraWorldToViewMatrix;
	extern TQ3Matrix4x4 gCameraViewToFrustumMatrix;
	extern long gNodesDrawn;
	extern SDL_Window* gSDLWindow;
	extern float gAdditionalClipping;
	extern float gFramesPerSecond;
	extern PrefsType gGamePrefs;
	extern short gPrefsFolderVRefNum;
	extern long gPrefsFolderDirID;
	extern long					gEatMouse;
	Boolean gMouseButtonPressed = false;
	extern Boolean	gMouseButtonState[3];
	char gTypedAsciiKey = '\0';
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

static OSErr ReadPictureFile(FSSpec* spec, Pomme::Graphics::ARGBPixmap& pict)
{
	short refNum;

	OSErr error = FSpOpenDF(spec, fsRdPerm, &refNum);
	if (noErr != error)
	{
		return error;
	}

	auto& stream = Pomme::Files::GetStream(refNum);
	pict = Pomme::Graphics::ReadPICT(stream, true);
	FSClose(refNum);

	return noErr;
}

OSErr DrawPictureIntoGWorld(FSSpec* spec, GWorldPtr* theGWorld)
{
	Pomme::Graphics::ARGBPixmap pict;
	OSErr err = ReadPictureFile(spec, pict);
	if (err != noErr)
	{
		return err;
	}

	Rect boundsRect;
	boundsRect.left = 0;
	boundsRect.top = 0;
	boundsRect.right = pict.width;
	boundsRect.bottom = pict.height;

	NewGWorld(theGWorld, 32, &boundsRect, 0, 0, 0);

	GrafPtr oldPort;
	GetPort(&oldPort);
	SetPort(*theGWorld);
	Pomme::Graphics::DrawARGBPixmap(0, 0, pict);
	SetPort(oldPort);

	return noErr;
}

OSErr DrawPictureToScreen(FSSpec* spec, short x, short y)
{
	Pomme::Graphics::ARGBPixmap pict;
	OSErr err = ReadPictureFile(spec, pict);
	if (err != noErr)
	{
		return err;
	}

	GrafPtr oldPort;
	GetPort(&oldPort);
	SetPort(Pomme::Graphics::GetScreenPort());
	Pomme::Graphics::DrawARGBPixmap(x, y, pict);
	SetPort(oldPort);

	return noErr;
}

void FlushQuesaErrors()
{
	TQ3Error err = Q3Error_Get(nil);
	if (err != kQ3ErrorNone)
	{
		printf("%s: %d\n", __func__, err);
	}
}

Boolean FlushMouseButtonPress()
{
	Boolean rc = gMouseButtonPressed;
	gMouseButtonPressed = false;
	return rc;
}


void DoSDLMaintenance()
{
	Overlay_Flush();

	SDL_GL_SwapWindow(gSDLWindow);

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

#if _DEBUG
	UInt32 now = SDL_GetTicks();
	UInt32 ticksElapsed = now - debugText.lastUpdateAt;
	if (ticksElapsed >= debugText.updateInterval) {
		float fps = 1000 * debugText.frameAccumulator / (float)ticksElapsed;
		snprintf(debugText.titleBuffer, 1024, "Bugdom - %d fps", (int)round(fps));
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

	// Reset these on every new frame
	gTypedAsciiKey = '\0';
	gMouseButtonPressed = false;
	MouseSmoothing_StartFrame();

	Uint32 mouseButtons = SDL_GetMouseState(nullptr, nullptr);
	gMouseButtonState[0] = mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT);
	gMouseButtonState[1] = mouseButtons & SDL_BUTTON(SDL_BUTTON_RIGHT);
	gMouseButtonState[2] = mouseButtons & SDL_BUTTON(SDL_BUTTON_MIDDLE);

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

			case SDL_TEXTINPUT:
				// The text event gives us UTF-8. Use the key only if it's a printable ASCII character.
				if (event.text.text[0] >= ' ' && event.text.text[0] <= '~')
				{
					gTypedAsciiKey = event.text.text[0];
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
				gMouseButtonPressed = true;
				break;

			case SDL_MOUSEMOTION:
				MouseSmoothing_OnMouseMotion(&event.motion);
				break;
		}
	}
	
	if (gEatMouse)
	{
		gEatMouse--;
		MouseSmoothing_ResetState();
		gMouseButtonPressed = false;
		gMouseButtonState[0] = false;
		gMouseButtonState[1] = false;
		gMouseButtonState[2] = false;
	}
}
