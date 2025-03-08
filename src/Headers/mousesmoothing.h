#pragma once

#include <SDL3/SDL.h>

void MouseSmoothing_ResetState(void);

void MouseSmoothing_StartFrame(void);

void MouseSmoothing_OnMouseMotion(const SDL_MouseMotionEvent* motion);

void MouseSmoothing_GetDelta(float* dxOut, float* dyOut);
