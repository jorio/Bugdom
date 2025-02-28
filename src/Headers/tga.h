#pragma once

typedef struct
{
	uint8_t		idFieldLength;
	uint8_t		colorMapType;
	uint8_t		imageType;
	uint8_t		paletteOriginLo;
	uint8_t		paletteOriginHi;
	uint8_t		paletteColorCountLo;
	uint8_t		paletteColorCountHi;
	uint8_t 	paletteBitsPerColor;
	uint16_t	xOrigin;
	uint16_t	yOrigin;
	uint16_t	width;
	uint16_t	height;
	uint8_t		bpp;
	uint8_t		imageDescriptor;
} TGAHeader;

enum
{
	TGA_IMAGETYPE_NONE			= 0,
	TGA_IMAGETYPE_RAW_CMAP		= 1,
	TGA_IMAGETYPE_RAW_BGR		= 2,
	TGA_IMAGETYPE_RAW_GRAYSCALE	= 3,
	TGA_IMAGETYPE_RLE_CMAP		= 9,
	TGA_IMAGETYPE_RLE_BGR		= 10,
	TGA_IMAGETYPE_RLE_GRAYSCALE	= 11,
	TGA_IMAGETYPE_CONVERTED_RGBA = 255,	// doesn't appear in actual files; set in memory header when pixel data was converted
};

OSErr ReadTGA(const FSSpec* spec, uint8_t** outPtr, TGAHeader* outHeader, bool forceRGBA);
