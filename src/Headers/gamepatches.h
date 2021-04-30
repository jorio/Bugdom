#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ALLOW_MSAA
	#define ALLOW_MSAA 0
#endif

void DoSDLMaintenance(void);

OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld);

OSErr DrawPictureToScreen(FSSpec* spec, short x, short y);

//-----------------------------------------------------------------------------
// Patch 3DMF models

void PatchSkeleton3DMF(const char* cName, TQ3Object newModel);

void PatchGrouped3DMF(const char* cName, TQ3Object* objects, int nObjects);

#ifdef __cplusplus
}
#endif
