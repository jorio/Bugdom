// SDL MAINTENANCE.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom

#include "game.h"
#if !OSXPPC
#include "killmacmouseacceleration.h"
#endif
#include "version.h"
#include <stdio.h>

char						gTypedAsciiKey = '\0';

static const uint32_t	kDebugTextUpdateInterval = 0;//50;
static uint32_t			gDebugTextFrameAccumulator = 0;
static uint32_t			gDebugTextLastUpdatedAt = 0;
static char				gDebugTextBuffer[1024];

static void UpdateDebugStats(void)
{
	uint32_t ticksNow = SDL_GetTicks();
	uint32_t ticksElapsed = ticksNow - gDebugTextLastUpdatedAt;
	if (ticksElapsed >= kDebugTextUpdateInterval)
	{
		float fps;
		if (kDebugTextUpdateInterval == 0)
			fps = gFramesPerSecond;
		else
			fps = 1000.0f * gDebugTextFrameAccumulator / (float)ticksElapsed;

		const char* debugModeName = "???";
		switch (gDebugMode)
		{
			case 1: debugModeName = ""; break;
			case 2: debugModeName = "show collisions"; break;
			case 3: debugModeName = "show splines"; break;
		}

		snprintf(
				gDebugTextBuffer, sizeof(gDebugTextBuffer),
				"fps: %d\ntris: %d\nmeshes: %d+%d\ntiles: %ld/%ld%s\nnodes: %d\nheap: %dK, %dp\n\nx: %d\nz: %d\ny: %.3f %s%s\n%s\n%s\n\n\n\n\n\n\n\n\n"
				"Bugdom %s\nOpenGL %s, %s @ %dx%d",
				(int)roundf(fps),
				gRenderStats.triangles,
				gRenderStats.meshesPass1,
				gRenderStats.meshesPass2,
				gSupertileBudget - gNumFreeSupertiles,
				gSupertileBudget,
				gSuperTileMemoryListExists ? "" : " (no terrain)",
				gNumObjNodes,
				(int)(Pomme_GetHeapSize() / 1024),
				(int)Pomme_GetNumAllocs(),
				(int)(gPlayerObj? gPlayerObj->Coord.x: 0),
				(int)(gPlayerObj? gPlayerObj->Coord.z: 0),
				gPlayerObj? gPlayerObj->Coord.y: 0,
				(gPlayerObj && gPlayerObj->StatusBits & STATUS_BIT_ONGROUND)? "G" : "",
				(gPlayerObj && gPlayerObj->MPlatform)? "M" : "",
				debugModeName,
				gLiquidCheat ? "Liquid cheat ON" : "",
				PROJECT_VERSION,
				glGetString(GL_VERSION),
				glGetString(GL_RENDERER),
				gWindowWidth,
				gWindowHeight
		);
		QD3D_UpdateDebugTextMesh(gDebugTextBuffer);
		gDebugTextFrameAccumulator = 0;
		gDebugTextLastUpdatedAt = ticksNow;
	}
	gDebugTextFrameAccumulator++;
}

void DoSDLMaintenance(void)
{
	switch (gDebugMode)
	{
		case DEBUG_MODE_OFF:
		case DEBUG_MODE_NOTEXTURES:
		case DEBUG_MODE_WIREFRAME:
			break;
		default:
			UpdateDebugStats();
			break;
	}

	// Reset these on every new frame
	gTypedAsciiKey = '\0';

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				CleanQuit();
				return;

			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_CLOSE:
						CleanQuit();
						return;

#if __APPLE__ && !OSXPPC
					case SDL_WINDOWEVENT_FOCUS_LOST:
						// On Mac, always restore system mouse accel if cmd-tabbing away from the game
						RestoreMacMouseAcceleration();
						break;
						
					case SDL_WINDOWEVENT_FOCUS_GAINED:
						// On Mac, kill mouse accel when focus is regained only if the game has captured the mouse
						if (SDL_GetRelativeMouseMode())
							KillMacMouseAcceleration();
						break;
#endif
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

	// Ensure the clipping pane gets resized properly after switching in or out of fullscreen mode
	int width, height;
	SDL_GL_GetDrawableSize(gSDLWindow, &width, &height);
	QD3D_OnWindowResized(width, height);

	if (GetNewKeyState_SDL(SDL_SCANCODE_F8))
	{
		gDebugMode++;
		gDebugMode %= NUM_DEBUG_MODES;
		gDebugTextLastUpdatedAt = 0;
		QD3D_UpdateDebugTextMesh(NULL);

		glPolygonMode(GL_FRONT_AND_BACK, gDebugMode == DEBUG_MODE_WIREFRAME? GL_LINE: GL_FILL);
	}
}
