#include "Pomme.h"
#include "PommeGraphics.h"
#include "PommeFiles.h"

#include <Quesa.h>
#include "gamepatches.h"

static OSErr ReadPictureFile(FSSpec* spec, Pomme::Graphics::ARGBPixmap& pict)
{
	short refNum;

	OSErr error = FSpOpenDF(spec, fsRdPerm, &refNum);
	if (noErr != error)
	{
		return error;
	}

	auto& stream = Pomme::Files::GetStream(refNum);
	pict = Pomme::Graphics::ReadPICT(stream, true);
	FSClose(refNum);

	return noErr;
}

OSErr DrawPictureIntoGWorld(FSSpec* spec, GWorldPtr* theGWorld)
{
	Pomme::Graphics::ARGBPixmap pict;
	OSErr err = ReadPictureFile(spec, pict);
	if (err != noErr)
	{
		return err;
	}

	Rect boundsRect;
	boundsRect.left = 0;
	boundsRect.top = 0;
	boundsRect.right = pict.width;
	boundsRect.bottom = pict.height;

	NewGWorld(theGWorld, 32, &boundsRect, 0, 0, 0);

	GrafPtr oldPort;
	GetPort(&oldPort);
	SetPort(*theGWorld);
	Pomme::Graphics::DrawARGBPixmap(0, 0, pict);
	SetPort(oldPort);

	return noErr;
}

OSErr DrawPictureToScreen(FSSpec* spec, short x, short y)
{
	Pomme::Graphics::ARGBPixmap pict;
	OSErr err = ReadPictureFile(spec, pict);
	if (err != noErr)
	{
		return err;
	}

	GrafPtr oldPort;
	GetPort(&oldPort);
	SetPort(Pomme::Graphics::GetScreenPort());
	Pomme::Graphics::DrawARGBPixmap(x, y, pict);
	SetPort(oldPort);

	return noErr;
}
