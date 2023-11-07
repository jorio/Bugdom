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
	int gFullscreenModeAppliedOnBoot = 0;

	int GameMain(void);
}

static fs::path FindGameData(const char* executablePath)
{
	fs::path dataPath;

	int attemptNum = 0;

#if !(__APPLE__)
	attemptNum++;		// skip macOS special case #0
#endif

	if (!executablePath)
		attemptNum = 2;

tryAgain:
	switch (attemptNum)
	{
		case 0:			// special case for macOS app bundles
			dataPath = executablePath;
			dataPath = dataPath.parent_path().parent_path() / "Resources";
			break;

		case 1:
			dataPath = executablePath;
			dataPath = dataPath.parent_path() / "Data";
			break;

		case 2:
			dataPath = "Data";
			break;

		default:
			throw std::runtime_error("Couldn't find the Data folder.");
	}

	attemptNum++;

	dataPath = dataPath.lexically_normal();

	// Set data spec -- Lets the game know where to find its asset files
	gDataSpec = Pomme::Files::HostPathToFSSpec(dataPath / "Skeletons");

	FSSpec dummySpec;
	if (noErr != FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:DoodleBug.3dmf", &dummySpec))
	{
		goto tryAgain;
	}

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
			gDebugMode = DEBUG_MODE_STATS;
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

static void Boot(const char* executablePath)
{
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

	// Start our "machine"
	Pomme::Init();

	// Initialize SDL video subsystem
	if (0 != SDL_Init(SDL_INIT_VIDEO))
	{
		throw std::runtime_error("Couldn't initialize SDL video subsystem.");
	}

	// Load our prefs
	InitPrefs();

	if (gCommandLine.msaa != 0)
		gGamePrefs.antialiasingLevel = gCommandLine.msaa;

tryAgain:
	// Set up GL attributes
#if !(OSXPPC)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
#endif
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	if (gGamePrefs.antialiasingLevel)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, gGamePrefs.antialiasingLevel);
	}
	gAntialiasingLevelAppliedOnBoot = gGamePrefs.antialiasingLevel;
	gFullscreenModeAppliedOnBoot = gGamePrefs.fullscreen;

	// Prepare window dimensions
	int display = 0;
	float screenFillRatio = 2.0f / 3.0f;

	SDL_Rect displayBounds = { .x = 0, .y = 0, .w = GAME_VIEW_WIDTH, .h = GAME_VIEW_HEIGHT };
#if SDL_VERSION_ATLEAST(2,0,5)
	SDL_GetDisplayUsableBounds(display, &displayBounds);
#else
	SDL_GetDisplayBounds(display, &displayBounds);
#endif
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
			SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

	if (!gSDLWindow)
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
	}

	// Find path to game data folder
	fs::path dataPath = FindGameData(executablePath);

#if !(NOJOYSTICK)
	// Init joystick subsystem
	{
		SDL_Init(SDL_INIT_JOYSTICK);
		auto gamecontrollerdbPath8 = (dataPath / "System" / "gamecontrollerdb.txt").u8string();
		if (-1 == SDL_GameControllerAddMappingsFromFile((const char*)gamecontrollerdbPath8.c_str()))
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Bugdom", "Couldn't load gamecontrollerdb.txt!", gSDLWindow);
		}
	}
#endif
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

int main(int argc, char** argv)
{
	int				returnCode				= 0;
	std::string		finalErrorMessage		= "";
	bool			showFinalErrorMessage	= false;

	const char* executablePath = argc > 0 ? argv[0] : NULL;

	// Start the game
	try
	{
		ParseCommandLine(argc, argv);
		Boot(executablePath);
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

	// Whether we failed or succeeded, always restore the user's mouse acceleration before exiting.
	// (NOTE: in debug builds, we might not get here because we don't catch what GameMain throws.)
	SetMacLinearMouse(0);

	Shutdown();

	if (showFinalErrorMessage)
	{
		std::cerr << "Uncaught exception: " << finalErrorMessage << "\n";
		SDL_ShowSimpleMessageBox(0, "Bugdom", finalErrorMessage.c_str(), nullptr);
	}

	return returnCode;
}
