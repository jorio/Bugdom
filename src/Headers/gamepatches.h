#pragma once

#include "GLBackdrop.h"

#ifdef __cplusplus
extern "C" {
#endif

TQ3StorageObject Q3FSSpecStorage_New(const FSSpec* spec);

TQ3Area GetAdjustedPane(int windowWidth, int windowHeight, Rect paneClip);

void DoSDLMaintenance(void);

OSErr DrawPictureToScreen(FSSpec* spec, short x, short y);

//-----------------------------------------------------------------------------
// Backdrop

void SetWindowGamma(int percent);

void ExclusiveOpenGLMode_Begin(void);

void ExclusiveOpenGLMode_End(void);

void AllocBackdropTexture(void);

void SetBackdropClipRegion(int width, int height);

void ClearBackdrop(UInt32 argb);

void RenderBackdropQuad(int fit);

void DisposeBackdropTexture(void);

#ifdef __cplusplus
}
#endif
