// TGA.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom

#include "bugdom.h"

static void DecompressRLE(short refNum, TGAHeader* header, Handle handle)
{
	OSErr err = noErr;

	// Get number of bytes until EOF
	long pos = 0;
	long eof = 0;
	long compressedLength = 0;
	GetFPos(refNum, &pos);
	GetEOF(refNum, &eof);
	compressedLength = eof - pos;

	// Prep compressed data buffer
	Ptr compressedData = NewPtr(compressedLength);

	// Read rest of file into compressed data buffer
	err = FSRead(refNum, &compressedLength, compressedData);

	GAME_ASSERT(err == noErr);
	GAME_ASSERT(compressedLength == eof-pos);	// Ensure we got as many bytes as we asked for

	const long bytesPerPixel	= header->bpp / 8;
	const long pixelCount		= header->width * header->height;
	long pixelsProcessed		= 0;

	const uint8_t*			in  = (uint8_t*) compressedData;
	uint8_t*				out = (uint8_t*) *handle;
	const uint8_t* const	eod = in + compressedLength;

	while (pixelsProcessed < pixelCount)
	{
		GAME_ASSERT(in < eod);

		uint8_t packetHeader = *(in++);
		uint8_t packetLength = 1 + (packetHeader & 0x7F);

		GAME_ASSERT(pixelsProcessed + packetLength <= pixelCount);

		if (packetHeader & 0x80)		// Run-length packet
		{
			GAME_ASSERT(in + bytesPerPixel <= eod);
			for (int i = 0; i < packetLength; i++)
			{
				BlockMove(in, out, bytesPerPixel);
				out += bytesPerPixel;
				pixelsProcessed++;
			}
			in += bytesPerPixel;
		}
		else							// Raw packet
		{
			long packetBytes = packetLength * bytesPerPixel;
			GAME_ASSERT(in + packetBytes <= eod);
			BlockMove(in, out, packetBytes);
			in  += packetBytes;
			out += packetBytes;
			pixelsProcessed += packetLength;
		}
	}

	DisposePtr(compressedData);
}

static Handle ConvertColormappedToRGB(const Handle handle, const TGAHeader* header, const Ptr palette)
{
	const int pixelCount				= header->width * header->height;
	const int bytesPerColor				= header->paletteBitsPerColor / 8;
	const uint16_t paletteColorCount	= header->paletteColorCountLo	| ((uint16_t)header->paletteColorCountHi	<< 8);

	Handle remapped = NewHandle(pixelCount * (header->paletteBitsPerColor / 8));

	const uint8_t*	in	= (uint8_t*) *handle;
	uint8_t*		out	= (uint8_t*) *remapped;

	for (int i = 0; i < pixelCount; i++)
	{
		uint8_t colorIndex = *in;

		GAME_ASSERT(colorIndex < paletteColorCount);

		BlockMove(palette + (colorIndex * bytesPerColor), out, bytesPerColor);

		in++;
		out += bytesPerColor;
	}

	return remapped;
}

static void FlipPixelData(Handle handle, TGAHeader* header)
{
	int rowBytes = header->width * (header->bpp / 8);

	Ptr topRow = *handle;
	Ptr bottomRow = topRow + rowBytes * (header->height - 1);
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
}

OSErr ReadTGA(const FSSpec* spec, Handle* outHandle, TGAHeader* outHeader)
{
	short		refNum;
	OSErr		err;
	long		readCount;
	TGAHeader	header;
	Handle		pixelDataHandle;

	// Open data fork
	err = FSpOpenDF(spec, fsRdPerm, &refNum);
	if (err != noErr)
		return err;

	// Read header
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
		case TGA_IMAGETYPE_RAW_CMAP:
		case TGA_IMAGETYPE_RAW_RGB:
		case TGA_IMAGETYPE_RAW_GRAYSCALE:
		case TGA_IMAGETYPE_RLE_CMAP:
		case TGA_IMAGETYPE_RLE_RGB:
		case TGA_IMAGETYPE_RLE_GRAYSCALE:
			break;
		default:
			FSClose(refNum);
			return badFormat;
	}

	// Extract some info from the header
	Boolean compressed		= header.imageType & 8;
	Boolean needFlip		= 0 == (header.imageDescriptor & (1u << 5u));
	long pixelDataLength	= header.width * header.height * (header.bpp / 8);

	// Ensure there's no identification field -- we don't support that
	GAME_ASSERT(header.idFieldLength == 0);

	// If there's palette data, load it in
	Ptr palette = nil;
	if (header.imageType == TGA_IMAGETYPE_RAW_CMAP || header.imageType == TGA_IMAGETYPE_RLE_CMAP)
	{
		uint16_t paletteColorCount	= header.paletteColorCountLo | ((uint16_t)header.paletteColorCountHi << 8);
		const long paletteBytes		= paletteColorCount * (header.paletteBitsPerColor / 8);

		GAME_ASSERT(8 == header.bpp);
		GAME_ASSERT(header.paletteOriginLo == 0 && header.paletteOriginHi == 0);
		GAME_ASSERT(paletteColorCount <= 256);

		palette = NewPtr(paletteBytes);

		readCount = paletteBytes;
		FSRead(refNum, &readCount, palette);
		GAME_ASSERT(readCount == paletteBytes);
	}

	// Allocate pixel data
	pixelDataHandle = NewHandle(pixelDataLength);

	// Read pixel data; decompress it if needed
	if (compressed)
	{
		DecompressRLE(refNum, &header, pixelDataHandle);
		header.imageType &= ~8;		// flip compressed bit
	}
	else
	{
		readCount = pixelDataLength;
		err = FSRead(refNum, &readCount, *pixelDataHandle);
		GAME_ASSERT(readCount == pixelDataLength);
		GAME_ASSERT(err == noErr);
	}

	// Close file -- we don't need it anymore
	FSClose(refNum);

	// If the image is color-mapped, map pixel data back to RGB
	if (palette)
	{
		Handle remapped = ConvertColormappedToRGB(pixelDataHandle, &header, palette);

		DisposePtr(palette);
		palette = nil;

		DisposeHandle(pixelDataHandle);
		pixelDataHandle = remapped;

		// Update header to make it an RGB image
		header.imageType = TGA_IMAGETYPE_RAW_RGB;
		header.bpp = header.paletteBitsPerColor;
	}

	// If pixel data is stored bottom-up, flip it vertically.
	if (needFlip)
	{
		FlipPixelData(pixelDataHandle, &header);

		// Set top-left origin bit
		header.imageDescriptor |= (1u << 5u);
	}

	// Store result
	if (outHeader != nil)
		*outHeader = header;
	*outHandle = pixelDataHandle;
	
	return noErr;
}
