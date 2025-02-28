// TGA.C
// (C) 2023 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom

#include "game.h"

static void DecompressRLE(short refNum, TGAHeader* header, uint8_t* out)
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
	Ptr compressedData = AllocPtr(compressedLength);

	// Read rest of file into compressed data buffer
	err = FSRead(refNum, &compressedLength, compressedData);

	GAME_ASSERT(err == noErr);
	GAME_ASSERT(compressedLength == eof-pos);	// Ensure we got as many bytes as we asked for

	const long bytesPerPixel	= header->bpp / 8;
	const long pixelCount		= header->width * header->height;
	long pixelsProcessed		= 0;

	const uint8_t*			in  = (uint8_t*) compressedData;
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

static uint8_t* ConvertColormappedToBGR(const uint8_t* in, const TGAHeader* header, const uint8_t* palette)
{
	const int pixelCount				= header->width * header->height;
	const int bytesPerColor				= header->paletteBitsPerColor / 8;
	const uint16_t paletteColorCount	= header->paletteColorCountLo	| ((uint16_t)header->paletteColorCountHi	<< 8);

	uint8_t* remapped = (uint8_t*) AllocPtr(pixelCount * (header->paletteBitsPerColor / 8));
	uint8_t* out = remapped;

	GAME_ASSERT(bytesPerColor == 3);

	for (int i = 0; i < pixelCount; i++)
	{
		uint8_t colorIndex = *in;

		GAME_ASSERT(colorIndex < paletteColorCount);

		out[0] = palette[colorIndex*3+0];	// TGA stores its palette as BGR!
		out[1] = palette[colorIndex*3+1];
		out[2] = palette[colorIndex*3+2];

		in++;
		out += bytesPerColor;
	}

	return remapped;
}

static uint8_t* ConvertToRGBA(const uint8_t* in, const TGAHeader* header)
{
	uint8_t* converted = (uint8_t*) AllocPtr(header->width * header->height * 4);

	struct TargetPixel { uint8_t r, g, b, a; } *outPixel = (struct TargetPixel*) converted;

	const int pixelCount = header->width * header->height;

	switch (header->bpp)
	{
		case 32:	// Targa source data is BGRA
		{
			for (int p = 0; p < pixelCount; p++)
			{
				*outPixel = (struct TargetPixel) { .r = in[2], .g = in[1], .b = in[0], .a = in[3] };
				outPixel++;
				in += 4;
			}
			break;
		}

		case 24:	// Targa source data is BGR
		{
			for (int p = 0; p < pixelCount; p++)
			{
				*outPixel = (struct TargetPixel){ .r = in[2], .g = in[1], .b = in[0], .a = 0xFF };
				outPixel++;
				in += 3;
			}
			break;
		}

		case 16:	// Targa source data is A1RGB5 (packed into a little-endian 16-bit word)
		{
			const uint16_t* inPtr16 = (const uint16_t*) in;

			if (header->imageDescriptor & 1)	// 1 alpha + 5-5-5 color
			{
				for (int p = 0; p < pixelCount; p++)
				{
					uint16_t inRGB16 = UnpackI16LE(inPtr16);
					outPixel->r = (((inRGB16 >> 10) & 0b11111) * 255) / 31;
					outPixel->g = (((inRGB16 >> 5) & 0b11111) * 255) / 31;
					outPixel->b = (((inRGB16 >> 0) & 0b11111) * 255) / 31;
					outPixel->a = (inRGB16 & 0x8000) ? (0xFF) : (0x00);

					outPixel++;
					inPtr16++;
				}
			}
			else	// 5-5-5 color without alpha
			{
				for (int p = 0; p < pixelCount; p++)
				{
					uint16_t inRGB16 = UnpackI16LE(inPtr16);
					outPixel->r = (((inRGB16 >> 10) & 0b11111) * 255) / 31;
					outPixel->g = (((inRGB16 >> 5) & 0b11111) * 255) / 31;
					outPixel->b = (((inRGB16 >> 0) & 0b11111) * 255) / 31;
					outPixel->a = 0xFF;

					outPixel++;
					inPtr16++;
				}
			}
			break;
		}

		case 8:		// Grayscale (note: for 8-bit indexed-color data, call ConvertColormappedToBGR first)
		{
			for (int p = 0; p < pixelCount; p++)
			{
				uint8_t gray = *in;
				*outPixel = (struct TargetPixel){ .r = gray, .g = gray, .b = gray, .a = 0xFF };
				outPixel++;
				in++;
			}
			break;
		}

		default:
			GAME_ASSERT_MESSAGE(false, "TGA: Unsupported bpp for conversion to RGBA");
			break;
	}

	return converted;
}

