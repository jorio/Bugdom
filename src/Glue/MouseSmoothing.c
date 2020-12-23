#include <SDL.h>
#include <stdbool.h>
#include "mousesmoothing.h"

static const int kMouseMotionHoldTicks = 16;

static struct
{
	Uint32 lastMouseMotionTimestamp;
	int dxAccu;
	int dyAccu;
	bool newFrame;
} gMouseSmoothingState;

void MouseSmoothing_ResetState(bool newFrame)
{
	memset(&gMouseSmoothingState, 0, sizeof(gMouseSmoothingState));
	gMouseSmoothingState.newFrame = newFrame;
}

void MouseSmoothing_StartFrame(void)
{
	Uint32 ticksNow = SDL_GetTicks();
	if (ticksNow - gMouseSmoothingState.lastMouseMotionTimestamp >= kMouseMotionHoldTicks)
	{
		MouseSmoothing_ResetState(true);
		gMouseSmoothingState.lastMouseMotionTimestamp = ticksNow;
	}
}

void MouseSmoothing_OnMouseMotion(const SDL_MouseMotionEvent* motion)
{
	if (gMouseSmoothingState.newFrame)
	{
		MouseSmoothing_ResetState(false);
	}

	gMouseSmoothingState.dxAccu += motion->xrel;
	gMouseSmoothingState.dyAccu += motion->yrel;
	gMouseSmoothingState.lastMouseMotionTimestamp = motion->timestamp;
	gMouseSmoothingState.newFrame = false;
}

void MouseSmoothing_GetDelta(int* dxOut, int* dyOut)
{
	*dxOut = gMouseSmoothingState.dxAccu;
	*dyOut = gMouseSmoothingState.dyAccu;
}
