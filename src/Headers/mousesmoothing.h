#pragma once

#include <SDL.h>

void MouseSmoothing_ResetState(void);

void MouseSmoothing_StartFrame(void);

void MouseSmoothing_OnMouseMotion(const SDL_MouseMotionEvent* motion);

void MouseSmoothing_GetDelta(int* dxOut, int* dyOut);