#pragma once

// Structure packing formats

#define STRUCTFORMAT_SkeletonFile_Header_Type		">4h"
#define STRUCTFORMAT_File_BoneDefinitionType		">i32c3f2H8I"
#define STRUCTFORMAT_SkeletonFile_AnimHeader_Type	">B32cxh"
#define STRUCTFORMAT_TQ3Point3D						">3f"
#define STRUCTFORMAT_AnimEventType					">hBB"
#define STRUCTFORMAT_JointKeyframeType				">ii9f"
#define STRUCTFORMAT_PlayfieldHeaderType			">I5i3f2i"
#define STRUCTFORMAT_TerrainItemEntryType			">3H4bH"
#define STRUCTFORMAT_SplinePointType				">2f"
#define STRUCTFORMAT_SplineItemType					">fH4bH"
#define STRUCTFORMAT_File_SplineDefType				">hxxiiihxxi4h"
#define STRUCTFORMAT_File_FenceDefType				">Hhi4h"
#define STRUCTFORMAT_FencePointType					">2i"

#define UNPACK_STRUCTS(type, n, ptr) \
	UnpackStructs(STRUCTFORMAT_##type, sizeof(type), n, ptr)

#define CHECK_HANDLE_SIZE_BEFORE_UNPACK(type, n, handle) \
do                                                                                           \
{                                                                                            \
	if ((n) * sizeof(type) != (unsigned long) GetHandleSize((Handle) (handle)))              \
		DoFatalAlert("byteswap handle: size mismatch (%d)", (int)n);                         \
} while(0)

#define UNPACK_STRUCTS_HANDLE(type, n, handle)                                               \
do                                                                                           \
{                                                                                            \
	CHECK_HANDLE_SIZE_BEFORE_UNPACK(type, n, handle);                                        \
	UnpackStructs(STRUCTFORMAT_##type, sizeof(type), (n), *(handle));                        \
} while(0)

#if !(__BIG_ENDIAN__)

//-----------------------------------------------------------------------------
// Unpack big-endian scalars on little-endian systems

#define UNPACK_BE_SCALARS_AUTOSIZEHANDLE(type, handle) \
	ByteswapInts(sizeof(type), GetHandleSize(handle)/sizeof(type), *(handle))

#define UNPACK_BE_SCALARS_HANDLE(type, n, handle) \
do                                                                                           \
{                                                                                            \
	CHECK_HANDLE_SIZE_BEFORE_UNPACK(type, n, handle);                                        \
	ByteswapInts(sizeof(type), (n), *(handle));                                              \
} while(0)

#else  // big endian

//-----------------------------------------------------------------------------
// Big-endian no-ops

// no-op
#define UNPACK_BE_SCALARS_AUTOSIZEHANDLE(type, handle)

// no-op but still check handle size
#define UNPACK_BE_SCALARS_HANDLE(type, n, handle) CHECK_HANDLE_SIZE_BEFORE_UNPACK(type, n, handle)

#endif  // big endian
