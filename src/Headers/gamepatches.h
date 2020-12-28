#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ALLOW_MSAA
	#define ALLOW_MSAA 0
#endif

TQ3StorageObject Q3FSSpecStorage_New(const FSSpec* spec);

TQ3Area GetAdjustedPane(
	int physicalWidth,
	int physicalHeight,
	int logicalWidth,
	int logicalHeight,
	Rect paneClip);

void DoSDLMaintenance(void);

OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld);

OSErr DrawPictureToScreen(FSSpec* spec, short x, short y);

void FlushQuesaErrors(void);

//-----------------------------------------------------------------------------
// Texture edge padding

void ApplyEdgePadding(const TQ3Mipmap* mipmap);

//-----------------------------------------------------------------------------
// Patch 3DMF models

void PatchSkeleton3DMF(const char* cName, TQ3Object newModel);

void PatchGrouped3DMF(const char* cName, TQ3Object* objects, int nObjects);

//-----------------------------------------------------------------------------
// Overlay

void Overlay_BeginExclusive(void);

void Overlay_EndExclusive(void);

void Overlay_Alloc(void);

void Overlay_Clear(UInt32 argb);

void Overlay_SubmitQuad(
		int srcX, int srcY, int srcW, int srcH,
		float dstX, float dstY, float dstW, float dstH);

void Overlay_Flush(void);

void Overlay_Dispose(void);

void Overlay_FadeOutFrozenFrame(float duration);

#ifdef __cplusplus
}
#endif
