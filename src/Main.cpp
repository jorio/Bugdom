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

#if __APPLE__
#include "killmacmouseacceleration.h"
#include <libproc.h>
#include <unistd.h>
#endif

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

static const char* GetWindowTitle()
{
	static char windowTitle[256];
	snprintf(windowTitle, sizeof(windowTitle), "Bugdom %s", PROJECT_VERSION);
	return windowTitle;
}

void ParseCommandLine(int argc, const char** argv)
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

int CommonMain(int argc, const char** argv)
{
	ParseCommandLine(argc, argv);

	// Start our "machine"
	Pomme::Init();

	// Initialize SDL video subsystem
	if (0 != SDL_Init(SDL_INIT_VIDEO))
		throw std::runtime_error("Couldn't initialize SDL video subsystem.");

	// Create window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	if (gCommandLine.msaa != 0)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, gCommandLine.msaa);
	}

	gSDLWindow = SDL_CreateWindow(
			GetWindowTitle(),
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			640,
			480,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
	if (!gSDLWindow)
		throw std::runtime_error("Couldn't create SDL window.");

	// Uncomment to dump the game's resources to a temporary directory.
	//Pomme_StartDumpingResources("/tmp/BugdomRezDump");

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

	// Start the game
	try
	{
		GameMain();
	}
	catch (Pomme::QuitRequest&)
	{
		// no-op, the game may throw this exception to shut us down cleanly
	}

	// Clean up
	Pomme::Shutdown();

	return 0;
}

int main(int argc, char** argv)
{
	int				returnCode				= 0;
	std::string		finalErrorMessage		= "";
	bool			showFinalErrorMessage	= false;

#if _DEBUG
	// In debug builds, if CommonMain throws, don't catch.
	// This way, it's easier to get a clean stack trace.
	returnCode = CommonMain(argc, const_cast<const char**>(argv));
#else
	// In release builds, catch anything that might be thrown by CommonMain
	// so we can show an error dialog to the user.
	try
	{
		returnCode = CommonMain(argc, const_cast<const char**>(argv));
	}
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
	// (NOTE: in debug builds, we might not get here because we don't catch what CommonMain throws.)
	RestoreMacMouseAcceleration();
#endif

	if (showFinalErrorMessage)
	{
		std::cerr << "Uncaught exception: " << finalErrorMessage << "\n";
		SDL_ShowSimpleMessageBox(0, "Bugdom", finalErrorMessage.c_str(), nullptr);
	}

	return returnCode;
}
