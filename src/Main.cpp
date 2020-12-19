#include "Pomme.h"
#include "PommeInit.h"
#include "PommeFiles.h"
#include "PommeGraphics.h"
#include "version.h"

#include <Quesa.h>
#include <SDL.h>

#include "gamepatches.h"

#include <iostream>
#include <cstring>

#if __APPLE__
#include <libproc.h>
#include <unistd.h>
#endif

extern "C"
{
	// bare minimum from Windows.c to satisfy externs in game code
	WindowPtr gCoverWindow = nullptr;
	UInt32* gCoverWindowPixPtr = nullptr;

	// Lets the game know where to find its asset files
	extern FSSpec gDataSpec;

	extern SDL_Window* gSDLWindow;

	extern int PRO_MODE;

	int GameMain(void);
}

static void FindGameData()
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

	// Set data spec
	gDataSpec = Pomme::Files::HostPathToFSSpec(dataPath / "Skeletons");

	// Use application resource file
	auto applicationSpec = Pomme::Files::HostPathToFSSpec(dataPath / "Application");
	short resFileRefNum = FSpOpenResFile(&applicationSpec, fsRdPerm);
	UseResFile(resFileRefNum);
}

static const char* GetWindowTitle()
{
	static char windowTitle[256];
	snprintf(windowTitle, sizeof(windowTitle), "Bugdom %s", PROJECT_VERSION);
	return windowTitle;
}

int CommonMain(int argc, const char** argv)
{
	// Start our "machine"
	Pomme::Init(GetWindowTitle());

	// Set up globals that the game expects
	gCoverWindow = Pomme::Graphics::GetScreenPort();
	gCoverWindowPixPtr = (UInt32*) GetPixBaseAddr(GetGWorldPixMap(gCoverWindow));

	// Clear window
	Overlay_BeginExclusive();
	Overlay_Clear(0xFFA5A5A5);
	Overlay_Flush();
	Overlay_EndExclusive();

	FindGameData();
#if !(__APPLE__)
	Pomme::Graphics::SetWindowIconFromIcl8Resource(128);
#endif

	// Initialize Quesa
	auto qd3dStatus = Q3Initialize();
	if (qd3dStatus != kQ3Success)
	{
		throw std::runtime_error("Couldn't initialize Quesa.");
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
#if _DEBUG
	// In debug builds, don't show a fancy dialog in case of an uncaught exception.
	// This way, it's easier to get a clean stack trace.
	return CommonMain(argc, const_cast<const char**>(argv));
#else
	std::string uncaught;

	try
	{
		return CommonMain(argc, const_cast<const char**>(argv));
	}
	catch (std::exception& ex)
	{
		uncaught = ex.what();
	}
	catch (...)
	{
		uncaught = "unknown";
	}

	std::cerr << "Uncaught exception: " << uncaught << "\n";
	SDL_ShowSimpleMessageBox(0, "Uncaught Exception", uncaught.c_str(), nullptr);
	return 1;
#endif
}
