#include "Pomme.h"
#include <Quesa.h>
#include <QuesaErrors.h>
#include <QuesaStorage.h>  // For TQ3StorageObject
#include "gamepatches.h"

extern float gAdditionalClipping;

TQ3StorageObject Q3FSSpecStorage_New(const FSSpec* spec)
{
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
}

TQ3Area GetAdjustedPane(
	int physicalWidth,
	int physicalHeight,
	int logicalWidth,
	int logicalHeight,
	Rect paneClip)
{
	TQ3Area pane;

	pane.min.x = paneClip.left;								// set bounds?
	pane.max.x = logicalWidth - paneClip.right;
	pane.min.y = paneClip.top;
	pane.max.y = logicalHeight - paneClip.bottom;

	pane.min.x += gAdditionalClipping;						// offset bounds by user clipping
	pane.max.x -= gAdditionalClipping;
	pane.min.y += gAdditionalClipping*.75;
	pane.max.y -= gAdditionalClipping*.75;

	// Source port addition
	pane.min.x *= physicalWidth / (float)(logicalWidth);	// scale clip pane to window size
	pane.max.x *= physicalWidth / (float)(logicalWidth);
	pane.min.y *= physicalHeight / (float)(logicalHeight);
	pane.max.y *= physicalHeight / (float)(logicalHeight);

	return pane;
}

void FlushQuesaErrors()
{
	TQ3Error err = Q3Error_Get(nil);
	if (err != kQ3ErrorNone)
	{
		printf("%s: %d\n", __func__, err);
	}
}
