// SDL MAINTENANCE.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom

#include "bugdom.h"
#include "killmacmouseacceleration.h"
#include "version.h"

extern SDL_Window*			gSDLWindow;
extern float				gFramesPerSecond;
extern PrefsType			gGamePrefs;
extern long					gEatMouse;
char						gTypedAsciiKey = '\0';
extern RenderStats			gRenderStats;
extern TQ3Point3D			gMyCoord;

#if _DEBUG
static const uint32_t	kDebugTextUpdateInterval = 50;
static uint32_t			gDebugTextFrameAccumulator = 0;
static uint32_t			gDebugTextLastUpdatedAt = 0;
static char				gDebugTextBuffer[1024];
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
	//if (gGamePrefs.debugInfoInTitleBar)
	{
		uint32_t ticksNow = SDL_GetTicks();
		uint32_t ticksElapsed = ticksNow - gDebugTextLastUpdatedAt;
		if (ticksElapsed >= kDebugTextUpdateInterval)
		{
			float fps = 1000 * gDebugTextFrameAccumulator / (float)ticksElapsed;
			snprintf(
					gDebugTextBuffer, sizeof(gDebugTextBuffer),
					"Bugdom %s - fps:%d tris:%d meshq:%d - x:%.0f z:%.0f",
					PROJECT_VERSION,
					(int)round(fps),
					gRenderStats.trianglesDrawn,
					gRenderStats.meshQueueSize,
					gMyCoord.x,
					gMyCoord.z
			);
			SDL_SetWindowTitle(gSDLWindow, gDebugTextBuffer);
			gDebugTextFrameAccumulator = 0;
			gDebugTextLastUpdatedAt = ticksNow;
		}
		gDebugTextFrameAccumulator++;
	}
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

					case SDL_WINDOWEVENT_FOCUS_LOST:
#if __APPLE__
						// On Mac, always restore system mouse accel if cmd-tabbing away from the game
						RestoreMacMouseAcceleration();
#endif
						break;
						
					case SDL_WINDOWEVENT_FOCUS_GAINED:
#if __APPLE__
						// On Mac, kill mouse accel when focus is regained only if the game has captured the mouse
						if (SDL_GetRelativeMouseMode())
							KillMacMouseAcceleration();
#endif
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
