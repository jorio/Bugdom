// BUGDOM ENTRY POINT
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom

#include "Pomme.h"
#include "PommeInit.h"
#include "PommeFiles.h"
#include "PommeGraphics.h"

#include <iostream>
#include <cstring>

#include "game.h"
#include "version.h"

#include <vitasdk.h>
#include <vitaGL.h>

#if __APPLE__
#include "killmacmouseacceleration.h"
#include <libproc.h>
#include <unistd.h>
#endif

// increase heap size
int _newlib_heap_size_user = 128 * 1024 * 1024;

extern "C"
{
	// bare minimum to satisfy externs in game code
	SDL_Window* gSDLWindow = nullptr;

	CommandLineOptions gCommandLine;

	// Tell Windows graphics driver that we prefer running on a dedicated GPU if available
#if 0 //_WIN32
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
#endif

	int gAntialiasingLevelAppliedOnBoot = 0;

	int GameMain(void);
}

static fs::path FindGameData()
{
	fs::path dataPath;

#if __APPLE__
	char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

	pid_t pid = getpid();
	int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
	if (ret <= 0)
	{
		throw std::runtime_error(std::string(__func__) + ": proc_pidpath failed: " + std::string(strerror(errno)));
	}

	dataPath = pathbuf;
	dataPath = dataPath.parent_path().parent_path() / "Resources";
#else
	dataPath = "Data";
#endif

	dataPath = dataPath.lexically_normal();

	// Set data spec -- Lets the game know where to find its asset files
	gDataSpec = Pomme::Files::HostPathToFSSpec(dataPath / "Skeletons");

	// Use application resource file
	auto applicationSpec = Pomme::Files::HostPathToFSSpec(dataPath / "System" / "Application");
	short resFileRefNum = FSpOpenResFile(&applicationSpec, fsRdPerm);

	if (resFileRefNum == -1)
	{
		throw std::runtime_error("Couldn't find the Data folder.");
	}

	UseResFile(resFileRefNum);

	return dataPath;
}

static void ParseCommandLine(int argc, char** argv)
{
	memset(&gCommandLine, 0, sizeof(gCommandLine));
	gCommandLine.msaa = 0;
	gCommandLine.vsync = 1;

	for (int i = 1; i < argc; i++)
	{
		std::string argument = argv[i];

		if (argument == "--stats")
			gShowDebugStats = true;
		else if (argument == "--no-vsync")
			gCommandLine.vsync = 0;
		else if (argument == "--vsync")
			gCommandLine.vsync = 1;
		else if (argument == "--adaptive-vsync")
			gCommandLine.vsync = -1;
		else if (argument == "--msaa2x")
			gCommandLine.msaa = 2;
		else if (argument == "--msaa4x")
			gCommandLine.msaa = 4;
		else if (argument == "--msaa8x")
			gCommandLine.msaa = 8;
		else if (argument == "--msaa16x")
			gCommandLine.msaa = 16;
		else if (argument == "--fullscreen-resolution")
		{
			GAME_ASSERT_MESSAGE(i + 2 < argc, "fullscreen width & height unspecified");
			gCommandLine.fullscreenWidth = atoi(argv[i + 1]);
			gCommandLine.fullscreenHeight = atoi(argv[i + 2]);
			i += 2;
		}
		else if (argument == "--fullscreen-refresh-rate")
		{
			GAME_ASSERT_MESSAGE(i + 1 < argc, "fullscreen refresh rate unspecified");
			gCommandLine.fullscreenRefreshRate = atoi(argv[i + 1]);
			i += 1;
		}
	}
}

