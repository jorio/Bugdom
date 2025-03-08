// MOUSE SMOOTHING.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom

#include "game.h"

static const int kAccumulatorWindowNanoseconds = 10 * 1e6;	// 10 ms

#define DELTA_MOUSE_MAX_SNAPSHOTS 64

static bool gNeedInit = true;

typedef struct
{
	Uint64 timestamp;
	float dx;
	float dy;
} DeltaMouseSnapshot;

static struct
{
	DeltaMouseSnapshot snapshots[DELTA_MOUSE_MAX_SNAPSHOTS];
	int ringStart;
	int ringLength;
	float dxAccu;
	float dyAccu;
} gState;

//-----------------------------------------------------------------------------

static void PopOldestSnapshot(void)
{
	gState.dxAccu -= gState.snapshots[gState.ringStart].dx;
	gState.dyAccu -= gState.snapshots[gState.ringStart].dy;

	gState.ringStart = (gState.ringStart + 1) % DELTA_MOUSE_MAX_SNAPSHOTS;
	gState.ringLength--;

	GAME_ASSERT(gState.ringLength != 0 || (gState.dxAccu == 0 && gState.dyAccu == 0));
}

void MouseSmoothing_ResetState(void)
{
	gState.ringLength = 0;
	gState.ringStart = 0;
	gState.dxAccu = 0;
	gState.dyAccu = 0;
}

void MouseSmoothing_StartFrame(void)
{
	if (gNeedInit)
	{
		MouseSmoothing_ResetState();
		gNeedInit = false;
	}

	Uint64 now = SDL_GetTicksNS();
	Uint64 cutoffTimestamp = now - kAccumulatorWindowNanoseconds;

	// Purge old snapshots
	while (gState.ringLength > 0 &&
		   gState.snapshots[gState.ringStart].timestamp < cutoffTimestamp)
	{
		PopOldestSnapshot();
	}
}

void MouseSmoothing_OnMouseMotion(const SDL_MouseMotionEvent* motion)
{
	if (gState.ringLength == DELTA_MOUSE_MAX_SNAPSHOTS)
	{
//		SDL_Log("%s: buffer full!!", __func__);
		PopOldestSnapshot();				// make room at start of ring buffer
	}

	int i = (gState.ringStart + gState.ringLength) % DELTA_MOUSE_MAX_SNAPSHOTS;
	gState.ringLength++;

	gState.snapshots[i].timestamp = motion->timestamp;
	gState.snapshots[i].dx = motion->xrel;
	gState.snapshots[i].dy = motion->yrel;

	gState.dxAccu += motion->xrel;
	gState.dyAccu += motion->yrel;
}

void MouseSmoothing_GetDelta(float* dxOut, float* dyOut)
{
	GAME_ASSERT(gState.ringLength != 0 || (gState.dxAccu == 0 && gState.dyAccu == 0));

	*dxOut = gState.dxAccu;
	*dyOut = gState.dyAccu;
}
