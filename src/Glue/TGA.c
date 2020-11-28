#include "Headers/MyPCH_Normal.pch"

OSErr ReadTGA(const FSSpec* spec, Handle* outHandle, TGAHeader* outHeader)
{
	short refNum;
	OSErr err;
	long readCount;
	int rowBytes;
	TGAHeader header;
	Handle handle;
	
	err = FSpOpenDF(spec, fsRdPerm, &refNum);
	if (err != noErr)
	{
		return err;
	}
	
	readCount = sizeof(TGAHeader);
	err = FSRead(refNum, &readCount, (Ptr) &header);
	if (err != noErr || readCount != sizeof(TGAHeader))
	{
		FSClose(refNum);
		return err;
	}
	
	// Make sure we support the format
	switch (header.imageType)
	{
		case TGA_IMAGETYPE_RAW_RGB:
			rowBytes = (header.bpp / 8) * header.width;
			break;
		case TGA_IMAGETYPE_RAW_GRAYSCALE:
			rowBytes = header.width;
			break;
		default:
			FSClose(refNum);
			return badFormat;
	}
	
	Boolean needFlip = 0 == (header.imageDescriptor & (1u << 5u));

	readCount = rowBytes * header.height;
	handle = NewHandle(readCount);

	err = FSRead(refNum, &readCount, *handle);
	FSClose(refNum);

	if (readCount != rowBytes * header.height)
	{
		DisposeHandle(handle);
		return ioErr;
	}
	
	if (err != noErr)
	{
		DisposeHandle(handle);
		return err;
	}
	
	if (needFlip)
	{
		Ptr topRow = *handle;
		Ptr bottomRow = topRow + rowBytes * (header.height - 1);
		Ptr topRowCopy = NewPtr(rowBytes);
		while (topRow < bottomRow)
		{
			BlockMove(topRow, topRowCopy, rowBytes);
			BlockMove(bottomRow, topRow, rowBytes);
			BlockMove(topRowCopy, bottomRow, rowBytes);
			topRow += rowBytes;
			bottomRow -= rowBytes;
		}
		DisposePtr(topRowCopy);
		// Set top-left bit
		header.imageDescriptor |= (1u << 5u);
	}
	
	if (outHeader != nil)
		*outHeader = header;
	*outHandle = handle;
	
	return noErr;
}
