#pragma once

void MouseSmoothing_ResetState(bool newFrame);

void MouseSmoothing_StartFrame(void);

void MouseSmoothing_OnMouseMotion(const SDL_MouseMotionEvent* motion);

void MouseSmoothing_GetDelta(int* dxOut, int* dyOut);