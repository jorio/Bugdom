#pragma once

#ifndef ALLOW_MSAA
	#define ALLOW_MSAA 0
#endif

void DoSDLMaintenance(void);

//-----------------------------------------------------------------------------
// Patch 3DMF models

void PatchSkeleton3DMF(const char* cName, TQ3MetaFile* newModel);

void PatchGrouped3DMF(const char* cName, int nGroups, TQ3TriMeshFlatGroup* groups);

