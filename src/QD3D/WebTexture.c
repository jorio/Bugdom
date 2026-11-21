#include "webtexture.h"
#include <stddef.h>
#include <string.h>

bool WebConvertTextureRGBA(uint8_t* destination, const void* source,
	int width, int height, int rowPixels, WebPixelFormat format, bool opaque)
{
	if (!source || !destination || width <= 0 || height <= 0) return false;
	if (format < WEB_PIXEL_RGB8 || format > WEB_PIXEL_ARGB8888) return false;
	if (rowPixels == 0) rowPixels = width;
	if (rowPixels < width || (size_t) rowPixels > SIZE_MAX / 4 / (size_t) height) return false;
	for (int y = 0; y < height; y++)
	for (int x = 0; x < width; x++)
	{
		size_t i = (size_t) y * rowPixels + x;
		uint8_t* dst = destination + ((size_t) y * width + x) * 4;
		if (format == WEB_PIXEL_ARGB1555)
		{
			uint16_t pixel;
			memcpy(&pixel, (const uint8_t*) source + i * 2, sizeof(pixel));
			dst[0] = ((pixel >> 10) & 31) * 255 / 31;
			dst[1] = ((pixel >> 5) & 31) * 255 / 31;
			dst[2] = (pixel & 31) * 255 / 31;
			dst[3] = (pixel & 0x8000) ? 255 : 0;
		}
		else if (format == WEB_PIXEL_ARGB8888)
		{
			uint32_t pixel;
			memcpy(&pixel, (const uint8_t*) source + i * 4, sizeof(pixel));
			dst[0] = pixel >> 16; dst[1] = pixel >> 8;
			dst[2] = pixel; dst[3] = pixel >> 24;
		}
		else
		{
			bool rgb = format == WEB_PIXEL_RGB8 || format == WEB_PIXEL_BGR8;
			bool bgr = format == WEB_PIXEL_BGR8 || format == WEB_PIXEL_BGRA8;
			const uint8_t* src = (const uint8_t*) source + i * (rgb ? 3 : 4);
			dst[0] = src[bgr ? 2 : 0]; dst[1] = src[1]; dst[2] = src[bgr ? 0 : 2];
			dst[3] = rgb ? 255 : src[3];
		}
		if (opaque) dst[3] = 255;
	}
	return true;
}
