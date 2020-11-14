// Toolbox functions missing an implementation

#include "Pomme.h"

void FrameArc(const Rect* r, short startAngle, short arcAngle)
{
	SOURCE_PORT_MINOR_PLACEHOLDER();
}

void GetMouse(Point* mouseLoc)
{
	mouseLoc->h = 0;
	mouseLoc->v = 0;
	SOURCE_PORT_MINOR_PLACEHOLDER();
}

void SndDoCommand(SndChannelPtr chan, SndCommand cmd, Boolean noWait)
{
	SOURCE_PORT_MINOR_PLACEHOLDER();
}

Rect* GetPortBounds(CGrafPtr port, const Rect* rect)
{
	SOURCE_PORT_PLACEHOLDER();
	return NULL;
}

CGrafPtr GetWindowPort(WindowPtr window)
{
	SOURCE_PORT_MINOR_PLACEHOLDER();
	return NULL;
}

void OffsetRect(Rect* rect, short dh, short dv)
{
	SOURCE_PORT_MINOR_PLACEHOLDER();
}

void PenNormal(void)
{
	SOURCE_PORT_MINOR_PLACEHOLDER();
}

void PenSize(short width, short height)
{
	SOURCE_PORT_MINOR_PLACEHOLDER();
}
