// SDL MAINTENANCE.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom

#include "bugdom.h"

extern SDL_Window*			gSDLWindow;
extern float				gFramesPerSecond;
extern PrefsType			gGamePrefs;
extern long					gEatMouse;
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

			case SDL_MOUSEMOTION:
				if (!gEatMouse)
				{
					MouseSmoothing_OnMouseMotion(&event.motion);
				}
				break;

			case SDL_JOYDEVICEADDED:	 // event.jdevice.which is the joy's INDEX (not an instance id!)
				TryOpenController(false);
				break;

			case SDL_JOYDEVICEREMOVED:	// event.jdevice.which is the joy's UNIQUE INSTANCE ID (not an index!)
				OnJoystickRemoved(event.jdevice.which);
				break;
		}
	}
}
