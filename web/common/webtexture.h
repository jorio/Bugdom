#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum WebPixelFormat
{
	WEB_PIXEL_RGB8, WEB_PIXEL_RGBA8, WEB_PIXEL_BGR8, WEB_PIXEL_BGRA8,
	WEB_PIXEL_ARGB1555, WEB_PIXEL_ARGB8888, WEB_PIXEL_ABGR8888,
} WebPixelFormat;

// Destination must hold width*height*4 bytes. Source row length is in pixels,
// not bytes; zero means tightly packed. No OpenGL context is needed.
bool WebConvertTextureRGBA(uint8_t* destination, const void* source,
	int width, int height, int rowPixels, WebPixelFormat format, bool opaque);
