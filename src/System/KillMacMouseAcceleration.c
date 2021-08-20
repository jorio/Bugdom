// KILL & RESTORE MOUSE ACCELERATION ON MACOS
// This file is part of Bugdom. https://github.com/jorio/bugdom
//
// On macOS, relative mouse input values from SDL_MOUSEMOTION events are
// accelerated. This code turns off mouse acceleration so we get "raw"
// readouts like on Windows. Be careful: mouse acceleration applies to the
// entire system. Restore it when your game is done with the mouse, otherwise
// linear mouse movement will stick around until a reboot.
//
// Tested on macOS 11.5.2.
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

#if !(__APPLE__)

typedef char BogusTypedef;  // work around ISO C warning about empty translation unit

#else

#include "killmacmouseacceleration.h"

#include <stdio.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include <IOKit/hidsystem/event_status_driver.h>

static const int64_t	kNoAcceleration			= -0x10000;
static int64_t			gAccelerationBackup		= 0;
static bool				gAccelerationTainted	= false;

void KillMacMouseAcceleration(void)
{
	if (gAccelerationTainted)
	{
		printf("%s: accel already tainted, bailing out.\n", __func__);
		return;
	}
	
	io_connect_t handle = NXOpenEventStatus();

	if (!handle)
	{
		printf("%s: NXOpenEventStatus failed!\n", __func__);
		return;
	}
	
	kern_return_t ret;
	
	IOByteCount actualSize;
	ret = IOHIDGetParameter(handle, CFSTR(kIOHIDMouseAccelerationType),
							sizeof(gAccelerationBackup), &gAccelerationBackup, &actualSize);
	
	if (ret != KERN_SUCCESS)
	{
		printf("%s: IOHIDGetParameter failed! Error %d.\n", __func__, (int)ret);
	}
	else if (actualSize != sizeof(gAccelerationBackup))
	{
		printf("%s: IOHIDGetParameter returned unexpected actualSize! (Got %d)\n", __func__, (int)actualSize);
	}
	else if (ret == KERN_SUCCESS)
	{
		ret = IOHIDSetParameter(handle, CFSTR(kIOHIDMouseAccelerationType), &kNoAcceleration, sizeof(kNoAcceleration));
		if (ret != KERN_SUCCESS)
		{
			printf("%s: IOHIDSetParameter failed! Error %d. (Current accel = %lld)\n", __func__, (int)ret, gAccelerationBackup);
		}
		else
		{
			gAccelerationTainted = true;
			printf("%s: success. Was %lld, now %lld.\n", __func__, gAccelerationBackup, kNoAcceleration);
		}
	}
	
	NXCloseEventStatus(handle);
}

void RestoreMacMouseAcceleration(void)
{
	if (!gAccelerationTainted)
	{
		printf("%s: Acceleration value not tainted (%lld).\n", __func__, gAccelerationBackup);
		return;
	}
	
	io_connect_t handle = NXOpenEventStatus();

	if (!handle)
	{
		printf("%s: NXOpenEventStatus failed!\n", __func__);
		return;
	}
	
	kern_return_t ret;
	
	ret = IOHIDSetParameter(handle, CFSTR(kIOHIDMouseAccelerationType), &gAccelerationBackup, sizeof(gAccelerationBackup));
	if (ret != KERN_SUCCESS)
	{
		printf("%s: IOHIDSetParameter failed! Error %d.\n", __func__, (int)ret);
	}
	else
	{
		gAccelerationTainted = false;
		printf("%s: success. Restored %lld.\n", __func__, gAccelerationBackup);
	}
	
	NXCloseEventStatus(handle);
}

#endif  // __APPLE__
