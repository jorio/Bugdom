// KILL & RESTORE MOUSE ACCELERATION ON MACOS
// This file is part of Bugdom. https://github.com/jorio/bugdom
//
// On macOS, relative mouse input values from SDL_MOUSEMOTION events are
// accelerated. This code turns off mouse acceleration so we get "raw"
// readouts like on Windows. Be careful: mouse acceleration applies to the
// entire system. Restore it when your game is done with the mouse, otherwise
// linear mouse movement will stick around until a reboot.
//
// Tested on macOS 14.0.
// Needs CoreFoundation and IOKit.
//
// Interesting reading:
//
// - https://bugzilla.libsdl.org/show_bug.cgi?id=445
//   I initially tried to patch SDL 2.0.14 with the IOHIDQueue approach above,
//   which works on High Sierra. However, on Big Sur, the OS prevents direct
//   mouse access with the following message: "$GAME would like to receive
//   keystrokes from any application. Grant access to this application in
//   Security & Privacy preferences." Turns out, creating an HID device filter
//   on (kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse) is now restricted
//   (joystick access is fine and doesn't trigger the message).
//
// - https://discourse.libsdl.org/t/macos-raw-mouse-woes/22895
//   The thread that let me to the current approach.
//
// - https://forums3.armagetronad.net/viewtopic.php?f=12&t=3364&start=15
//   The people who found the basis for the current approach.

#if !(__APPLE__) || OSXPPC

void SetMacLinearMouse(int linear)
{
	// no-op on non-Apple systems
	(void) linear;
}

#else

#include <SDL3/SDL.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include <IOKit/hidsystem/event_status_driver.h>

static const int64_t	kNoAcceleration			= -0x10000;
static int64_t			gAccelerationBackup		= 0;
static bool				gAccelerationTainted	= false;

static void KillMacMouseAcceleration(void)
{
	if (gAccelerationTainted)
	{
		SDL_Log("%s: acceleration value already tainted\n", __func__);
		return;
	}

	io_connect_t handle = NXOpenEventStatus();

	if (!handle)
	{
		SDL_Log("%s: NXOpenEventStatus failed!\n", __func__);
		return;
	}

	union
	{
		int64_t i64;
		int32_t i32;
	} value = { 0 };

	IOByteCount valueSize;
	kern_return_t ret = IOHIDGetParameter(handle, CFSTR(kIOHIDMouseAccelerationType), sizeof(value), &value, &valueSize);

	if (ret != KERN_SUCCESS)
	{
		SDL_Log("%s: IOHIDGetParameter error %d\n", __func__, (int)ret);
	}
	else if (valueSize != 4 && valueSize != 8)
	{
		SDL_Log("%s: IOHIDGetParameter returned unexpected size! (Got %d)\n", __func__, (int)valueSize);
	}
	else if (ret == KERN_SUCCESS)
	{
		gAccelerationBackup = valueSize == 4? value.i32: value.i64;

		ret = IOHIDSetParameter(handle, CFSTR(kIOHIDMouseAccelerationType), &kNoAcceleration, sizeof(kNoAcceleration));
		if (ret != KERN_SUCCESS)
		{
			SDL_Log("%s: IOHIDSetParameter error %d (current acceleration = %d)\n", __func__, (int)ret, (int)gAccelerationBackup);
		}
		else
		{
			gAccelerationTainted = true;
			SDL_Log("%s: was %d, now %d\n", __func__, (int)gAccelerationBackup, (int)kNoAcceleration);
		}
	}

	NXCloseEventStatus(handle);
}

static void RestoreMacMouseAcceleration(void)
{
	if (!gAccelerationTainted)
	{
		SDL_Log("%s: acceleration value not tainted (%d)\n", __func__, (int)gAccelerationBackup);
		return;
	}

	io_connect_t handle = NXOpenEventStatus();

	if (!handle)
	{
		SDL_Log("%s: NXOpenEventStatus failed!\n", __func__);
		return;
	}

	kern_return_t ret = IOHIDSetParameter(handle, CFSTR(kIOHIDMouseAccelerationType), &gAccelerationBackup, sizeof(gAccelerationBackup));
	if (ret != KERN_SUCCESS)
	{
		SDL_Log("%s: IOHIDSetParameter error %d\n", __func__, (int)ret);
	}
	else
	{
		gAccelerationTainted = false;
		SDL_Log("%s: restored %d\n", __func__, (int)gAccelerationBackup);
	}

	NXCloseEventStatus(handle);
}

void SetMacLinearMouse(int linear)
{
	if (linear)
	{
		KillMacMouseAcceleration();
	}
	else
	{
		RestoreMacMouseAcceleration();
	}
}

#endif  // __APPLE__
