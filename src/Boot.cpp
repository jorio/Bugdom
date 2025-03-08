// BUGDOM ENTRY POINT
// (C) 2025 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Pomme.h"
#include "PommeInit.h"
#include "PommeFiles.h"

extern "C"
{
	#include "game.h"

	SDL_Window* gSDLWindow = nullptr;
	FSSpec gDataSpec;
	int gCurrentAntialiasingLevel;

#if 0 //_WIN32
	// Tell Windows graphics driver that we prefer running on a dedicated GPU if available
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
#endif
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
	gDataSpec = Pomme::Files::HostPathToFSSpec(dataPath / "System");

	FSSpec someDataFileSpec;
	OSErr iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":System:gamecontrollerdb.txt", &someDataFileSpec);
	if (iErr)
	{
		goto tryAgain;
	}

	return dataPath;
}

static void Boot(int argc, char** argv)
{
	const char* executablePath = argc > 0 ? argv[0] : NULL;

	SDL_SetAppMetadata(GAME_FULL_NAME, GAME_VERSION, GAME_IDENTIFIER);
#if _DEBUG
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
#else
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
#endif

	// Start our "machine"
	Pomme::Init();

	// Load game prefs before starting
	InitPrefs();

retryVideo:
	// Initialize SDL video subsystem
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		throw std::runtime_error("Couldn't initialize SDL video subsystem.");
	}

	// Create window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	gCurrentAntialiasingLevel = gGamePrefs.antialiasingLevel;
	if (gCurrentAntialiasingLevel != 0)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 1 << gCurrentAntialiasingLevel);
	}

	// Determine display
	SDL_DisplayID display = gGamePrefs.displayNumMinus1 + 1;
	if ((int) display > GetNumDisplays())
	{
		gGamePrefs.displayNumMinus1 = 0;
		display = 1;
	}

	// Determine initial window size
	int initialWidth = 640;
	int initialHeight = 480;
	GetDefaultWindowSize(display, &initialWidth, &initialHeight);

	gSDLWindow = SDL_CreateWindow(
		GAME_FULL_NAME " " GAME_VERSION,
		initialWidth,
		initialHeight,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

	MoveToPreferredDisplay();

	if (!gSDLWindow)
	{
		if (gCurrentAntialiasingLevel != 0)
		{
			SDL_Log("Couldn't create SDL window with the requested MSAA level. Retrying without MSAA...");

			// retry without MSAA
			gGamePrefs.antialiasingLevel = 0;
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
			goto retryVideo;
		}
		else
		{
			throw std::runtime_error("Couldn't create SDL window.");
		}
	}

	// Find path to game data folder
	fs::path dataPath = FindGameData(executablePath);

	// Init joystick subsystem
	{
		SDL_Init(SDL_INIT_GAMEPAD);
		auto gamecontrollerdbPath8 = (dataPath / "System" / "gamecontrollerdb.txt").u8string();
		if (-1 == SDL_AddGamepadMappingsFromFile((const char*)gamecontrollerdbPath8.c_str()))
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, GAME_FULL_NAME, "Couldn't load gamecontrollerdb.txt!", gSDLWindow);
		}
	}
}

static void Shutdown()
{
	// Always restore the user's mouse acceleration before exiting.
	SetMacLinearMouse(false);

	Pomme::Shutdown();

	if (gSDLWindow)
	{
		SDL_DestroyWindow(gSDLWindow);
		gSDLWindow = NULL;
	}

	SDL_Quit();
}

int main(int argc, char** argv)
{
	bool success = true;
	std::string uncaught = "";

	try
	{
		Boot(argc, argv);
		GameMain();
	}
	catch (Pomme::QuitRequest&)
	{
		// no-op, the game may throw this exception to shut us down cleanly
	}
#if !(_DEBUG)
	// In release builds, catch anything that might be thrown by GameMain
	// so we can show an error dialog to the user.
	catch (std::exception& ex)		// Last-resort catch
	{
		success = false;
		uncaught = ex.what();
	}
	catch (...)						// Last-resort catch
	{
		success = false;
		uncaught = "unknown";
	}
#endif

	Shutdown();

	if (!success)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Uncaught exception: %s", uncaught.c_str());
		SDL_ShowSimpleMessageBox(0, GAME_FULL_NAME ": Uncaught exception", uncaught.c_str(), nullptr);
	}

	return success ? 0 : 1;
}
