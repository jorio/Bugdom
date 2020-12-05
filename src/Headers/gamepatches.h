#pragma once

#include "GLOverlay.h"

#ifdef __cplusplus
extern "C" {
#endif

TQ3StorageObject Q3FSSpecStorage_New(const FSSpec* spec);

TQ3Area GetAdjustedPane(int windowWidth, int windowHeight, Rect paneClip);

void DoSDLMaintenance(void);

OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld);

OSErr DrawPictureToScreen(FSSpec* spec, short x, short y);

void FlushQuesaErrors(void);

Boolean FlushMouseButtonPress(void);

//-----------------------------------------------------------------------------
// Overlay

void Overlay_BeginExclusive(void);

void Overlay_EndExclusive(void);

void Overlay_Alloc(void);

void Overlay_Clear(UInt32 argb);

void Overlay_RenderQuad(int fit);

void Overlay_Dispose(void);

#ifdef __cplusplus
}
#endif