static void Boot()
{
	// Start our "machine"
	Pomme::Init();

	// Initialize SDL video subsystem
	/*if (0 != SDL_Init(SDL_INIT_VIDEO))
	{
		throw std::runtime_error("Couldn't initialize SDL video subsystem.");
	}*/

	// Load our prefs
	InitPrefs();

	if (gCommandLine.msaa != 0)
		gGamePrefs.antialiasingLevel = gCommandLine.msaa;

tryAgain:
	// Set up GL attributes
	/*SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	if (gGamePrefs.antialiasingLevel)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, gGamePrefs.antialiasingLevel);
	}*/
	gAntialiasingLevelAppliedOnBoot = gGamePrefs.antialiasingLevel;

	// Prepare window dimensions
	//int display = 0;
	float screenFillRatio = 2.0f / 3.0f;

	/*SDL_Rect displayBounds = { .x = 0, .y = 0, .w = GAME_VIEW_WIDTH, .h = GAME_VIEW_HEIGHT };
	SDL_GetDisplayUsableBounds(display, &displayBounds);
	TQ3Vector2D fitted = FitRectKeepAR(GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT, displayBounds.w, displayBounds.h);
	int initialWidth  = (int) (fitted.x * screenFillRatio);
	int initialHeight = (int) (fitted.y * screenFillRatio);

	// Create the window
	gSDLWindow = SDL_CreateWindow(
			"Bugdom " PROJECT_VERSION,
			SDL_WINDOWPOS_UNDEFINED_DISPLAY(display),
			SDL_WINDOWPOS_UNDEFINED_DISPLAY(display),
			initialWidth,
			initialHeight,
            SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN);
			//SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN);*/

    int vita_height = 544;
    int vita_width = 960;

    gWindowWidth = vita_width;
    gWindowHeight = vita_height;

    InitLogVita();
    vglInitExtended(0, 960, 544, 0x800000, SCE_GXM_MULTISAMPLE_4X);

	/*if (!gSDLWindow)
	{
		if (gGamePrefs.antialiasingLevel == 0)
		{
			throw std::runtime_error("Couldn't create SDL window.");
		}
		else
		{
			gGamePrefs.antialiasingLevel = 0;
			goto tryAgain;
		}
	}*/

	// Find path to game data folder
	fs::path dataPath = FindGameData();

	// Init joystick subsystem
	{
		SDL_Init(SDL_INIT_JOYSTICK);
		auto gamecontrollerdbPath8 = (dataPath / "System" / "gamecontrollerdb.txt").u8string();
		if (-1 == SDL_GameControllerAddMappingsFromFile((const char*)gamecontrollerdbPath8.c_str()))
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Bugdom", "Couldn't load gamecontrollerdb.txt!", gSDLWindow);
		}
	}
}

static void Shutdown()
{
	Pomme::Shutdown();

	if (gSDLWindow)
	{
		SDL_DestroyWindow(gSDLWindow);
		gSDLWindow = nullptr;
	}

	SDL_Quit();
}

int bugdom_main(unsigned int argc, void* argv)
{
	int				returnCode				= 0;
	std::string		finalErrorMessage		= "";
	bool			showFinalErrorMessage	= false;

	// Start the game
	try
	{
		ParseCommandLine(argc, (char**) argv);
		Boot();
		returnCode = GameMain();
	}
	catch (Pomme::QuitRequest&)
	{
		// no-op, the game may throw this exception to shut us down cleanly
	}
#if !(_DEBUG)
	// In release builds, catch anything that might be thrown by CommonMain
	// so we can show an error dialog to the user.
	catch (std::exception& ex)		// Last-resort catch
	{
		returnCode = 1;
		finalErrorMessage = ex.what();
		showFinalErrorMessage = true;
	}
	catch (...)						// Last-resort catch
	{
		returnCode = 1;
		finalErrorMessage = "unknown";
		showFinalErrorMessage = true;
	}
#endif

#if __APPLE__
	// Whether we failed or succeeded, always restore the user's mouse acceleration before exiting.
	// (NOTE: in debug builds, we might not get here because we don't catch what GameMain throws.)
	RestoreMacMouseAcceleration();
#endif

	Shutdown();

	if (showFinalErrorMessage)
	{
		std::cerr << "Uncaught exception: " << finalErrorMessage << "\n";
		SDL_ShowSimpleMessageBox(0, "Bugdom", finalErrorMessage.c_str(), nullptr);
	}

	return returnCode;
}

// Set clock frequencies, check for libshacccg.suprx, and increase
// stack size, following Rinnegatamante's Quake3 port.
int main(int argc, char **argv)
{
	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);
	
    // Throw an error if libshacccg.suprx can't be found.
	SceIoStat st1, st2;
	if (!(sceIoGetstat("ur0:/data/libshacccg.suprx", &st1) >= 0 || sceIoGetstat("ur0:/data/external/libshacccg.suprx", &st2) >= 0))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bugdom", "Error: libshacccg.suprx not found. For an installation guide, see https://samilops2.gitbook.io/vita-troubleshooting-guide/shader-compiler/extract-libshacccg.suprx .", NULL);
		sceKernelExitProcess(0);
	}
	
	// Increase stack size
	SceUID main_thread = sceKernelCreateThread("Bugdom", bugdom_main, 0x40, 0x200000, 0, 0, NULL);
	if (main_thread >= 0)
    {
		sceKernelStartThread(main_thread, 0, NULL);
		sceKernelWaitThreadEnd(main_thread, NULL, NULL);
	}

	return 0;
}