static void FlipPixelData(uint8_t* data, TGAHeader* header)
{
	int rowBytes = header->width * (header->bpp / 8);

	uint8_t* topRow = data;
	uint8_t* bottomRow = topRow + rowBytes * (header->height - 1);
	uint8_t* topRowCopy = (uint8_t*) AllocPtr(rowBytes);
	while (topRow < bottomRow)
	{
		BlockMove(topRow, topRowCopy, rowBytes);
		BlockMove(bottomRow, topRow, rowBytes);
		BlockMove(topRowCopy, bottomRow, rowBytes);
		topRow += rowBytes;
		bottomRow -= rowBytes;
	}
	DisposePtr((Ptr) topRowCopy);
}

OSErr ReadTGA(const FSSpec* spec, uint8_t** outPtr, TGAHeader* outHeader, bool forceRGBA)
{
	short		refNum;
	OSErr		err;
	long		readCount;
	TGAHeader	header;
	uint8_t*	pixelData;

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

	// Byteswap it on on big-endian systems (TGA is a little-endian format)
	UnpackStructs("<8B4H2B", sizeof(TGAHeader), 1, &header);

	// Make sure we support the format
	switch (header.imageType)
	{
		case TGA_IMAGETYPE_RAW_CMAP:
		case TGA_IMAGETYPE_RAW_BGR:
		case TGA_IMAGETYPE_RAW_GRAYSCALE:
		case TGA_IMAGETYPE_RLE_CMAP:
		case TGA_IMAGETYPE_RLE_BGR:
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
	uint8_t* palette = nil;
	if (header.imageType == TGA_IMAGETYPE_RAW_CMAP || header.imageType == TGA_IMAGETYPE_RLE_CMAP)
	{
		uint16_t paletteColorCount	= header.paletteColorCountLo | ((uint16_t)header.paletteColorCountHi << 8);
		const long paletteBytes		= paletteColorCount * (header.paletteBitsPerColor / 8);

		GAME_ASSERT(8 == header.bpp);
		GAME_ASSERT(header.paletteOriginLo == 0 && header.paletteOriginHi == 0);
		GAME_ASSERT(paletteColorCount <= 256);

		palette = (uint8_t*) AllocPtr(paletteBytes);

		readCount = paletteBytes;
		FSRead(refNum, &readCount, (Ptr) palette);
		GAME_ASSERT(readCount == paletteBytes);
	}

	// Allocate pixel data
	pixelData = (uint8_t*) AllocPtr(pixelDataLength);

	// Read pixel data; decompress it if needed
	if (compressed)
	{
		DecompressRLE(refNum, &header, pixelData);
		header.imageType &= ~8;		// flip compressed bit
	}
	else
	{
		readCount = pixelDataLength;
		err = FSRead(refNum, &readCount, (Ptr) pixelData);
		GAME_ASSERT(readCount == pixelDataLength);
		GAME_ASSERT(err == noErr);
	}

	// Close file -- we don't need it anymore
	FSClose(refNum);

	// If pixel data is stored bottom-up, flip it vertically.
	if (needFlip)
	{
		FlipPixelData(pixelData, &header);

		// Set top-left origin bit
		header.imageDescriptor |= (1u << 5u);
	}

	// If the image is color-mapped, map pixel data back to BGR
	if (palette)
	{
		uint8_t* remapped = ConvertColormappedToBGR(pixelData, &header, palette);

		DisposePtr((Ptr) palette);
		palette = nil;

		DisposePtr((Ptr) pixelData);
		pixelData = remapped;

		// Update header to make it an BGR image
		header.imageType = TGA_IMAGETYPE_RAW_BGR;
		header.bpp = header.paletteBitsPerColor;
	}

	// Convert to RGBA if required
	if (forceRGBA)
	{
		uint8_t* converted = ConvertToRGBA(pixelData, &header);

		header.imageType = TGA_IMAGETYPE_CONVERTED_RGBA;
		header.bpp = 32;

		DisposePtr((Ptr) pixelData);
		pixelData = converted;
	}

	// Store result
	if (outHeader != nil)
		*outHeader = header;
	*outPtr = pixelData;
	
	return noErr;
}
