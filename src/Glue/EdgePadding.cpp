#include "MyPCH_Normal.pch"

#define EDGE_PADDING_REPEAT 8

template<typename T>
static void _EdgePadding(
		T* const pixelData,
		const int width,
		const int height,
		const int rowBytes,
		const T alphaMask)
{
	GAME_ASSERT(rowBytes % sizeof(T) == 0);
	const int rowAdvance = rowBytes / sizeof(T);

	T* const firstRow = pixelData;
	T* const lastRow = firstRow + (height-1) * rowAdvance;

	for (int i = 0; i < EDGE_PADDING_REPEAT; i++)
	{
		// Dilate horizontally, row by row
		for (T* row = firstRow; row <= lastRow; row += rowAdvance)
		{
			// Expand east
			for (int x = 0; x < width-1; x++)
				if (!row[x])
					row[x] = row[x+1] & ~alphaMask;

			// Expand west
			for (int x = width-1; x > 0; x--)
				if (!row[x])
					row[x] = row[x-1] & ~alphaMask;
		}

		// Dilate vertically, column by column
		for (int x = 0; x < width; x++)
		{
			// Expand south
			for (T* row = firstRow; row < lastRow; row += rowAdvance)
				if (!row[x])
					row[x] = row[x + rowAdvance] & ~alphaMask;

			// Expand north
			for (T* row = lastRow; row > firstRow; row -= rowAdvance)
				if (!row[x])
					row[x] = row[x - rowAdvance] & ~alphaMask;
		}
	}
}

void ApplyEdgePadding(const TQ3Mipmap* mipmap)
{
	unsigned char* buffer;
	TQ3Uns32 validSize;
	TQ3Uns32 bufferSize;

	GAME_ASSERT(kQ3StorageTypeMemory == Q3Storage_GetType(mipmap->image));

	TQ3Status status = Q3MemoryStorage_GetBuffer(mipmap->image, &buffer, &validSize, &bufferSize);
	GAME_ASSERT(status);

	const TQ3MipmapImage& mip0 = mipmap->mipmaps[0];

	GAME_ASSERT(!mipmap->useMipmapping);
	GAME_ASSERT(mip0.offset == 0);
	GAME_ASSERT(mip0.rowBytes * mip0.height == validSize);
	GAME_ASSERT(mip0.rowBytes * mip0.height == bufferSize);

	switch (mipmap->pixelType)
	{
		case kQ3PixelTypeARGB16:
			GAME_ASSERT(mip0.rowBytes >= mip0.width * 2);
			_EdgePadding<uint16_t>(
					(uint16_t *) buffer,
					mip0.width,
					mip0.height,
					mip0.rowBytes,
					0x0080);
			break;

		case kQ3PixelTypeARGB32:
			GAME_ASSERT(mip0.rowBytes >= mip0.width * 4);
			_EdgePadding<uint32_t>(
					(uint32_t *) buffer,
					mip0.width,
					mip0.height,
					mip0.rowBytes,
					0x000000FF);
			break;

		case kQ3PixelTypeRGB16:
		case kQ3PixelTypeRGB16_565:
		case kQ3PixelTypeRGB24:
		case kQ3PixelTypeRGB32:
			printf("unnecessary to apply edge padding here\n");
			break;

		default:
			GAME_ASSERT_MESSAGE(false, "pixel type unsupported");
			break;
	}
}
