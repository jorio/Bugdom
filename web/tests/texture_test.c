#include "webtexture.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    uint8_t out[16];
    const uint16_t packed16[] = {0xfc00, 0x03e0, 0x801f};
    const uint8_t expected16[] = {255,0,0,255, 0,255,0,0, 0,0,255,255};
    assert(WebConvertTextureRGBA(out, packed16, 3, 1, 0, WEB_PIXEL_ARGB1555, false));
    assert(memcmp(out, expected16, sizeof(expected16)) == 0);
    assert(WebConvertTextureRGBA(out, packed16, 3, 1, 0, WEB_PIXEL_ARGB1555, true));
    assert(out[7] == 255);  // Terrain's unused alpha bit must not make it transparent.

    const uint32_t packed32[] = {0x80123456, 0x00ff00cc};
    const uint8_t expected32[] = {0x12,0x34,0x56,0x80, 0xff,0,0xcc,0};
    assert(WebConvertTextureRGBA(out, packed32, 2, 1, 0, WEB_PIXEL_ARGB8888, false));
    assert(memcmp(out, expected32, sizeof(expected32)) == 0);

    const uint32_t abgr[] = {0x56341280};
    assert(WebConvertTextureRGBA(out, abgr, 1, 1, 0, WEB_PIXEL_ABGR8888, false));
    assert(memcmp(out, expected32, 4) == 0);

    const uint8_t rgb[] = {1,2,3, 4,5,6};
    assert(WebConvertTextureRGBA(out, rgb, 2, 1, 0, WEB_PIXEL_RGB8, false));
    assert(memcmp(out, (uint8_t[]){1,2,3,255,4,5,6,255}, 8) == 0);
    assert(WebConvertTextureRGBA(out, rgb, 2, 1, 0, WEB_PIXEL_BGR8, false));
    assert(memcmp(out, (uint8_t[]){3,2,1,255,6,5,4,255}, 8) == 0);

    // Dirty HUD subrectangle, with an unused pixel at the end of each source row.
    const uint8_t rows[] = {1,2,3,0, 4,5,6,0, 99,99,99,99,
                            7,8,9,0, 10,11,12,0, 99,99,99,99};
    assert(WebConvertTextureRGBA(out, rows, 2, 2, 3, WEB_PIXEL_RGBA8, true));
    assert(memcmp(out, (uint8_t[]){1,2,3,255,4,5,6,255,7,8,9,255,10,11,12,255}, 16) == 0);
    assert(WebConvertTextureRGBA(out, rows, 2, 1, 0, WEB_PIXEL_BGRA8, false));
    assert(memcmp(out, (uint8_t[]){3,2,1,0,6,5,4,0}, 8) == 0);

    // Packed sources need not be naturally aligned (e.g. a resource-file offset).
    uint8_t unaligned[sizeof(packed32)+1];
    memcpy(unaligned+1, packed32, sizeof(packed32));
    assert(WebConvertTextureRGBA(out, unaligned+1, 2, 1, 0, WEB_PIXEL_ARGB8888, false));
    assert(memcmp(out, expected32, 8) == 0);
    assert(!WebConvertTextureRGBA(out, rows, 2, 2, 1, WEB_PIXEL_RGBA8, true));
    assert(!WebConvertTextureRGBA(out, rows, -1, 2, 0, WEB_PIXEL_RGBA8, true));
    assert(!WebConvertTextureRGBA(out, rows, 2, 1, 0, (WebPixelFormat)99, true));
    puts("PASS: packed colors, alpha, strided updates, unaligned input and validation");
}
