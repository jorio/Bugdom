// QUESA EXTRAS.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom

#include "Pomme.h"
#include <QD3D.h>
//#include <QuesaErrors.h>
//#include <QuesaStorage.h>  // For TQ3StorageObject
#include <stdio.h>
#include "gamepatches.h"
#include <math.h>

TQ3StorageObject Q3FSSpecStorage_New(const FSSpec* spec)
{
	printf("TODO NOQUESA: %s\n", __func__);
	return nil;
#if 0	// NOQUESA
	short refNum;
	long fileLength;
	
	OSErr err = FSpOpenDF(spec, fsRdPerm, &refNum);
	if (err != noErr)
	{
		printf("Q3FSSpecStorage_New failed!\n");
		return nil;
	}
	
	GetEOF(refNum, &fileLength);
	Ptr buffer = NewPtr(fileLength);
	FSRead(refNum, &fileLength, buffer);
	FSClose(refNum);
	
	TQ3StorageObject storageObject = Q3MemoryStorage_New((const unsigned char*) buffer, fileLength);

	// Q3MemoryStorage_New creates a copy of the buffer, so we don't need to keep it around
	DisposePtr(buffer);

	return storageObject;
#endif
}

TQ3Area GetAdjustedPane(
	int physicalWidth,
	int physicalHeight,
	int logicalWidth,
	int logicalHeight,
	Rect paneClip)
{
	float scaleX = physicalWidth / (float)(logicalWidth);	// scale clip pane to window size
	float scaleY = physicalHeight / (float)(logicalHeight);
	
	return (TQ3Area)
	{
		{	// Floor min to avoid seam at edges of HUD if scale ratio is dirty
			floorf( scaleX * paneClip.left ),	// min X
			floorf( scaleY * paneClip.top  ),	// min Y
		},
		{	// Ceil max to avoid seam at edges of HUD if scale ratio is dirty
			ceilf( scaleX * (logicalWidth  - paneClip.right ) ),	// max X
			ceilf( scaleY * (logicalHeight - paneClip.bottom) ),	// max Y
		}
	};
}

void FlushQuesaErrors()
{
	printf("TODO NOQUESA: %s\n", __func__);
#if 0	// NOQUESA
	TQ3Error err = Q3Error_Get(nil);
	if (err != kQ3ErrorNone)
	{
		printf("%s: %d\n", __func__, err);
	}
#endif
}
