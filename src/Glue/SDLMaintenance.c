#include "bugdom.h"

extern SDL_Window*			gSDLWindow;
extern float				gFramesPerSecond;
extern PrefsType			gGamePrefs;
extern long					gEatMouse;
Boolean						gMouseButtonPressed = false;
extern Boolean				gMouseButtonState[3];
char						gTypedAsciiKey = '\0';

#if _DEBUG
static struct
{
	UInt32 lastUpdateAt;
	UInt32 frameAccumulator;
	char titleBuffer[1024];
} debugText;

static const int kDebugTextUpdateInterval = 250;
#endif

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
	if (ticksElapsed >= kDebugTextUpdateInterval) {
		float fps = 1000 * debugText.frameAccumulator / (float)ticksElapsed;
		snprintf(debugText.titleBuffer, 1024, "Bugdom - %d fps", (int)round(fps));
		SDL_SetWindowTitle(gSDLWindow, debugText.titleBuffer);
		debugText.frameAccumulator = 0;
		debugText.lastUpdateAt = now;
	}
	debugText.frameAccumulator++;
#endif

	// Reset these on every new frame
	gTypedAsciiKey = '\0';
	gMouseButtonPressed = false;
	MouseSmoothing_StartFrame();

	uint32_t mouseButtons = SDL_GetMouseState(NULL, NULL);
	gMouseButtonState[0] = mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT);
	gMouseButtonState[1] = mouseButtons & SDL_BUTTON(SDL_BUTTON_RIGHT);
	gMouseButtonState[2] = mouseButtons & SDL_BUTTON(SDL_BUTTON_MIDDLE);

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				ExitToShell();			// throws Pomme::QuitRequest
				return;

			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_CLOSE:
						ExitToShell();	// throws Pomme::QuitRequest
						return;

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
